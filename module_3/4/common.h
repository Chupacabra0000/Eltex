
#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define SHM_SIZE 4096

#define SEM_KEY 0x1234
#define SHM_KEY 0x5678

#define MAX_NUMBERS 10

#define PRODUCER_SLEEP 2
#define CONSUMER_SLEEP 2

/*
 * Header of each block in shared memory.
 *
 * count = 0 means that the block was processed.
 * next = 0 means that this is the last block.
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
 * Semaphore union required by semctl.
 */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

/*
 * Lock shared memory.
 */
static inline void lock_memory(int semid)
{
    struct sembuf operation = {0, -1, SEM_UNDO};

    if (semop(semid, &operation, 1) == -1) {
        perror("semop lock");
        exit(EXIT_FAILURE);
    }
}

/*
 * Unlock shared memory.
 */
static inline void unlock_memory(int semid)
{
    struct sembuf operation = {0, 1, SEM_UNDO};

    if (semop(semid, &operation, 1) == -1) {
        perror("semop unlock");
        exit(EXIT_FAILURE);
    }
}

#endif
