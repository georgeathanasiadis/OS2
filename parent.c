#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include "ipc_utils.h"
#include <time.h> 

#define MAX_CHILDREN 10

pid_t child_pids[MAX_CHILDREN] = {0};
int active_children = 0;
int active_child_indices[MAX_CHILDREN];
SharedMemory *shared_mem;

// Signal handler for termination
// void terminate_child(int child_index) {
//     if (child_pids[child_index] != 0) {
//         // Send termination signal to the child process
//         kill(child_pids[child_index], SIGTERM);

//         // Wait for the child process to terminate
//         waitpid(child_pids[child_index], NULL, 0);

//         // Clear the PID and decrement active children count
//         child_pids[child_index] = 0;
//         active_children--;
//         // Print termination message
//         printf("Terminating child process C%d\n", child_index + 1);
//     }
// }


void shuffle_active_children() {
    for (int i = active_children - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = active_child_indices[i];
        active_child_indices[i] = active_child_indices[j];
        active_child_indices[j] = temp;
    }
}

void terminate_child(int child_index) {
    if (child_pids[child_index] != 0) {
        //printf("Terminating child process C%d\n", child_index + 1);

        kill(child_pids[child_index], SIGTERM);
        waitpid(child_pids[child_index], NULL, 0);
        child_pids[child_index] = 0;

        // Remove from active children list
        for (int i = 0; i < active_children; i++) {
            if (active_child_indices[i] == child_index) {
                // Shift remaining indices to fill the gap
                for (int j = i; j < active_children - 1; j++) {
                    active_child_indices[j] = active_child_indices[j + 1];
                }
                active_children--;
                printf("Terminating child process C%d\n", child_index + 1);

                break;
            }
        }
    }
}



void spawn_child(int child_index) {
    if (active_children >= MAX_CHILDREN) {
        fprintf(stderr, "Maximum number of children reached\n");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // In child process
        char process_label[10];
        snprintf(process_label, sizeof(process_label), "C%d", child_index + 1);
        char semaphore_index[10];
        snprintf(semaphore_index, sizeof(semaphore_index), "%d", child_index); // Pass semaphore index
        execl("./child", "child", process_label, semaphore_index, NULL);
        perror("Failed to exec child process");
        exit(EXIT_FAILURE);
    }

    // In parent process
    child_pids[child_index] = pid;
    active_child_indices[active_children] = child_index; // Add to active indices
    active_children++;

    printf("Spawning child process C%d\n", child_index + 1);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "ipc_utils.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <command_file> <text_file> <max_children>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    const char *command_file = argv[1];
    const char *text_file = argv[2];
    int max_children = atoi(argv[3]);

    if (max_children > MAX_CHILDREN) {
        fprintf(stderr, "Maximum allowed children is %d\n", MAX_CHILDREN);
        exit(EXIT_FAILURE);
    }

    shared_mem = initialize_shared_memory();

    // Read command file
    FILE *cmd_fp = fopen(command_file, "r");
    if (!cmd_fp) {
        perror("Failed to open command file");
        exit(EXIT_FAILURE);
    }

    // Read text file into memory
    FILE *text_fp = fopen(text_file, "r");
    if (!text_fp) {
        perror("Failed to open text file");
        exit(EXIT_FAILURE);
    }

    char text_lines[100][MAX_LINE_LENGTH];
    int line_count = 0;
    while (fgets(text_lines[line_count], MAX_LINE_LENGTH, text_fp)) {
        line_count++;
    }
    fclose(text_fp);

    char line[256];
    int current_time = 0; // Simulated time

    while (1) {
        // Process commands scheduled at the current time
        if (fgets(line, sizeof(line), cmd_fp)) {
            int timestamp;
            char process_label[10], command;

            if (sscanf(line, "%d %s %c", &timestamp, process_label, &command) != 3) {
                fprintf(stderr, "Invalid command format: %s\n", line);
                continue;
            }

            if (timestamp == current_time) {
                int child_index = process_label[1] - '1';

                if (command == 'S') {
                    spawn_child(child_index);
                } else if (command == 'T') {
                    terminate_child(child_index);
                }
            } else {
                // Rewind the line if not for the current time
                fseek(cmd_fp, -strlen(line), SEEK_CUR);
            }
        }

        // If there are active children, send a random line to a random child
        if (active_children > 0) {
            int random_child_index = active_child_indices[rand() % active_children];

            // Send a random line from the text file
            strncpy(shared_mem->message, text_lines[rand() % line_count], MAX_LINE_LENGTH);

            // Notify the selected child
            printf("Sending message to child [C%d]\n", random_child_index + 1);
            sem_post(&shared_mem->sem_children[random_child_index]);

            // Wait for the child to process the message
            sem_wait(&shared_mem->sem_parent);
        }

        printf("Active children: ");
        for (int i = 0; i < active_children; i++) {
            printf("C%d ", active_child_indices[i] + 1);
        }
        printf("\n");
        printf("--------------------------------------------------\n");

        // Simulate time passing
        sleep(1); // 1 second per cycle
        current_time++;

        // Break the loop if no active children and no commands remain
        if (active_children == 0 && feof(cmd_fp)) {
            break;
        }
    }

    fclose(cmd_fp);

    // Clean up remaining children
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (child_pids[i] != 0) {
            terminate_child(i);
        }
    }

    cleanup_shared_memory(shared_mem);

    return 0;
}

