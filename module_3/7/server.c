
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

typedef struct Output {
    unsigned char *data;
    size_t length;
    size_t sent;
    struct Output *next;
} Output;

typedef struct {
    int fd;
    char name[MAX_NAME];

    unsigned char input[BUFFER_SIZE];
    size_t input_length;

    Output *out_head;
    Output *out_tail;

} Client;

static Client clients[MAX_CLIENTS];
static int server_fd = -1;
static volatile sig_atomic_t running = 1;

void stop_server(int sig)
{
    (void)sig;
    running = 0;
}

int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void remove_client(int index);

void clear_output(Client *client)
{
    Output *current = client->out_head;

    while (current != NULL) {
        Output *next = current->next;

        free(current->data);
        free(current);

        current = next;
    }

    client->out_head = NULL;
    client->out_tail = NULL;
}

// Add a complete frame to a client's outgoing queue.
int queue_data(Client *client,
               const unsigned char *data,
               size_t length)
{
    Output *item = malloc(sizeof(Output));

    if (!item)
        return -1;

    item->data = malloc(length);

    if (!item->data) {
        free(item);
        return -1;
    }

    memcpy(item->data, data, length);

    item->length = length;
    item->sent = 0;
    item->next = NULL;

    if (client->out_tail == NULL) {
        client->out_head = item;
    } else {
        client->out_tail->next = item;
    }

    client->out_tail = item;

    return 0;
}

// Construct and broadcast a frame.
void broadcast_frame(int sender,
                     uint32_t type,
                     const void *data,
                     uint32_t length)
{
    size_t total = sizeof(FrameHeader) + length;

    unsigned char *frame = malloc(total);

    if (!frame)
        return;

    FrameHeader header;

    header.type = htonl(type);
    header.length = htonl(length);

    memcpy(frame, &header, sizeof(header));

    if (length > 0)
        memcpy(frame + sizeof(header), data, length);

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i].fd < 0)
            continue;

        // Do not send a client's own messages back to it.
        if (i == sender)
            continue;

        if (queue_data(&clients[i], frame, total) < 0) {
            remove_client(i);
        }
    }

    free(frame);
}

void broadcast_system(const char *message)
{
    broadcast_frame(
        -1,
        MSG_SYSTEM,
        message,
        (uint32_t)strlen(message)
    );
}

void remove_client(int index)
{
    if (clients[index].fd < 0)
        return;

    char message[128];

    snprintf(
        message,
        sizeof(message),
        "%s disconnected",
        clients[index].name
    );

    printf("%s\n", message);

    close(clients[index].fd);

    clients[index].fd = -1;
    clients[index].input_length = 0;

    clear_output(&clients[index]);

    memset(clients[index].name, 0, MAX_NAME);

    broadcast_system(message);
}

// Process one complete frame received from a client.
void process_frame(int index,
                   uint32_t type,
                   const unsigned char *data,
                   uint32_t length)
{
    Client *client = &clients[index];

    switch (type) {

        case MSG_NAME:
        {
            if (length == 0 || length >= MAX_NAME)
                break;

            memcpy(client->name, data, length);
            client->name[length] = '\0';

            char message[128];

            snprintf(
                message,
                sizeof(message),
                "%s joined the chat",
                client->name
            );

            printf("%s\n", message);

            broadcast_system(message);
            break;
        }

        case MSG_CHAT:
        {
            if (client->name[0] == '\0')
                break;

            if (length > 4096)
                break;

            char message[4200];

            snprintf(
                message,
                sizeof(message),
                "[%s]: %.*s",
                client->name,
                (int)length,
                (const char *)data
            );

            printf("%s\n", message);

            broadcast_frame(
                index,
                MSG_CHAT,
                message,
                (uint32_t)strlen(message)
            );

            break;
        }

        case MSG_FILE_BEGIN:
        case MSG_FILE_DATA:
        case MSG_FILE_END:
        {
            if (client->name[0] == '\0')
                break;

            if (type == MSG_FILE_BEGIN) {
                printf(
                    "%s is sending a file\n",
                    client->name
                );
            }

            // Forward file information and binary data
            // without modifying the payload.
            broadcast_frame(index, type, data, length);

            break;
        }

        default:
            break;
    }
}

