
#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stddef.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <semaphore.h>

#define SHM_NAME "/producer_consumer_shm"
#define SEM_NAME "/producer_consumer_sem"

#define SHM_SIZE 4096

#define MAX_NUMBERS 10

#define PRODUCER_SLEEP 2
#define CONSUMER_SLEEP 2

/*
 * One block of the simulated linked list.
 *
 * count = 0 means processed.
 * next = 0 means last block.
 */
typedef struct {
    int count;
    size_t next;
} Block;

/*
 * Shared memory header.
 */
typedef struct {
    size_t first;
    size_t last;
    size_t used;

    int finished;
} SharedHeader;

/*
 * Align an offset for Block.
 */
static inline size_t align_offset(size_t offset)
{
    size_t alignment = _Alignof(Block);

    return (offset + alignment - 1) / alignment * alignment;
}

/*
 * Lock shared memory.
 */
static inline void lock_memory(sem_t *sem)
{
    while (sem_wait(sem) == -1) {

        if (errno == EINTR) {
            continue;
        }

        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
}

/*
 * Unlock shared memory.
 */
static inline void unlock_memory(sem_t *sem)
{
    if (sem_post(sem) == -1) {
        perror("sem_post");
        exit(EXIT_FAILURE);
    }
}

#endif
