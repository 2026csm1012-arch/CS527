### Topic 6: Cache, TLB, and Cycle-Accurate Timing

To implement cycle-accurate timing impacts, you need to simulate a Translation Lookaside Buffer (TLB) to cache page table translations, and an L1 Data/Instruction Cache to hold frequently accessed physical memory blocks. We will assign realistic clock-cycle penalties to hits and misses and integrate them into the CPU's fetch and memory stages.

#### **Step 1: Define TLB, Cache, and Timing Constants (`memory.h`)**

We will implement a small fully-associative TLB (16 entries) and a direct-mapped L1 Cache (32 lines).

```c
// Inside memory.h
#include <stdint.h>
#include <stdbool.h>

#define TLB_SIZE 16
#define CACHE_LINES 32
#define CACHE_BLOCK_SIZE 16 // Bytes per cache line

// Cycle Penalties
#define CYCLE_TLB_HIT 1
#define CYCLE_TLB_MISS_PAGE_WALK 50
#define CYCLE_CACHE_HIT 1
#define CYCLE_CACHE_MISS_RAM 100

// TLB Entry Structure
typedef struct {
    uint32_t vpn;
    uint32_t pfn;
    bool valid;
    uint32_t last_used_tick; // For LRU replacement
} TLBEntry;

// Direct-Mapped Cache Line Structure
typedef struct {
    uint32_t tag;
    bool valid;
    // Data array would go here if we were caching actual values,
    // but for timing simulation, we just need the tags.
} CacheLine;

// Global Hardware Structures per Core
extern TLBEntry TLB[4][TLB_SIZE];      // 4 Cores[cite: 1]
extern CacheLine L1_Cache[4][CACHE_LINES];
extern uint32_t core_ticks[4];         // Local clock ticks for LRU

void init_cache_tlb(void);
int simulate_memory_access(int core_id, uint32_t logical_addr, uint32_t ptbr);

```

#### **Step 2: Implement Cache and TLB Lookup Logic (`memory.c`)**

Create the simulation logic that intercepts a memory request, checks the TLB, then checks the Cache, returning the total cycles consumed.

```c
// Inside memory.c
#include "memory.h"
#include "os.h"

TLBEntry TLB[4][TLB_SIZE];
CacheLine L1_Cache[4][CACHE_LINES];
uint32_t core_ticks[4] = {0};

void init_cache_tlb() {
    for (int core = 0; core < 4; core++) {
        for (int i = 0; i < TLB_SIZE; i++) TLB[core][i].valid = false;
        for (int i = 0; i < CACHE_LINES; i++) L1_Cache[core][i].valid = false;
    }
}

// Returns the number of clock cycles taken for this memory access
int simulate_memory_access(int core_id, uint32_t logical_addr, uint32_t ptbr) {
    int cycles = 0;
    core_ticks[core_id]++;

    uint32_t vpn = logical_addr / 512; // PAGE_SIZE = 512[cite: 1]
    uint32_t offset = logical_addr % 512;
    uint32_t pfn = 0;
    bool tlb_hit = false;

    // 1. Check TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[core_id][i].valid && TLB[core_id][i].vpn == vpn) {
            pfn = TLB[core_id][i].pfn;
            TLB[core_id][i].last_used_tick = core_ticks[core_id];
            tlb_hit = true;
            cycles += CYCLE_TLB_HIT;
            break;
        }
    }

    // 2. TLB Miss -> Hardware Page Walk (from Topic 3)
    if (!tlb_hit) {
        cycles += CYCLE_TLB_MISS_PAGE_WALK;

        // Use the hardware MMU to find the PFN (Assume hardware_mmu_translate from Topic 3)
        uint32_t physical_addr_out;
        if (!hardware_mmu_translate(ptbr, logical_addr, &physical_addr_out)) {
            return cycles; // Page fault occurred
        }
        pfn = physical_addr_out / 512;

        // Evict LRU from TLB and install new mapping
        int lru_index = 0;
        uint32_t oldest_tick = 0xFFFFFFFF;
        for (int i = 0; i < TLB_SIZE; i++) {
            if (!TLB[core_id][i].valid) { lru_index = i; break; }
            if (TLB[core_id][i].last_used_tick < oldest_tick) {
                oldest_tick = TLB[core_id][i].last_used_tick;
                lru_index = i;
            }
        }
        TLB[core_id][lru_index].valid = true;
        TLB[core_id][lru_index].vpn = vpn;
        TLB[core_id][lru_index].pfn = pfn;
        TLB[core_id][lru_index].last_used_tick = core_ticks[core_id];
    }

    // 3. Physical Address calculated
    uint32_t physical_addr = (pfn * 512) + offset;

    // 4. Check L1 Cache
    uint32_t cache_index = (physical_addr / CACHE_BLOCK_SIZE) % CACHE_LINES;
    uint32_t cache_tag = physical_addr / (CACHE_BLOCK_SIZE * CACHE_LINES);

    if (L1_Cache[core_id][cache_index].valid && L1_Cache[core_id][cache_index].tag == cache_tag) {
        // Cache Hit
        cycles += CYCLE_CACHE_HIT;
    } else {
        // Cache Miss
        cycles += CYCLE_CACHE_MISS_RAM;
        // Bring block into cache
        L1_Cache[core_id][cache_index].valid = true;
        L1_Cache[core_id][cache_index].tag = cache_tag;
    }

    return cycles;
}

```

