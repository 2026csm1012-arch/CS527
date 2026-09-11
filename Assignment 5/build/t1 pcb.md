### **Topic 1 & 1a: Context Switching & True Multiprocessing**

To decouple processes from hardware, we must give the Process Control Block (PCB) the ability to store a complete snapshot of the CPU's state. This allows a single CPU (Topic 1) or a pool of $N$ CPUs (Topic 1a) to safely swap processes in and out during scheduling without corrupting registers.

#### **Step 1: Expand the PCB for Context Storage (`os.h`)**

Update your `PCB` structure to include storage for the Program Counter, 256 scalar registers, 32 vector registers, and the 4 condition flags.

```c
// Inside os.h
typedef struct {
    int pid;
    int state;
    int assigned_core; // -1 if waiting

    // --- Hardware Context Snapshot ---
    uint32_t saved_pc;
    int32_t saved_scalar_regs[256];
    int32_t saved_vector_regs[32][8];
    
    // Condition flags (Z=Zero, N=Negative, C=Carry, V=Overflow)
    int saved_Z;
    int saved_N;
    int saved_C;
    int saved_V;

    // Existing fields...
    char program_name[256];
    int logical_pages[10]; 
} PCB;

```

#### **Step 2: Implement Context Save & Restore APIs (`processor.c`)**

Create functions to transfer data between the active CPU core (represented by your processor arrays) and the PCB.

```c
// Inside processor.c (Expose these in processor.h)
#include "os.h"

extern int32_t Register[4][256];          // Existing global CPU registers
extern int32_t VectorRegister[4][32][8];  // Existing global vector registers
extern uint32_t PC[4];                    // Existing Program Counters
extern int Z[4], N[4], C[4], V[4];        // Existing condition flags

void context_save(PCB *pcb, int core_id) {
    // 1. Save Program Counter
    pcb->saved_pc = PC[core_id];

    // 2. Save condition flags
    pcb->saved_Z = Z[core_id];
    pcb->saved_N = N[core_id];
    pcb->saved_C = C[core_id];
    pcb->saved_V = V[core_id];

    // 3. Save scalar registers
    for (int i = 0; i < 256; i++) {
        pcb->saved_scalar_regs[i] = Register[core_id][i];
    }

    // 4. Save vector registers
    for (int v = 0; v < 32; v++) {
        for (int lane = 0; lane < 8; lane++) {
            pcb->saved_vector_regs[v][lane] = VectorRegister[core_id][v][lane];
        }
    }
}

void context_restore(PCB *pcb, int core_id) {
    // 1. Restore Program Counter
    PC[core_id] = pcb->saved_pc;

    // 2. Restore condition flags
    Z[core_id] = pcb->saved_Z;
    N[core_id] = pcb->saved_N;
    C[core_id] = pcb->saved_C;
    V[core_id] = pcb->saved_V;

    // 3. Restore scalar registers
    for (int i = 0; i < 256; i++) {
        Register[core_id][i] = pcb->saved_scalar_regs[i];
    }

    // 4. Restore vector registers
    for (int v = 0; v < 32; v++) {
        for (int lane = 0; lane < 8; lane++) {
            VectorRegister[core_id][v][lane] = pcb->saved_vector_regs[v][lane];
        }
    }
}

```

#### **Step 3: Update the OS Scheduler (`os.c`)**

Modify the Round-Robin scheduler to use the context switch functions. This implementation inherently handles **Topic 1a (Multiprocessor)** by looping over all available `NUM_CORES`. Set `NUM_CORES = 1` for **Topic 1 (Single Processor)**.

