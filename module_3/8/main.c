
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <poll.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>

#include <arpa/inet.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>

#define BUFFER_SIZE 65536
#define CHAT_PORT 50000
#define DNS_PORT 53

#define IPV4_MORE_FRAGMENTS 0x2000
#define IPV4_FRAGMENT_OFFSET 0x1FFF

/* Same structure as the Task 6 chat program. */
typedef struct {
    uint32_t type;
    uint32_t id;
    char name[32];
    char text[512];
} ChatPacket;

typedef enum {
    FILTER_CHAT = 1,
    FILTER_DNS = 2,
    FILTER_ALL_UDP = 3
} FilterType;

static volatile sig_atomic_t running = 1;
static struct timespec capture_start;
static FILE *report_file = NULL;
static unsigned long packet_count = 0;

/* Stop capture when Ctrl+C is pressed. */
void handle_signal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

/* Calculate elapsed time since capture started. */
double get_elapsed_time(void)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    return (double)(now.tv_sec - capture_start.tv_sec) +
           (double)(now.tv_nsec - capture_start.tv_nsec) / 1000000000.0;
}

/* Print a MAC address. */
void print_mac(const unsigned char *mac)
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

/* Print an IPv4 address. */
void print_ip(uint32_t address)
{
    char buffer[INET_ADDRSTRLEN];

    struct in_addr ip;
    ip.s_addr = address;

    if (inet_ntop(AF_INET, &ip, buffer, sizeof(buffer)) != NULL) {
        printf("%s", buffer);
    } else {
        printf("Unknown");
    }
}

/* Print payload as readable ASCII. */
void print_ascii(const unsigned char *data, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        unsigned char c = data[i];

        if (c >= 32 && c <= 126) {
            putchar(c);
        } else {
            putchar('.');
        }
    }
}

/* Save payload as hexadecimal characters. */
void write_hex(FILE *file, const unsigned char *data, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        fprintf(file, "%02X ", data[i]);

        if ((i + 1) % 16 == 0) {
            fprintf(file, "\n");
        }
    }

    if (length % 16 != 0) {
        fprintf(file, "\n");
    }
}

/* Decode a Task 6 chat packet. */
void decode_chat_packet(const unsigned char *payload, size_t length)
{
    uint32_t type;
    uint32_t id;

    char name[33];
    char message[513];

    if (length < 40) {
        printf("Chat packet is too short to decode.\n");
        return;
    }

    memcpy(&type, payload, sizeof(type));
    memcpy(&id, payload + 4, sizeof(id));

    memcpy(name, payload + 8, 32);
    name[32] = '\0';

    size_t message_length = length - 40;

    if (message_length > 512) {
        message_length = 512;
    }

    memcpy(message, payload + 40, message_length);
    message[message_length] = '\0';

    printf("Chat packet details:\n");

    printf("  Type: %u", type);

    switch (type) {
        case 1:
            printf(" (JOIN)\n");
            break;

        case 2:
            printf(" (CHAT)\n");
            break;

        case 3:
            printf(" (LEAVE)\n");
            break;

        default:
            printf(" (unknown)\n");
            break;
    }

    printf("  ID: %u\n", id);
    printf("  Name: %s\n", name);
    printf("  Message: %s\n", message);
}

/* Decode a basic DNS question name. */
void decode_dns_packet(const unsigned char *payload, size_t length)
{
    size_t position = 12;

    if (length < 12) {
        printf("DNS packet is too short.\n");
        return;
    }

    uint16_t transaction_id;
    uint16_t flags;

    memcpy(&transaction_id, payload, sizeof(transaction_id));
    memcpy(&flags, payload + 2, sizeof(flags));

    printf("DNS details:\n");
    printf("  Transaction ID: 0x%04X\n", ntohs(transaction_id));
    printf("  Flags: 0x%04X\n", ntohs(flags));

    printf("  Query name: ");

    if (position >= length || payload[position] == 0) {
        printf("(not available)\n");
        return;
    }

    while (position < length) {
        unsigned int label_length = payload[position++];

        if (label_length == 0) {
            break;
        }

        /* DNS compression pointers are not decoded here. */
        if ((label_length & 0xC0) != 0) {
            printf("(compressed or unsupported DNS name)\n");
            return;
        }

        if (position + label_length > length) {
            printf("(invalid DNS name)\n");
            return;
        }

        for (unsigned int i = 0; i < label_length; i++) {
            putchar(payload[position + i]);
        }

        position += label_length;

        if (position < length && payload[position] != 0) {
            putchar('.');
        }
    }

    putchar('\n');
}

