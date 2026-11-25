#include "model.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

int parse_ip(const char *input, uint32_t *ip)
{
    struct in_addr address;

    if (inet_pton(AF_INET, input, &address) != 1)
        return -1;

    *ip = ntohl(address.s_addr);

    return 0;
}

int parse_mask(const char *input, uint32_t *mask)
{
    if (input[0] == '/') {

        char *end;
        long prefix;

        errno = 0;
        prefix = strtol(input + 1, &end, 10);

        if (errno != 0 || end == input + 1 ||
            *end != '\0' || prefix < 0 || prefix > 32)
            return -1;

        if (prefix == 0)
            *mask = 0;
        else
            *mask = UINT32_MAX << (32 - prefix);

        return 0;
    }

    struct in_addr address;

    if (inet_pton(AF_INET, input, &address) != 1)
        return -1;

    uint32_t value = ntohl(address.s_addr);

    /* Validate that the mask contains consecutive 1 bits */

    uint32_t inverse = ~value;

    if ((inverse & (inverse + 1)) != 0)
        return -1;

    *mask = value;

    return 0;
}

void initialize_network(Network *network)
{
    network->network = network->gateway & network->mask;
}

int is_local_network(uint32_t destination,
                     const Network *network)
{
    return (destination & network->mask) == network->network;
}

uint32_t generate_random_ip(void)
{
    uint32_t a = rand() % 256;
    uint32_t b = rand() % 256;
    uint32_t c = rand() % 256;
    uint32_t d = rand() % 256;

    return (a << 24) | (b << 16) | (c << 8) | d;
}

void process_packets(const Network *network,
                     unsigned int count,
                     Statistics *statistics)
{
    statistics->local_packets = 0;
    statistics->external_packets = 0;
    statistics->total_packets = count;

    for (unsigned int i = 0; i < count; i++) {

        uint32_t destination = generate_random_ip();

        if (is_local_network(destination, network)) {
            statistics->local_packets++;
        }
        else {
            statistics->external_packets++;
        }
    }
}