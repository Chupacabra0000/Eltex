#include "model.h"
#include "view.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

// 128+64+32+16+8+4+2+1
// 1   2  3  4  5 6 7 8
//50%   25% 12.5% 6.25% 3.125% 1.5625% 0.78125% 0.390625%
//./network_sim 198.168.1.1 /3 10000


int main(int argc, char *argv[])
{
    Network network;
    Statistics statistics;

    /* Check the number of arguments */

    if (argc != 4) {
        printf("Usage: %s <gateway> <mask> <packet_count>\n",
               argv[0]);

        printf("Example: %s 192.168.1.1 /24 100\n",
               argv[0]);

        return 1;
    }

    /* Parse gateway IP */

    if (parse_ip(argv[1], &network.gateway) == -1) {
        view_show_error("Invalid gateway IP address.");
        return 1;
    }

    /* Parse subnet mask */

    if (parse_mask(argv[2], &network.mask) == -1) {
        view_show_error("Invalid subnet mask.");
        return 1;
    }

    /* Parse packet count */

    char *end;
    errno = 0;

    unsigned long count = strtoul(argv[3], &end, 10);

    if (errno != 0 ||
        argv[3][0] == '-' ||
        end == argv[3] ||
        *end != '\0' ||
        count == 0 ||
        count > UINT_MAX) {

        view_show_error("Invalid packet count.");
        return 1;
    }

    /* Calculate network address */

    initialize_network(&network);

    /* Initialize random generator */

    srand((unsigned int)time(NULL));

    /* Display network configuration */

    view_show_network(&network);

    /* Simulate packets */

    process_packets(&network,
                    (unsigned int)count,
                    &statistics);

    /* Display results */

    view_show_statistics(&statistics);

    return 0;
}