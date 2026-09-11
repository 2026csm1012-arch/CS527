### Topic 2: Advanced CPU Scheduling Schemes (FCFS, SJF, Priority, & MLFQ)

To upgrade the existing Round-Robin scheduler (`TIME_SLICE = 10`) into a multi-algorithm scheduling engine, you need to expand the Process Control Block (PCB) to track scheduling metadata and replace the standard FIFO queue extraction with algorithm-specific selection logic.

#### **Step 1: Update the PCB and OS Headers (`os.h`)**

Expand the `PCB` structure to hold the necessary metrics for all scheduling types. Add a global configuration enum so the simulator can toggle between algorithms at runtime.

```c
// Inside os.h
typedef enum {
    SCHED_RR,       // Existing Round Robin[cite: 1]
    SCHED_FCFS,     // First-Come, First-Served
    SCHED_SJF,      // Shortest Job First
    SCHED_PRIORITY, // Priority Scheduling
    SCHED_MLFQ      // Multi-Level Feedback Queue
} SchedulerType;

extern SchedulerType current_scheduler;

typedef struct {
    int pid;
    int state;
    int assigned_core;

    // --- Scheduling Metadata ---
    int arrival_time;         // For FCFS (Global tick when loaded)
    int expected_burst_time;  // For SJF (Can be estimated by program bytecode size)
    int priority;             // For Priority (Lower value = higher priority)
    int mlfq_level;           // For MLFQ (0 = highest priority queue, 1 = medium, etc.)
    int time_slice_used;      // Tracks how much of the quantum was consumed

    // Hardware Context (from Topic 1)
    uint32_t saved_pc;
    // ... existing registers, program name, logical pages ...[cite: 1]
} PCB;

```

#### **Step 2: Implement the Scheduler Selection Logic (`os.c`)**

Instead of blindly popping the first process off a waiting queue, implement a search function that scans all `PROC_READY` processes and selects the best candidate based on the active algorithm.

```c
// Inside os.c
#include <limits.h>

SchedulerType current_scheduler = SCHED_MLFQ; // Default to MLFQ for testing
int global_os_ticks = 0; // Increment this every scheduler_tick()

// Assume process_table[MAX_PROCESSES] holds all PCBs
extern PCB process_table[];

PCB* select_next_process() {
    PCB *best_candidate = NULL;

    for (int i = 0; i < 64; i++) { // MAX_PROCESSES = 64[cite: 1]
        if (process_table[i].state == PROC_READY) {

            // If this is the first ready process we found, tentatively pick it
            if (best_candidate == NULL) {
                best_candidate = &process_table[i];
                continue;
            }

            // Compare against the best candidate based on the active algorithm
            switch (current_scheduler) {
                case SCHED_FCFS:
                    if (process_table[i].arrival_time < best_candidate->arrival_time) {
                        best_candidate = &process_table[i];
                    }
                    break;

                case SCHED_SJF:
                    if (process_table[i].expected_burst_time < best_candidate->expected_burst_time) {
                        best_candidate = &process_table[i];
                    }
                    break;

                case SCHED_PRIORITY:
                    if (process_table[i].priority < best_candidate->priority) { // Lower is better
                        best_candidate = &process_table[i];
                    }
                    break;

                case SCHED_MLFQ:
                    // Priority queue 0 beats 1, beats 2. Tie-break with arrival time (FCFS)
                    if (process_table[i].mlfq_level < best_candidate->mlfq_level) {
                        best_candidate = &process_table[i];
                    } else if (process_table[i].mlfq_level == best_candidate->mlfq_level) {
                        if (process_table[i].arrival_time < best_candidate->arrival_time) {
                            best_candidate = &process_table[i];
                        }
                    }
                    break;

                case SCHED_RR:
                default:
                    // For standard RR, fall back to the old FIFO logic (arrival time)
                    if (process_table[i].arrival_time < best_candidate->arrival_time) {
                        best_candidate = &process_table[i];
                    }
                    break;
            }
        }
    }
    return best_candidate;
}

```