#### **Step 3: Integrate Timing into Processor Execution (`processor.c`)**

Every time the processor fetches an instruction or reads/writes data memory, it must invoke the timing simulator and add the resulting penalties to the PCB's `cycles_consumed` metric (added in Topic 5).

```c
// Inside processor.c

void process_instructions(int core_id, int num_instructions) {
    PCB *current_pcb = get_pcb_on_core(core_id);
    if (!current_pcb) return;

    for (int i = 0; i < num_instructions; i++) {
        uint32_t current_pc = PC[core_id]; // logical address of instruction[cite: 1]

        // 1. Instruction Fetch Phase Timing
        int fetch_cycles = simulate_memory_access(core_id, current_pc, current_pcb->ptbr_frame);
        current_pcb->cycles_consumed += fetch_cycles;

        // Perform actual fetch()[cite: 1]
        // decode()[cite: 1]
        // opcode, dest, src1, src2 extracted...

        // 2. Memory Execution Phase Timing (If opcode is a LOAD or STORE)
        // Assume opcode 0x10 is LOAD and 0x11 is STORE (Replace with your actual memory opcodes from compiler.h)
        if (opcode == OP_LOAD || opcode == OP_STORE) {
            uint32_t target_logical_addr = Register[core_id][src1]; // Address to read/write

            int mem_cycles = simulate_memory_access(core_id, target_logical_addr, current_pcb->ptbr_frame);
            current_pcb->cycles_consumed += mem_cycles;
        }

        // Perform actual execute()[cite: 1]
        current_pcb->instructions_executed++;
    }
}

```

#### **Step 4: Handle Context Switches (`os.c`)**

When a context switch occurs, the new process has a different address space. To prevent the new process from getting unauthorized TLB hits from the previous process's mappings, you must flush the TLB.

```c
// Inside os.c -> scheduler_tick() (from Topic 1 & 2)

// When assigning a new process to a free core:
if (is_core_free(core_id)) {
    PCB *next = select_next_process();
    if (next != NULL) {
        next->state = PROC_RUNNING;
        next->assigned_core = core_id;
        context_restore(next, core_id);

        // FLUSH TLB on context switch
        for (int i = 0; i < TLB_SIZE; i++) {
            TLB[core_id][i].valid = false;
        }
        // Note: L1 Cache does not strictly need to be flushed if it's Physically Indexed,
        // Physically Tagged (PIPT), which it is in our simulation using physical_addr.
    }
}

```

### Verification & Testing

1. Add `init_cache_tlb()` to your `main.c` or `os_init()` startup logic.

2. Run the simulator in OS mode and use the `perf` command (from Topic 5) to observe the output.
3. **Expected Behavior:**

- A program with tight spatial locality (like `sum_array.txt` iterating over sequential memory) will show a high IPC (Instructions Per Cycle) because the Cache and TLB will hit frequently (costing only 1 cycle).

- A program with random memory accesses or one that jumps around frequently will suffer TLB and Cache misses (costing 50 to 100 cycles), resulting in a much lower IPC visible in the `perf` output.

Would you like to move on to **Topic 7: Pipelined Processor Architecture**?\

1. Cache & TLB Subsystem with Timing (tlb_cache.h)Path: tlb_cache.h
   Purpose: Implements Extension #6 by adding a Translation Lookaside Buffer (TLB) and an associative instruction/data cache with cycle-accurate timing penalties that integrate directly into the processor and memory lookup pipeline. C

