
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mqueue.h>

#define MAX_DRIVERS 100
#define MAX_EVENTS 10
#define MAX_LINE 256

typedef enum {
    CMD_TASK = 1,
    CMD_STATUS,
    CMD_STOP,
    RESP_OK,
    RESP_BUSY,
    RESP_AVAILABLE
} MessageType;

typedef struct {
    MessageType type;
    int seconds;
} Message;

typedef struct {
    pid_t pid;
    mqd_t command_queue;
    mqd_t response_queue;
    char command_name[64];
    char response_name[64];
} Driver;

static Driver drivers[MAX_DRIVERS];
static int driver_count = 0;

static volatile sig_atomic_t running = 1;

/* =========================================
   SIGNAL HANDLING
   ========================================= */

void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* =========================================
   DRIVER PROCESS
   ========================================= */

int remaining_time(const struct timespec *deadline)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    time_t seconds = deadline->tv_sec - now.tv_sec;
    long nanoseconds = deadline->tv_nsec - now.tv_nsec;

    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000L;
    }

    if (seconds < 0)
        return 0;

    int result = (int)seconds;

    if (nanoseconds > 0)
        result++;

    return result;
}

void send_response(mqd_t queue, MessageType type, int seconds)
{
    Message message = {
        .type = type,
        .seconds = seconds
    };

    if (mq_send(queue, (const char *)&message,
                sizeof(message), 0) < 0) {
        perror("mq_send");
    }
}

void driver_process(const char *command_name,
                    const char *response_name)
{
    mqd_t command_queue = mq_open(
        command_name, O_RDONLY | O_NONBLOCK
    );

    if (command_queue == (mqd_t)-1) {
        perror("driver mq_open command");
        _exit(EXIT_FAILURE);
    }

    mqd_t response_queue = mq_open(
        response_name, O_WRONLY | O_NONBLOCK
    );

    if (response_queue == (mqd_t)-1) {
        perror("driver mq_open response");
        mq_close(command_queue);
        _exit(EXIT_FAILURE);
    }

    int timer_fd = timerfd_create(
        CLOCK_MONOTONIC, TFD_NONBLOCK
    );

    if (timer_fd < 0) {
        perror("timerfd_create");
        mq_close(command_queue);
        mq_close(response_queue);
        _exit(EXIT_FAILURE);
    }

    int epoll_fd = epoll_create1(0);

    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(timer_fd);
        mq_close(command_queue);
        mq_close(response_queue);
        _exit(EXIT_FAILURE);
    }

    struct epoll_event event;

    memset(&event, 0, sizeof(event));

    event.events = EPOLLIN;
    event.data.fd = (int)command_queue;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
                  (int)command_queue, &event) < 0) {
        perror("epoll_ctl command");
        _exit(EXIT_FAILURE);
    }

    event.data.fd = timer_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
                  timer_fd, &event) < 0) {
        perror("epoll_ctl timer");
        _exit(EXIT_FAILURE);
    }

    int busy = 0;
    int active = 1;

    struct timespec deadline = {0};

    printf("[DRIVER %d] Started. Available.\n", getpid());
    fflush(stdout);

    struct epoll_event events[MAX_EVENTS];

    while (active) {

        int count = epoll_wait(
            epoll_fd,
            events,
            MAX_EVENTS,
            -1
        );

        if (count < 0) {
            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < count; i++) {

            /* TIMER EVENT */

            if (events[i].data.fd == timer_fd) {

                uint64_t expirations;

                if (read(timer_fd, &expirations,
                         sizeof(expirations)) < 0) {
                    if (errno != EAGAIN)
                        perror("timer read");
                    continue;
                }

                busy = 0;

                printf("[DRIVER %d] Task completed. Available.\n",
                       getpid());

                fflush(stdout);

                continue;
            }

            /* COMMAND QUEUE EVENT */

            if (events[i].data.fd == (int)command_queue) {

                Message message;

                while (mq_receive(
                    command_queue,
                    (char *)&message,
                    sizeof(message),
                    NULL
                ) >= 0) {

                    if (message.type == CMD_STOP) {
                        active = 0;
                        break;
                    }

                    if (message.type == CMD_STATUS) {

                        if (busy) {
                            send_response(
                                response_queue,
                                RESP_BUSY,
                                remaining_time(&deadline)
                            );
                        } else {
                            send_response(
                                response_queue,
                                RESP_AVAILABLE,
                                0
                            );
                        }
                    }

                    else if (message.type == CMD_TASK) {

                        if (busy) {

                            int remaining = remaining_time(&deadline);

                            send_response(
                                response_queue,
                                RESP_BUSY,
                                remaining
                            );

                            printf("[DRIVER %d] Rejected task. Busy %d.\n",
                                   getpid(), remaining);

                            fflush(stdout);
                        }

                        else {

                            struct itimerspec timer = {0};

                            timer.it_value.tv_sec = message.seconds;

                            if (timerfd_settime(
                                timer_fd, 0, &timer, NULL
                            ) < 0) {
                                perror("timerfd_settime");
                                continue;
                            }

                            clock_gettime(CLOCK_MONOTONIC, &deadline);
                            deadline.tv_sec += message.seconds;

                            busy = 1;

                            printf("[DRIVER %d] Task accepted for %d seconds.\n",
                                   getpid(), message.seconds);

                            fflush(stdout);

                            send_response(
                                response_queue,
                                RESP_OK,
                                message.seconds
                            );
                        }
                    }
                }

                if (errno != EAGAIN && errno != EINTR) {
                    perror("mq_receive");
                }
            }
        }
    }

    printf("[DRIVER %d] Stopping.\n", getpid());
    fflush(stdout);

    close(epoll_fd);
    close(timer_fd);

    mq_close(command_queue);
    mq_close(response_queue);

    _exit(EXIT_SUCCESS);
}

