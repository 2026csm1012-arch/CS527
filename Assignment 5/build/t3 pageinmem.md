### Topic 3: Storing Page Tables in Physical Memory (Hardware Table Walk)

To implement this extension, you must remove the OS-level 2D array (`pageTable[NP][NUM_LOGICAL_PAGES]`). Instead, the OS must allocate a dedicated physical memory frame to hold the page table entries (PTEs) for each process. The CPU's Memory Management Unit (MMU) will then perform a "hardware table walk" by reading directly from the `physical_memory` array to translate addresses.

#### **Step 1: Update the PCB to track the Page Table Base Register (`os.h`)**

Each process needs a pointer to the physical frame that holds its page table.

```c
// Inside os.h
typedef struct {
    int pid;
    int state;
    int assigned_core;
    
    // --- Hardware Page Table Base Register (PTBR) ---
    // Stores the physical frame number (PFN) where this process's page table resides
    uint32_t ptbr_frame; 

    // ... existing hardware context, program name, etc. ...[cite: 1]
} PCB;

```

#### **Step 2: Implement PTE Read/Write in Physical Memory (`memory.c`)**

Since the simulator uses an 8192-byte physical memory array with 512-byte pages, we encode a 32-bit (4-byte) PTE and store it byte-by-byte into the `physical_memory` array.

```c
// Inside memory.c

// Assuming physical memory is defined globally[cite: 1]
extern uint8_t physical_memory[8192]; 
#define PAGE_SIZE 512

// OS uses this to write mappings into the physical page table during loading
void write_pte_to_physical(uint32_t ptbr_frame, uint32_t vpn, uint32_t target_pfn) {
    // 1. Calculate where the Page Table starts in physical RAM
    uint32_t pt_base_addr = ptbr_frame * PAGE_SIZE;
    
    // 2. Calculate the exact address of the PTE (4 bytes per entry)
    uint32_t pte_addr = pt_base_addr + (vpn * 4);
    
    // 3. Encode PTE: bits [31:1] = target_pfn, bit [0] = Valid bit (1)
    uint32_t pte_val = (target_pfn << 1) | 1;
    
    // 4. Write 32-bit value into 8-bit physical memory array (Little Endian)
    physical_memory[pte_addr]     = pte_val & 0xFF;
    physical_memory[pte_addr + 1] = (pte_val >> 8) & 0xFF;
    physical_memory[pte_addr + 2] = (pte_val >> 16) & 0xFF;
    physical_memory[pte_addr + 3] = (pte_val >> 24) & 0xFF;
}

```

#### **Step 3: Implement the Hardware MMU Table Walk (`memory.c`)**

Replace your existing logical-to-physical address translation logic. The MMU must now fetch the PTE directly from physical memory.

```c
// Inside memory.c

// Hardware MMU translation used by processor fetch(), mem_read_32(), and mem_write_32()
bool hardware_mmu_translate(uint32_t ptbr_frame, uint32_t logical_addr, uint32_t *physical_addr) {
    uint32_t vpn = logical_addr / PAGE_SIZE;     // e.g., logical_addr / 512[cite: 1]
    uint32_t offset = logical_addr % PAGE_SIZE;  // e.g., logical_addr % 512[cite: 1]
    
    // Calculate PTE address based on the current process's PTBR
    uint32_t pt_base_addr = ptbr_frame * PAGE_SIZE;
    uint32_t pte_addr = pt_base_addr + (vpn * 4);
    
    // Safety check against exceeding memory bounds (8192 bytes)[cite: 1]
    if (pte_addr + 3 >= 8192) { 
        return false; // Hardware fault
    }
    
    // Read the 32-bit PTE from physical memory
    uint32_t pte_val = physical_memory[pte_addr] |
                      (physical_memory[pte_addr + 1] << 8) |
                      (physical_memory[pte_addr + 2] << 16) |
                      (physical_memory[pte_addr + 3] << 24);
                      
    // Decode PTE
    bool valid = (pte_val & 1) != 0;
    if (!valid) {
        return false; // Page Fault: Entry not valid
    }
    
    uint32_t pfn = pte_val >> 1;
    *physical_addr = (pfn * PAGE_SIZE) + offset;
    
    return true;
}

```

#### **Step 4: Update the OS Loader to Allocate the Page Table Frame (`os.c`)**

When `loader()` brings a new program into the system, it must request an extra physical frame exclusively for the page table, save it to `ptbr_frame`, and populate it using the new write function.

```c
// Inside os.c -> loader()
void loader(const char *filename) {
    // ... setup PCB ...[cite: 1]
    PCB *new_pcb = create_pcb();
    
    // 1. Allocate a frame specifically for the Page Table
    int pt_frame = allocate_physical_frame(); 
    if (pt_frame == -1) {
        // Handle out-of-memory error
        return; 
    }
    new_pcb->ptbr_frame = pt_frame;
    
    // 2. Allocate frames for code and data (10 logical pages total)[cite: 1]
    for (int vpn = 0; vpn < 10; vpn++) {
        int target_frame = allocate_physical_frame();
        
        // 3. Instead of updating an OS array, write directly to the physical page table
        write_pte_to_physical(new_pcb->ptbr_frame, vpn, target_frame);
        
        // Track for cleanup later
        new_pcb->logical_pages[vpn] = target_frame; 
    }
    
    // ... load instructions and data into target frames ...[cite: 1]
}

```

#### **Step 5: Link PTBR to the CPU execution loop (`processor.c`)**

Before executing instructions, the active processor must know which PTBR to use for memory translations.

```c
// Inside processor.c -> process_instructions()
void process_instructions(int core_id, int num_instructions) {
    PCB *current_pcb = get_pcb_on_core(core_id);
    uint32_t active_ptbr = current_pcb->ptbr_frame;
    
    for (int i = 0; i < num_instructions; i++) {
        uint32_t logical_pc = PC[core_id];
        uint32_t physical_pc;
        
        // Use the new hardware translation for fetching[cite: 1]
        if (!hardware_mmu_translate(active_ptbr, logical_pc, &physical_pc)) {
            // Trigger Page Fault Exception
            break;
        }
        
        // fetch(physical_pc), decode(), execute() ...[cite: 1]
    }
}

```

### Verification & Testing

1. **Compilation Check:** Run `make clean && make` to ensure the new memory read/write byte manipulations compile.


2. **Execute Single Program:** Run `./simulator tests/sum_n_fixed.txt`. The program should execute identically to the original version. The mathematical outcome (`55`) will prove that variables in logical memory successfully mapped to physical frames via the in-memory page table.


3. **Verify Frame Limits:** Submit more than one program. Since you now allocate an extra frame per process for the page table itself, you will exhaust the 16 physical frames faster than before. Ensure the `loader()` properly handles frame exhaustion by routing subsequent processes to the WAITING queue.