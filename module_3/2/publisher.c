
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "publisher.h"
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


void run_publisher(const char *topic)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_signal;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Publisher: sigaction");
        return;
    }

    /* Connect to the broker's queue */
    int msgid = msgget(MSG_KEY, 0666);

    if (msgid == -1) {
        perror("Publisher: Cannot connect to broker");
        return;
    }

    pid_t my_pid = getpid();

    char text[MAX_TEXT];


    /* Register publisher */

    snprintf(text, sizeof(text),
             "register,%d", (int)my_pid);

    if (send_control(msgid, text) == -1) {
        perror("Publisher: Registration failed");
        return;
    }

    printf("================================\n");
    printf("        PUBLISHER STARTED\n");
    printf("================================\n");

    printf("PID: %d\n", (int)my_pid);
    printf("Topic: %s\n", topic);
    printf("Type 'exit' to stop.\n\n");


    /* Publish messages */

    while (running) {

        char payload[MAX_TEXT];

        printf("> ");
        fflush(stdout);

        if (fgets(payload, sizeof(payload), stdin) == NULL) {

            if (!running) {
                break;
            }

            if (feof(stdin)) {
                break;
            }

            if (errno == EINTR) {
                clearerr(stdin);
                continue;
            }

            perror("Publisher: fgets");
            break;
        }

        if (!running) {
            break;
        }

        payload[strcspn(payload, "\n")] = '\0';

        if (strcmp(payload, "exit") == 0) {
            break;
        }

        if (payload[0] == '\0') {
            continue;
        }


        /*
         * Required publication format:
         *
         * send,pid,topic,payload
         */

        int length = snprintf(text, sizeof(text),
                              "send,%d,%s,%s",
                              (int)my_pid,
                              topic,
                              payload);

        if (length < 0 || (size_t)length >= sizeof(text)) {

            printf("Publisher: Message is too long.\n");

            continue;
        }

        Message msg;

        msg.mtype = MSG_TYPE_CONTROL;

        snprintf(msg.text, sizeof(msg.text), "%s", text);

        if (msgsnd(msgid,
                   &msg,
                   strlen(msg.text) + 1,
                   0) == -1) {

            if (errno == EINTR && !running) {
                break;
            }

            if (errno == EIDRM || errno == EINVAL) {

                printf("Publisher: Broker queue unavailable.\n");

                running = 0;

                break;
            }

            perror("Publisher: msgsnd");

            break;
        }

        printf("Publisher: Message sent.\n");
    }


    /* Unregister publisher */

    snprintf(text, sizeof(text),
             "unregister,%d", (int)my_pid);

    if (send_control(msgid, text) == -1) {

        if (errno != EIDRM && errno != EINVAL &&
            errno != EINTR) {

            perror("Publisher: Unregistration failed");
        }
    }

    printf("\nPublisher stopped.\n");
}