/* =========================================
   FIND DRIVER
   ========================================= */

Driver *find_driver(pid_t pid)
{
    for (int i = 0; i < driver_count; i++) {

        if (drivers[i].pid == pid)
            return &drivers[i];
    }

    return NULL;
}

/* =========================================
   CREATE DRIVER
   ========================================= */

void create_driver(void)
{
    if (driver_count >= MAX_DRIVERS) {
        printf("Maximum number of drivers reached.\n");
        return;
    }

    Driver *driver = &drivers[driver_count];

    snprintf(
        driver->command_name,
        sizeof(driver->command_name),
        "/taxi_cmd_%d_%d",
        getpid(),
        driver_count
    );

    snprintf(
        driver->response_name,
        sizeof(driver->response_name),
        "/taxi_resp_%d_%d",
        getpid(),
        driver_count
    );

    struct mq_attr attr = {0};

    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Message);

    /* Create command queue */

    mqd_t command_queue = mq_open(
        driver->command_name,
        O_CREAT | O_EXCL | O_WRONLY | O_NONBLOCK,
        0600,
        &attr
    );

    if (command_queue == (mqd_t)-1) {
        perror("mq_open command");
        return;
    }

    /* Create response queue */

    mqd_t response_queue = mq_open(
        driver->response_name,
        O_CREAT | O_EXCL | O_RDONLY,
        0600,
        &attr
    );

    if (response_queue == (mqd_t)-1) {
        perror("mq_open response");
        mq_close(command_queue);
        mq_unlink(driver->command_name);
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");

        mq_close(command_queue);
        mq_close(response_queue);

        mq_unlink(driver->command_name);
        mq_unlink(driver->response_name);

        return;
    }

    if (pid == 0) {

        /* Child process */

        mq_close(command_queue);
        mq_close(response_queue);

        driver_process(
            driver->command_name,
            driver->response_name
        );

        _exit(EXIT_SUCCESS);
    }

    /* Parent process */

    driver->pid = pid;
    driver->command_queue = command_queue;
    driver->response_queue = response_queue;

    driver_count++;

    printf("Driver created successfully. PID: %d\n", pid);
}

/* =========================================
   SEND COMMAND AND RECEIVE RESPONSE
   ========================================= */

int send_command(Driver *driver, MessageType type, int seconds,
                 Message *response)
{
    Message message = {
        .type = type,
        .seconds = seconds
    };

    if (mq_send(
        driver->command_queue,
        (const char *)&message,
        sizeof(message),
        0
    ) < 0) {
        perror("mq_send");
        return -1;
    }

    ssize_t received = mq_receive(
        driver->response_queue,
        (char *)response,
        sizeof(*response),
        NULL
    );

    if (received < 0) {
        perror("mq_receive response");
        return -1;
    }

    if (received != sizeof(*response)) {
        fprintf(stderr, "Invalid driver response.\n");
        return -1;
    }

    return 0;
}

/* =========================================
   SEND TASK
   ========================================= */

void send_task(pid_t pid, int seconds)
{
    Driver *driver = find_driver(pid);

    if (!driver) {
        printf("Driver %d not found.\n", pid);
        return;
    }

    if (seconds <= 0) {
        printf("Task timer must be positive.\n");
        return;
    }

    Message response;

    if (send_command(
        driver, CMD_TASK, seconds, &response
    ) < 0)
        return;

    if (response.type == RESP_OK) {
        printf("Driver %d accepted the task for %d seconds.\n",
               pid, response.seconds);
    }

    else if (response.type == RESP_BUSY) {
        printf("Busy %d\n", response.seconds);
    }
}

/* =========================================
   GET STATUS
   ========================================= */

