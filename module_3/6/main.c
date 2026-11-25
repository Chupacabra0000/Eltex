
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 50000
#define BUFFER_SIZE 512
#define NAME_SIZE 32

#define TYPE_JOIN  1
#define TYPE_CHAT  2
#define TYPE_LEAVE 3

// Structure of a message sent over the network
typedef struct {
    uint32_t type;
    uint32_t id;
    char name[NAME_SIZE];
    char text[BUFFER_SIZE];
} Packet;

// Global variables
int sockfd;
uint32_t client_id;

volatile sig_atomic_t running = 1;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

struct sockaddr_in broadcast_addr;

// Handle Ctrl+C and SIGTERM
void signal_handler(int signum)
{
    (void)signum;
    running = 0;
}

// Print messages without mixing output from threads
void print_message(const char *message)
{
    pthread_mutex_lock(&print_mutex);

    printf("%s\n", message);
    fflush(stdout);

    pthread_mutex_unlock(&print_mutex);
}

// Send a packet to all clients
int send_packet(uint32_t type, const char *text)
{
    Packet packet;

    memset(&packet, 0, sizeof(packet));

    packet.type = type;
    packet.id = client_id;

    // Store client's name
    // The name is initialized separately in main
    extern char client_name[NAME_SIZE];

    strncpy(packet.name, client_name, NAME_SIZE - 1);

    if (text != NULL) {
        strncpy(packet.text, text, BUFFER_SIZE - 1);
    }

    ssize_t sent = sendto(
        sockfd,
        &packet,
        sizeof(packet),
        0,
        (struct sockaddr *)&broadcast_addr,
        sizeof(broadcast_addr)
    );

    if (sent < 0) {
        perror("sendto");
        return -1;
    }

    return 0;
}

char client_name[NAME_SIZE];

// Thread responsible for receiving messages
void *receive_messages(void *arg)
{
    (void)arg;

    Packet packet;

    while (running) {

        fd_set readfds;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        // Wait for incoming messages, with a timeout
        struct timeval timeout;

        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        int result = select(
            sockfd + 1,
            &readfds,
            NULL,
            NULL,
            &timeout
        );

        if (result < 0) {

            if (errno == EINTR) {
                continue;
            }

            perror("select");
            break;
        }

        if (result == 0) {
            continue;
        }

        // Receive a UDP packet
        ssize_t received = recvfrom(
            sockfd,
            &packet,
            sizeof(packet),
            0,
            NULL,
            NULL
        );

        if (received < 0) {

            if (errno == EINTR) {
                continue;
            }

            perror("recvfrom");
            continue;
        }

        // Ignore incomplete or invalid packets
        if (received != sizeof(packet)) {
            continue;
        }

        packet.name[NAME_SIZE - 1] = '\0';
        packet.text[BUFFER_SIZE - 1] = '\0';

        // Do not display our own messages
        if (packet.id == client_id) {
            continue;
        }

        char output[BUFFER_SIZE + NAME_SIZE + 100];

        switch (packet.type) {

            case TYPE_JOIN:

                snprintf(
                    output,
                    sizeof(output),
                    "[SYSTEM] %s has joined the chat.",
                    packet.name
                );

                print_message(output);
                break;

            case TYPE_CHAT:

                snprintf(
                    output,
                    sizeof(output),
                    "[%s]: %s",
                    packet.name,
                    packet.text
                );

                print_message(output);
                break;

            case TYPE_LEAVE:

                snprintf(
                    output,
                    sizeof(output),
                    "[SYSTEM] %s has left the chat.",
                    packet.name
                );

                print_message(output);
                break;

            default:
                break;
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // Get the client's name
    if (argc >= 2) {

        strncpy(client_name, argv[1], NAME_SIZE - 1);
        client_name[NAME_SIZE - 1] = '\0';

    } else {

        snprintf(
            client_name,
            sizeof(client_name),
            "Client-%d",
            getpid()
        );
    }

    // Generate a simple client identifier
    client_id = (uint32_t)time(NULL)
                ^ ((uint32_t)getpid() << 16)
                ^ (uint32_t)getuid();

    // Register signal handlers
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Create UDP socket
    sockfd = socket(
        AF_INET,
        SOCK_DGRAM,
        0
    );

    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // Enable address reuse
    int reuse = 1;

    if (setsockopt(
            sockfd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)
        ) < 0) {

        perror("setsockopt SO_REUSEADDR");
        close(sockfd);
        return 1;
    }

    // Enable broadcast
    int broadcast = 1;

    if (setsockopt(
            sockfd,
            SOL_SOCKET,
            SO_BROADCAST,
            &broadcast,
            sizeof(broadcast)
        ) < 0) {

        perror("setsockopt SO_BROADCAST");
        close(sockfd);
        return 1;
    }

    // Configure local address
    struct sockaddr_in local_addr;

    memset(&local_addr, 0, sizeof(local_addr));

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket to port
    if (bind(
            sockfd,
            (struct sockaddr *)&local_addr,
            sizeof(local_addr)
        ) < 0) {

        perror("bind");
        close(sockfd);
        return 1;
    }

    // Configure broadcast destination
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));

    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);

    inet_pton(
        AF_INET,
        "255.255.255.255",
        &broadcast_addr.sin_addr
    );

    // Create receiving thread
    pthread_t receiver_thread;

    if (pthread_create(
            &receiver_thread,
            NULL,
            receive_messages,
            NULL
        ) != 0) {

        perror("pthread_create");
        close(sockfd);
        return 1;
    }

    printf("=================================\n");
    printf("       UDP GROUP CHAT\n");
    printf("=================================\n");
    printf("Your name: %s\n", client_name);
    printf("UDP port: %d\n", PORT);
    printf("Type /quit to leave the chat.\n");
    printf("---------------------------------\n");

    // Announce that a new client has joined
    send_packet(TYPE_JOIN, NULL);

    char message[BUFFER_SIZE];

    while (running) {

        printf("> ");
        fflush(stdout);

        if (fgets(message, sizeof(message), stdin) == NULL) {
            break;
        }

        // Remove newline character
        message[strcspn(message, "\n")] = '\0';

        // Ignore empty messages
        if (strlen(message) == 0) {
            continue;
        }

        // Leave the chat
        if (strcmp(message, "/quit") == 0) {
            break;
        }

        // Send chat message to all clients
        send_packet(TYPE_CHAT, message);
    }

    // Announce that the client is leaving
    send_packet(TYPE_LEAVE, NULL);

    // Stop receiving thread
    running = 0;

    pthread_join(receiver_thread, NULL);

    // Close socket
    close(sockfd);

    pthread_mutex_destroy(&print_mutex);

    printf("\nYou have left the chat.\n");

    return 0;
}
