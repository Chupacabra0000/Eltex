#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>

typedef struct {
    uint32_t gateway;
    uint32_t mask;
    uint32_t network;
} Network;

typedef struct {
    unsigned int local_packets;
    unsigned int external_packets;
    unsigned int total_packets;
} Statistics;

/* IP and mask conversion */
int parse_ip(const char *input, uint32_t *ip);
int parse_mask(const char *input, uint32_t *mask);

/* Network operations */
void initialize_network(Network *network);

int is_local_network(uint32_t destination,
                     const Network *network);

/* Packet simulation */
uint32_t generate_random_ip(void);

void process_packets(const Network *network,
                     unsigned int count,
                     Statistics *statistics);

#endif