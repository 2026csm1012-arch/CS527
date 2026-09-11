### Topic 9: Security and Access Permissions in Page Tables

To implement hardware-level memory protection, you must embed permission bits directly into the Page Table Entries (PTEs). The MMU will check these bits during every logical-to-physical translation. If a process attempts an unauthorized action (e.g., writing to read-only code, or a user process accessing supervisor memory), the MMU will block the access and trigger a protection fault.

#### **Step 1: Define Access Types and Privilege Levels (`memory.h`)**

Create enumerations to represent the type of memory access being attempted and the current privilege level of the CPU.

```c
// Inside memory.h

typedef enum {
    ACC_READ,
    ACC_WRITE,
    ACC_EXECUTE
} AccessType;

typedef enum {
    PRIV_USER = 0,
    PRIV_SUPERVISOR = 1
} PrivilegeLevel;

// Updated function signature for hardware translation
bool hardware_mmu_translate(uint32_t ptbr_frame, uint32_t logical_addr, AccessType access_type, PrivilegeLevel current_priv, uint32_t *physical_addr);

```

#### **Step 2: Update PTE Encoding and MMU Translation (`memory.c`)**

Expand the 32-bit PTE format. Previously, we only used bit `[0]` for the Valid flag. We will now use bits `[1:4]` for permissions (Read, Write, Execute, User-Accessible) and shift the physical frame number (PFN) to bits `[12:31]`.

```c
// Inside memory.c
#include "memory.h"
#include <stdio.h>

extern uint8_t physical_memory[8192]; // 8192-byte physical memory[cite: 1]
#define PAGE_SIZE 512                 // 512-byte pages[cite: 1]

// Updated PTE Writer
void write_secure_pte_to_physical(uint32_t ptbr_frame, uint32_t vpn, uint32_t target_pfn, bool r, bool w, bool x, bool user_acc) {
    uint32_t pt_base_addr = ptbr_frame * PAGE_SIZE;
    uint32_t pte_addr = pt_base_addr + (vpn * 4);
    
    // Encode PTE: 
    // Bit 0: Valid, Bit 1: Read, Bit 2: Write, Bit 3: Execute, Bit 4: User/Supervisor
    // Bits 12-31: PFN
    uint32_t pte_val = 1; // Valid bit
    if (r) pte_val |= (1 << 1);
    if (w) pte_val |= (1 << 2);
    if (x) pte_val |= (1 << 3);
    if (user_acc) pte_val |= (1 << 4);
    pte_val |= (target_pfn << 12);
    
    // Write 32-bit value into physical memory[cite: 1]
    physical_memory[pte_addr]     = pte_val & 0xFF;
    physical_memory[pte_addr + 1] = (pte_val >> 8) & 0xFF;
    physical_memory[pte_addr + 2] = (pte_val >> 16) & 0xFF;
    physical_memory[pte_addr + 3] = (pte_val >> 24) & 0xFF;
}

// Updated MMU Translation with Security Checks
bool hardware_mmu_translate(uint32_t ptbr_frame, uint32_t logical_addr, AccessType access_type, PrivilegeLevel current_priv, uint32_t *physical_addr) {
    uint32_t vpn = logical_addr / PAGE_SIZE;
    uint32_t offset = logical_addr % PAGE_SIZE;
    
    uint32_t pte_addr = (ptbr_frame * PAGE_SIZE) + (vpn * 4);
    if (pte_addr + 3 >= 8192) return false;
    
    uint32_t pte_val = physical_memory[pte_addr] |
                      (physical_memory[pte_addr + 1] << 8) |
                      (physical_memory[pte_addr + 2] << 16) |
                      (physical_memory[pte_addr + 3] << 24);
                      
    // 1. Check Valid Bit
    if ((pte_val & 1) == 0) {
        printf("MMU FAULT: Page not present (VPN %d)\n", vpn);
        return false;
    }
    
    // 2. Privilege Level Check
    bool is_user_accessible = (pte_val & (1 << 4)) != 0;
    if (current_priv == PRIV_USER && !is_user_accessible) {
        printf("MMU FAULT: Privilege violation at VPN %d\n", vpn);
        return false;
    }
    
    // 3. Specific Access Checks
    bool can_read = (pte_val & (1 << 1)) != 0;
    bool can_write = (pte_val & (1 << 2)) != 0;
    bool can_exec = (pte_val & (1 << 3)) != 0;
    
    if (access_type == ACC_READ && !can_read) {
        printf("MMU FAULT: Read permission denied at VPN %d\n", vpn);
        return false;
    }
    if (access_type == ACC_WRITE && !can_write) {
        printf("MMU FAULT: Write permission denied at VPN %d\n", vpn);
        return false;
    }
    if (access_type == ACC_EXECUTE && !can_exec) {
        printf("MMU FAULT: Execute permission denied at VPN %d\n", vpn);
        return false;
    }
    
    uint32_t pfn = pte_val >> 12;
    *physical_addr = (pfn * PAGE_SIZE) + offset;
    return true;
}

```