void get_status(pid_t pid)
{
    Driver *driver = find_driver(pid);

    if (!driver) {
        printf("Driver %d not found.\n", pid);
        return;
    }

    Message response;

    if (send_command(
        driver, CMD_STATUS, 0, &response
    ) < 0)
        return;

    if (response.type == RESP_BUSY) {
        printf("Driver %d: Busy %d\n",
               pid, response.seconds);
    }

    else if (response.type == RESP_AVAILABLE) {
        printf("Driver %d: Available\n", pid);
    }
}

/* =========================================
   GET ALL DRIVERS
   ========================================= */

void get_drivers(void)
{
    if (driver_count == 0) {
        printf("No drivers created.\n");
        return;
    }

    printf("\n========== DRIVERS ==========\n");

    for (int i = 0; i < driver_count; i++) {

        Driver *driver = &drivers[i];

        Message response;

        printf("PID %d: ", driver->pid);

        if (send_command(
            driver, CMD_STATUS, 0, &response
        ) < 0) {
            printf("Communication error\n");
            continue;
        }

        if (response.type == RESP_BUSY) {
            printf("Busy %d\n", response.seconds);
        }

        else if (response.type == RESP_AVAILABLE) {
            printf("Available\n");
        }
    }

    printf("=============================\n");
}

/* =========================================
   CLEANUP
   ========================================= */

void cleanup(void)
{
    printf("\n[TAXI] Shutting down...\n");

    for (int i = 0; i < driver_count; i++) {

        Driver *driver = &drivers[i];

        Message message = {
            .type = CMD_STOP,
            .seconds = 0
        };

        if (mq_send(
            driver->command_queue,
            (const char *)&message,
            sizeof(message),
            0
        ) < 0) {
            perror("mq_send stop");
        }
    }

    for (int i = 0; i < driver_count; i++) {

        int status;

        while (waitpid(drivers[i].pid, &status, 0) < 0) {
            if (errno == EINTR)
                continue;

            if (errno == ECHILD)
                break;

            perror("waitpid");
            break;
        }

        mq_close(drivers[i].command_queue);
        mq_close(drivers[i].response_queue);

        mq_unlink(drivers[i].command_name);
        mq_unlink(drivers[i].response_name);
    }

    printf("[TAXI] All drivers stopped.\n");
}

/* =========================================
   MAIN CLI
   ========================================= */

int main(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    char line[MAX_LINE];

    printf("========== TAXI STATION ==========\n");
    printf("Commands:\n");
    printf("create_driver\n");
    printf("send_task <pid> <task_timer>\n");
    printf("get_status <pid>\n");
    printf("get_drivers\n");
    printf("exit\n");
    printf("==================================\n");

    while (running) {

        printf("\nTaxi> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {

            if (errno == EINTR) {
                clearerr(stdin);
                break;
            }

            if (feof(stdin))
                break;

            perror("fgets");
            break;
        }

        char *command = strtok(line, " \t\n");

        if (!command)
            continue;

        if (strcmp(command, "create_driver") == 0) {

            create_driver();
        }

        else if (strcmp(command, "send_task") == 0) {

            char *pid_arg = strtok(NULL, " \t\n");
            char *timer_arg = strtok(NULL, " \t\n");

            if (!pid_arg || !timer_arg ||
                strtok(NULL, " \t\n") != NULL) {
                printf("Usage: send_task <pid> <task_timer>\n");
                continue;
            }

            char *end1;
            char *end2;

            errno = 0;
            long pid_value = strtol(pid_arg, &end1, 10);

            if (errno || *end1 != '\0' ||
                pid_value <= 0 || pid_value > INT_MAX) {
                printf("Invalid PID.\n");
                continue;
            }

            errno = 0;
            long timer_value = strtol(timer_arg, &end2, 10);

            if (errno || *end2 != '\0' ||
                timer_value <= 0 || timer_value > INT_MAX) {
                printf("Invalid task timer.\n");
                continue;
            }

            send_task((pid_t)pid_value, (int)timer_value);
        }

        else if (strcmp(command, "get_status") == 0) {

            char *pid_arg = strtok(NULL, " \t\n");

            if (!pid_arg || strtok(NULL, " \t\n") != NULL) {
                printf("Usage: get_status <pid>\n");
                continue;
            }

            char *end;
            errno = 0;

            long pid_value = strtol(pid_arg, &end, 10);

            if (errno || *end != '\0' ||
                pid_value <= 0 || pid_value > INT_MAX) {
                printf("Invalid PID.\n");
                continue;
            }

            get_status((pid_t)pid_value);
        }

        else if (strcmp(command, "get_drivers") == 0 ||
                 strcmp(command, "get_drivers7") == 0) {

            if (strtok(NULL, " \t\n") != NULL) {
                printf("Usage: get_drivers\n");
                continue;
            }

            get_drivers();
        }

        else if (strcmp(command, "exit") == 0 ||
                 strcmp(command, "quit") == 0) {

            break;
        }

        else {
            printf("Unknown command: %s\n", command);
        }
    }

    running = 0;

    cleanup();

    return 0;
}
