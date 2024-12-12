#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include "ipc_utils.h"

SharedMemory *shared_mem;
int message_count = 0;  // Tracks number of messages received
int start_cycle = 0;    // Cycle when the child was activated
int current_cycle = 0;  // Tracks the current cycle based on parent updates

void handle_termination(int sig) {
    printf("Child process exiting.\n");
    printf("Messages received: %d\n", message_count);
    printf("Total active cycles: %d\n", current_cycle - start_cycle); // Correctly calculate active cycles

    munmap(shared_mem, sizeof(SharedMemory));
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 4) { // Expecting activation cycle as an additional argument
        fprintf(stderr, "Usage: %s <child_label> <semaphore_index> <start_cycle>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *child_label = argv[1];       // Get the child label (e.g., "C1")
    int semaphore_index = atoi(argv[2]);    // Get the semaphore index
    start_cycle = atoi(argv[3]);            // Activation cycle passed from the parent
    current_cycle = start_cycle;            // Initialize current cycle to start cycle

    // Attach to shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Failed to open shared memory");
        exit(EXIT_FAILURE);
    }

    shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("Failed to map shared memory");
        exit(EXIT_FAILURE);
    }

    signal(SIGTERM, handle_termination);

while (1) {
    // Increment the current cycle on each iteration
    current_cycle++;

    if (sem_trywait(&shared_mem->sem_children[semaphore_index]) == 0) {
        // Parse the parent's current cycle and task message
        char task_message[MAX_LINE_LENGTH] = {0};
        int parent_cycle = 0;

        if (sscanf(shared_mem->message, "%d|%[^\n]", &parent_cycle, task_message) == 2) {
            // Synchronize the child's current cycle with the parent's cycle
            current_cycle = parent_cycle;

            // Print the received task message
            printf("--> Received message: %s \n", task_message);
            message_count++;
        } else {
            fprintf(stderr, "[%s] Failed to parse message from shared memory.\n", child_label);
        }

        sem_post(&shared_mem->sem_parent); // Notify the parent
    }

    // Simulate the passing of one cycle
    sleep(1);
}



    return 0;
}
