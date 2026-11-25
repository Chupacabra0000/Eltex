
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define INPUT_SIZE 1024
#define FILE_CHUNK 32768

static int sockfd = -1;
static volatile sig_atomic_t running = 1;

static unsigned char input_buffer[BUFFER_SIZE];
static size_t input_length = 0;

static FILE *received_file = NULL;
static uint64_t remaining_file_size = 0;

void stop_client(int sig)
{
    (void)sig;
    running = 0;
}

// Send all bytes, even if send() writes only part.
int send_all(int fd, const void *data, size_t length)
{
    const unsigned char *ptr = data;

    while (length > 0) {

        ssize_t sent = send(
            fd,
            ptr,
            length,
            MSG_NOSIGNAL
        );

        if (sent < 0) {

            if (errno == EINTR)
                continue;

            return -1;
        }

        if (sent == 0)
            return -1;

        ptr += sent;
        length -= (size_t)sent;
    }

    return 0;
}

// Send one complete protocol frame.
int send_frame(uint32_t type,
               const void *data,
               uint32_t length)
{
    FrameHeader header;

    header.type = htonl(type);
    header.length = htonl(length);

    if (send_all(sockfd, &header, sizeof(header)) < 0)
        return -1;

    if (length > 0) {

        if (send_all(sockfd, data, length) < 0)
            return -1;
    }

    return 0;
}

// Extract a safe filename from a received path.
const char *safe_filename(const char *name)
{
    const char *result = name;

    for (const char *p = name; *p != '\0'; p++) {

        if (*p == '/' || *p == '\\')
            result = p + 1;
    }

    return result;
}

// Process a complete frame received from server.
void process_frame(uint32_t type,
                   const unsigned char *data,
                   uint32_t length)
{
    switch (type) {

        case MSG_SYSTEM:
        case MSG_CHAT:
        {
            printf(
                "\n%.*s\n> ",
                (int)length,
                (const char *)data
            );

            fflush(stdout);
            break;
        }

        case MSG_FILE_BEGIN:
        {
            if (length < 12)
                break;

            uint32_t name_length;
            uint32_t size_high;
            uint32_t size_low;

            memcpy(&name_length, data, 4);
            memcpy(&size_high, data + 4, 4);
            memcpy(&size_low, data + 8, 4);

            name_length = ntohl(name_length);
            size_high = ntohl(size_high);
            size_low = ntohl(size_low);

            if (name_length == 0 ||
                name_length > MAX_FILENAME ||
                length != 12 + name_length) {
                break;
            }

            char filename[MAX_FILENAME + 1];

            memcpy(
                filename,
                data + 12,
                name_length
            );

            filename[name_length] = '\0';

            const char *safe_name = safe_filename(filename);

            if (*safe_name == '\0' ||
                strcmp(safe_name, ".") == 0 ||
                strcmp(safe_name, "..") == 0) {
                break;
            }

            // Close a previous incomplete file.
            if (received_file != NULL) {
                fclose(received_file);
                received_file = NULL;
            }

            uint64_t file_size =
                ((uint64_t)size_high << 32) | size_low;

            mkdir("downloads", 0755);

            char path[MAX_FILENAME + 32];

            snprintf(
                path,
                sizeof(path),
                "downloads/%s",
                safe_name
            );

            received_file = fopen(path, "wb");

            if (received_file == NULL) {
                perror("fopen");
                remaining_file_size = 0;
                break;
            }

            remaining_file_size = file_size;

            printf(
                "\nReceiving file: %s (%llu bytes)\n> ",
                safe_name,
                (unsigned long long)file_size
            );

            fflush(stdout);

            break;
        }

        case MSG_FILE_DATA:
        {
            if (received_file == NULL)
                break;

            if ((uint64_t)length > remaining_file_size) {
                fclose(received_file);
                received_file = NULL;
                remaining_file_size = 0;
                break;
            }

            if (fwrite(data, 1, length, received_file) != length) {
                perror("fwrite");
                fclose(received_file);
                received_file = NULL;
                remaining_file_size = 0;
                break;
            }

            remaining_file_size -= length;

            break;
        }

        case MSG_FILE_END:
        {
            if (received_file != NULL) {

                fclose(received_file);
                received_file = NULL;

                if (remaining_file_size == 0) {
                    printf("\nFile received successfully!\n> ");
                } else {
                    printf("\nIncomplete file received.\n> ");
                }

                remaining_file_size = 0;

                fflush(stdout);
            }

            break;
        }

        default:
            break;
    }
}

// Extract complete frames from the receive buffer.
void process_received_data(void)
{
    while (input_length >= sizeof(FrameHeader)) {

        FrameHeader header;

        memcpy(
            &header,
            input_buffer,
            sizeof(header)
        );

        uint32_t type = ntohl(header.type);
        uint32_t length = ntohl(header.length);

        if (length > MAX_DATA + MAX_FILENAME + 12) {
            running = 0;
            return;
        }

        size_t frame_size = sizeof(FrameHeader) + length;

        if (input_length < frame_size)
            return;

        process_frame(
            type,
            input_buffer + sizeof(FrameHeader),
            length
        );

        memmove(
            input_buffer,
            input_buffer + frame_size,
            input_length - frame_size
        );

        input_length -= frame_size;
    }
}

// Receive available data from the server.
int receive_data(void)
{
    if (input_length == sizeof(input_buffer))
        return -1;

    ssize_t received = recv(
        sockfd,
        input_buffer + input_length,
        sizeof(input_buffer) - input_length,
        0
    );

    if (received <= 0)
        return -1;

    input_length += (size_t)received;

    process_received_data();

    return running ? 0 : -1;
}

