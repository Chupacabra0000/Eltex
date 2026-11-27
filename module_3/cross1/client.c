
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define BUFFER_SIZE 65536
#define MAX_MESSAGE 1400
#define CLOSE_MESSAGE "__RAW_ECHO_CLOSE__"

#pragma pack(push, 1)
typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t zero;
    uint8_t protocol;
    uint16_t udp_length;
} PseudoHeader;
#pragma pack(pop)

static volatile sig_atomic_t running = 1;

/* ---------- SIGNAL HANDLING ---------- */

void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* ---------- CHECKSUM ---------- */

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

    unsigned short result =
        calculate_checksum(buffer, total_size);

    free(buffer);

    return result ? result : htons(0xFFFF);
}

/* ---------- RAW UDP SENDING ---------- */

int send_message(
    int sockfd,
    struct in_addr local_ip,
    struct in_addr server_ip,
    unsigned short client_port,
    unsigned short server_port,
    const char *message)
{
    unsigned char packet[
        sizeof(struct udphdr) + MAX_MESSAGE
    ];

    int message_length = strlen(message);

    int udp_length =
        sizeof(struct udphdr) + message_length;

    if (udp_length > sizeof(packet))
        return -1;

    memset(packet, 0, udp_length);

    struct udphdr *udp = (struct udphdr *)packet;

    udp->source = client_port;
    udp->dest = server_port;
    udp->len = htons(udp_length);
    udp->check = 0;

    memcpy(packet + sizeof(struct udphdr),
           message, message_length);

    udp->check = udp_checksum(
        local_ip,
        server_ip,
        packet,
        udp_length
    );

    struct sockaddr_in destination = {0};

    destination.sin_family = AF_INET;
    destination.sin_addr = server_ip;

    ssize_t sent = sendto(
        sockfd,
        packet,
        udp_length,
        0,
        (struct sockaddr *)&destination,
        sizeof(destination)
    );

    if (sent != udp_length)
        return -1;

    return 0;
}

/* ---------- RECEIVE SERVER RESPONSE ---------- */

void receive_response(
    int sockfd,
    struct in_addr server_ip,
    unsigned short server_port,
    unsigned short client_port)
{
    unsigned char buffer[BUFFER_SIZE];

    ssize_t received = recvfrom(
        sockfd,
        buffer,
        sizeof(buffer),
        0,
        NULL,
        NULL
    );

    if (received < 0) {
        if (errno != EINTR)
            perror("recvfrom");

        return;
    }

    if (received < (ssize_t)sizeof(struct iphdr))
        return;

    struct iphdr *ip = (struct iphdr *)buffer;

    int ip_length = ip->ihl * 4;

    if (ip->version != 4 ||
        ip->protocol != IPPROTO_UDP ||
        ip_length < (int)sizeof(struct iphdr) ||
        received < ip_length + (int)sizeof(struct udphdr)) {
        return;
    }

    if (ntohs(ip->frag_off) & 0x3FFF)
        return;

    int total_ip_length = ntohs(ip->tot_len);

    if (total_ip_length < ip_length + (int)sizeof(struct udphdr) ||
        total_ip_length > received) {
        return;
    }

    struct udphdr *udp =
        (struct udphdr *)(buffer + ip_length);

    /* Accept only packets from our server to our client port. */
    if (ip->saddr != server_ip.s_addr ||
        udp->source != server_port ||
        udp->dest != client_port) {
        return;
    }

    int udp_length = ntohs(udp->len);

    if (udp_length < (int)sizeof(struct udphdr) ||
        udp_length != total_ip_length - ip_length) {
        return;
    }

    int message_length =
        udp_length - sizeof(struct udphdr);

    if (message_length <= 0 ||
        message_length >= MAX_MESSAGE) {
        return;
    }

    char response[MAX_MESSAGE];

    memcpy(
        response,
        buffer + ip_length + sizeof(struct udphdr),
        message_length
    );

    response[message_length] = '\0';

    printf("\n[Server]: %s\n", response);
    printf("[You]: ");
    fflush(stdout);
}

