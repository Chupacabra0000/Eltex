#include "view.h"

#include <stdio.h>
#include <arpa/inet.h>

static void print_ip(uint32_t ip)
{
    struct in_addr address;

    address.s_addr = htonl(ip);

    char buffer[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &address, buffer, sizeof(buffer));

    printf("%s", buffer);
}

void view_show_network(const Network *network)
{
    printf("\n===== NETWORK CONFIGURATION =====\n");

    printf("Gateway: ");
    print_ip(network->gateway);

    printf("\nSubnet mask: ");
    print_ip(network->mask);

    printf("\nNetwork address: ");
    print_ip(network->network);

    printf("\n");
}

void view_show_statistics(const Statistics *statistics)
{
    double local_percentage =
        (double)statistics->local_packets /
        statistics->total_packets * 100.0;

    double external_percentage =
        (double)statistics->external_packets /
        statistics->total_packets * 100.0;

    printf("\n===== SIMULATION RESULTS =====\n");

    printf("Total packets: %u\n",
           statistics->total_packets);

    printf("\nLocal subnet:\n");

    printf("Packets: %u\n",
           statistics->local_packets);

    printf("Percentage: %.2f%%\n",
           local_percentage);

    printf("\nOther networks:\n");

    printf("Packets: %u\n",
           statistics->external_packets);

    printf("Percentage: %.2f%%\n",
           external_percentage);
}

void view_show_error(const char *message)
{
    printf("Error: %s\n", message);
}