```c
#ifndef TLB_CACHE_H
#define TLB_CACHE_H

#include <stdint.h>
#include <stdbool.h>

#define TLB_SIZE 16
#define L1_CACHE_LINES 32
#define L1_BLOCK_SIZE 16

typedef struct {
    uint32_t vpn;
    uint32_t pfn;
    bool valid;
    uint32_t lru_counter;
} TLBEntry;

typedef struct {
    uint32_t tag;
    bool valid;
    bool dirty;
    uint8_t data[L1_BLOCK_SIZE];
} CacheLine;

typedef struct {
    TLBEntry tlb[TLB_SIZE];
    CacheLine cache[L1_CACHE_LINES];
    uint64_t total_cycles;
    uint64_t tlb_hits;
    uint64_t tlb_misses;
    uint64_t cache_hits;
    uint64_t cache_misses;
} MemorySubsystemState;

void init_memory_subsystem(MemorySubsystemState *sub);
bool tlb_lookup(MemorySubsystemState *sub, uint32_t vpn, uint32_t *pfn);
bool cache_access(MemorySubsystemState *sub, uint32_t physical_addr, bool is_write);
uint64_t get_elapsed_cycles(const MemorySubsystemState *sub);

#endif

```

2. Physical Memory-Stored Page Tables (physical_pt.h)Path: physical_pt.h
   Purpose: Implements Extension #3 by shifting page table storage from OS host data structures directly into allocated physical RAM frames, managed via a Page Table Base Register (PTBR).

```c
#ifndef PHYSICAL_PT_H
#define PHYSICAL_PT_H

#include <stdint.h>
#include <stdbool.h>

#define PTE_SIZE 4 // 4 bytes per Page Table Entry: [31:12] PFN, [0] Valid

void init_physical_page_table(uint8_t *physical_memory, uint32_t ptbr_address, uint32_t vpn, uint32_t pfn);
bool walk_physical_page_table(const uint8_t *physical_memory, uint32_t ptbr_address, uint32_t virtual_address, uint32_t page_size, uint32_t *physical_address);

#endif
```

3. Memory-Mapped I/O Controller (mmio.h)Path: mmio.h
   Purpose: Implements Extension #4 by intercepting memory loads and stores targeting designated MMIO address ranges (e.g., terminal output at 0xFF000004) and routing them to peripheral device handlers.

```c
#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t (*MMIOReadHandler)(uint32_t address);
typedef void (*MMIOWriteHandler)(uint32_t address, uint32_t value);

typedef struct {
    uint32_t start_addr;
    uint32_t end_addr;
    MMIOReadHandler read_fn;
    MMIOWriteHandler write_fn;
} MMIODevice;

#define MAX_MMIO_DEVICES 4

typedef struct {
    MMIODevice devices[MAX_MMIO_DEVICES];
    int device_count;
} MMIOBus;

void init_mmio_bus(MMIOBus *bus);
bool register_mmio_device(MMIOBus *bus, uint32_t start, uint32_t end, MMIOReadHandler r_fn, MMIOWriteHandler w_fn);
bool handle_mmio_read(MMIOBus *bus, uint32_t address, uint32_t *out_value);
bool handle_mmio_write(MMIOBus *bus, uint32_t address, uint32_t value);

#endif
```

4. Secure Page Table with Access Permissions (secure_paging.h)
   Path: secure_paging.h
   Purpose: Implements Extension #9 by embedding Read (R), Write (W), Execute (X), and User/Supervisor privilege bits into page table entries to trap permission violations.

```c
#ifndef SECURE_PAGING_H
#define SECURE_PAGING_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ACC_READ,
    ACC_WRITE,
    ACC_EXECUTE
} AccessType;

typedef enum {
    PRIV_USER = 0,
    PRIV_SUPERVISOR = 1
} PrivilegeLevel;

typedef struct {
    uint32_t pfn : 20;
    bool valid : 1;
    bool readable : 1;
    bool writable : 1;
    bool executable : 1;
    bool user_accessible : 1;
} SecurePTE;

bool validate_secure_access(SecurePTE *pte_entry, AccessType access, PrivilegeLevel current_priv);

#endif

```

5. System Utilities & Performance Viewer (sys_utils.h)
   Path: sys_utils.h
   Purpose: Implements Extension #5 by providing runtime diagnostic hooks to dump memory maps, inspect process control blocks (PCBs), and monitor core utilization metrics.

````c
#ifndef SYS_UTILS_H
#define SYS_UTILS_H

