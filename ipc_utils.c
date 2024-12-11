// ipc_utils.c
#include "ipc_utils.h"
#include <unistd.h>
//#define MAX_CHILDREN 10

// Function to initialize shared memory
SharedMemory* initialize_shared_memory() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(SharedMemory)) == -1) {
        perror("Failed to allocate shared memory");
        exit(EXIT_FAILURE);
    }

    SharedMemory* shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("Failed to map shared memory");
        exit(EXIT_FAILURE);
    }

    // Initialize parent semaphore
    if (sem_init(&shared_mem->sem_parent, 1, 0) == -1) {
        perror("Failed to initialize parent semaphore");
        exit(EXIT_FAILURE);
    }

    // Initialize child semaphores
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (sem_init(&shared_mem->sem_children[i], 1, 0) == -1) {
            perror("Failed to initialize child semaphore");
            exit(EXIT_FAILURE);
        }
    }

    return shared_mem;
}

// Function to clean up shared memory
void cleanup_shared_memory(SharedMemory* shared_mem) {
    // Destroy parent semaphore
    if (sem_destroy(&shared_mem->sem_parent) == -1) {
        perror("Failed to destroy parent semaphore");
    }

    // Destroy child semaphores
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (sem_destroy(&shared_mem->sem_children[i]) == -1) {
            perror("Failed to destroy child semaphore");
        }
    }

    // Unmap shared memory
    if (munmap(shared_mem, sizeof(SharedMemory)) == -1) {
        perror("Failed to unmap shared memory");
    }

    // Unlink shared memory
    if (shm_unlink(SHM_NAME) == -1) {
        perror("Failed to unlink shared memory");
    }
}

// Function to initialize semaphores
void initialize_semaphores(sem_t* sem_parent, sem_t sem_children[MAX_CHILDREN]) {
    if (sem_init(sem_parent, 1, 0) == -1) {
        perror("Failed to initialize parent semaphore");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (sem_init(&sem_children[i], 1, 0) == -1) {
            perror("Failed to initialize child semaphore");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to destroy semaphores
void destroy_semaphores(sem_t* sem_parent, sem_t sem_children[MAX_CHILDREN]) {
    if (sem_destroy(sem_parent) == -1) {
        perror("Failed to destroy parent semaphore");
    }

    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (sem_destroy(&sem_children[i]) == -1) {
            perror("Failed to destroy child semaphore");
        }
    }
}