/* Check whether packet matches selected filter. */
int matches_filter(FilterType filter,
                   uint16_t source_port,
                   uint16_t destination_port)
{
    switch (filter) {
        case FILTER_CHAT:
            return source_port == CHAT_PORT ||
                   destination_port == CHAT_PORT;

        case FILTER_DNS:
            return source_port == DNS_PORT ||
                   destination_port == DNS_PORT;

        case FILTER_ALL_UDP:
            return 1;

        default:
            return 0;
    }
}

/* Process one captured Ethernet frame. */
void process_packet(const unsigned char *buffer,
                    size_t packet_length,
                    FilterType filter)
{
    struct ethhdr ethernet_header;
    struct iphdr ip_header;
    struct udphdr udp_header;

    size_t offset = 0;
    size_t ip_header_length;
    size_t ip_total_length;
    size_t udp_length;
    size_t payload_length;

    const unsigned char *payload;

    uint16_t ether_type;
    uint16_t source_port;
    uint16_t destination_port;

    double elapsed;

    /* Check Ethernet header. */
    if (packet_length < sizeof(struct ethhdr)) {
        return;
    }

    memcpy(&ethernet_header, buffer, sizeof(ethernet_header));

    offset = sizeof(struct ethhdr);
    ether_type = ntohs(ethernet_header.h_proto);

    /*
     * Handle VLAN tags manually.
     * Each VLAN tag contains 4 bytes.
     */
    while (ether_type == ETH_P_8021Q || ether_type == ETH_P_8021AD) {

        if (packet_length < offset + 4) {
            return;
        }

        uint16_t encapsulated_protocol;

        memcpy(&encapsulated_protocol,
               buffer + offset + 2,
               sizeof(encapsulated_protocol));

        ether_type = ntohs(encapsulated_protocol);

        offset += 4;
    }

    /* Process IPv4 only. */
    if (ether_type != ETH_P_IP) {
        return;
    }

    if (packet_length < offset + sizeof(struct iphdr)) {
        return;
    }

    memcpy(&ip_header, buffer + offset, sizeof(ip_header));

    if (ip_header.version != 4 ||
        ip_header.protocol != IPPROTO_UDP) {
        return;
    }

    ip_header_length = (size_t)ip_header.ihl * 4;

    if (ip_header_length < sizeof(struct iphdr)) {
        return;
    }

    if (packet_length < offset + ip_header_length) {
        return;
    }

    ip_total_length = ntohs(ip_header.tot_len);

    if (ip_total_length < ip_header_length + sizeof(struct udphdr)) {
        return;
    }

    if (ip_total_length > packet_length - offset) {
        return;
    }

    /*
     * Skip fragmented IPv4 packets.
     * This program does not reassemble fragments.
     */
    if (ntohs(ip_header.frag_off) &
        (IPV4_MORE_FRAGMENTS | IPV4_FRAGMENT_OFFSET)) {
        return;
    }

    offset += ip_header_length;

    if (packet_length < offset + sizeof(struct udphdr)) {
        return;
    }

    memcpy(&udp_header, buffer + offset, sizeof(udp_header));

    source_port = ntohs(udp_header.source);
    destination_port = ntohs(udp_header.dest);
    udp_length = ntohs(udp_header.len);

    if (udp_length < sizeof(struct udphdr)) {
        return;
    }

    if (udp_length > ip_total_length - ip_header_length) {
        return;
    }

    /* Apply selected filter. */
    if (!matches_filter(filter, source_port, destination_port)) {
        return;
    }

    payload = buffer + offset + sizeof(struct udphdr);
    payload_length = udp_length - sizeof(struct udphdr);

    elapsed = get_elapsed_time();
    packet_count++;

    /* Display packet information. */
    printf("\n==================================================\n");

    printf("Packet #%lu\n", packet_count);
    printf("Time since capture started: %.6f seconds\n", elapsed);

    printf("Source MAC:      ");
    print_mac(ethernet_header.h_source);
    printf("\n");

    printf("Destination MAC: ");
    print_mac(ethernet_header.h_dest);
    printf("\n");

    printf("Source IP:       ");
    print_ip(ip_header.saddr);
    printf("\n");

    printf("Destination IP:  ");
    print_ip(ip_header.daddr);
    printf("\n");

    printf("Source UDP port:      %u\n", source_port);
    printf("Destination UDP port: %u\n", destination_port);

    printf("UDP segment length: %zu bytes\n", udp_length);
    printf("UDP payload length: %zu bytes\n", payload_length);

    printf("Payload (ASCII): ");
    print_ascii(payload, payload_length);
    printf("\n");

    /* Decode application data. */
    if (filter == FILTER_CHAT) {
        decode_chat_packet(payload, payload_length);
    } else if (filter == FILTER_DNS) {
        decode_dns_packet(payload, payload_length);
    }

    /* Save packet to report.txt. */
    if (report_file != NULL) {

        fprintf(report_file, "==================================================\n");
        fprintf(report_file, "Packet #%lu\n", packet_count);
        fprintf(report_file,
                "Time since capture started: %.6f seconds\n", elapsed);

        fprintf(report_file, "Source MAC: ");

        for (int i = 0; i < ETH_ALEN; i++) {
            fprintf(report_file, "%02X%s",
                    ethernet_header.h_source[i],
                    i == ETH_ALEN - 1 ? "" : ":");
        }

        fprintf(report_file, "\nDestination MAC: ");

        for (int i = 0; i < ETH_ALEN; i++) {
            fprintf(report_file, "%02X%s",
                    ethernet_header.h_dest[i],
                    i == ETH_ALEN - 1 ? "" : ":");
        }

        char source_ip[INET_ADDRSTRLEN];
        char destination_ip[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &ip_header.saddr,
                  source_ip, sizeof(source_ip));

        inet_ntop(AF_INET, &ip_header.daddr,
                  destination_ip, sizeof(destination_ip));

        fprintf(report_file, "\nSource IP: %s\n", source_ip);
        fprintf(report_file, "Destination IP: %s\n", destination_ip);

        fprintf(report_file, "Source UDP port: %u\n", source_port);
        fprintf(report_file, "Destination UDP port: %u\n", destination_port);

        fprintf(report_file, "UDP segment length: %zu bytes\n", udp_length);
        fprintf(report_file, "UDP payload length: %zu bytes\n", payload_length);

        fprintf(report_file, "Payload (ASCII): ");

        for (size_t i = 0; i < payload_length; i++) {
            unsigned char c = payload[i];

            fputc((c >= 32 && c <= 126) ? c : '.', report_file);
        }

        fprintf(report_file, "\nPayload (HEX):\n");

        write_hex(report_file, payload, payload_length);

        fflush(report_file);
    }
}

