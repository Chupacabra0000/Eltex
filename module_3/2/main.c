
#include <stdio.h>
#include <string.h>

#include "broker.h"
#include "publisher.h"
#include "subscriber.h"
#include "message.h"


/* Validate topic name */
static int valid_topic(const char *topic)
{
    if (topic == NULL || topic[0] == '\0') {
        return 0;
    }

    if (strlen(topic) >= MAX_TOPIC_LENGTH) {
        return 0;
    }

    /* Commas are reserved as separators */
    if (strchr(topic, ',') != NULL) {
        return 0;
    }

    return 1;
}


int main(int argc, char *argv[])
{
    if (argc < 2) {

        printf("Usage:\n\n");

        printf("  %s -b\n", argv[0]);
        printf("      Start broker\n\n");

        printf("  %s -p topic\n", argv[0]);
        printf("      Start publisher\n\n");

        printf("  %s -s topic1 [topic2 ...]\n", argv[0]);
        printf("      Start subscriber\n");

        return 1;
    }


    /* BROKER */

    if (strcmp(argv[1], "-b") == 0) {

        if (argc != 2) {

            fprintf(stderr,
                    "Broker: -b takes no additional arguments.\n");

            return 1;
        }

        run_broker();

        return 0;
    }


    /* PUBLISHER */

    if (strcmp(argv[1], "-p") == 0) {

        if (argc != 3) {

            fprintf(stderr,
                    "Publisher: Specify exactly one topic.\n");

            return 1;
        }

        if (!valid_topic(argv[2])) {

            fprintf(stderr,
                    "Publisher: Invalid topic.\n");

            return 1;
        }

        run_publisher(argv[2]);

        return 0;
    }


    /* SUBSCRIBER */

    if (strcmp(argv[1], "-s") == 0) {

        if (argc < 3) {

            fprintf(stderr,
                    "Subscriber: Specify at least one topic.\n");

            return 1;
        }

        for (int i = 2; i < argc; i++) {

            if (!valid_topic(argv[i])) {

                fprintf(stderr,
                        "Subscriber: Invalid topic '%s'.\n",
                        argv[i]);

                return 1;
            }
        }

        run_subscriber(argc - 2, &argv[2]);

        return 0;
    }


    /* INVALID ARGUMENT */

    fprintf(stderr,
            "Unknown argument: %s\n",
            argv[1]);

    return 1;
}
