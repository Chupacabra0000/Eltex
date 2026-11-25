
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "broker.h"
#include "message.h"


/* =========================
   DATA STRUCTURES
   ========================= */

typedef struct TopicNode {
    char name[MAX_TOPIC_LENGTH];
    struct TopicNode *next;
} TopicNode;


typedef struct SubscriberNode {
    pid_t pid;
    TopicNode *topics;
    struct SubscriberNode *next;
} SubscriberNode;


typedef struct PublisherNode {
    pid_t pid;
    struct PublisherNode *next;
} PublisherNode;


/* =========================
   GLOBAL VARIABLES
   ========================= */

static int msgid = -1;

static SubscriberNode *subscribers = NULL;
static PublisherNode *publishers = NULL;

static volatile sig_atomic_t running = 1;


/* =========================
   SIGNAL HANDLING
   ========================= */

static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}


/* =========================
   MESSAGE SENDING
   ========================= */

static int send_message(long type, const char *text)
{
    Message msg;

    msg.mtype = type;

    snprintf(msg.text, sizeof(msg.text), "%s", text);

    return msgsnd(msgid,
                  &msg,
                  strlen(msg.text) + 1,
                  0);
}


/* =========================
   PUBLISHER MANAGEMENT
   ========================= */

static void add_publisher(pid_t pid)
{
    PublisherNode *current = publishers;

    while (current != NULL) {

        if (current->pid == pid) {
            return;
        }

        current = current->next;
    }

    PublisherNode *node = malloc(sizeof(PublisherNode));

    if (node == NULL) {
        perror("Broker: malloc");
        return;
    }

    node->pid = pid;
    node->next = publishers;

    publishers = node;

    printf("Broker: Publisher %d registered.\n", (int)pid);
}


static void remove_publisher(pid_t pid)
{
    PublisherNode **current = &publishers;

    while (*current != NULL) {

        if ((*current)->pid == pid) {

            PublisherNode *temp = *current;

            *current = temp->next;

            free(temp);

            printf("Broker: Publisher %d disconnected.\n",
                   (int)pid);

            return;
        }

        current = &((*current)->next);
    }
}


/* =========================
   TOPIC MANAGEMENT
   ========================= */

static void add_topic(SubscriberNode *subscriber,
                      const char *topic)
{
    TopicNode *current = subscriber->topics;

    while (current != NULL) {

        if (strcmp(current->name, topic) == 0) {
            return;
        }

        current = current->next;
    }

    TopicNode *node = malloc(sizeof(TopicNode));

    if (node == NULL) {
        perror("Broker: malloc");
        return;
    }

    snprintf(node->name, sizeof(node->name), "%s", topic);

    node->next = subscriber->topics;

    subscriber->topics = node;
}


static void remove_topic(SubscriberNode *subscriber,
                         const char *topic)
{
    TopicNode **current = &subscriber->topics;

    while (*current != NULL) {

        if (strcmp((*current)->name, topic) == 0) {

            TopicNode *temp = *current;

            *current = temp->next;

            free(temp);

            return;
        }

        current = &((*current)->next);
    }
}


static int has_topic(SubscriberNode *subscriber,
                     const char *topic)
{
    TopicNode *current = subscriber->topics;

    while (current != NULL) {

        if (strcmp(current->name, topic) == 0) {
            return 1;
        }

        current = current->next;
    }

    return 0;
}


/* =========================
   SUBSCRIBER MANAGEMENT
   ========================= */

