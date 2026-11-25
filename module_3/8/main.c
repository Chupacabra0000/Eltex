
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <poll.h>

#include <sys/socket.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define BUFFER_SIZE 65536
#define CHAT_PORT 50000
#define DNS_PORT 53

#define FILTER_CHAT 1
#define FILTER_DNS  2
#define FILTER_ALL  3

static volatile sig_atomic_t running = 1;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

/* Return elapsed seconds since capture started. */
static double elapsed_time(struct timespec start)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    return (double)(now.tv_sec - start.tv_sec) +
           (double)(now.tv_nsec - start.tv_nsec) / 1000000000.0;
}

/* Print a MAC address. */
static void print_mac(const unsigned char *mac)
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

/* Convert an IPv4 address into readable text. */
static void ip_to_string(uint32_t address, char *buffer, size_t size)
{
    struct in_addr ip;

    ip.s_addr = address;
    inet_ntop(AF_INET, &ip, buffer, size);
}

/* Print payload as readable ASCII. */
static void print_ascii(const unsigned char *data, size_t length)
{
    size_t i;

    for (i = 0; i < length; i++) {
        unsigned char c = data[i];

        if (c >= 32 && c <= 126)
            putchar(c);
        else
            putchar('.');
    }

    putchar('\n');
}

/* Print payload as hexadecimal bytes. */
static void print_hex(const unsigned char *data, size_t length)
{
    size_t i;

    for (i = 0; i < length; i++) {
        printf("%02X ", data[i]);

        if ((i + 1) % 16 == 0)
            printf("\n");
    }

    if (length % 16 != 0)
        printf("\n");
}

/*
 * Decode the chat packet from Task 6.
 *
 * This assumes Task 6 uses:
 *
 * typedef struct {
 *     uint32_t type;
 *     uint32_t id;
 *     char name[32];
 *     char text[512];
 * } Packet;
 *
 * Total size: 552 bytes.
 */
static void decode_chat_packet(const unsigned char *data, size_t length)
{
    uint32_t type;
    uint32_t id;
    char name[33] = {0};
    char message[513] = {0};

    if (length < 40)
        return;

    memcpy(&type, data, sizeof(type));
    memcpy(&id, data + 4, sizeof(id));
    memcpy(name, data + 8, 32);

    printf("Task 6 chat packet:\n");
    printf("  Type: %u\n", type);
    printf("  ID: %u\n", id);
    printf("  Name: %s\n", name);

    if (length >= 552) {
        memcpy(message, data + 40, 512);
        printf("  Message: %s\n", message);
    } else {
        printf("  Message field is incomplete.\n");
    }
}

/*
 * Decode a basic DNS question name.
 * This handles ordinary uncompressed DNS QNAMEs.
 */
static void decode_dns_packet(const unsigned char *data, size_t length)
{
    size_t position = 12;
    int first = 1;

    if (length < 12) {
        printf("DNS payload is too short.\n");
        return;
    }

    printf("DNS information:\n");
    printf("  Transaction ID: 0x%02X%02X\n", data[0], data[1]);

    if (length < 13)
        return;

    printf("  Query name: ");

    while (position < length) {
        unsigned int label_length = data[position++];

        if (label_length == 0)
            break;

        /* Compression pointers are not decoded here. */
        if ((label_length & 0xC0) != 0) {
            printf("[compressed or unsupported name]");
            break;
        }

        if (position + label_length > length) {
            printf("[incomplete name]");
            break;
        }

        if (!first)
            putchar('.');

        for (unsigned int i = 0; i < label_length; i++)
            putchar(data[position + i]);

        position += label_length;
        first = 0;
    }

    putchar('\n');
}

/* Save one captured packet to the report file. */
static void save_report(
    FILE *report,
    double elapsed,
    const unsigned char *source_mac,
    const unsigned char *destination_mac,
    const char *source_ip,
    const char *destination_ip,
    uint16_t source_port,
    uint16_t destination_port,
    size_t udp_length,
    const unsigned char *payload,
    size_t payload_length)
{
    fprintf(report, "----------------------------------------\n");
    fprintf(report, "Time since capture start: %.6f seconds\n", elapsed);

    fprintf(report, "Source MAC: ");
    for (int i = 0; i < 6; i++)
        fprintf(report, "%02X%s", source_mac[i], i == 5 ? "\n" : ":");

    fprintf(report, "Destination MAC: ");
    for (int i = 0; i < 6; i++)
        fprintf(report, "%02X%s", destination_mac[i], i == 5 ? "\n" : ":");

    fprintf(report, "Source IP: %s\n", source_ip);
    fprintf(report, "Destination IP: %s\n", destination_ip);
    fprintf(report, "Source UDP port: %u\n", source_port);
    fprintf(report, "Destination UDP port: %u\n", destination_port);
    fprintf(report, "UDP segment length: %zu bytes\n", udp_length);
    fprintf(report, "UDP payload length: %zu bytes\n", payload_length);

    fprintf(report, "Payload (ASCII): ");
    for (size_t i = 0; i < payload_length; i++) {
        unsigned char c = payload[i];
        fputc((c >= 32 && c <= 126) ? c : '.', report);
    }
    fprintf(report, "\n");

    fprintf(report, "Payload (hex):\n");
    for (size_t i = 0; i < payload_length; i++) {
        fprintf(report, "%02X ", payload[i]);

        if ((i + 1) % 16 == 0)
            fprintf(report, "\n");
    }

    if (payload_length % 16 != 0)
        fprintf(report, "\n");

    fprintf(report, "\n");
    fflush(report);
}

