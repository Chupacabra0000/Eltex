
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <sys/select.h>
#include <time.h>

#define MAX_MESSAGE 256
#define MAX_QUEUE_NAME 200

#define NORMAL_PRIORITY 0
#define EXIT_PRIORITY 10

#define MAX_MESSAGES 10

volatile sig_atomic_t running = 1;

mqd_t send_queue;
mqd_t receive_queue;

int is_creator = 0;

char queue1[MAX_QUEUE_NAME];
char queue2[MAX_QUEUE_NAME];

/* Handle Ctrl+C */
void handle_sigint(int signal_number)
{
    (void)signal_number;
    running = 0;
}

/* Get current time + timeout */
void get_timeout(struct timespec *ts)
{
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += 1;
}

/* Receiver thread */
void *receive_messages(void *arg)
{
    (void)arg;

    char message[MAX_MESSAGE+1];
    unsigned int priority;

    while (running)
    {
        struct timespec timeout;
        get_timeout(&timeout);

        ssize_t bytes = mq_timedreceive(
            receive_queue,
            message,
            sizeof(message),
            &priority,
            &timeout
        );

        if (bytes < 0)
        {
            if (errno == ETIMEDOUT)
                continue;

            if (errno == EINTR)
                continue;

            if (running)
                perror("mq_timedreceive");

            break;
        }

        message[bytes] = '\0';

        if (priority == EXIT_PRIORITY)
        {
            printf("\n[INFO] The other participant has left the chat.\n");
            running = 0;
            break;
        }

        printf("\n[Partner]: %s\n", message);
        printf("[You]: ");
        fflush(stdout);
    }

    return NULL;
}

/* Send termination message */
void send_exit_message(void)
{
    const char *message = "EXIT";

    if (mq_send(
        send_queue,
        message,
        strlen(message) + 1,
        EXIT_PRIORITY
    ) == -1)
    {
        perror("mq_send (exit)");
    }
}

/* Create or open queues */
int initialize_queues(const char *name)
{
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MESSAGE;
    attr.mq_curmsgs = 0;

    /*
       The queue names must start with '/'.
       Example: /chat_1 and /chat_2
    */

    if (snprintf(queue1, sizeof(queue1), "/%s_1", name)
        >= sizeof(queue1))
    {
        fprintf(stderr, "Queue name is too long.\n");
        return -1;
    }

    if (snprintf(queue2, sizeof(queue2), "/%s_2", name)
        >= sizeof(queue2))
    {
        fprintf(stderr, "Queue name is too long.\n");
        return -1;
    }

    /*
       Try to create the first queue exclusively.

       If successful, this process is the creator.
       If it already exists, this process is the second participant.
    */

    send_queue = mq_open(
        queue1,
        O_CREAT | O_EXCL | O_RDWR,
        0666,
        &attr
    );

    if (send_queue != (mqd_t)-1)
    {
        is_creator = 1;

        /* Creator sends through queue 2 */
        receive_queue = mq_open(
            queue2,
            O_CREAT | O_EXCL | O_RDWR,
            0666,
            &attr
        );

        if (receive_queue == (mqd_t)-1)
        {
            perror("mq_open queue2");

            mq_close(send_queue);
            mq_unlink(queue1);

            return -1;
        }

        /*
           Creator receives from queue 1
           and sends through queue 2.
        */

        mqd_t temp = send_queue;
        send_queue = receive_queue;
        receive_queue = temp;

        printf("[INFO] You created the queues.\n");
    }
    else
    {
        if (errno != EEXIST)
        {
            perror("mq_open queue1");
            return -1;
        }

        /*
           Second participant:
           Sends through queue 1.
           Receives through queue 2.
        */

        send_queue = mq_open(queue1, O_RDWR);

        if (send_queue == (mqd_t)-1)
        {
            perror("mq_open queue1");
            return -1;
        }

        /*
           Wait for the creator to finish creating queue 2.
        */

        int attempts = 0;

        while (attempts < 50)
        {
            receive_queue = mq_open(queue2, O_RDWR);

            if (receive_queue != (mqd_t)-1)
                break;

            if (errno != ENOENT)
            {
                perror("mq_open queue2");
                mq_close(send_queue);
                return -1;
            }

            struct timespec delay = {0, 100000000};
            nanosleep(&delay, NULL);
            attempts++;
        }

        if (receive_queue == (mqd_t)-1)
        {
            fprintf(stderr, "Could not open the second queue.\n");
            mq_close(send_queue);
            return -1;
        }

        printf("[INFO] Connected to existing queues.\n");
    }

    return 0;
}

/* Close queues and remove them if this process created them */
void cleanup(void)
{
    mq_close(send_queue);
    mq_close(receive_queue);

    if (is_creator)
    {
        mq_unlink(queue1);
        mq_unlink(queue2);

        printf("[INFO] Queues deleted.\n");
    }
}

/* Main function */
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <queue_name>\n", argv[0]);
        return 1;
    }

    /* Register SIGINT handler */

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    /* Initialize message queues */

    if (initialize_queues(argv[1]) == -1)
        return 1;

    printf("\n========== P2P CHAT ==========\n");
    printf("Queue 1: %s\n", queue1);
    printf("Queue 2: %s\n", queue2);
    printf("Type /quit to leave the chat.\n");
    printf("Press Ctrl+C to leave the chat.\n");
    printf("==============================\n\n");

    /* Start receiver thread */

    pthread_t receiver_thread;

    if (pthread_create(
        &receiver_thread,
        NULL,
        receive_messages,
        NULL
    ) != 0)
    {
        fprintf(stderr, "Could not create receiver thread.\n");
        send_exit_message();
        cleanup();
        return 1;
    }

    char input[MAX_MESSAGE];

    /*
       Main thread sends messages.
       Receiver thread receives messages.

       select() allows us to check SIGINT and the receiver's
       termination flag without blocking forever on fgets().
    */

while (running)
{
    fd_set read_fds;

    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    int result = select(
        STDIN_FILENO + 1,
        &read_fds,
        NULL,
        NULL,
        &timeout
    );

    if (!running)
        break;

    if (result < 0)
    {
        if (errno == EINTR)
            continue;

        perror("select");
        break;
    }

    /* No keyboard input yet */
    if (result == 0)
        continue;

    /* Print prompt only when input is available */
    printf("[You]: ");
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL)
    {
        if (feof(stdin))
            break;

        if (errno == EINTR)
        {
            clearerr(stdin);
            continue;
        }

        perror("fgets");
        break;
    }

    input[strcspn(input, "\n")] = '\0';

    if (strcmp(input, "/quit") == 0)
        break;

    if (strlen(input) == 0)
        continue;

    if (mq_send(
        send_queue,
        input,
        strlen(input) + 1,
        NORMAL_PRIORITY
    ) == -1)
    {
        perror("mq_send");
        break;
    }
}
     /*
       Send termination message when:
       - User enters /quit.
       - User presses Ctrl+C.
       - Input is closed.
    */

    send_exit_message();

    running = 0;

    pthread_join(receiver_thread, NULL);

    cleanup();

    printf("[INFO] Chat closed.\n");

    return 0;
}
