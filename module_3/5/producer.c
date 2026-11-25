
#include "common.h"

int main(void)
{
    srand(time(NULL) ^ getpid());

    /*
     * 1. Create POSIX shared memory.
     */
    int shm_fd = shm_open(
        SHM_NAME,
        O_CREAT | O_EXCL | O_RDWR,
        0666
    );

    if (shm_fd == -1) {
        perror("shm_open");

        fprintf(stderr,
                "Shared memory may already exist.\n");

        return EXIT_FAILURE;
    }

    /*
     * 2. Set shared memory size.
     */
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");

        close(shm_fd);
        shm_unlink(SHM_NAME);

        return EXIT_FAILURE;
    }

    /*
     * 3. Map shared memory.
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
        shm_unlink(SHM_NAME);

        return EXIT_FAILURE;
    }

    /*
     * 4. Create POSIX named semaphore.
     *
     * Initial value = 1.
     */
    sem_t *sem = sem_open(
        SEM_NAME,
        O_CREAT | O_EXCL,
        0666,
        1
    );

    if (sem == SEM_FAILED) {
        perror("sem_open");

        munmap(memory, SHM_SIZE);
        close(shm_fd);
        shm_unlink(SHM_NAME);

        return EXIT_FAILURE;
    }

    /*
     * 5. Initialize shared memory header.
     */
    SharedHeader *header = (SharedHeader *)memory;

    header->first = 0;
    header->last = 0;
    header->used = align_offset(sizeof(SharedHeader));
    header->finished = 0;

    printf("Producer started. PID: %d\n", getpid());

    /*
     * 6. Producer main loop.
     */
    while (1) {

        lock_memory(sem);

        /*
         * Generate random array size.
         */
        int count = 1 + rand() % MAX_NUMBERS;

        int generated[MAX_NUMBERS];

        printf("\nProducer generated: ");

        for (int i = 0; i < count; i++) {

            generated[i] = rand() % 100;

            printf("%d ", generated[i]);
        }

        printf("\n");

        /*
         * Calculate required memory.
         */
        size_t offset = align_offset(header->used);

        size_t block_size =
            sizeof(Block) + count * sizeof(int);

        /*
         * Check available memory.
         */
        if (offset > SHM_SIZE ||
            block_size > SHM_SIZE - offset) {

            /*
             * Cannot store another block.
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

            unlock_memory(sem);

            if (all_processed) {
                printf("Producer: all blocks processed.\n");
                break;
            }

            sleep(PRODUCER_SLEEP);

            continue;
        }

        /*
         * 7. Create new block.
         */
        Block *block = (Block *)(memory + offset);

        block->count = count;
        block->next = 0;

        /*
         * Store array immediately after Block.
         */
        int *numbers = (int *)(block + 1);

        for (int i = 0; i < count; i++) {
            numbers[i] = generated[i];
        }

        /*
         * Connect previous block with new block.
         */
        if (header->first == 0) {

            header->first = offset;

        } else {

            Block *previous =
                (Block *)(memory + header->last);

            previous->next = offset;
        }

        /*
         * Update list information.
         */
        header->last = offset;

        header->used = offset + block_size;

        printf("Producer stored block at offset %zu\n",
               offset);

        unlock_memory(sem);

        /*
         * Wait before generating another array.
         */
        sleep(PRODUCER_SLEEP);
    }

    /*
     * 8. Cleanup.
     */
    sem_unlink(SEM_NAME);

    sem_close(sem);

    munmap(memory, SHM_SIZE);

    close(shm_fd);

    shm_unlink(SHM_NAME);

    printf("Producer finished.\n");

    return EXIT_SUCCESS;
}
