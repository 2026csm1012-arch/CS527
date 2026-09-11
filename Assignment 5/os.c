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

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Checks stdin without blocking on POSIX; terminal state flows to a Boolean key-available result so scheduling can continue.
 */
static int posix_kbhit(void)
{
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}
#endif

char pageTable[NP][NUM_LOGICAL_PAGES];
char freePages[NUM_PHYSICAL_PAGES];

static PCB process_table[MAX_PROCESSES];
static int next_pid = 1;
static bool shell_active = true;
static int processor_to_pid[NP];
static char shell_buf[256];
static int shell_buf_pos = 0;
static bool prompt_needed = true;

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Scans the physical-frame bitmap and reserves the first free frame; free-frame state flows to a frame number used by a process page table.
 */
int getFreePage(void)
{
    for (int i = 1; i < NUM_PHYSICAL_PAGES; i++)
    {
        if (freePages[i] == 0)
        {
            freePages[i] = 1;
            return i;
        }
    }
    fprintf(stderr, "ERROR: No free memory frames available!\n");
    return -1;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Translates a process logical address to physical RAM using its page table and page offset; logical address flows through page number + frame lookup to physical address.
 */
int getPhysicallAddress(int proc_id, int isFetch, int address)
{
    // Determine the logical page number.
    // Instruction memory is placed first (starts at logical page 0).
    // Data memory follows after instruction memory (starts at offset 1024 / PAGESIZE).
    int logical_page = (isFetch) ? (address / PAGESIZE) : (address / PAGESIZE + (1024 / PAGESIZE));

    // Bounds check to prevent illegal memory access
    if (logical_page < 0 || logical_page >= NUM_LOGICAL_PAGES)
        return -1;

    // Look up the physical frame number assigned to this process's logical page
    int physical_frame = (unsigned char)pageTable[proc_id][logical_page];
    if (physical_frame == 0xFF || physical_frame == -1)
        return -1; // Page not allocated / Page fault

    // The physical address is the base address of the frame plus the offset within the page
    int offset = address % PAGESIZE;
    return (physical_frame * PAGESIZE) + offset;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Resets RAM, page tables, frame ownership, process table and shell state; startup data flows into a clean OS state.
 */
void os_init(void)
{
    memory_init();
    memset(freePages, 0, sizeof(freePages));
    freePages[0] = 1; /* Frame 0 is reserved */

    for (int p = 0; p < NP; p++)
    {
        for (int pg = 0; pg < NUM_LOGICAL_PAGES; pg++)
        {
            pageTable[p][pg] = -1;
        }
        processor_to_pid[p] = -1;
    }

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        process_table[i].pid = 0;
        process_table[i].proc_id = -1;
        process_table[i].state = PROC_UNUSED;
        process_table[i].data_byte_count = 0;
    }

    next_pid = 1;
    shell_active = true;
    shell_buf_pos = 0;
    prompt_needed = true;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Loads program/data files into newly allocated pages; file bytes flow through temporary buffers into physical frames while page-table mappings record ownership.
 */

bool os_initialize_memory(int proc_id, const char *prog_file, const char *data_file, int *data_size)
{
    if (proc_id < 0 || proc_id >= NP)
        return false;

    FILE *fp_prog = fopen(prog_file, "r");
    if (!fp_prog)
    {
        printf("[OS MMU] Error: Cannot open program bytecode '%s'\n", prog_file);
        return false;
    }

    uint8_t p_bytes[1024];
    int p_count = 0;
    unsigned int b0, b1, b2, b3;
    while (fscanf(fp_prog, "%x %x %x %x", &b0, &b1, &b2, &b3) == 4 && p_count + 4 <= 1024)
    {
        p_bytes[p_count++] = (uint8_t)b0;
        p_bytes[p_count++] = (uint8_t)b1;
        p_bytes[p_count++] = (uint8_t)b2;
        p_bytes[p_count++] = (uint8_t)b3;
    }
    fclose(fp_prog);

    int prog_pages = (p_count + PAGESIZE - 1) / PAGESIZE;
    if (prog_pages == 0)
        prog_pages = 1;
    for (int pg = 0; pg < prog_pages; pg++)
    {
        int f = getFreePage();
        if (f == -1)
            return false;
        pageTable[proc_id][pg] = (char)f;
        int chunk = ((pg + 1) * PAGESIZE > p_count) ? (p_count - pg * PAGESIZE) : PAGESIZE;
        for (int i = 0; i < chunk; i++)
        {
            mem_write_byte_phys(f * PAGESIZE + i, p_bytes[pg * PAGESIZE + i]);
        }
    }

    int d_count = 0;
    uint8_t d_bytes[4096];
    if (data_file && strlen(data_file) > 0)
    {
        FILE *fp_data = fopen(data_file, "r");
        if (!fp_data && (strcmp(data_file, "data.byte") == 0 || strcmp(data_file, "tests/data.byte") == 0))
        {
            fp_data = fopen("tests/data.byte", "r");
            if (!fp_data)
                fp_data = fopen("data.byte", "r");
        }
        if (fp_data)
        {
            while (fscanf(fp_data, "%x %x %x %x", &b0, &b1, &b2, &b3) == 4 && d_count + 4 <= 4096)
            {
                d_bytes[d_count++] = (uint8_t)b0;
                d_bytes[d_count++] = (uint8_t)b1;
                d_bytes[d_count++] = (uint8_t)b2;
                d_bytes[d_count++] = (uint8_t)b3;
            }
            fclose(fp_data);
        }
    }

    *data_size = (d_count > 0) ? d_count : 256;
    int data_pages = (*data_size + PAGESIZE - 1) / PAGESIZE;
    int data_page_offset = 1024 / PAGESIZE;

    for (int pg = 0; pg < data_pages; pg++)
    {
        int f = getFreePage();
        if (f == -1)
            return false;
        pageTable[proc_id][data_page_offset + pg] = (char)f;
        int chunk = ((pg + 1) * PAGESIZE > d_count) ? (d_count - pg * PAGESIZE) : PAGESIZE;
        if (chunk > 0)
        {
            for (int i = 0; i < chunk; i++)
            {
                mem_write_byte_phys(f * PAGESIZE + i, d_bytes[pg * PAGESIZE + i]);
            }
        }
    }

    return true;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Dumps a completed process's logical data to its output file and frees its frames; process memory flows to persistent output, then resources return to the free pool.
 */

bool os_finalize_memory(int proc_id, const char *output_file, int data_size)
{
    if (proc_id < 0 || proc_id >= NP || !output_file)
        return false;

    FILE *out = fopen(output_file, "w");
    if (out)
    {
        int dump_size = ((data_size + 3) / 4) * 4;
        if (dump_size < 16)
            dump_size = 16;
        for (int i = 0; i < dump_size; i += 4)
        {
            uint8_t b[4] = {0};
            for (int j = 0; j < 4; j++)
            {
                int pa = getPhysicallAddress(proc_id, 0, i + j);
                b[j] = (pa >= 0) ? mem_read_byte_phys(pa) : 0;
            }
            fprintf(out, "%02X %02X %02X %02X\n", b[0], b[1], b[2], b[3]);
        }
        fclose(out);
    }

    for (int pg = 0; pg < NUM_LOGICAL_PAGES; pg++)
    {
        int f = (unsigned char)pageTable[proc_id][pg];
        if (f > 0 && f < NUM_PHYSICAL_PAGES)
        {
            freePages[f] = 0;
        }
        pageTable[proc_id][pg] = -1;
    }

    return true;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Scans core ownership for an idle processor; processor state flows to either an available core id or 'none'.
 */
static int find_free_processor(void)
{
    for (int i = 0; i < NP; i++)
    {
        if (processor_to_pid[i] == -1)
            return i;
    }
    return -1;
}

static PCB *get_process_by_pid(int pid)
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_table[i].pid == pid && process_table[i].state != PROC_UNUSED)
        {
            return &process_table[i];
        }
    }
    return NULL;
}
/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Binds a PCB to a core, initializes paged memory, resets the CPU and marks it READY; process metadata flows into core/page-table state.
 */
static bool load_process_on_processor(PCB *proc, int proc_id)
{
    proc->proc_id = proc_id;
    processor_to_pid[proc_id] = proc->pid;

    if (!os_initialize_memory(proc_id, proc->prog_byte, proc->data_byte, &proc->data_byte_count))
    {
        printf("[OS] Error: Paged allocation failed for PID %d on Processor %d\n", proc->pid, proc_id);
        processor_to_pid[proc_id] = -1;
        proc->proc_id = -1;
        proc->state = PROC_TERMINATED;
        return false;
    }

    reset(proc_id);
    proc->state = PROC_READY;
    printf("[OS] PID %d assigned to Processor %d (Running '%s')\n", proc->pid, proc_id, proc->prog_source);
    return true;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Admits a new program: program/data paths -> PCB/PID -> optional compilation -> processor assignment or WAITING queue. This is the main process creation path.
 */
int loader(const char *program_file, const char *data_file)
{
    if (!program_file || strlen(program_file) == 0)
        return -1;

    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_table[i].state == PROC_UNUSED || process_table[i].state == PROC_TERMINATED)
        {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return -1;

    int pid = next_pid++;
    PCB *proc = &process_table[slot];
    proc->pid = pid;
    proc->proc_id = -1;
    strncpy(proc->prog_source, program_file, sizeof(proc->prog_source) - 1);

    if (data_file && strlen(data_file) > 0)
    {
        strncpy(proc->data_byte, data_file, sizeof(proc->data_byte) - 1);
    }
    else
    {
        // Default to a single shared 'data.byte' file if none is specified
        strncpy(proc->data_byte, "data.byte", sizeof(proc->data_byte) - 1);
    }

    snprintf(proc->output_data, sizeof(proc->output_data), "data_out_pid%d.byte", pid);

    const char *dot = strrchr(program_file, '.');
    if (dot && (strcmp(dot, ".txt") == 0 || strcmp(dot, ".asm") == 0))
    {
        snprintf(proc->prog_byte, sizeof(proc->prog_byte), "prog_pid%d.byte", pid);
        if (!compile(proc->prog_source, proc->prog_byte))
        {
            proc->state = PROC_TERMINATED;
            return -1;
        }
    }
    else
    {
        strncpy(proc->prog_byte, program_file, sizeof(proc->prog_byte) - 1);
    }

    int free_proc = find_free_processor();
    if (free_proc != -1)
    {
        load_process_on_processor(proc, free_proc);
    }
    else
    {
        proc->state = PROC_WAITING;
        printf("[OS] All %d processors are busy. PID %d placed in WAITING queue\n", NP, pid);
    }

    return pid;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Selects the platform-specific non-blocking key test; terminal input flows to a Boolean used by the shell.
 */
static bool is_key_pressed(void)
{
#ifdef _WIN32
    return _kbhit() != 0;
#else
    return posix_kbhit() != 0;
#endif
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Reads one terminal character through the platform-specific API; keyboard input flows to the shell command buffer.
 */
static int read_key(void)
{
#ifdef _WIN32
    return _getch();
#else
    return getchar();
#endif
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Consumes non-blocking terminal input and dispatches commands to loader/status/exit; user text flows to OS process-management actions.
 */
void shell(void)
{
    if (!shell_active)
        return;

    // Show command prompt if we are waiting for a new line
    if (prompt_needed)
    {
        printf("$ ");
        fflush(stdout);
        prompt_needed = false;
    }

    // Since this is a time-sliced system, the shell must be NON-BLOCKING.
    // If the user hasn't pressed a key, return immediately so other processes can execute.
    if (!is_key_pressed())
        return;

    // A key was pressed; read it
    int ch = read_key();

    if (ch == '\r' || ch == '\n')
    {
        printf("\n");
        shell_buf[shell_buf_pos] = '\0';

        char cmd[128] = {0}, arg1[128] = {0}, arg2[128] = {0};
        int args = sscanf(shell_buf, "%s %s %s", cmd, arg1, arg2);
        if (args >= 1)
        {
            if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0)
            {
                printf("[OS] Shell exiting. Finishing active processes...\n");
                shell_active = false;
            }
            else if (strcmp(cmd, "status") == 0 || strcmp(cmd, "ps") == 0)
            {
                printf("[OS] Active Process Table:\n");
                for (int i = 0; i < MAX_PROCESSES; i++)
                {
                    if (process_table[i].state != PROC_UNUSED && process_table[i].state != PROC_TERMINATED)
                    {
                        const char *st = (process_table[i].state == PROC_READY) ? "READY/RUNNING" : "WAITING";
                        printf("  PID %d | Proc %d | State: %s | Prog: %s\n",
                               process_table[i].pid, process_table[i].proc_id, st, process_table[i].prog_source);
                    }
                }
            }
            else
            {
                loader(cmd, (args >= 2) ? arg1 : NULL);
            }
        }

        shell_buf_pos = 0;
        prompt_needed = true;
    }
    else if (ch == 8 || ch == 127)
    {
        if (shell_buf_pos > 0)
        {
            shell_buf_pos--;
            printf("\b \b");
            fflush(stdout);
        }
    }
    else if (ch >= 32 && ch <= 126)
    {
        if (shell_buf_pos < (int)sizeof(shell_buf) - 2)
        {
            shell_buf[shell_buf_pos++] = (char)ch;
            putchar(ch);
            fflush(stdout);
        }
    }
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Finds completed cores, writes results, frees pages and assigns waiting PCBs; completion flows into resource reclamation and the next queued process.
 */
static void check_and_schedule_waiting(void)
{
    for (int i = 0; i < NP; i++)
    {
        int pid = processor_to_pid[i];
        if (pid != -1 && end_of_simulation[i] == 1)
        {
            PCB *proc = get_process_by_pid(pid);
            if (proc)
            {
                os_finalize_memory(i, proc->output_data, proc->data_byte_count);
                printf("[OS] PID %d completed! Data memory saved to '%s'\n", proc->pid, proc->output_data);
                proc->state = PROC_TERMINATED;
            }

            processor_to_pid[i] = -1;

            for (int j = 0; j < MAX_PROCESSES; j++)
            {
                if (process_table[j].state == PROC_WAITING)
                {
                    load_process_on_processor(&process_table[j], i);
                    break;
                }
            }
        }
    }
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Gives each busy core TIME_SLICE instructions, handles completions, then services shell input; CPU execution flows through scheduling and process-state updates.
 */
void scheduler(void)
{
    // 1. Preemptive Round-Robin Scheduling:
    // Give every busy processor a small time slice (e.g. 10 instructions) to execute.
    for (int i = 0; i < NP; i++)
    {
        int pid = processor_to_pid[i];
        if (pid != -1 && end_of_simulation[i] == 0)
        {
            process_instructions(i, TIME_SLICE);
        }
    }

    // 2. State Management:
    // Check for completed tasks, dump their memory, and assign waiting tasks to newly freed processors.
    check_and_schedule_waiting();

    // 3. System Shell:
    // Process any asynchronous input from the user (new processes to load, or status commands).
    shell();
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Checks shell and PCB states; active system state flows to a Boolean controlling the top-level OS loop.
 */
bool os_is_active(void)
{
    if (shell_active)
        return true;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_table[i].state == PROC_READY || process_table[i].state == PROC_WAITING)
            return true;
    }
    return false;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Changes the shell_active flag; caller intent flows into whether interactive input is serviced.
 */
void os_set_shell_active(bool active)
{
    shell_active = active;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Repeats scheduler cycles until no shell/process work remains; OS state flows through scheduling to final shutdown.
 */
void os_run(void)   
{
    while (os_is_active())
    {
        scheduler();
    }
    printf("[OS] All processes finished. Simulation complete.\n");
}