static SubscriberNode *find_subscriber(pid_t pid)
{
    SubscriberNode *current = subscribers;

    while (current != NULL) {

        if (current->pid == pid) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}


static void add_subscriber(pid_t pid, const char *topic)
{
    SubscriberNode *subscriber = find_subscriber(pid);

    if (subscriber == NULL) {

        subscriber = malloc(sizeof(SubscriberNode));

        if (subscriber == NULL) {
            perror("Broker: malloc");
            return;
        }

        subscriber->pid = pid;
        subscriber->topics = NULL;

        subscriber->next = subscribers;

        subscribers = subscriber;
    }

    add_topic(subscriber, topic);

    printf("Broker: Subscriber %d subscribed to '%s'.\n",
           (int)pid, topic);
}


static void remove_subscriber(pid_t pid, const char *topic)
{
    SubscriberNode **current = &subscribers;

    while (*current != NULL) {

        if ((*current)->pid == pid) {

            SubscriberNode *subscriber = *current;

            remove_topic(subscriber, topic);

            if (subscriber->topics == NULL) {

                *current = subscriber->next;

                free(subscriber);

                printf("Broker: Subscriber %d disconnected.\n",
                       (int)pid);
            }
            else {

                printf("Broker: Subscriber %d unsubscribed "
                       "from '%s'.\n",
                       (int)pid, topic);
            }

            return;
        }

        current = &((*current)->next);
    }
}


/* =========================
   PID VALIDATION
   ========================= */

static int parse_pid(char *text, pid_t *pid)
{
    char *end;

    errno = 0;

    long value = strtol(text, &end, 10);

    if (errno != 0 ||
        *text == '\0' ||
        *end != '\0' ||
        value <= 0 ||
        value > INT_MAX) {

        return 0;
    }

    *pid = (pid_t)value;

    return 1;
}


/* =========================
   MESSAGE PROCESSING
   ========================= */

static void process_message(char *text)
{
    char *first_comma = strchr(text, ',');

    if (first_comma == NULL) {
        printf("Broker: Invalid message.\n");
        return;
    }

    *first_comma = '\0';

    char *operation = text;
    char *rest = first_comma + 1;


    /* REGISTER PUBLISHER */

    if (strcmp(operation, "register") == 0) {

        pid_t pid;

        if (!parse_pid(rest, &pid)) {
            printf("Broker: Invalid publisher PID.\n");
            return;
        }

        add_publisher(pid);

        return;
    }


    /* UNREGISTER PUBLISHER */

    if (strcmp(operation, "unregister") == 0) {

        pid_t pid;

        if (!parse_pid(rest, &pid)) {
            return;
        }

        remove_publisher(pid);

        return;
    }


    /* OTHER OPERATIONS */

    char *second_comma = strchr(rest, ',');

    if (second_comma == NULL) {
        printf("Broker: Invalid message format.\n");
        return;
    }

    *second_comma = '\0';

    pid_t pid;

    if (!parse_pid(rest, &pid)) {
        printf("Broker: Invalid PID.\n");
        return;
    }

    char *topic = second_comma + 1;


    /* SUBSCRIBE */

    if (strcmp(operation, "subscribe") == 0) {

        if (*topic == '\0' ||
            strlen(topic) >= MAX_TOPIC_LENGTH ||
            strchr(topic, ',') != NULL) {

            printf("Broker: Invalid topic.\n");
            return;
        }

        add_subscriber(pid, topic);

        return;
    }


    /* UNSUBSCRIBE */

    if (strcmp(operation, "unsubscribe") == 0) {

        if (*topic == '\0' ||
            strlen(topic) >= MAX_TOPIC_LENGTH) {

            return;
        }

        remove_subscriber(pid, topic);

        return;
    }


    /* PUBLISH */

    if (strcmp(operation, "send") == 0) {

        char *third_comma = strchr(topic, ',');

        if (third_comma == NULL) {
            printf("Broker: Invalid publication.\n");
            return;
        }

        *third_comma = '\0';

        char *payload = third_comma + 1;

        if (*topic == '\0' ||
            strlen(topic) >= MAX_TOPIC_LENGTH) {

            printf("Broker: Invalid topic.\n");
            return;
        }

        /* Verify publisher registration */

        PublisherNode *publisher = publishers;

        while (publisher != NULL &&
               publisher->pid != pid) {

            publisher = publisher->next;
        }

        if (publisher == NULL) {

            printf("Broker: Unknown publisher %d.\n",
                   (int)pid);

            return;
        }

        char outgoing[MAX_TEXT];

        snprintf(outgoing, sizeof(outgoing),
                 "send,%d,%s,%s",
                 (int)pid, topic, payload);

        printf("\nBroker: Publication received.\n");
        printf("Topic: %s\n", topic);
        printf("Payload: %s\n", payload);

        SubscriberNode *subscriber = subscribers;

        while (subscriber != NULL && running) {

            if (has_topic(subscriber, topic)) {

                if (send_message((long)subscriber->pid,
                                 outgoing) == -1) {

                    if (errno == EIDRM || errno == EINVAL) {
                        running = 0;
                        break;
                    }

                    perror("Broker: msgsnd");
                }
                else {

                    printf("Broker: Delivered to subscriber %d.\n",
                           (int)subscriber->pid);
                }
            }

            subscriber = subscriber->next;
        }

        return;
    }

    printf("Broker: Unknown operation '%s'.\n", operation);
}


/* =========================
   SHUTDOWN
   ========================= */

static void notify_processes(void)
{
    PublisherNode *publisher = publishers;

    printf("\nBroker: Notifying publishers...\n");

    while (publisher != NULL) {

        if (kill(publisher->pid, SIGINT) == -1 &&
            errno != ESRCH) {

            perror("Broker: kill publisher");
        }

        publisher = publisher->next;
    }

    SubscriberNode *subscriber = subscribers;

    printf("Broker: Notifying subscribers...\n");

    while (subscriber != NULL) {

        if (kill(subscriber->pid, SIGINT) == -1 &&
            errno != ESRCH) {

            perror("Broker: kill subscriber");
        }

        subscriber = subscriber->next;
    }
}


static void free_lists(void)
{
    while (publishers != NULL) {

        PublisherNode *temp = publishers;

        publishers = publishers->next;

        free(temp);
    }

    while (subscribers != NULL) {

        SubscriberNode *temp = subscribers;

        subscribers = subscribers->next;

        while (temp->topics != NULL) {

            TopicNode *topic = temp->topics;

            temp->topics = topic->next;

            free(topic);
        }

        free(temp);
    }
}


/* =========================
   MAIN BROKER FUNCTION
   ========================= */

void run_broker(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_signal;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Broker: sigaction");
        return;
    }

    /* Prevent a second broker */

    msgid = msgget(MSG_KEY, IPC_CREAT | IPC_EXCL | 0666);

    if (msgid == -1) {

        if (errno == EEXIST) {

            fprintf(stderr,
                    "Broker: Message queue already exists.\n");
        }
        else {
            perror("Broker: msgget");
        }

        return;
    }

    printf("================================\n");
    printf("         BROKER STARTED\n");
    printf("================================\n");

    printf("Queue ID: %d\n", msgid);
    printf("Waiting for messages...\n\n");

    time_t last_activity = time(NULL);

    while (running) {

        Message msg;

        if (msgrcv(msgid,
                   &msg,
                   sizeof(msg.text),
                   MSG_TYPE_CONTROL,
                   IPC_NOWAIT) == -1) {

            if (errno == EINTR) {
                continue;
            }

            if (errno == ENOMSG) {

                if (time(NULL) - last_activity >=
                    BROKER_IDLE_TIMEOUT_SEC) {

                    printf("Broker: Inactivity timeout reached.\n");
                    break;
                }

                sleep(1);
                continue;
            }

            if (errno == EIDRM || errno == EINVAL) {

                printf("Broker: Queue unavailable.\n");
                break;
            }

            perror("Broker: msgrcv");
            break;
        }

        if (!running) {
            break;
        }

        last_activity = time(NULL);

        process_message(msg.text);
    }

    running = 0;

    /* Notify all registered processes */

    notify_processes();

    /* Remove message queue */

    if (msgid != -1) {

        if (msgctl(msgid, IPC_RMID, NULL) == -1) {
            perror("Broker: msgctl");
        }

        msgid = -1;
    }

    free_lists();

    printf("\nBroker stopped.\n");
    printf("Message queue removed.\n");
}