// Send a file to the server.
int send_file(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    if (fseeko(file, 0, SEEK_END) != 0) {
        perror("fseeko");
        fclose(file);
        return -1;
    }

    off_t size = ftello(file);

    if (size < 0) {
        perror("ftello");
        fclose(file);
        return -1;
    }

    rewind(file);

    const char *filename = safe_filename(path);

    size_t name_length = strlen(filename);

    if (name_length == 0 || name_length > MAX_FILENAME) {
        printf("Invalid filename.\n");
        fclose(file);
        return -1;
    }

    // FILE_BEGIN:
    // 4 bytes filename length
    // 4 bytes file size high
    // 4 bytes file size low
    // filename

    unsigned char metadata[12 + MAX_FILENAME];

    uint32_t name_net = htonl((uint32_t)name_length);

    uint64_t file_size = (uint64_t)size;

    uint32_t high = htonl((uint32_t)(file_size >> 32));
    uint32_t low = htonl((uint32_t)file_size);

    memcpy(metadata, &name_net, 4);
    memcpy(metadata + 4, &high, 4);
    memcpy(metadata + 8, &low, 4);

    memcpy(
        metadata + 12,
        filename,
        name_length
    );

    printf("Sending file: %s\n", filename);

    if (send_frame(
        MSG_FILE_BEGIN,
        metadata,
        (uint32_t)(12 + name_length)
    ) < 0) {

        fclose(file);
        return -1;
    }

    // Send file in smaller chunks.
    unsigned char buffer[FILE_CHUNK];

    size_t bytes_read;

    while ((bytes_read = fread(
                buffer,
                1,
                sizeof(buffer),
                file
            )) > 0) {

        if (send_frame(
            MSG_FILE_DATA,
            buffer,
            (uint32_t)bytes_read
        ) < 0) {

            fclose(file);
            return -1;
        }
    }

    if (ferror(file)) {
        perror("fread");
        fclose(file);
        return -1;
    }

    fclose(file);

    if (send_frame(MSG_FILE_END, NULL, 0) < 0)
        return -1;

    printf("File sent successfully.\n");

    return 0;
}

// Handle keyboard commands.
void process_input(char *input)
{
    input[strcspn(input, "\n")] = '\0';

    if (strlen(input) == 0)
        return;

    if (strcmp(input, "/quit") == 0) {

        running = 0;
        return;
    }

    if (strncmp(input, "/file ", 6) == 0) {

        if (input[6] == '\0') {
            printf("Usage: /file filename\n");
            return;
        }

        if (send_file(input + 6) < 0) {
            printf("File transfer failed.\n");
            running = 0;
        }

        return;
    }

    if (send_frame(
        MSG_CHAT,
        input,
        (uint32_t)strlen(input)
    ) < 0) {

        printf("Failed to send message.\n");
        running = 0;
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, stop_client);
    signal(SIGTERM, stop_client);
    signal(SIGPIPE, SIG_IGN);

    if (argc < 3) {

        printf("Usage: %s <server_ip> <username>\n", argv[0]);

        return 1;
    }

    if (strlen(argv[2]) == 0 ||
        strlen(argv[2]) >= MAX_NAME) {

        printf("Username must contain 1-31 characters.\n");
        return 1;
    }

    // Create TCP socket.
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(
        AF_INET,
        argv[1],
        &server_addr.sin_addr
    ) != 1) {

        printf("Invalid server IP address.\n");
        close(sockfd);
        return 1;
    }

    // Connect to server.
    if (connect(
        sockfd,
        (struct sockaddr *)&server_addr,
        sizeof(server_addr)
    ) < 0) {

        perror("connect");
        close(sockfd);
        return 1;
    }

    printf("Connected to TCP chat server.\n");

    // Send username to server.
    if (send_frame(
        MSG_NAME,
        argv[2],
        (uint32_t)strlen(argv[2])
    ) < 0) {

        perror("send");
        close(sockfd);
        return 1;
    }

    printf("Username: %s\n", argv[2]);
    printf("Commands:\n");
    printf("  /file filename - send a file\n");
    printf("  /quit          - leave chat\n");
    printf("--------------------------------\n");

    char input[INPUT_SIZE];

    while (running) {

        fd_set readfds;

        FD_ZERO(&readfds);

        // Monitor keyboard.
        FD_SET(STDIN_FILENO, &readfds);

        // Monitor TCP socket.
        FD_SET(sockfd, &readfds);

        int max_fd = sockfd > STDIN_FILENO
                   ? sockfd
                   : STDIN_FILENO;

        int result = select(
            max_fd + 1,
            &readfds,
            NULL,
            NULL,
            NULL
        );

        if (result < 0) {

            if (errno == EINTR)
                continue;

            perror("select");
            break;
        }

        // Check incoming network data.
        if (FD_ISSET(sockfd, &readfds)) {

            if (receive_data() < 0) {

                printf("\nServer disconnected.\n");
                break;
            }
        }

        // Check keyboard input.
        if (FD_ISSET(STDIN_FILENO, &readfds)) {

            if (fgets(input, sizeof(input), stdin) == NULL) {
                running = 0;
                break;
            }

            process_input(input);
        }
    }

    if (received_file != NULL)
        fclose(received_file);

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    printf("\nDisconnected from chat.\n");

    return 0;
}