#### **Step 3: Modify the Scheduler Tick for Preemption & MLFQ Demotion (`os.c`)**

Update your execution loop. FCFS and SJF are typically non-preemptive, meaning they run until termination. MLFQ and Priority are preemptive.

```c
// Inside os.c -> scheduler_tick()

void scheduler_tick() {
    global_os_ticks++;

    // 1. Check currently running processes
    for (int core_id = 0; core_id < 4; core_id++) { // NP = 4[cite: 1]
        PCB *current = get_pcb_on_core(core_id);
        if (current != NULL) {

            if (current->state == PROC_TERMINATED) {
                os_finalize_memory(current->pid);[cite: 1]
                current->assigned_core = -1;
            }
            else if (current->state == PROC_RUNNING) {
                // If using MLFQ, RR, or Priority, enforce time slices
                if (current_scheduler == SCHED_MLFQ || current_scheduler == SCHED_RR || current_scheduler == SCHED_PRIORITY) {

                    context_save(current, core_id);
                    current->state = PROC_READY;
                    current->assigned_core = -1;

                    // MLFQ Logic: If it used a full time slice without finishing, demote it
                    if (current_scheduler == SCHED_MLFQ) {
                        if (current->mlfq_level < 3) { // Max 3 queue levels
                            current->mlfq_level++;
                        }
                    }
                }
                // If FCFS or SJF, do nothing here. Let it keep the core.
            }
        }

        // 2. Assign new processes to free cores
        if (is_core_free(core_id)) {
            PCB *next = select_next_process();
            if (next != NULL) {
                next->state = PROC_RUNNING;
                next->assigned_core = core_id;
                context_restore(next, core_id);
            }
        }
    }

    // 3. Execute instructions
    for (int core_id = 0; core_id < 4; core_id++) {
        if (!is_core_free(core_id)) {
            // For FCFS/SJF, run larger chunks. For RR/MLFQ, run TIME_SLICE[cite: 1]
            int run_amount = (current_scheduler == SCHED_FCFS || current_scheduler == SCHED_SJF) ? 100 : TIME_SLICE;
            process_instructions(core_id, run_amount);[cite: 1]
        }
    }
}

```

#### **Step 4: Update the Loader to Initialize Metadata (`os.c`)**

When a new program is submitted via the interactive shell, the `loader()` must initialize the new PCB scheduling fields.

```c
// Inside loader() in os.c
void loader(const char *filename) {
    // ... existing compile() and allocate pages logic ...[cite: 1]

    PCB *new_pcb = create_pcb();
    new_pcb->arrival_time = global_os_ticks;
    new_pcb->mlfq_level = 0; // Always start in the highest priority queue

    // Mock SJF estimation: use the number of logical pages or bytecode size
    new_pcb->expected_burst_time = get_program_bytecode_size(filename);

    // Priority can be assigned randomly for testing, or parsed from a command argument
    new_pcb->priority = rand() % 5;

    // ... assign processor or put in WAITING ...[cite: 1]
}

```

### Verification & Testing

1. **Testing FCFS/SJF:** Temporarily set `current_scheduler = SCHED_SJF;`. Load a massive program (e.g., `tests/sum_arrays_runtime.txt`) followed immediately by a tiny program (`tests/sum_n_fixed.txt`).

- _Expected:_ The scheduler should bypass the large program and assign the tiny program to the core first, running it completely to termination before switching back.

2. **Testing MLFQ:** Set `current_scheduler = SCHED_MLFQ;`. Add a `printf` in the demotion block: `printf("PID %d demoted to level %d\n", current->pid, current->mlfq_level);`.

- _Expected:_ Long-running mathematical programs like `complex_multiply.txt` or `matrix_det.txt` should print demotion logs as they consume multiple `TIME_SLICE` rounds, while short tasks stay at level 0.
