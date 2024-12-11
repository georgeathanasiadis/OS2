#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Shared memory name and size constraints
#define SHM_NAME "shared_memory"
#define MAX_LINE_LENGTH 256

#define MAX_CHILDREN 10

typedef struct {
    char message[MAX_LINE_LENGTH]; // Message buffer
    sem_t sem_parent;              // Parent semaphore
    sem_t sem_children[MAX_CHILDREN]; // Semaphore array for children
} SharedMemory;


// Function declarations
SharedMemory* initialize_shared_memory();
void cleanup_shared_memory(SharedMemory* shared_mem);
void initialize_semaphores(sem_t* sem_parent, sem_t* sem_child);
void destroy_semaphores(sem_t* sem_parent, sem_t* sem_child);

#endif // IPC_UTILS_H