/* Display filter menu. */
void print_filter_menu(void)
{
    printf("\nChoose packet filter:\n");
    printf("1. Task 6 chat messages (UDP port %d)\n", CHAT_PORT);
    printf("2. DNS packets (UDP port %d)\n", DNS_PORT);
    printf("3. All IPv4 UDP packets\n");
    printf("Your choice: ");
}

int main(void)
{
    int raw_socket;
    int filter_choice;
    int duration;
    int ch;

    char interface_name[IFNAMSIZ];

    struct sockaddr_ll address;
    struct pollfd poll_descriptor;

    unsigned char buffer[BUFFER_SIZE];

    printf("========== UDP RAW PACKET CAPTURE ==========\n");

    /* Select filter. */
    print_filter_menu();

    if (scanf("%d", &filter_choice) != 1 ||
        filter_choice < 1 || filter_choice > 3) {

        fprintf(stderr, "Invalid filter choice.\n");
        return EXIT_FAILURE;
    }

    /* Select capture duration. */
    printf("Enter capture duration in seconds (0 = until Ctrl+C): ");

    if (scanf("%d", &duration) != 1 || duration < 0) {
        fprintf(stderr, "Invalid capture duration.\n");
        return EXIT_FAILURE;
    }

    /* Select network interface. */
    printf("Enter network interface (e.g. eth0, ens33, enp0s3).\n");
    printf("Leave empty to capture on all interfaces: ");

    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* Discard remaining input. */
    }

    if (fgets(interface_name, sizeof(interface_name), stdin) == NULL) {
        interface_name[0] = '\0';
    }

    interface_name[strcspn(interface_name, "\r\n")] = '\0';

    /* Create raw packet socket. */
    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (raw_socket < 0) {
        perror("socket");
        fprintf(stderr, "Run the program with sudo/root privileges.\n");
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(address));

    address.sll_family = AF_PACKET;
    address.sll_protocol = htons(ETH_P_ALL);

    /* Bind to selected interface, if specified. */
    if (strlen(interface_name) > 0) {

        unsigned int interface_index = if_nametoindex(interface_name);

        if (interface_index == 0) {
            fprintf(stderr, "Unknown interface: %s\n", interface_name);
            close(raw_socket);
            return EXIT_FAILURE;
        }

        address.sll_ifindex = (int)interface_index;

        if (bind(raw_socket, (struct sockaddr *)&address,
                 sizeof(address)) < 0) {

            perror("bind");
            close(raw_socket);
            return EXIT_FAILURE;
        }
    }

    /* Open report file. */
    report_file = fopen("report.txt", "w");

    if (report_file == NULL) {
        perror("fopen report.txt");
        close(raw_socket);
        return EXIT_FAILURE;
    }

    fprintf(report_file, "UDP RAW packet capture report\n");
    fprintf(report_file, "Filter: ");

    if (filter_choice == FILTER_CHAT) {
        fprintf(report_file, "Task 6 chat, UDP port %d\n", CHAT_PORT);
    } else if (filter_choice == FILTER_DNS) {
        fprintf(report_file, "DNS, UDP port %d\n", DNS_PORT);
    } else {
        fprintf(report_file, "All IPv4 UDP packets\n");
    }

    if (duration == 0) {
        fprintf(report_file, "Capture duration: until Ctrl+C\n\n");
    } else {
        fprintf(report_file, "Capture duration: %d seconds\n\n", duration);
    }

    fflush(report_file);

    /* Install Ctrl+C handler. */
    signal(SIGINT, handle_signal);

    /* Record capture start time. */
    clock_gettime(CLOCK_MONOTONIC, &capture_start);

    poll_descriptor.fd = raw_socket;
    poll_descriptor.events = POLLIN;
    poll_descriptor.revents = 0;

    printf("\nCapture started.\n");
    printf("Saving results to report.txt\n");
    printf("Press Ctrl+C to stop capture.\n");

    /* Main packet capture loop. */
    while (running) {

        int poll_result = poll(&poll_descriptor, 1, 500);

        if (poll_result < 0) {

            if (errno == EINTR) {
                continue;
            }

            perror("poll");
            break;
        }

        if (poll_result > 0 &&
            (poll_descriptor.revents & POLLIN)) {

            ssize_t received = recvfrom(raw_socket,
                                        buffer,
                                        sizeof(buffer),
                                        0,
                                        NULL,
                                        NULL);

            if (received < 0) {

                if (errno == EINTR) {
                    continue;
                }

                perror("recvfrom");
                break;
            }

            process_packet(buffer,
                           (size_t)received,
                           (FilterType)filter_choice);
        }

        /* Stop automatically after selected duration. */
        if (duration > 0 &&
            get_elapsed_time() >= (double)duration) {
            break;
        }
    }

    printf("\nCapture finished.\n");
    printf("Total captured matching packets: %lu\n", packet_count);
    printf("Results saved to report.txt\n");

    fprintf(report_file, "\nCapture finished.\n");
    fprintf(report_file, "Total captured matching packets: %lu\n",
            packet_count);

    fclose(report_file);
    close(raw_socket);

    return EXIT_SUCCESS;
}