#include <stdio.h>
#include <stdint.h>

void dump_memory_map(const uint8_t *physical_memory, size_t total_size, size_t page_size);
void print_process_performance(int pid, uint64_t instruction_count, uint64_t cycles_taken, double cache_hit_rate);
void dump_system_status_table(void);

#endif

    ```

    6. Multi-Processor Scheduler (multiprocessor.h)Path: multiprocessor.h
Purpose: Implements Extension #1a by upgrading the single-processor/pseudo-multitasking scheduler into a true multi-core scheduling engine capable of dispatching concurrent tasks across all $N$ physical CPU cores simultaneously.

```c
#ifndef MULTIPROCESSOR_H
#define MULTIPROCESSOR_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_CORES 4

typedef struct {
    int core_id;
    int current_pid;
    bool is_active;
    uint64_t instructions_executed_slice;
} CPUCore;

typedef struct {
    CPUCore cores[MAX_CORES];
    int active_core_count;
} MultiProcessorSystem;

void init_multiprocessor(MultiProcessorSystem *mp_sys, int num_cores);
int assign_process_to_free_core(MultiProcessorSystem *mp_sys, int pid);
void handle_core_time_slice(MultiProcessorSystem *mp_sys);

#endif
````

7. Advanced CPU Scheduling Schemes (schedulers.h)Path: schedulers.h
   Purpose: Implements Extension #2 by replacing basic Round-Robin with priority-based and Multi-Level Feedback Queue (MLFQ) scheduling algorithms.

```c
#ifndef SCHEDULERS_H
#define SCHEDULERS_H

#include <stdint.h>

typedef enum {
    SCHED_ROUND_ROBIN,
    SCHED_PRIORITY,
    SCHED_MLFQ
} SchedulerType;

typedef struct {
    int pid;
    int priority;
    int queue_level; // For MLFQ
    uint64_t remaining_burst;
} SchedulableProcess;

int select_next_process_priority(SchedulableProcess *processes, int count);
int select_next_process_mlfq(SchedulableProcess *processes, int count);

#endif
```

8. Pipelined Processor Architecture (pipeline.h)Path: pipeline.h
   Purpose: Implements Extension #7 by decomposing instruction execution into a classical 5-stage RISC pipeline (Fetch, Decode, Execute, Memory, Writeback) with hazard detection and forwarding paths. C

```c
#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int pc;
    uint32_t instruction;
    bool active;
} PipelineStage;

typedef struct {
    PipelineStage fetch;
    PipelineStage decode;
    PipelineStage execute;
    PipelineStage memory;
    PipelineStage writeback;
    bool stall_flag;
} ProcessorPipeline;

void init_pipeline(ProcessorPipeline *pipe);
void step_pipeline(ProcessorPipeline *pipe, const uint8_t *instruction_memory);

#endif
```

9. Inter-Process Communication via Shared Memory (ipc_shm.h)Path: ipc_shm.h
   Purpose: Implements Extension #8 by establishing shared memory segments mapped across multiple process page tables, enabling zero-copy data exchange between distinct processes.

````c
#ifndef IPC_SHM_H
#define IPC_SHM_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SHM_REGIONS 8
#define SHM_REGION_SIZE 512

typedef struct {
    int shm_id;
    uint8_t data[SHM_REGION_SIZE];
    int attached_pids[4];
    int ref_count;
} SharedMemoryRegion;

typedef struct {
    SharedMemoryRegion regions[MAX_SHM_REGIONS];
} SHMManager;

void init_shm_manager(SHMManager *manager);
int create_shm_region(SHMManager *manager, int shm_id);
bool attach_shm_to_process(SHMManager *manager, int shm_id, int pid, uint32_t virtual_addr);

#endif```

10. Register Allocation & Spill Management (reg_alloc.h)Path: reg_alloc.h
Purpose: Implements Extension #10 by providing graph-coloring liveness analysis and register-spill routines to map arbitrary virtual registers onto physical CPU registers.  C

```c
#ifndef REG_ALLOC_H
#define REG_ALLOC_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_VIRTUAL_REGS 256
#define PHYSICAL_REGS_AVAILABLE 16

typedef struct {
    int vreg;
    int assigned_preg;
    bool is_spilled;
    int stack_spill_offset;
} AllocationMapping;

void compute_liveness_and_allocate(int *instruction_stream, int instruction_count, AllocationMapping *out_map);

#endif```

````
