### Topic 5: System Utilities (Memory Maps & Process Performance)

To build utilities on top of the existing system, you must capture performance metrics during execution and provide command-line hooks in the interactive OS shell to display the current state of physical memory and process health.

#### **Step 1: Update the PCB for Performance Tracking (`os.h`)**

Expand the Process Control Block to track the total number of instructions executed and the total clock cycles consumed.

```c
// Inside os.h
typedef struct {
    int pid;
    int state;
    int assigned_core;
    
    // --- Performance Metrics ---
    uint64_t instructions_executed;
    uint64_t cycles_consumed; // Useful if Topic 6 (Cache/Timing) is implemented
    
    // ... existing fields (ptbr_frame, logical_pages, saved_pc, etc.) ...[cite: 1]
} PCB;

```

#### **Step 2: Track Metrics During Execution (`processor.c`)**

Increment these counters whenever the simulated processor executes an instruction.

```c
// Inside processor.c -> process_instructions()
void process_instructions(int core_id, int num_instructions) {
    PCB *current_pcb = get_pcb_on_core(core_id);
    
    for (int i = 0; i < num_instructions; i++) {
        // ... fetch, decode, execute logic ...[cite: 1]
        
        // Increment performance trackers
        if (current_pcb != NULL) {
            current_pcb->instructions_executed++;
            // If cache timing (Topic 6) is not yet implemented, assume 1 cycle per instruction
            current_pcb->cycles_consumed += 1; 
        }
        
        // Break early if HALT is encountered[cite: 1]
    }
}

```

#### **Step 3: Implement the Utility Functions (`os.c`)**

Add functions to format and print the memory map and performance metrics. The simulator has 16 physical frames of 512 bytes each, with Frame 0 reserved.

```c
// Inside os.c

#define NUM_PHYSICAL_PAGES 16 // From existing configuration[cite: 1]

// Utility 1: Dump Physical Memory Map
void dump_memory_map() {
    printf("\n=== System Memory Map ===\n");
    printf("Physical RAM Size: 8192 bytes | Frames: 16 | Page Size: 512 bytes\n");
    printf("--------------------------------------------------\n");
    printf("| Frame | PADDR Range   | Owner PID | Logical Page |\n");
    printf("--------------------------------------------------\n");
    
    for (int frame = 0; frame < NUM_PHYSICAL_PAGES; frame++) {
        if (frame == 0) {
            printf("| %-5d | 0000 - 0511   | OS RSVD   | N/A          |\n", frame);
            continue;
        }
        
        int owner_pid = -1;
        int logical_page_mapped = -1;
        
        // Scan all processes to see who owns this frame
        for (int i = 0; i < 64; i++) { // MAX_PROCESSES = 64[cite: 1]
            if (process_table[i].state != PROC_UNUSED) {
                // Check if it's the Page Table frame itself (Topic 3)
                if (process_table[i].ptbr_frame == frame) {
                    owner_pid = process_table[i].pid;
                    logical_page_mapped = -99; // Arbitrary code for PTBR
                    break;
                }
                // Check standard data/code logical pages
                for (int lp = 0; lp < 10; lp++) { // 10 logical pages per process[cite: 1]
                    if (process_table[i].logical_pages[lp] == frame) {
                        owner_pid = process_table[i].pid;
                        logical_page_mapped = lp;
                        break;
                    }
                }
            }
        }
        
        uint32_t start_addr = frame * 512;
        uint32_t end_addr = start_addr + 511;
        
        if (owner_pid != -1) {
            if (logical_page_mapped == -99) {
                printf("| %-5d | %04d - %04d   | PID %-5d | %-12s |\n", frame, start_addr, end_addr, owner_pid, "PAGE TABLE");
            } else {
                printf("| %-5d | %04d - %04d   | PID %-5d | LP %-9d |\n", frame, start_addr, end_addr, owner_pid, logical_page_mapped);
            }
        } else {
            printf("| %-5d | %04d - %04d   | FREE      | N/A          |\n", frame, start_addr, end_addr);
        }
    }
    printf("--------------------------------------------------\n\n");
}

// Utility 2: Process Performance Viewer
void print_process_performance() {
    printf("\n=== Process Performance Metrics ===\n");
    printf("-----------------------------------------------------------------\n");
    printf("| PID | State      | Core | Instructions | Cycles | IPC       |\n");
    printf("-----------------------------------------------------------------\n");
    
    for (int i = 0; i < 64; i++) {
        if (process_table[i].state != PROC_UNUSED) {
            PCB *p = &process_table[i];
            
            // Calculate Instructions Per Cycle (IPC)
            double ipc = (p->cycles_consumed > 0) ? (double)p->instructions_executed / p->cycles_consumed : 0.0;
            
            const char* state_str = "UNKNOWN";
            if (p->state == PROC_WAITING) state_str = "WAITING";
            else if (p->state == PROC_READY) state_str = "READY";
            else if (p->state == PROC_RUNNING) state_str = "RUNNING";
            else if (p->state == PROC_TERMINATED) state_str = "TERMINATED";
            
            printf("| %-3d | %-10s | %-4d | %-12llu | %-6llu | %-9.2f |\n", 
                p->pid, state_str, p->assigned_core, p->instructions_executed, p->cycles_consumed, ipc);
        }
    }
    printf("-----------------------------------------------------------------\n\n");
}

```

#### **Step 4: Integrate into the OS Shell (`os.c`)**

Hook these functions into the interactive shell so they can be triggered by the user at any time.

```c
// Inside the main shell loop in os.c
void run_os_mode() {
    char input[256];
    
    while (1) {
        printf("OS> ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        
        // Strip newline
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "exit") == 0) {
            break; // Existing exit behavior[cite: 1]
        } else if (strcmp(input, "status") == 0) {
            // Existing status command[cite: 1]
            print_status();
        } else if (strcmp(input, "pmap") == 0) {
            // New memory map command
            dump_memory_map();
        } else if (strcmp(input, "perf") == 0) {
            // New performance viewer command
            print_process_performance();
        } else if (strlen(input) > 0) {
            // Assume it's a program file to load[cite: 1]
            loader(input);
        }
        
        scheduler_tick(); // Advance the system
    }
}

```

### Verification & Testing

1. Launch the simulator without arguments to enter the multi-process OS mode (`./simulator`).


2. Load a few test programs: `tests/sum_n_fixed.txt` and `tests/complex_multiply.txt`.


3. Type `pmap` into the shell. You should see a detailed ASCII table showing exactly which physical frames (1 through 15) are assigned to PID 1 and PID 2, and which frames are still `FREE`. Frame 0 should show as `OS RSVD`.


4. Type `perf` into the shell. You will see an active breakdown of the instructions executed by each PID.

Would you like to move on to **Topic 6 (Cache and TLB with cycle-accurate timing)**, or **Topic 7 (Pipelined Processor Architecture)** next?