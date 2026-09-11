### Topic 8: Inter-Process Communication (Shared Memory) across all system layers

To implement zero-copy shared memory, you must introduce a mechanism that spans from the user's assembly source code all the way down to the physical page tables. This requires adding a new instruction, updating the compiler to parse it, extending the ISA to execute it, and modifying the OS to map multiple virtual pages to a single physical frame.

#### **1. Programming Model & ISA (Instruction Set Architecture)**

Introduce a new assembly instruction that allows a process to request access to a shared memory region by a unique ID, and map it to a specific logical page in its address space.

- **Syntax:** `SHM_MAP dest_logical_page, shm_id`
- **Example:** `SHM_MAP 5, 123` (Maps shared memory region ID `123` to logical page `5`).
- **ISA Update:** Define a new opcode for this instruction.

#### **2. Compiler Updates (`compiler.h` & `compiler.c`)**

The compiler must recognize the `SHM_MAP` mnemonic and encode it into the 4-byte machine bytecode.

**File Changes (`compiler.h`):**

```c
#define OP_SHM_MAP 0x30 // New opcode for shared memory mapping

```

**File Changes (`compiler.c`):**
Modify the instruction parser to handle the new operation.

```c
// Inside parse_instruction() in compiler.c
if (strcmp(opcode_str, "SHM_MAP") == 0) {
    uint8_t opcode = OP_SHM_MAP;

    // Parse the logical page number (destination)
    uint8_t dest = parse_constant(token_dest);

    // Parse the shared memory ID (source 1)
    uint8_t src1 = parse_constant(token_src1);

    // Write 4-byte bytecode[cite: 1]
    fputc(opcode, out_file);
    fputc(dest, out_file);
    fputc(src1, out_file);
    fputc(0, out_file); // Unused src2
    return;
}

```

#### **3. OS & Memory Management (`os.h` & `os.c`)**

The operating system must maintain a global table of shared memory regions. When a process requests a `shm_id`, the OS either allocates a new physical frame for it or returns the existing frame if another process already created it.

**File Changes (`os.h`):**

```c
#define MAX_SHM_REGIONS 8

typedef struct {
    int shm_id;
    int physical_frame;
    bool is_active;
} SharedMemoryRegion;

extern SharedMemoryRegion shm_table[MAX_SHM_REGIONS];

// Initialize the table during os_init()
void init_shm_table(void);

// OS system call to resolve a shared memory ID to a physical frame
int get_or_create_shm_frame(int shm_id);

```

**File Changes (`os.c`):**

```c
SharedMemoryRegion shm_table[MAX_SHM_REGIONS];

void init_shm_table() {
    for (int i = 0; i < MAX_SHM_REGIONS; i++) {
        shm_table[i].is_active = false;
    }
}

int get_or_create_shm_frame(int shm_id) {
    // 1. Check if the region already exists
    for (int i = 0; i < MAX_SHM_REGIONS; i++) {
        if (shm_table[i].is_active && shm_table[i].shm_id == shm_id) {
            return shm_table[i].physical_frame;
        }
    }

    // 2. If it does not exist, allocate a new frame and register it
    for (int i = 0; i < MAX_SHM_REGIONS; i++) {
        if (!shm_table[i].is_active) {
            int new_frame = allocate_physical_frame(); // Existing memory allocator[cite: 1]
            if (new_frame != -1) {
                shm_table[i].shm_id = shm_id;
                shm_table[i].physical_frame = new_frame;
                shm_table[i].is_active = true;
                return new_frame;
            }
        }
    }
    return -1; // Out of shared memory slots or physical frames
}

```

#### **4. Processor Execution (`processor.c`)**

When the CPU executes the `OP_SHM_MAP` instruction, it triggers an OS-level mapping update. It fetches the physical frame from the OS and rewrites the process's page table to point the requested logical page to the shared physical frame.

**File Changes (`processor.c`):**

```c
// Inside execute() in processor.c
#include "os.h"
#include "memory.h" // Requires write_pte_to_physical() from Topic 3

void execute(int core_id, uint8_t opcode, uint8_t dest, uint8_t src1, uint8_t src2) {
    // ... existing opcodes ...[cite: 1]

    if (opcode == OP_SHM_MAP) {
        int logical_page = dest;
        int shm_id = src1;

        // 1. Get the physical frame for this shared memory ID
        int shared_frame = get_or_create_shm_frame(shm_id);

        if (shared_frame != -1) {
            // 2. Get current process's Page Table Base Register (from PCB)
            PCB *current_pcb = get_pcb_on_core(core_id);
            uint32_t ptbr = current_pcb->ptbr_frame;

            // 3. Update the page table to map the logical page to the shared frame
            // This replaces the isolated frame allocated during loading with the shared one
            write_pte_to_physical(ptbr, logical_page, shared_frame);
        } else {
            // Handle error: SHM allocation failed
            printf("[CPU %d] SHM_MAP Failed for ID %d\n", core_id, shm_id);
        }
        return;
    }
}

```

### Verification & Testing

1. **Write two test programs:** Create `tests/shm_writer.txt` and `tests/shm_reader.txt`.
2. **Writer Program:**

- Map shared memory: `SHM_MAP 5, 99` (Map ID 99 to logical page 5).
- Write data: Write the value `1024` to logical address `2560` (Page 5 starts at $5 \times 512 = 2560$).

3. **Reader Program:**

- Map shared memory: `SHM_MAP 6, 99` (Map ID 99 to logical page 6).
- Read data: Read from logical address `3072` (Page 6 starts at $6 \times 512 = 3072$).

- Print the read value.

4. **Execute:** Run both in the multi-process OS mode. The reader should successfully print `1024`, proving that independent logical pages in different processes resolved to the exact same physical frame.


Yes, this implementation is completely sufficient to fulfill the requirements of the extension. It successfully bridges all four required layers:Programming Model & ISA: It gives the programmer a concrete instruction (SHM_MAP) to request shared memory.Compiler: It translates that human-readable instruction into the 4-byte machine binary.  OS: It manages the physical frames and ensures multiple requests for the same Shared Memory ID point to the exact same physical frame in RAM.Hardware/Page Tables: The processor intercepts the instruction and rewrites the process's page table mapping so that a specific logical page routes directly to that shared physical frame.  With this in place, if Process A writes a value to its logical page 5, and Process B reads from its logical page 6, they will instantly see the same data without the OS needing to copy anything.