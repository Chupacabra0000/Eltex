
#include "common.h"

int main(void)
{
    srand(time(NULL) ^ getpid());

    /* Create shared memory */
    int shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);

    if (shmid == -1) {
        perror("shmget");
        return EXIT_FAILURE;
    }

    /* Attach shared memory */
    char *memory = shmat(shmid, NULL, 0);

    if (memory == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }

    /* Create semaphore */
    int semid = semget(SEM_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);

    if (semid == -1) {
        perror("semget");

        shmdt(memory);
        shmctl(shmid, IPC_RMID, NULL);

        return EXIT_FAILURE;
    }

    /* Initialize semaphore to 1 */
    union semun argument;
    argument.val = 1;

    if (semctl(semid, 0, SETVAL, argument) == -1) {
        perror("semctl");

        shmdt(memory);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);

        return EXIT_FAILURE;
    }

    SharedHeader *header = (SharedHeader *)memory;

    header->first = 0;
    header->last = 0;
    header->used = sizeof(SharedHeader);
    header->finished = 0;

    printf("Producer started. PID: %d\n", getpid());

    while (1) {

        lock_memory(semid);

        /*
         * Check whether there is enough space
         * for another block.
         */
        size_t remaining = SHM_SIZE - header->used;

        size_t block_size =
            sizeof(Block) + MAX_NUMBERS * sizeof(int);

        if (remaining >= block_size) {

            /*
             * Generate random number of elements.
             */
            int count = 1 + rand() % MAX_NUMBERS;

            Block *block = (Block *)(memory + header->used);

            block->count = count;
            block->next = 0;

            int *numbers = (int *)(block + 1);

            printf("\nProducer generated: ");

            for (int i = 0; i < count; i++) {
                numbers[i] = rand() % 100;

                printf("%d ", numbers[i]);
            }

            printf("\n");

            /*
             * Connect previous block with new block.
             */
            if (header->first == 0) {
                header->first = header->used;
            } else {
                Block *previous =
                    (Block *)(memory + header->last);

                previous->next = header->used;
            }

            header->last = header->used;

            header->used += block_size;

            printf("Producer stored block at offset %zu\n",
                   header->last);

            unlock_memory(semid);

            sleep(PRODUCER_SLEEP);
        }
        else {

            /*
             * No more blocks can be generated.
             * Tell consumers that production is finished.
             */
            header->finished = 1;

            printf("\nProducer: shared memory is full.\n");
            printf("Producer: waiting for consumers...\n");

            /*
             * Check whether all blocks are processed.
             */
            int all_processed = 1;

            size_t current = header->first;

            while (current != 0) {

                Block *block =
                    (Block *)(memory + current);

                if (block->count != 0) {
                    all_processed = 0;
                    break;
                }

                current = block->next;
            }

            unlock_memory(semid);

            if (all_processed) {
                printf("Producer: all blocks processed.\n");
                break;
            }

            sleep(PRODUCER_SLEEP);
        }
    }

    /*
     * Detach and remove shared memory.
     */
    shmdt(memory);
    shmctl(shmid, IPC_RMID, NULL);

    /* Remove semaphore */
    semctl(semid, 0, IPC_RMID);

    printf("Producer finished.\n");

    return EXIT_SUCCESS;
}