#### **Step 3: Update the OS Loader for Segment-Specific Permissions (`os.c`)**

The `loader()` creates the logical pages. It must assign strict permissions based on the memory segment: code pages (logical pages 0-1) should be executable but read-only, while data pages (logical pages 2-9) should be writable but non-executable.

```c
// Inside os.c -> loader()

void loader(const char *filename) {
    // ... setup PCB and ptbr_frame ...[cite: 1]
    
    // Allocate logical pages[cite: 1]
    for (int vpn = 0; vpn < 10; vpn++) {
        int target_frame = allocate_physical_frame();
        
        if (vpn < 2) {
            // Code pages: Read=1, Write=0, Execute=1, User=1
            write_secure_pte_to_physical(new_pcb->ptbr_frame, vpn, target_frame, true, false, true, true);
        } else {
            // Data pages: Read=1, Write=1, Execute=0, User=1
            write_secure_pte_to_physical(new_pcb->ptbr_frame, vpn, target_frame, true, true, false, true);
        }
        
        new_pcb->logical_pages[vpn] = target_frame;
    }
    
    // ... load program instructions into pages 0-1, data into 2-9 ...[cite: 1]
}

```

#### **Step 4: Update Processor Execution and Fault Handling (`processor.c`)**

The CPU must specify the intent of its memory accesses (Fetch = Execute, Load = Read, Store = Write). If `hardware_mmu_translate` returns false due to a permission fault, the CPU must trap the exception and terminate the process.

```c
// Inside processor.c

// Simulate the CPU's current privilege mode (default to USER for loaded programs)
PrivilegeLevel cpu_privilege_mode[4] = {PRIV_USER, PRIV_USER, PRIV_USER, PRIV_USER};

void process_instructions(int core_id, int num_instructions) {
    PCB *current_pcb = get_pcb_on_core(core_id);
    
    for (int i = 0; i < num_instructions; i++) {
        uint32_t logical_pc = PC[core_id];
        uint32_t physical_pc;
        
        // 1. Instruction Fetch (Requires EXECUTE permission)
        if (!hardware_mmu_translate(current_pcb->ptbr_frame, logical_pc, ACC_EXECUTE, cpu_privilege_mode[core_id], &physical_pc)) {
            printf("[CPU %d] Segmentation Fault: Instruction Fetch at address %d\n", core_id, logical_pc);
            current_pcb->state = PROC_TERMINATED; // Kill the process[cite: 1]
            break;
        }
        
        // ... execute fetch and decode ...[cite: 1]
        
        // 2. Memory Operations (Requires READ or WRITE permission)
        // Adjust mem_read_32 and mem_write_32 signatures to accept ptbr, access_type, and priv_mode.
        if (opcode == OP_LOAD) { // Assuming OP_LOAD from compiler.c[cite: 1]
            uint32_t physical_data_addr;
            if (!hardware_mmu_translate(current_pcb->ptbr_frame, logical_data_addr, ACC_READ, cpu_privilege_mode[core_id], &physical_data_addr)) {
                printf("[CPU %d] Segmentation Fault: Data Read at address %d\n", core_id, logical_data_addr);
                current_pcb->state = PROC_TERMINATED;
                break;
            }
            // Read from physical_memory[physical_data_addr][cite: 1]
        }
        else if (opcode == OP_STORE) {
            uint32_t physical_data_addr;
            if (!hardware_mmu_translate(current_pcb->ptbr_frame, logical_data_addr, ACC_WRITE, cpu_privilege_mode[core_id], &physical_data_addr)) {
                printf("[CPU %d] Segmentation Fault: Data Write at address %d\n", core_id, logical_data_addr);
                current_pcb->state = PROC_TERMINATED;
                break;
            }
            // Write to physical_memory[physical_data_addr][cite: 1]
        }
    }
}

```

### Verification & Testing

1. **Valid Execution Test:** Run `./simulator tests/matrix_det.txt`. It should execute successfully, as standard data arrays live in the data pages (VPN 2-9) which are marked writable.


2. **Write-to-Code Fault Test:** Create a malicious assembly file `tests/malware.txt`. Instruct the program to store a value into address `0` (Logical Page 0, which holds the code).


* *Expected:* The MMU will detect the write attempt on an `R-X` page, deny access, and print `MMU FAULT: Write permission denied at VPN 0`. The processor will terminate the process and schedule the next one.