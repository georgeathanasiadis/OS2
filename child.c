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

struct timespec start_time, end_time;

SharedMemory *shared_mem;
int message_count = 0;
//time_t start_time, end_time;

// Signal handler for termination
// void handle_termination(int sig) {
//     time(&end_time);
//     double active_duration = difftime(end_time, start_time);

//     printf("Child process exiting.\n");
//     printf("Messages received: %d\n", message_count);
//     printf("Active duration: %.2f seconds\n", active_duration);

//     munmap(shared_mem, sizeof(SharedMemory));
//     exit(0);
// }

void handle_termination(int sig) {
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double active_duration = (end_time.tv_sec - start_time.tv_sec) +
                             (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Child process exiting.\n");
    printf("Messages received: %d\n", message_count);
    printf("Active duration: %.6f seconds\n", active_duration);

    munmap(shared_mem, sizeof(SharedMemory));
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <child_label> <semaphore_index>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    const char *child_label = argv[1];       // Get the child label (e.g., "C1")
    int semaphore_index = atoi(argv[2]);    // Get the semaphore index

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
    time(&start_time);

    while (1) {
        sem_wait(&shared_mem->sem_children[semaphore_index]); // Wait on specific semaphore
        printf("[%s] Received message: %s", child_label, shared_mem->message);

        message_count++;
        sem_post(&shared_mem->sem_parent); // Notify the parent
    }

    return 0;
}