```c
// Inside os.c

#define NUM_CORES 4 // Set to 1 for strict Topic 1 (Single Core), 4 for Topic 1a (Multi-core)
#define TIME_SLICE 10 // Instructions per slice

void scheduler_tick() {
    for (int core_id = 0; core_id < NUM_CORES; core_id++) {
        PCB *current_pcb = get_pcb_on_core(core_id);

        if (current_pcb != NULL) {
            // Process has finished its 10-instruction slice[cite: 1]
            if (current_pcb->state == PROC_RUNNING) {
                // Topic 1: Save context before removing from core
                context_save(current_pcb, core_id);
                current_pcb->state = PROC_READY;
                current_pcb->assigned_core = -1;
                enqueue_ready_queue(current_pcb); 
            } else if (current_pcb->state == PROC_TERMINATED) {
                // Free memory and clean up[cite: 1]
                os_finalize_memory(current_pcb->pid);
                current_pcb->assigned_core = -1;
            }
        }

        // Topic 1a: Assign the next waiting process to this free core
        if (is_core_free(core_id)) {
            PCB *next_pcb = dequeue_ready_queue();
            if (next_pcb != NULL) {
                next_pcb->state = PROC_RUNNING;
                next_pcb->assigned_core = core_id;
                
                // Topic 1: Restore context into hardware before executing
                context_restore(next_pcb, core_id);
            }
        }
    }

    // Execute the time slice for all active cores
    for (int core_id = 0; core_id < NUM_CORES; core_id++) {
        if (!is_core_free(core_id)) {
            process_instructions(core_id, TIME_SLICE);
        }
    }
}

```


To verify that your context switching and multitasking scheduler is working flawlessly, you need to prove that a program's state is preserved exactly as it was before it was paused.

Here is the step-by-step verification strategy to test the new implementations.

### 1. Add Strategic Debug Logging

Before compiling, temporarily add `printf` statements inside your new `context_save` and `context_restore` functions to trace the scheduler's behavior in the terminal.

```c
// Inside context_save
printf("[SCHEDULER] Pausing PID %d on Core %d (Saved PC: %d)\n", pcb->pid, core_id, pcb->saved_pc);

// Inside context_restore
printf("[SCHEDULER] Resuming PID %d on Core %d (Restored PC: %d)\n", pcb->pid, core_id, pcb->saved_pc);

```

### 2. Compile and Run the Baseline Test

Ensure your compilation pipeline isn't broken and that the single-program execution path still works without interference.

1. Run the build commands: `make clean` followed by `make`.


2. Run a single program in standalone mode: `./simulator tests/sum_n_fixed.txt`.


3. **Expected Result:** The program should complete successfully. If `NUM_CORES = 1`, you should see it load, execute, and generate the expected output (e.g., a mathematical sum of 55 for N=10) without any context switching logs, since it never has to yield the CPU to another process.



### 3. The Core Overload Test (Forcing Context Switches)

To truly test context saving, you must force the system to run out of physical cores, forcing processes to share the CPU time slices.

1. Launch the interactive OS shell by running the simulator without arguments: `./simulator`.


2. If `NUM_CORES` is set to 4, submit 5 or 6 programs rapidly to the shell so that at least one process gets sent to the waiting queue:


```text
$ tests/sum_n_fixed.txt
$ tests/matrix_det.txt
$ tests/fir_filter.txt
$ tests/complex_multiply.txt
$ tests/vector_add.txt

```


3. Type `status` in the shell while they run.


4. **Expected Result:**
* You should see your debug logs printing `[SCHEDULER] Pausing PID...` and `[SCHEDULER] Resuming PID...`.
* The `status` command should show programs alternating between `RUNNING`, `READY`, and `WAITING` states.


* If testing Topic 1 specifically (by setting `NUM_CORES = 1`), you will see heavy interleaved logging as a single core juggles all submitted tasks round-robin style.



### 4. Mathematical Output Validation

The ultimate proof of correct context switching is data integrity. If registers are corrupted during a save/restore cycle, the mathematical outputs will be entirely wrong.

1. After all programs terminate, check the generated output memory dumps (e.g., `data_out_pid1.byte`, `data_out_pid2.byte`).


2. Check the `simulation.log` file.


3. **Expected Result:** Compare the outputs against known good values. For example, `tests/complex_multiply.txt` should still output `Real = -5` and `Imag = 10`, and `tests/sum_n_fixed.txt` must still equal `55`. If you get garbage numbers, it means your `context_save` or `context_restore` functions missed a register, or you are swapping the wrong PCB structure.



Do you want to proceed to **Topic 2: Advanced CPU Scheduling Schemes (Priority & MLFQ)**, or do you need help debugging any issues from this first topic?