### Topic 4: Implement Memory-Mapped I/O (MMIO)

To implement MMIO, you must designate a region of the physical address space—outside the standard 8192-byte RAM—for hardware devices. Standard CPU `LOAD` and `STORE` instructions remain exactly the same; however, the memory controller intercepts operations targeting these specific physical addresses and routes them to I/O device handlers instead of the physical memory array.

#### **Step 1: Define MMIO Address Space (`memory.h`)**

Define the physical addresses representing the peripheral devices. Since physical RAM occupies addresses `0` to `8191` (16 frames of 512 bytes), we place the MMIO space higher up in memory (e.g., starting at physical frame number 20).

```c
// Inside memory.h

// Designate Frame 20 for MMIO (Physical Address: 20 * 512 = 10240)
#define MMIO_FRAME_PFN 20
#define MMIO_BASE_PADDR (MMIO_FRAME_PFN * 512)

// Specific Memory-Mapped Device Registers
#define MMIO_UART_TX_PADDR (MMIO_BASE_PADDR + 0) // Write here to print a character
#define MMIO_UART_RX_PADDR (MMIO_BASE_PADDR + 4) // Read here to get keyboard input (mocked)
#define MMIO_TIMER_PADDR   (MMIO_BASE_PADDR + 8) // Read here to get current system ticks

```

#### **Step 2: Intercept Read/Write Operations (`memory.c`)**

Modify the 32-bit memory access functions to check the translated physical address. If it falls within the MMIO region, bypass the physical RAM array completely and execute the corresponding C functions (like `putchar`).

```c
// Inside memory.c
#include <stdio.h>
#include "memory.h"
#include "os.h" // To access global_os_ticks (if implemented in Topic 2)

extern int global_os_ticks; // Or any counter you use for time

// Helper to handle MMIO Writes
void handle_mmio_write(uint32_t paddr, uint32_t val) {
    if (paddr == MMIO_UART_TX_PADDR) {
        // Output device: Print character to terminal
        putchar((char)val);
        fflush(stdout);
    } else {
        printf("[MMIO ERROR] Write to unhandled MMIO address: %d\n", paddr);
    }
}

// Helper to handle MMIO Reads
uint32_t handle_mmio_read(uint32_t paddr) {
    if (paddr == MMIO_TIMER_PADDR) {
        // Timer device: Return system time/ticks
        return global_os_ticks;
    }
    printf("[MMIO ERROR] Read from unhandled MMIO address: %d\n", paddr);
    return 0;
}

// Update existing mem_write_32[cite: 1]
void mem_write_32(uint32_t logical_addr, uint32_t val) {
    uint32_t physical_addr;

    // Assume hardware_mmu_translate() from Topic 3, or your existing MMU[cite: 1]
    if (!hardware_mmu_translate(logical_addr, &physical_addr)) {
        return; // Page fault
    }

    // Intercept MMIO
    if (physical_addr >= MMIO_BASE_PADDR) {
        handle_mmio_write(physical_addr, val);
        return;
    }

    // Normal RAM Write (split val into 4 bytes and store in physical_memory array)[cite: 1]
    // ...
}

// Update existing mem_read_32[cite: 1]
uint32_t mem_read_32(uint32_t logical_addr) {
    uint32_t physical_addr;

    if (!hardware_mmu_translate(logical_addr, &physical_addr)) {
        return 0; // Page fault
    }

    // Intercept MMIO
    if (physical_addr >= MMIO_BASE_PADDR) {
        return handle_mmio_read(physical_addr);
    }

    // Normal RAM Read (reconstruct 32-bit int from physical_memory array)[cite: 1]
    // ...
}

```

#### **Step 3: Map the MMIO Frame into the Process Page Table (`os.c`)**

For a user program to access the MMIO physical frame, the OS must map it to a logical page during the `loader()` phase. We will dedicate the final logical page (Page 9) to I/O devices.