/* Check whether the UDP packet matches the selected filter. */
static int matches_filter(
    int filter,
    uint16_t source_port,
    uint16_t destination_port)
{
    if (filter == FILTER_CHAT)
        return source_port == CHAT_PORT ||
               destination_port == CHAT_PORT;

    if (filter == FILTER_DNS)
        return source_port == DNS_PORT ||
               destination_port == DNS_PORT;

    return filter == FILTER_ALL;
}

/*
 * Parse an Ethernet frame:
 * Ethernet -> optional VLAN -> IPv4 -> UDP.
 */
static void process_packet(
    const unsigned char *buffer,
    size_t packet_length,
    int filter,
    struct timespec capture_start,
    FILE *report,
    unsigned long *packet_count)
{
    size_t offset = 0;
    uint16_t ether_type;
    struct iphdr ip_header;
    struct udphdr udp_header;

    char source_ip[INET_ADDRSTRLEN];
    char destination_ip[INET_ADDRSTRLEN];

    const unsigned char *source_mac;
    const unsigned char *destination_mac;

    size_t ip_header_length;
    size_t ip_total_length;
    size_t udp_length;
    size_t payload_length;

    const unsigned char *payload;

    /* Ethernet header must be present. */
    if (packet_length < sizeof(struct ethhdr))
        return;

    source_mac = buffer + 6;
    destination_mac = buffer;

    memcpy(&ether_type, buffer + 12, sizeof(ether_type));
    ether_type = ntohs(ether_type);

    offset = sizeof(struct ethhdr);

    /* Handle one VLAN tag (802.1Q). */
    if (ether_type == 0x8100 || ether_type == 0x88A8) {
        if (packet_length < offset + 4)
            return;

        memcpy(&ether_type, buffer + offset + 2, sizeof(ether_type));
        ether_type = ntohs(ether_type);

        offset += 4;
    }

    /* We only process IPv4. */
    if (ether_type != ETH_P_IP)
        return;

    if (packet_length < offset + sizeof(struct iphdr))
        return;

    memcpy(&ip_header, buffer + offset, sizeof(ip_header));

    ip_header_length = (size_t)ip_header.ihl * 4;

    if (ip_header_length < sizeof(struct iphdr))
        return;

    if (packet_length < offset + ip_header_length)
        return;

    if (ip_header.protocol != IPPROTO_UDP)
        return;

    /*
     * Ignore fragmented IPv4 packets. This program does not
     * reassemble fragments.
     */
    if (ntohs(ip_header.frag_off) & (IP_MF | IP_OFFMASK))
        return;

    ip_total_length = ntohs(ip_header.tot_len);

    if (ip_total_length < ip_header_length + sizeof(struct udphdr))
        return;

    if (ip_total_length > packet_length - offset)
        return;

    offset += ip_header_length;

    if (packet_length < offset + sizeof(struct udphdr))
        return;

    memcpy(&udp_header, buffer + offset, sizeof(udp_header));

    udp_length = ntohs(udp_header.len);

    if (udp_length < sizeof(struct udphdr))
        return;

    if (udp_length > ip_total_length - ip_header_length)
        return;

    payload_length = udp_length - sizeof(struct udphdr);
    payload = buffer + offset + sizeof(struct udphdr);

    {
        uint16_t source_port = ntohs(udp_header.source);
        uint16_t destination_port = ntohs(udp_header.dest);

        if (!matches_filter(filter, source_port, destination_port))
            return;

        ip_to_string(ip_header.saddr, source_ip, sizeof(source_ip));
        ip_to_string(ip_header.daddr, destination_ip, sizeof(destination_ip));

        (*packet_count)++;

        printf("\n========================================\n");
        printf("Captured UDP packet #%lu\n", *packet_count);
        printf("Time since capture start: %.6f seconds\n",
               elapsed_time(capture_start));

        printf("Source MAC: ");
        print_mac(source_mac);
        printf("\n");

        printf("Destination MAC: ");
        print_mac(destination_mac);
        printf("\n");

        printf("Source IP: %s\n", source_ip);
        printf("Destination IP: %s\n", destination_ip);

        printf("Source UDP port: %u\n", source_port);
        printf("Destination UDP port: %u\n", destination_port);

        printf("UDP segment length: %zu bytes\n", udp_length);
        printf("UDP payload length: %zu bytes\n", payload_length);

        printf("\nPayload (ASCII):\n");
        print_ascii(payload, payload_length);

        printf("\nPayload (hexadecimal):\n");
        print_hex(payload, payload_length);

        if (filter == FILTER_CHAT) {
            printf("\n");
            decode_chat_packet(payload, payload_length);
        } else if (filter == FILTER_DNS) {
            printf("\n");
            decode_dns_packet(payload, payload_length);
        }

        save_report(
            report,
            elapsed_time(capture_start),
            source_mac,
            destination_mac,
            source_ip,
            destination_ip,
            source_port,
            destination_port,
            udp_length,
            payload,
            payload_length
        );
    }
}

