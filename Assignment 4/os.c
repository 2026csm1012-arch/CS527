#include "os.h"
#include "compiler.h"
#include "processor.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

static int posix_kbhit(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}
#endif

static PCB process_table[MAX_PROCESSES];
static int next_pid = 1;
static bool shell_active = true;
static int processor_to_pid[NP];
static char shell_buf[256];
static int shell_buf_pos = 0;
static bool prompt_needed = true;

void os_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].proc_id = -1;
        process_table[i].state = PROC_UNUSED;
    }
    for (int i = 0; i < NP; i++) {
        processor_to_pid[i] = -1;
    }
    next_pid = 1;
    shell_active = true;
    shell_buf_pos = 0;
    prompt_needed = true;
}

static int find_free_processor(void) {
    for (int i = 0; i < NP; i++) {
        if (processor_to_pid[i] == -1) {
            return i;
        }
    }
    return -1;
}

static PCB *get_process_by_pid(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROC_UNUSED) {
            return &process_table[i];
        }
    }
    return NULL;
}

static bool load_process_on_processor(PCB *proc, int proc_id) {
    proc->proc_id = proc_id;
    processor_to_pid[proc_id] = proc->pid;

    /* Initialize processor memory with bytecode and data */
    if (!memory_initialize(proc_id, proc->prog_byte, proc->data_byte)) {
        printf("[OS] Error: Failed to initialize memory for PID %d on Processor %d\n",
               proc->pid, proc_id);
        processor_to_pid[proc_id] = -1;
        proc->proc_id = -1;
        proc->state = PROC_TERMINATED;
        return false;
    }

    /* Reset processor registers and state */
    reset(proc_id);
    proc->state = PROC_READY;

    printf("[OS] PID %d assigned to Processor %d (Running '%s')\n",
           proc->pid, proc_id, proc->prog_source);
    return true;
}

int loader(const char *program_file, const char *data_file) {
    if (!program_file || strlen(program_file) == 0) return -1;

    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_UNUSED || process_table[i].state == PROC_TERMINATED) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        printf("[OS] Error: Maximum process limit reached\n");
        return -1;
    }

    int pid = next_pid++;
    PCB *proc = &process_table[slot];
    proc->pid = pid;
    proc->proc_id = -1;
    strncpy(proc->prog_source, program_file, sizeof(proc->prog_source) - 1);

    if (data_file && strlen(data_file) > 0) {
        strncpy(proc->data_byte, data_file, sizeof(proc->data_byte) - 1);
    } else {
        proc->data_byte[0] = '\0';
    }

    snprintf(proc->output_data, sizeof(proc->output_data), "data_out_pid%d.byte", pid);

    /* Determine if we need to compile from assembly source */
    const char *dot = strrchr(program_file, '.');
    if (dot && (strcmp(dot, ".txt") == 0 || strcmp(dot, ".asm") == 0)) {
        snprintf(proc->prog_byte, sizeof(proc->prog_byte), "prog_pid%d.byte", pid);
        if (!compile(proc->prog_source, proc->prog_byte)) {
            printf("[OS] Compilation failed for PID %d ('%s')\n", pid, proc->prog_source);
            proc->state = PROC_TERMINATED;
            return -1;
        }
    } else {
        strncpy(proc->prog_byte, program_file, sizeof(proc->prog_byte) - 1);
    }

            /* Allocate processor */
    int free_proc = find_free_processor();
    if (free_proc != -1) {
        load_process_on_processor(proc, free_proc);
    } else {
        proc->state = PROC_WAITING;
        printf("[OS] All %d processors are busy. PID %d placed in WAITING queue\n", NP, pid);
    }

    return pid;
}

void shell(void) {
    if (!shell_active) return;

    if (prompt_needed) {
        printf("$ ");
        fflush(stdout);
        prompt_needed = false;
    }

    int char_available = 0;
#ifdef _WIN32
    char_available = _kbhit();
#else
    char_available = posix_kbhit();
#endif

    if (!char_available) return;

#ifdef _WIN32
    int ch = _getch();
#else
    int ch = getchar();
#endif

    if (ch == '\r' || ch == '\n') {
        printf("\n");
        shell_buf[shell_buf_pos] = '\0';

        char cmd[128] = {0};
        char arg1[128] = {0};
        char arg2[128] = {0};

        int args = sscanf(shell_buf, "%s %s %s", cmd, arg1, arg2);
        if (args >= 1) {
            if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
                printf("[OS] Shell exiting. Finishing active processes...\n");
                shell_active = false;
            } else if (strcmp(cmd, "status") == 0 || strcmp(cmd, "ps") == 0) {
                printf("[OS] Active Process Table:\n");
                for (int i = 0; i < MAX_PROCESSES; i++) {
                    if (process_table[i].state != PROC_UNUSED && process_table[i].state != PROC_TERMINATED) {
                        const char *st = (process_table[i].state == PROC_READY) ? "READY/RUNNING" : "WAITING";
                        printf("  PID %d | Proc %d | State: %s | Prog: %s\n",
                               process_table[i].pid, process_table[i].proc_id, st, process_table[i].prog_source);
                    }
                }
            } else {
                const char *prog_arg = cmd;
                const char *data_arg = (args >= 2) ? arg1 : NULL;
                loader(prog_arg, data_arg);
            }
        }

        shell_buf_pos = 0;
        prompt_needed = true;
    } else if (ch == 8 || ch == 127) { /* Backspace */
        if (shell_buf_pos > 0) {
            shell_buf_pos--;
            printf("\b \b");
            fflush(stdout);
        }
    } else if (ch >= 32 && ch <= 126) { /* Printable characters */
        if (shell_buf_pos < (int)sizeof(shell_buf) - 2) {
            shell_buf[shell_buf_pos++] = (char)ch;
            putchar(ch);
            fflush(stdout);
        }
    }
}


static void check_and_schedule_waiting(void) {
    for (int i = 0; i < NP; i++) {
        int pid = processor_to_pid[i];
        if (pid != -1) {
            if (end_of_simulation[i] == 1) {
                PCB *proc = get_process_by_pid(pid);
                if (proc) {
                    memory_finalize(i, proc->output_data);
                    printf("[OS] PID %d completed! Data memory saved to '%s'\n",
                           proc->pid, proc->output_data);
                    proc->state = PROC_TERMINATED;
                }

                processor_to_pid[i] = -1;

                /* Admit next waiting process */
                for (int j = 0; j < MAX_PROCESSES; j++) {
                    if (process_table[j].state == PROC_WAITING) {
                        load_process_on_processor(&process_table[j], i);
                        break;
                    }
                }
            }
        }
    }
}

void scheduler(void) {
    for (int i = 0; i < NP; i++) {
        int pid = processor_to_pid[i];
        if (pid != -1 && end_of_simulation[i] == 0) {
            process_instructions(i, TIME_SLICE);
        }
    }

    check_and_schedule_waiting();
    shell();
}

bool os_is_active(void) {
    if (shell_active) return true;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_READY || process_table[i].state == PROC_WAITING) {
            return true;
        }
    }
    return false;
}

void os_run(void) {
    while (os_is_active()) {
        scheduler();
    }
    printf("[OS] All processes finished. Simulation complete.\n");
}
