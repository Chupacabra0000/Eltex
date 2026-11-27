
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define BUFFER_SIZE 65536
#define MAX_MESSAGE 1400
#define CLOSE_MESSAGE "__RAW_ECHO_CLOSE__"

typedef struct Client {
    struct in_addr ip;
    unsigned short port;
    unsigned int counter;
    struct Client *next;
} Client;

#pragma pack(push, 1)
typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t zero;
    uint8_t protocol;
    uint16_t udp_length;
} PseudoHeader;
#pragma pack(pop)

static Client *clients = NULL;
static volatile sig_atomic_t running = 1;

/* ---------- SIGNAL HANDLING ---------- */

void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* ---------- CHECKSUM ---------- */

/* Calculates Internet checksum from network-order bytes. */
unsigned short calculate_checksum(const void *data, int length)
{
    const unsigned char *ptr = data;
    uint32_t sum = 0;

    while (length > 1) {
        sum += ((uint16_t)ptr[0] << 8) | ptr[1];
        ptr += 2;
        length -= 2;
    }

    if (length == 1) {
        sum += (uint16_t)ptr[0] << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return htons((unsigned short)(~sum));
}

/* Calculates UDP checksum using IPv4 pseudo-header. */
unsigned short udp_checksum(
    struct in_addr src,
    struct in_addr dst,
    unsigned char *udp_data,
    int udp_length)
{
    int total_size = sizeof(PseudoHeader) + udp_length;

    unsigned char *buffer = calloc(1, total_size);

    if (!buffer) {
        perror("calloc");
        return 0;
    }

    PseudoHeader pseudo = {0};

    pseudo.src_ip = src.s_addr;
    pseudo.dst_ip = dst.s_addr;
    pseudo.zero = 0;
    pseudo.protocol = IPPROTO_UDP;
    pseudo.udp_length = htons(udp_length);

    memcpy(buffer, &pseudo, sizeof(pseudo));

    memcpy(buffer + sizeof(pseudo),
           udp_data, udp_length);

    unsigned short result = calculate_checksum(buffer, total_size);

    free(buffer);

    /* A zero UDP checksum means no checksum for IPv4. */
    return result ? result : htons(0xFFFF);
}

/* ---------- LINKED LIST (САОД) ---------- */

Client *find_client(struct in_addr ip, unsigned short port)
{
    Client *current = clients;

    while (current != NULL) {
        if (current->ip.s_addr == ip.s_addr &&
            current->port == port) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

Client *add_client(struct in_addr ip, unsigned short port)
{
    Client *client = malloc(sizeof(Client));

    if (!client) {
        perror("malloc");
        return NULL;
    }

    client->ip = ip;
    client->port = port;
    client->counter = 0;

    client->next = clients;
    clients = client;

    return client;
}

void remove_client(struct in_addr ip, unsigned short port)
{
    Client **current = &clients;

    while (*current != NULL) {
        Client *client = *current;

        if (client->ip.s_addr == ip.s_addr &&
            client->port == port) {

            *current = client->next;

            char address[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &ip,
                      address, sizeof(address));

            printf("[SERVER] Client removed: %s:%u\n",
                   address, ntohs(port));

            free(client);
            return;
        }

        current = &client->next;
    }
}

void free_clients(void)
{
    while (clients != NULL) {
        Client *temp = clients;

        clients = clients->next;

        free(temp);
    }
}

/* ---------- RAW UDP TRANSMISSION ---------- */

int send_response(
    int sockfd,
    struct in_addr src_ip,
    struct in_addr dst_ip,
    unsigned short src_port,
    unsigned short dst_port,
    const char *message)
{
    unsigned char packet[
        sizeof(struct udphdr) + MAX_MESSAGE
    ];

    int message_length = strlen(message);

    int udp_length =
        sizeof(struct udphdr) + message_length;

    if (udp_length > sizeof(packet)) {
        return -1;
    }

    memset(packet, 0, udp_length);

    struct udphdr *udp = (struct udphdr *)packet;

    udp->source = src_port;
    udp->dest = dst_port;
    udp->len = htons(udp_length);
    udp->check = 0;

    memcpy(packet + sizeof(struct udphdr),
           message, message_length);

    udp->check = udp_checksum(
        src_ip,
        dst_ip,
        packet,
        udp_length
    );

    struct sockaddr_in destination = {0};

    destination.sin_family = AF_INET;
    destination.sin_addr = dst_ip;

    ssize_t sent = sendto(
        sockfd,
        packet,
        udp_length,
        0,
        (struct sockaddr *)&destination,
        sizeof(destination)
    );

    if (sent != udp_length) {
        return -1;
    }

    return 0;
}

/* ---------- MAIN ---------- */

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <server_port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    if (port < 1 || port > 65535) {
        printf("Invalid server port\n");
        return 1;
    }

    unsigned short server_port = htons(port);

    struct sigaction sa = {0};

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Create raw UDP socket. */
    int raw_fd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    if (raw_fd < 0) {
        perror("raw socket");
        return 1;
    }

    /* Create UDP guard socket. */
    int guard_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (guard_fd < 0) {
        perror("guard socket");
        close(raw_fd);
        return 1;
    }

    struct sockaddr_in local = {0};

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = server_port;

    if (bind(guard_fd,
             (struct sockaddr *)&local,
             sizeof(local)) < 0) {

        perror("guard bind");

        close(guard_fd);
        close(raw_fd);

        return 1;
    }

    printf("========== RAW ECHO SERVER ==========\n");
    printf("Listening on UDP port: %d\n", port);
    printf("Press Ctrl+C to stop.\n");
    printf("=====================================\n\n");

    unsigned char buffer[BUFFER_SIZE];

    struct pollfd pfd = {
        .fd = raw_fd,
        .events = POLLIN
    };

    while (running) {

        int ready = poll(&pfd, 1, 500);

        if (ready < 0) {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        if (ready == 0)
            continue;

        if (!(pfd.revents & POLLIN))
            continue;

        ssize_t received = recvfrom(
            raw_fd,
            buffer,
            sizeof(buffer),
            0,
            NULL,
            NULL
        );

        if (received < 0) {
            if (errno == EINTR)
                continue;

            perror("recvfrom");
            continue;
        }

        /* Validate IPv4 header. */
        if (received < (ssize_t)sizeof(struct iphdr))
            continue;

        struct iphdr *ip = (struct iphdr *)buffer;

        int ip_length = ip->ihl * 4;

        if (ip->version != 4 ||
            ip->protocol != IPPROTO_UDP ||
            ip_length < (int)sizeof(struct iphdr) ||
            received < ip_length + (int)sizeof(struct udphdr)) {
            continue;
        }

        /* Ignore fragmented packets. */
        if (ntohs(ip->frag_off) & 0x3FFF)
            continue;

        int total_ip_length = ntohs(ip->tot_len);

        if (total_ip_length < ip_length + (int)sizeof(struct udphdr) ||
            total_ip_length > received) {
            continue;
        }

        struct udphdr *udp =
            (struct udphdr *)(buffer + ip_length);

        if (udp->dest != server_port)
            continue;

        int udp_length = ntohs(udp->len);

        if (udp_length < (int)sizeof(struct udphdr) ||
            udp_length != total_ip_length - ip_length) {
            continue;
        }

        int message_length =
            udp_length - sizeof(struct udphdr);

        if (message_length <= 0 ||
            message_length >= MAX_MESSAGE) {
            continue;
        }

        char message[MAX_MESSAGE];

        memcpy(
            message,
            buffer + ip_length + sizeof(struct udphdr),
            message_length
        );

        message[message_length] = '\0';

        struct in_addr client_ip = {
            .s_addr = ip->saddr
        };

        struct in_addr server_ip = {
            .s_addr = ip->daddr
        };

        unsigned short client_port = udp->source;

        char address[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &client_ip,
                  address, sizeof(address));

        printf("[SERVER] Received from %s:%u: %s\n",
               address,
               ntohs(client_port),
               message);

        /* Handle client disconnection. */
        if (strcmp(message, CLOSE_MESSAGE) == 0) {
            remove_client(client_ip, client_port);
            continue;
        }

        /* Find or create client. */
        Client *client = find_client(
            client_ip,
            client_port
        );

        if (!client) {
            client = add_client(
                client_ip,
                client_port
            );

            if (!client)
                continue;

            printf("[SERVER] New client: %s:%u\n",
                   address, ntohs(client_port));
        }

        client->counter++;

        char response[MAX_MESSAGE];

        int response_length = snprintf(
            response,
            sizeof(response),
            "%s %u",
            message,
            client->counter
        );

        if (response_length < 0 ||
            response_length >= sizeof(response)) {
            continue;
        }

        printf("[SERVER] Sending: %s\n", response);

        if (send_response(
                raw_fd,
                server_ip,
                client_ip,
                server_port,
                client_port,
                response) < 0) {
            perror("send_response");
        }
    }

    printf("\n[SERVER] Shutting down...\n");

    free_clients();

    close(guard_fd);
    close(raw_fd);

    printf("[SERVER] Resources released.\n");

    return 0;
}