int main(void)
{
    int raw_socket;
    int filter;
    int duration;
    char interface_name[IFNAMSIZ];
    char input[32];

    unsigned long packet_count = 0;

    struct sockaddr_ll socket_address;
    struct pollfd poll_descriptor;

    unsigned char buffer[BUFFER_SIZE];

    struct timespec capture_start;
    FILE *report;

    printf("========== UDP RAW SNIFFER ==========\n\n");

    printf("Choose a filter:\n");
    printf("1. Task 6 chat messages (UDP port %d)\n", CHAT_PORT);
    printf("2. DNS traffic (UDP port %d)\n", DNS_PORT);
    printf("3. All UDP traffic\n");
    printf("Your choice: ");

    if (fgets(input, sizeof(input), stdin) == NULL)
        return 1;

    filter = atoi(input);

    if (filter < FILTER_CHAT || filter > FILTER_ALL) {
        fprintf(stderr, "Invalid filter.\n");
        return 1;
    }

    printf("Capture duration in seconds: ");

    if (fgets(input, sizeof(input), stdin) == NULL)
        return 1;

    duration = atoi(input);

    if (duration <= 0) {
        fprintf(stderr, "Duration must be greater than zero.\n");
        return 1;
    }

    printf("Network interface (for example eth0, enp0s3).\n");
    printf("Leave empty to capture on all interfaces: ");

    if (fgets(interface_name, sizeof(interface_name), stdin) == NULL)
        return 1;

    interface_name[strcspn(interface_name, "\n")] = '\0';

    /* Create a raw packet socket. */
    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (raw_socket < 0) {
        perror("socket");
        fprintf(stderr, "Run the program with sudo.\n");
        return 1;
    }

    /* Bind to a selected network interface, if specified. */
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_ALL);

    if (strlen(interface_name) > 0) {
        unsigned int interface_index = if_nametoindex(interface_name);

        if (interface_index == 0) {
            perror("if_nametoindex");
            close(raw_socket);
            return 1;
        }

        socket_address.sll_ifindex = (int)interface_index;

        if (bind(raw_socket,
                 (struct sockaddr *)&socket_address,
                 sizeof(socket_address)) < 0) {
            perror("bind");
            close(raw_socket);
            return 1;
        }
    }

    report = fopen("report.txt", "w");

    if (report == NULL) {
        perror("fopen report.txt");
        close(raw_socket);
        return 1;
    }

    fprintf(report, "UDP RAW capture report\n");
    fprintf(report, "Filter: %d\n", filter);
    fprintf(report, "Duration: %d seconds\n\n", duration);

    poll_descriptor.fd = raw_socket;
    poll_descriptor.events = POLLIN;

    signal(SIGINT, handle_signal);

    clock_gettime(CLOCK_MONOTONIC, &capture_start);

    printf("\nCapture started. Press Ctrl+C to stop early.\n");
    printf("Results will be saved to report.txt\n");

    while (running && elapsed_time(capture_start) < duration) {
        int result = poll(&poll_descriptor, 1, 500);

        if (result < 0) {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        if (result == 0)
            continue;

        if (poll_descriptor.revents & POLLIN) {
            ssize_t received = recvfrom(
                raw_socket,
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
                break;
            }

            process_packet(
                buffer,
                (size_t)received,
                filter,
                capture_start,
                report,
                &packet_count
            );
        }
    }

    printf("\n========================================\n");
    printf("Capture finished.\n");
    printf("Total captured matching packets: %lu\n", packet_count);
    printf("Elapsed time: %.3f seconds\n", elapsed_time(capture_start));
    printf("Report saved to report.txt\n");

    fclose(report);
    close(raw_socket);

    return 0;
}