```c
// Inside os.c -> loader()

void loader(const char *filename) {
    // ... initialize PCB ...[cite: 1]

    // Allocate normal RAM frames for Code (Pages 0-1) and Data (Pages 2-8)[cite: 1]
    for (int vpn = 0; vpn < 9; vpn++) {
        int target_frame = allocate_physical_frame();
        write_pte_to_physical(new_pcb->ptbr_frame, vpn, target_frame); // From Topic 3
        new_pcb->logical_pages[vpn] = target_frame;
    }

    // Map Logical Page 9 directly to the hardware MMIO Physical Frame
    // We do NOT allocate physical RAM for this; we hardcode the MMIO PFN.
    write_pte_to_physical(new_pcb->ptbr_frame, 9, MMIO_FRAME_PFN);
    new_pcb->logical_pages[9] = MMIO_FRAME_PFN;

    // ... assign processor or put in WAITING ...[cite: 1]
}

```

### Verification & Testing

To test this, write an assembly program that writes to Logical Page 9. Since the page size is 512 bytes, Logical Page 9 starts at logical address `4608` ($9 \times 512$).

1. **Create a test file (`tests/mmio_test.txt`)**:

- Address `4608` maps to `MMIO_UART_TX_PADDR`.
- Address `4616` maps to `MMIO_TIMER_PADDR`.

```text
// Load character 'H' (ASCII 72) into register x1
x1 = 72

// Store register x1 into logical memory at address 4608 (UART TX)
STORE x1, 4608

// Load character 'I' (ASCII 73) into register x1
x1 = 73
STORE x1, 4608

// Read the current system timer from address 4616 (TIMER RX)
LOAD x2, 4616

// Print the timer value using the simulator's built-in print (if available)
PRINT x2
HALT

```

2. **Execute**: Run `./simulator tests/mmio_test.txt`.

3. **Expected Output**: The simulator should immediately print `HI` directly to your terminal, bypassing the normal output data file (`data_out_pid*.byte`). The timer value will also be logged based on how many ticks the OS scheduler has executed.

#

#

#

##

#

#

#

# Extension 5 Implementation Guide: Memory Maps and Process Performance Utilities

**Course:** CS527 Lab 5 (Mini-Computer / OS Simulator)  
**Feature Added:** Diagnostic utilities (`memmap` and `perf`) for inspecting MMU page table mappings and process performance states via the interactive OS shell.

---

## 1. Summary of Changes

To implement Extension 5 without disrupting existing scheduling or core instruction execution loops, two utility functions were added to the operating system layer (`os.c` and `os.h`):

- **`os_print_memory_map()`**: Iterates through all active Process Control Blocks (PCBs) and reads the per-processor page tables (`pageTable[NP][NUM_LOGICAL_PAGES]`), displaying logical page-to-physical frame allocations.
- **`os_print_performance()`**: Iterates through the global process array to report PIDs, assigned processor IDs, current process states (`READY`, `RUNNING`, `WAITING`, `TERMINATED`), and target program source paths.
- **Interactive Shell Command Extension**: Added command parsers inside the OS shell loop to capture `memmap` and `perf` inputs.

---

## 2. Updated Header File (`os.h`)

```c
#ifndef OS_H
#define OS_H

#include <stdint.h>

#define MAX_PROCESSES 64
#define TIME_SLICE 10
#define NUM_LOGICAL_PAGES 10
#define NP 4

// Process states
typedef enum {
    PROC_UNUSED = 0,
    PROC_WAITING,
    PROC_READY,
    PROC_RUNNING,
    PROC_TERMINATED
} ProcessState;

// Process Control Block (PCB)
typedef struct {
    int pid;
    int processor_id;
    ProcessState state;
    char source_file[256];
    int data_size;
} PCB;

// Global OS structures declared in os.c
extern PCB pcbs[MAX_PROCESSES];
extern int pageTable[NP][NUM_LOGICAL_PAGES];

// Core OS functions
void os_init(void);
int loader(const char *source_file, const char *data_file);
void scheduler(void);
void os_finalize_memory(int pid);

// New utility functions for Extension 5
void os_print_memory_map(void);
void os_print_performance(void);

#endif // OS_H


```

