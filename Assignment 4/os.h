#ifndef OS_H
#define OS_H

#include "processor.h"
#include "memory.h"
#include <stdbool.h>

#define MAX_PROCESSES 64
#define TIME_SLICE 10   /* Instructions executed per scheduling round */

/* Process Execution States */
typedef enum {
    PROC_UNUSED,
    PROC_WAITING,
    PROC_READY,
    PROC_RUNNING,
    PROC_TERMINATED
} ProcessState;

/* Process Control Block (PCB) */
typedef struct {
    int pid;
    int proc_id;                /* Allocated hardware processor ID (0 to NP-1, or -1 if waiting) */
    char prog_source[128];      /* Assembly source file name (.txt) */
    char prog_byte[128];        /* Compiled bytecode file name (.byte) */
    char data_byte[128];        /* Initial data file name (.byte) */
    char output_data[128];      /* Output data dump file name */
    ProcessState state;
} PCB;


void os_init(void);
int loader(const char *program_file, const char *data_file);

/**
 * Non-blocking interactive Shell:
 * - Checks STDIN for user input without blocking
 * - Accumulates characters into a command buffer
 * - Parses "program.txt data.byte" and submits to loader
 * - Handles "exit" command to shut down the shell
 */
void shell(void);

/**
 * Round-Robin Task Scheduler:
 * - Iterates through active ready tasks
 * - Runs each task for TIME_SLICE instructions on its assigned processor
 * - Cleans up terminated processes and admits waiting tasks
 * - Calls shell() to accept new user commands
 */
void scheduler(void);

/**
 * Run the OS main execution loop until all tasks complete and shell exits.
 */
void os_run(void);

/**
 * Returns true if the shell is active or tasks are still running.
 */
bool os_is_active(void);

#endif 