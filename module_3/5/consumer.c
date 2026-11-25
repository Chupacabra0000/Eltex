
#include "common.h"

int main(void)
{
    /*
     * 1. Open existing shared memory.
     */
    int shm_fd = shm_open(
        SHM_NAME,
        O_RDWR,
        0666
    );

    if (shm_fd == -1) {
        perror("shm_open");

        return EXIT_FAILURE;
    }

    /*
     * 2. Map shared memory.
     */
    char *memory = mmap(
        NULL,
        SHM_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );

    if (memory == MAP_FAILED) {
        perror("mmap");

        close(shm_fd);

        return EXIT_FAILURE;
    }

    /*
     * 3. Open existing semaphore.
     */
    sem_t *sem = sem_open(SEM_NAME, 0);

    if (sem == SEM_FAILED) {
        perror("sem_open");

        munmap(memory, SHM_SIZE);
        close(shm_fd);

        return EXIT_FAILURE;
    }

    SharedHeader *header = (SharedHeader *)memory;

    printf("Consumer started. PID: %d\n", getpid());

    /*
     * 4. Consumer main loop.
     */
    while (1) {

        int processed_something = 0;
        int all_processed = 1;

        lock_memory(sem);

        /*
         * Check producer status.
         */
        int producer_finished = header->finished;

        /*
         * Start from the first block.
         */
        size_t current = header->first;

        /*
         * Search for an unprocessed block.
         */
        while (current != 0) {

            Block *block =
                (Block *)(memory + current);

            size_t next = block->next;

            /*
             * Skip processed blocks.
             */
            if (block->count == 0) {

                current = next;

                continue;
            }

            /*
             * Found an unprocessed block.
             */
            int *numbers = (int *)(block + 1);

            int min = numbers[0];
            int max = numbers[0];

            /*
             * Find minimum and maximum.
             */
            for (int i = 1; i < block->count; i++) {

                if (numbers[i] < min) {
                    min = numbers[i];
                }

                if (numbers[i] > max) {
                    max = numbers[i];
                }
            }

            /*
             * Print results.
             */
            printf("\nConsumer PID: %d\n", getpid());

            printf("Block offset: %zu\n", current);

            printf("Numbers: ");

            for (int i = 0; i < block->count; i++) {
                printf("%d ", numbers[i]);
            }

            printf("\nMinimum: %d\n", min);
            printf("Maximum: %d\n", max);

            /*
             * Mark block as processed.
             */
            block->count = 0;

            printf("Block processed.\n");

            processed_something = 1;

            /*
             * Process only one block per iteration.
             */
            break;
        }

        /*
         * Check whether all blocks are processed.
         */
        current = header->first;

        all_processed = 1;

        while (current != 0) {

            Block *block =
                (Block *)(memory + current);

            if (block->count != 0) {

                all_processed = 0;

                break;
            }

            current = block->next;
        }

        unlock_memory(sem);

        /*
         * 5. Check termination condition.
         */
        if (producer_finished && all_processed) {

            printf("\nConsumer PID %d: "
                   "all blocks processed. Exiting.\n",
                   getpid());

            break;
        }

        /*
         * 6. Sleep before next iteration.
         */
        if (processed_something) {

            sleep(CONSUMER_SLEEP);

        } else {

            sleep(1);
        }
    }

    /*
     * 7. Cleanup.
     */
    sem_close(sem);

    munmap(memory, SHM_SIZE);

    close(shm_fd);

    printf("Consumer PID %d finished.\n", getpid());

    return EXIT_SUCCESS;
}