/* ---------- MAIN ---------- */

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("Usage: %s <server_ip> <server_port> <client_port>\n",
               argv[0]);
        return 1;
    }

    /* Validate ports. */
    int server_port_number = atoi(argv[2]);
    int client_port_number = atoi(argv[3]);

    if (server_port_number < 1 ||
        server_port_number > 65535 ||
        client_port_number < 1 ||
        client_port_number > 65535) {
        printf("Invalid port number\n");
        return 1;
    }

    unsigned short server_port = htons(server_port_number);
    unsigned short client_port = htons(client_port_number);

    /* Prepare server address. */
    struct sockaddr_in server_address = {0};

    server_address.sin_family = AF_INET;
    server_address.sin_port = server_port;

    if (inet_pton(AF_INET, argv[1],
                  &server_address.sin_addr) != 1) {
        printf("Invalid server IP address\n");
        return 1;
    }

    /* Install signal handlers. */
    struct sigaction sa = {0};

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Create raw socket. */
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

    /* Bind guard to the selected client port. */
    struct sockaddr_in local = {0};

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = client_port;

    if (bind(guard_fd,
             (struct sockaddr *)&local,
             sizeof(local)) < 0) {

        perror("guard bind");

        close(guard_fd);
        close(raw_fd);

        return 1;
    }

    /*
     * Connect only the ordinary UDP guard socket.
     * This determines the local IP address.
     */
    if (connect(guard_fd,
                (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0) {

        perror("guard connect");

        close(guard_fd);
        close(raw_fd);

        return 1;
    }

    struct sockaddr_in local_address = {0};

    socklen_t local_length = sizeof(local_address);

    if (getsockname(guard_fd,
                    (struct sockaddr *)&local_address,
                    &local_length) < 0) {

        perror("getsockname");

        close(guard_fd);
        close(raw_fd);

        return 1;
    }

    struct in_addr local_ip = local_address.sin_addr;

    char local_ip_string[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &local_ip,
              local_ip_string, sizeof(local_ip_string));

    /* Display connection information. */
    printf("\n========== RAW ECHO CLIENT ==========\n");

    printf("Server: %s:%d\n",
           argv[1], server_port_number);

    printf("Client: %s:%d\n",
           local_ip_string, client_port_number);

    printf("Type messages and press Enter.\n");
    printf("Type /quit or press Ctrl+C to exit.\n");

    printf("=====================================\n\n");

    /* Poll monitors both keyboard and raw socket. */
    struct pollfd fds[2];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = raw_fd;
    fds[1].events = POLLIN;

    char message[MAX_MESSAGE];

    while (running) {

        int ready = poll(fds, 2, 500);

        if (ready < 0) {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        if (ready == 0)
            continue;

        /* Keyboard input. */
        if (fds[0].revents & (POLLIN | POLLHUP)) {

            if (fgets(message, sizeof(message), stdin) == NULL) {
                running = 0;
                break;
            }

            message[strcspn(message, "\n")] = '\0';

            if (strcmp(message, "/quit") == 0) {
                running = 0;
                break;
            }

            if (strlen(message) == 0) {
                printf("[You]: ");
                fflush(stdout);
                continue;
            }

            if (send_message(
                    raw_fd,
                    local_ip,
                    server_address.sin_addr,
                    client_port,
                    server_port,
                    message) < 0) {

                perror("send_message");

            } else {

                printf("[You]: %s\n", message);
                printf("[You]: ");
                fflush(stdout);
            }
        }

        /* Incoming server response. */
        if (fds[1].revents & POLLIN) {

            receive_response(
                raw_fd,
                server_address.sin_addr,
                server_port,
                client_port
            );
        }

        if (fds[0].revents & (POLLERR | POLLNVAL))
            break;

        if (fds[1].revents & (POLLERR | POLLNVAL))
            break;
    }

    /* Graceful client shutdown. */
    printf("\n[CLIENT] Sending closing notification...\n");

    for (int i = 0; i < 3; i++) {

        if (send_message(
                raw_fd,
                local_ip,
                server_address.sin_addr,
                client_port,
                server_port,
                CLOSE_MESSAGE) < 0) {

            perror("send close message");
            break;
        }

        /* Small delay between UDP transmissions. */
        struct timespec delay = {
            .tv_sec = 0,
            .tv_nsec = 100000000
        };

        nanosleep(&delay, NULL);
    }

    close(guard_fd);
    close(raw_fd);

    printf("[CLIENT] Disconnected.\n");

    return 0;
}
