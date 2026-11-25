
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#define BUFFER_SIZE 4096

#define MSG_FILE 1
#define MSG_END  2

#define FIFO_IN_SUFFIX  ".in"
#define FIFO_OUT_SUFFIX ".out"

typedef struct {
    uint32_t type;
    uint32_t name_length;
    uint64_t file_size;
} Message;

// Write exactly count bytes.
int write_all(int fd, const void *buffer, size_t count) {
    const char *ptr = buffer;
    size_t total = 0;

    while (total < count) {
        ssize_t n = write(fd, ptr + total, count - total);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0)
            return -1;

        total += n;
    }

    return 0;
}

// Read exactly count bytes.
// Returns 1 on success, 0 on EOF, -1 on error.
int read_all(int fd, void *buffer, size_t count) {
    char *ptr = buffer;
    size_t total = 0;

    while (total < count) {
        ssize_t n = read(fd, ptr + total, count - total);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0)
            return 0;

        total += n;
    }

    return 1;
}

// ---------------- CHILD PROCESS ----------------

void child_process(int input_fd, int output_fd) {
    char buffer[BUFFER_SIZE];

    // Tell parent that child is ready.
    uint32_t ready = 1;

    if (write_all(output_fd, &ready, sizeof(ready)) < 0) {
        perror("Child: READY");
        exit(EXIT_FAILURE);
    }

    while (1) {
        Message message;

        int result = read_all(input_fd, &message, sizeof(message));

        if (result != 1) {
            fprintf(stderr, "Child: cannot receive message\n");
            exit(EXIT_FAILURE);
        }

        // Parent finished sending files.
        if (message.type == MSG_END)
            break;

        if (message.type != MSG_FILE ||
            message.name_length == 0 ||
            message.name_length >= PATH_MAX) {
            fprintf(stderr, "Child: invalid message\n");
            exit(EXIT_FAILURE);
        }

        char filename[PATH_MAX];

        if (read_all(input_fd, filename,
                     message.name_length) != 1) {
            fprintf(stderr, "Child: cannot receive filename\n");
            exit(EXIT_FAILURE);
        }

        filename[message.name_length] = '\0';

        // Create output filename.
        char output_name[PATH_MAX];

        int len = snprintf(output_name, sizeof(output_name),
                           "%s.copy", filename);

        if (len < 0 || (size_t)len >= sizeof(output_name)) {
            fprintf(stderr, "Child: output filename too long\n");
            exit(EXIT_FAILURE);
        }

        int output_file = open(output_name,
                               O_WRONLY | O_CREAT | O_TRUNC,
                               0644);

        if (output_file < 0) {
            perror(output_name);
            exit(EXIT_FAILURE);
        }

        printf("Child: receiving %s (%llu bytes)\n",
               filename,
               (unsigned long long)message.file_size);

        uint64_t remaining = message.file_size;

        while (remaining > 0) {
            size_t chunk = remaining < BUFFER_SIZE
                           ? (size_t)remaining
                           : BUFFER_SIZE;

            int r = read_all(input_fd, buffer, chunk);

            if (r != 1) {
                fprintf(stderr, "Child: incomplete file data\n");
                close(output_file);
                exit(EXIT_FAILURE);
            }

            if (write_all(output_file, buffer, chunk) < 0) {
                perror("Child: writing output file");
                close(output_file);
                exit(EXIT_FAILURE);
            }

            remaining -= chunk;
        }

        close(output_file);

        printf("Child: created %s\n", output_name);
    }

    printf("Child: finished\n");

    close(input_fd);
    close(output_fd);

    exit(EXIT_SUCCESS);
}

// ---------------- PARENT PROCESS ----------------