3. Updated Source Implementation (os.c)Add the following functions and update your interactive shell loop within os.c:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "memory.h"
#include "processor.h"
#include "compiler.h"

// Global state variables
PCB pcbs[MAX_PROCESSES];
int pageTable[NP][NUM_LOGICAL_PAGES];

/**
 * Data Flow: Iterates through all active/running PCBs and their page table mappings.
 * Working: Prints logical page to physical frame allocations for active processes.
 * Significance: Fulfills Extension 5 memory map utility requirements.
 */
void os_print_memory_map(void) {
    printf("\n=== System Memory Map (Logical -> Physical) ===\n");
    int active_found = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pcbs[i].state != PROC_UNUSED && pcbs[i].state != PROC_TERMINATED) {
            active_found = 1;
            printf("PID: %d | Processor ID: %d | State: %d\n", pcbs[i].pid, pcbs[i].processor_id, pcbs[i].state);
            for (int p = 0; p < NUM_LOGICAL_PAGES; p++) {
                int frame = pageTable[pcbs[i].processor_id][p];
                if (frame != -1) {
                    printf("  Logical Page %2d -> Physical Frame %2d\n", p, frame);
                }
            }
        }
    }
    if (!active_found) {
        printf("No active processes found in memory.\n");
    }
    printf("===============================================\n");
}

/**
 * Data Flow: Iterates through PCBs to display process execution metrics.
 * Working: Summarizes PID, current state, assigned processor, and program source.
 * Significance: Fulfills Extension 5 process performance viewing requirements.
 */
void os_print_performance(void) {
    printf("\n=== Process Performance & Status Report ===\n");
    printf("%-6s | %-6s | %-12s | %-20s\n", "PID", "ProcID", "State", "Program Source");
    printf("-----------------------------------------------\n");
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pcbs[i].state != PROC_UNUSED) {
            count++;
            const char *state_str = "UNKNOWN";
            switch (pcbs[i].state) {
                case PROC_WAITING:    state_str = "WAITING"; break;
                case PROC_READY:      state_str = "READY"; break;
                case PROC_RUNNING:    state_str = "RUNNING"; break;
                case PROC_TERMINATED: state_str = "TERMINATED"; break;
                default: break;
            }
            printf("%-6d | %-6d | %-12s | %-20s\n",
                pcbs[i].pid,
                pcbs[i].processor_id,
                state_str,
                pcbs[i].source_file);
        }
    }
    if (count == 0) {
        printf("No processes recorded.\n");
    }
    printf("===============================================\n");
}
```

4. Shell Command IntegrationInside your interactive command processing loop in os.c, include checks for the new commands:

```c
char command[256];
while (1) {
    printf("OS-Shell> ");
    if (fgets(command, sizeof(command), stdin) == NULL) break;

    // Remove trailing newline characters
    command[strcspn(command, "\r\n")] = 0;

    if (strlen(command) == 0) continue;

    if (strcmp(command, "exit") == 0) {
        break;
    } else if (strcmp(command, "memmap") == 0) {
        os_print_memory_map();
    } else if (strcmp(command, "perf") == 0) {
        os_print_performance();
    } else {
        // Fallback: treat input as a program path to load
        loader(command, NULL);
    }
}

```

5. Verification and Testing
   Rebuild the simulation environment[cite: 1]:

Bash
make clean
make
Launch the OS interactive shell[cite: 1]:

Bash
./simulator
Submit a workload (e.g., tests/sum_n_fixed.txt)[cite: 1].

Type memmap to inspect physical frame mappings or perf to review scheduler performance metrics[cite: 1].