// Read available TCP data and extract complete frames.
void receive_from_client(int index)
{
    Client *client = &clients[index];

    if (client->input_length == sizeof(client->input)) {
        remove_client(index);
        return;
    }

    ssize_t received = recv(
        client->fd,
        client->input + client->input_length,
        sizeof(client->input) - client->input_length,
        0
    );

    if (received <= 0) {

        if (received == 0) {
            remove_client(index);
            return;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        if (errno == EINTR)
            return;

        remove_client(index);
        return;
    }

    client->input_length += (size_t)received;

    while (client->input_length >= sizeof(FrameHeader)) {

        FrameHeader header;

        memcpy(
            &header,
            client->input,
            sizeof(header)
        );

        uint32_t type = ntohl(header.type);
        uint32_t length = ntohl(header.length);

        if (length > MAX_DATA + MAX_FILENAME + 12) {
            remove_client(index);
            return;
        }

        size_t frame_size = sizeof(FrameHeader) + length;

        if (client->input_length < frame_size)
            break;

        process_frame(
            index,
            type,
            client->input + sizeof(FrameHeader),
            length
        );

        if (clients[index].fd < 0)
            return;

        memmove(
            client->input,
            client->input + frame_size,
            client->input_length - frame_size
        );

        client->input_length -= frame_size;
    }
}

// Send queued data without blocking.
void send_to_client(int index)
{
    Client *client = &clients[index];

    while (client->out_head != NULL) {

        Output *item = client->out_head;

        ssize_t sent = send(
            client->fd,
            item->data + item->sent,
            item->length - item->sent,
            MSG_NOSIGNAL
        );

        if (sent < 0) {

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            if (errno == EINTR)
                continue;

            remove_client(index);
            return;
        }

        if (sent == 0) {
            remove_client(index);
            return;
        }

        item->sent += (size_t)sent;

        if (item->sent < item->length)
            return;

        client->out_head = item->next;

        if (client->out_head == NULL)
            client->out_tail = NULL;

        free(item->data);
        free(item);
    }
}

int main(void)
{
    signal(SIGINT, stop_server);
    signal(SIGTERM, stop_server);
    signal(SIGPIPE, SIG_IGN);

    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    // Create TCP socket.
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int reuse = 1;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );

    struct sockaddr_in address;

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(
        server_fd,
        (struct sockaddr *)&address,
        sizeof(address)
    ) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    if (set_nonblocking(server_fd) < 0) {
        perror("fcntl");
        close(server_fd);
        return 1;
    }

    printf("TCP CHAT SERVER STARTED\n");
    printf("Port: %d\n", PORT);
    printf("Waiting for clients...\n");

    while (running) {

        struct pollfd fds[MAX_CLIENTS + 1];

        int indices[MAX_CLIENTS + 1];

        int count = 1;

        fds[0].fd = server_fd;
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        indices[0] = -1;

        // Add connected clients to poll.
        for (int i = 0; i < MAX_CLIENTS; i++) {

            if (clients[i].fd < 0)
                continue;

            fds[count].fd = clients[i].fd;

            fds[count].events = POLLIN;

            if (clients[i].out_head != NULL)
                fds[count].events |= POLLOUT;

            fds[count].revents = 0;

            indices[count] = i;

            count++;
        }

        int result = poll(fds, count, 500);

        if (result < 0) {

            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        // New connection.
        if (fds[0].revents & POLLIN) {

            while (1) {

                int new_fd = accept(server_fd, NULL, NULL);

                if (new_fd < 0) {

                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK)
                        break;

                    if (errno == EINTR)
                        continue;

                    perror("accept");
                    break;
                }

                if (set_nonblocking(new_fd) < 0) {
                    close(new_fd);
                    continue;
                }

                int slot = -1;

                for (int i = 0; i < MAX_CLIENTS; i++) {

                    if (clients[i].fd < 0) {
                        slot = i;
                        break;
                    }
                }

                if (slot < 0) {
                    close(new_fd);
                    continue;
                }

                clients[slot].fd = new_fd;
                clients[slot].input_length = 0;
                clients[slot].out_head = NULL;
                clients[slot].out_tail = NULL;

                memset(
                    clients[slot].name,
                    0,
                    MAX_NAME
                );

                printf("New TCP connection accepted\n");
            }
        }

        // Handle connected clients.
        for (int i = 1; i < count; i++) {

            int index = indices[i];

            if (clients[index].fd < 0)
                continue;

            short events = fds[i].revents;

            if (events & (POLLERR | POLLHUP | POLLNVAL)) {
                remove_client(index);
                continue;
            }

            if (events & POLLIN) {
                receive_from_client(index);
            }

            if (clients[index].fd >= 0 &&
                (events & POLLOUT)) {

                send_to_client(index);
            }
        }
    }

    printf("\nStopping server...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i].fd >= 0)
            remove_client(i);
    }

    close(server_fd);

    return 0;
}