void parent_process(int input_fd, int output_fd,
                    char **files, int file_count) {

    // Wait until child is ready.
    uint32_t ready;

    if (read_all(input_fd, &ready, sizeof(ready)) != 1 ||
        ready != 1) {
        fprintf(stderr, "Parent: child is not ready\n");
        exit(EXIT_FAILURE);
    }

    printf("Parent: child is ready\n");

    char buffer[BUFFER_SIZE];

    for (int i = 0; i < file_count; i++) {

        const char *filename = files[i];

        int input_file = open(filename, O_RDONLY);

        if (input_file < 0) {
            fprintf(stderr, "Parent: file '%s' does not exist "
                            "or cannot be opened: %s\n",
                    filename, strerror(errno));
            continue;
        }

        struct stat st;

        if (fstat(input_file, &st) < 0 || !S_ISREG(st.st_mode)
            || st.st_size < 0) {
            fprintf(stderr, "Parent: cannot determine file size: %s\n",
                    filename);
            close(input_file);
            continue;
        }

        size_t name_length = strlen(filename);

        Message message = {
            .type = MSG_FILE,
            .name_length = (uint32_t)name_length,
            .file_size = (uint64_t)st.st_size
        };

        // Send file information.
        if (write_all(output_fd, &message, sizeof(message)) < 0 ||
            write_all(output_fd, filename, name_length) < 0) {
            perror("Parent: sending file information");
            close(input_file);
            exit(EXIT_FAILURE);
        }

        printf("Parent: sending %s (%llu bytes)\n",
               filename,
               (unsigned long long)message.file_size);

        uint64_t remaining = message.file_size;

        while (remaining > 0) {
            size_t chunk = remaining < BUFFER_SIZE
                           ? (size_t)remaining
                           : BUFFER_SIZE;

            ssize_t n = read(input_file, buffer, chunk);

            if (n < 0) {
                if (errno == EINTR)
                    continue;

                perror("Parent: reading source file");
                close(input_file);
                exit(EXIT_FAILURE);
            }

            if (n == 0) {
                fprintf(stderr, "Parent: file changed during transfer: %s\n",
                        filename);
                close(input_file);
                exit(EXIT_FAILURE);
            }

            if (write_all(output_fd, buffer, (size_t)n) < 0) {
                perror("Parent: sending file data");
                close(input_file);
                exit(EXIT_FAILURE);
            }

            remaining -= (uint64_t)n;
        }

        close(input_file);

        printf("Parent: sent %s\n", filename);
    }

    // Tell child that all files have been sent.
    Message end_message = {
        .type = MSG_END,
        .name_length = 0,
        .file_size = 0
    };

    if (write_all(output_fd, &end_message, sizeof(end_message)) < 0) {
        perror("Parent: sending END");
        exit(EXIT_FAILURE);
    }

    printf("Parent: all files sent\n");

    close(input_fd);
    close(output_fd);
}

// ---------------- MAIN ----------------

int main(int argc, char *argv[]) {

    int use_fifo = 0;
    const char *pipe_name = NULL;

    int first_file = 1;

    // Parse -p option.
    if (argc >= 3 && strcmp(argv[1], "-p") == 0) {
        use_fifo = 1;
        pipe_name = argv[2];
        first_file = 3;
    }

    if (first_file >= argc) {
        fprintf(stderr,
                "Usage: %s [-p pipe_name] file1 [file2 ...]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    int parent_to_child[2];
    int child_to_parent[2];

    char fifo_in[PATH_MAX];
    char fifo_out[PATH_MAX];

    if (use_fifo) {

        int n1 = snprintf(fifo_in, sizeof(fifo_in),
                          "%s%s", pipe_name, FIFO_IN_SUFFIX);

        int n2 = snprintf(fifo_out, sizeof(fifo_out),
                          "%s%s", pipe_name, FIFO_OUT_SUFFIX);

        if (n1 < 0 || n2 < 0 ||
            (size_t)n1 >= sizeof(fifo_in) ||
            (size_t)n2 >= sizeof(fifo_out)) {
            fprintf(stderr, "Pipe name is too long\n");
            return EXIT_FAILURE;
        }

        if (mkfifo(fifo_in, 0600) < 0) {
            perror("mkfifo input");
            return EXIT_FAILURE;
        }

        if (mkfifo(fifo_out, 0600) < 0) {
            perror("mkfifo output");
            unlink(fifo_in);
            return EXIT_FAILURE;
        }

    } else {

        if (pipe(parent_to_child) < 0 ||
            pipe(child_to_parent) < 0) {
            perror("pipe");
            return EXIT_FAILURE;
        }
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (use_fifo) {

        if (pid == 0) {

            int in = open(fifo_in, O_RDONLY);
            int out = open(fifo_out, O_WRONLY);

            if (in < 0 || out < 0) {
                perror("Child: opening FIFO");
                exit(EXIT_FAILURE);
            }

            child_process(in, out);

        } else {

            int out = open(fifo_in, O_WRONLY);
            int in = open(fifo_out, O_RDONLY);

            if (in < 0 || out < 0) {
                perror("Parent: opening FIFO");
                exit(EXIT_FAILURE);
            }

            parent_process(in, out,
                           &argv[first_file],
                           argc - first_file);
        }

    } else {

        if (pid == 0) {

            close(parent_to_child[1]);
            close(child_to_parent[0]);

            child_process(parent_to_child[0],
                          child_to_parent[1]);

        } else {

            close(parent_to_child[0]);
            close(child_to_parent[1]);

            parent_process(child_to_parent[0],
                           parent_to_child[1],
                           &argv[first_file],
                           argc - first_file);
        }
    }

    if (pid > 0) {

        int status;

        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
        } else if (WIFEXITED(status)) {
            printf("Parent: child exited with status %d\n",
                   WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Parent: child terminated by signal %d\n",
                    WTERMSIG(status));
        }

        if (use_fifo) {
            unlink(fifo_in);
            unlink(fifo_out);
        }
    }

    return EXIT_SUCCESS;
}
