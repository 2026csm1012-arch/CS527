#ifndef OS_H
#define OS_H

#include "processor.h"
#include "memory.h"
#include <stdbool.h>

#define MAX_PROCESSES 64
#define TIME_SLICE 10

typedef enum {
    PROC_UNUSED,
    PROC_WAITING,
    PROC_READY,
    PROC_RUNNING,
    PROC_TERMINATED
} ProcessState;

typedef struct {
    int pid;
    int proc_id;
    char prog_source[128];
    char prog_byte[128];
    char data_byte[128];
    char output_data[128];
    ProcessState state;
    int data_byte_count;
} PCB;

extern char pageTable[NP][NUM_LOGICAL_PAGES];
extern char freePages[NUM_PHYSICAL_PAGES];

void os_init(void);
int getFreePage(void);
int getPhysicallAddress(int proc_id, int isFetch, int address);
bool os_initialize_memory(int proc_id, const char *prog_file, const char *data_file, int *data_size);
bool os_finalize_memory(int proc_id, const char *output_file, int data_size);

int loader(const char *program_file, const char *data_file);
void shell(void);
void scheduler(void);
void os_run(void);
bool os_is_active(void);
void os_set_shell_active(bool active);

#endif