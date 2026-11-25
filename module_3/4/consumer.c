
#include "common.h"

int main(void)
{
    /* Get existing shared memory */
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);

    if (shmid == -1) {
        perror("shmget");
        return EXIT_FAILURE;
    }

    /* Attach shared memory */
    char *memory = shmat(shmid, NULL, 0);

    if (memory == (void *)-1) {
        perror("shmat");
        return EXIT_FAILURE;
    }

    /* Get existing semaphore */
    int semid = semget(SEM_KEY, 1, 0666);

    if (semid == -1) {
        perror("semget");
        shmdt(memory);
        return EXIT_FAILURE;
    }

    SharedHeader *header = (SharedHeader *)memory;

    printf("Consumer started. PID: %d\n", getpid());

    while (1) {

        int processed_something = 0;
        int all_processed = 1;
        int producer_finished = 0;

        lock_memory(semid);

        producer_finished = header->finished;

        /*
         * Start from the first block.
         */
        size_t current = header->first;

        while (current != 0) {

            Block *block =
                (Block *)(memory + current);

            size_t next = block->next;

            /*
             * Skip already processed blocks.
             */
            if (block->count == 0) {
                current = next;
                continue;
            }

            all_processed = 0;

            /*
             * Process one unprocessed block.
             */
            int *numbers = (int *)(block + 1);

            int min = numbers[0];
            int max = numbers[0];

            for (int i = 1; i < block->count; i++) {

                if (numbers[i] < min) {
                    min = numbers[i];
                }

                if (numbers[i] > max) {
                    max = numbers[i];
                }
            }

            printf("\nConsumer PID %d\n", getpid());

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
         * Check all blocks again.
         */
        all_processed = 1;

        current = header->first;

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

        /*
         * Exit only if production is finished
         * and every block has been processed.
         */
        if (producer_finished && all_processed) {
            printf("Consumer PID %d: all blocks processed. Exiting.\n",
                   getpid());

            break;
        }

        if (processed_something) {
            sleep(CONSUMER_SLEEP);
        } else {
            sleep(1);
        }
    }

    shmdt(memory);

    printf("Consumer PID %d finished.\n", getpid());

    return EXIT_SUCCESS;
}
