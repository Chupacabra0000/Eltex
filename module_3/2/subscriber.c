
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "subscriber.h"
#include "message.h"


static volatile sig_atomic_t running = 1;


static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}


/* Send a control message to the broker */
static int send_control(int msgid, const char *text)
{
    Message msg;

    msg.mtype = MSG_TYPE_CONTROL;

    snprintf(msg.text, sizeof(msg.text), "%s", text);

    return msgsnd(msgid,
                  &msg,
                  strlen(msg.text) + 1,
                  0);
}


void run_subscriber(int topic_count, char *topics[])
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_signal;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Subscriber: sigaction");
        return;
    }


    /* Connect to broker */

    int msgid = msgget(MSG_KEY, 0666);

    if (msgid == -1) {
        perror("Subscriber: Cannot connect to broker");
        return;
    }

    pid_t my_pid = getpid();

    char text[MAX_TEXT];

    int subscribed = 0;


    printf("================================\n");
    printf("       SUBSCRIBER STARTED\n");
    printf("================================\n");

    printf("PID: %d\n", (int)my_pid);


    /* Subscribe to requested topics */

    for (int i = 0; i < topic_count; i++) {

        if (!running) {
            break;
        }

        snprintf(text, sizeof(text),
                 "subscribe,%d,%s",
                 (int)my_pid,
                 topics[i]);

        if (send_control(msgid, text) == -1) {

            if (errno == EIDRM || errno == EINVAL) {

                printf("Subscriber: Broker queue unavailable.\n");
            }
            else {
                perror("Subscriber: Subscription failed");
            }

            running = 0;

            break;
        }

        subscribed++;

        printf("Subscribed to topic: %s\n", topics[i]);
    }

    if (subscribed == 0) {
        running = 0;
    }

    printf("\nWaiting for messages...\n");


    /* Receive messages */

    while (running) {

        Message msg;

        /*
         * Receive only messages addressed
         * to this subscriber's PID.
         */

        if (msgrcv(msgid,
                   &msg,
                   sizeof(msg.text),
                   (long)my_pid,
                   0) == -1) {

            if (errno == EINTR) {
                continue;
            }

            if (errno == EIDRM || errno == EINVAL) {

                printf("\nSubscriber: Broker queue unavailable.\n");

                running = 0;

                break;
            }

            perror("Subscriber: msgrcv");

            running = 0;

            break;
        }

        if (!running) {
            break;
        }


        /*
         * Expected message:
         *
         * send,publisher_pid,topic,payload
         */

        char *first = strchr(msg.text, ',');

        if (first == NULL) {
            continue;
        }

        char *second = strchr(first + 1, ',');

        if (second == NULL) {
            continue;
        }

        char *third = strchr(second + 1, ',');

        if (third == NULL) {
            continue;
        }

        *first = '\0';
        *second = '\0';
        *third = '\0';


        printf("\n================================\n");
        printf("        MESSAGE RECEIVED\n");
        printf("================================\n");

        printf("Publisher PID: %s\n", first + 1);

        printf("Topic: %s\n", second + 1);

        printf("Payload: %s\n", third + 1);

        printf("================================\n");
    }


    /* Unsubscribe from all successfully registered topics */

    for (int i = 0; i < subscribed; i++) {

        snprintf(text, sizeof(text),
                 "unsubscribe,%d,%s",
                 (int)my_pid,
                 topics[i]);

        if (send_control(msgid, text) == -1) {

            if (errno != EIDRM &&
                errno != EINVAL &&
                errno != EINTR) {

                perror("Subscriber: Unsubscription failed");
            }
        }
    }

    printf("\nSubscriber stopped.\n");
}
