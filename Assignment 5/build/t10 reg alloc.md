### Topic 10: Virtual to Physical Register Allocation (Liveness, Graph Coloring, & Spill Code)

To implement register allocation, we must modify the compiler. Currently, the compiler directly maps assembly registers (`x0` to `x255`) to the 256 physical scalar registers in the CPU hardware.

To implement Topic 10, we will restrict the CPU to a small number of physical registers (e.g., 16). The 256 registers in the assembly file will now be treated as **virtual registers**. The compiler must perform liveness analysis, build an interference graph, assign the 16 physical registers using graph coloring, and generate memory `LOAD`/`STORE` instructions (spill code) for virtual registers that do not fit.

#### **Step 1: Define Intermediate Representation (IR) and Allocator Structures (`reg_alloc.h`)**

Instead of immediately writing machine bytecode to `program.byte`, the compiler must first parse the assembly into an Intermediate Representation (IR) array so it can analyze the whole program.

```c
// Inside reg_alloc.h
#ifndef REG_ALLOC_H
#define REG_ALLOC_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_VIRTUAL_REGS 256
#define NUM_PHYSICAL_REGS 16
#define MAX_INSTRUCTIONS 1024

// Intermediate Representation of an instruction
typedef struct {
    uint8_t opcode;
    int dest_vreg;   // -1 if not used
    int src1_vreg;   // -1 if not used
    int src2_vreg;   // -1 if not used
    int constant_val;
} IR_Instruction;

// Mapping output for a virtual register
typedef struct {
    bool is_used;
    int physical_reg;      // 0 to 15
    bool is_spilled;       // True if it didn't fit in physical registers
    uint32_t spill_offset; // Memory address offset for spilling
} RegMapping;

void allocate_registers(IR_Instruction *ir_stream, int num_instr, RegMapping *allocation_map);

#endif

```

#### **Step 2: Implement Liveness Analysis & Graph Coloring (`reg_alloc.c`)**

This implementation uses straight-line liveness analysis to build an interference matrix, followed by a greedy graph-coloring algorithm.

```c
// Inside reg_alloc.c
#include "reg_alloc.h"
#include <string.h>

bool interference_graph[MAX_VIRTUAL_REGS][MAX_VIRTUAL_REGS];
int live_ranges_start[MAX_VIRTUAL_REGS];
int live_ranges_end[MAX_VIRTUAL_REGS];

void allocate_registers(IR_Instruction *ir_stream, int num_instr, RegMapping *allocation_map) {
    memset(interference_graph, 0, sizeof(interference_graph));

    // Initialize live ranges
    for (int i = 0; i < MAX_VIRTUAL_REGS; i++) {
        live_ranges_start[i] = -1;
        live_ranges_end[i] = -1;
        allocation_map[i].is_used = false;
        allocation_map[i].physical_reg = -1;
        allocation_map[i].is_spilled = false;
    }

    // 1. Liveness Analysis (Calculate Start and End of each Virtual Register's lifespan)
    for (int i = 0; i < num_instr; i++) {
        int regs[3] = {ir_stream[i].dest_vreg, ir_stream[i].src1_vreg, ir_stream[i].src2_vreg};

        for (int j = 0; j < 3; j++) {
            int vreg = regs[j];
            if (vreg != -1) {
                allocation_map[vreg].is_used = true;
                if (live_ranges_start[vreg] == -1) live_ranges_start[vreg] = i;
                live_ranges_end[vreg] = i; // Push the end time forward
            }
        }
    }

    // 2. Build Interference Graph (Edges between overlapping live ranges)
    for (int i = 0; i < MAX_VIRTUAL_REGS; i++) {
        if (!allocation_map[i].is_used) continue;
        for (int j = i + 1; j < MAX_VIRTUAL_REGS; j++) {
            if (!allocation_map[j].is_used) continue;

            // Check if ranges overlap
            if (live_ranges_start[i] <= live_ranges_end[j] && live_ranges_start[j] <= live_ranges_end[i]) {
                interference_graph[i][j] = true;
                interference_graph[j][i] = true;
            }
        }
    }

    // 3. Greedy Graph Coloring & Spill Assignment
    uint32_t current_spill_address = 4096; // Example: Spill to start of data memory[cite: 1]

    for (int vreg = 0; vreg < MAX_VIRTUAL_REGS; vreg++) {
        if (!allocation_map[vreg].is_used) continue;

        bool available_colors[NUM_PHYSICAL_REGS];
        memset(available_colors, true, sizeof(available_colors));

        // Remove colors used by interfering neighbors
        for (int neighbor = 0; neighbor < MAX_VIRTUAL_REGS; neighbor++) {
            if (interference_graph[vreg][neighbor] && allocation_map[neighbor].physical_reg != -1) {
                available_colors[allocation_map[neighbor].physical_reg] = false;
            }
        }

        // Assign the first available color (physical register)
        bool colored = false;
        for (int color = 0; color < NUM_PHYSICAL_REGS; color++) {
            if (available_colors[color]) {
                allocation_map[vreg].physical_reg = color;
                colored = true;
                break;
            }
        }

        // 4. Spill if no physical registers are available
        if (!colored) {
            allocation_map[vreg].is_spilled = true;
            allocation_map[vreg].spill_offset = current_spill_address;
            current_spill_address += 4; // 4 bytes per 32-bit register
        }
    }
}

```

#### **Step 3: Update the Compiler pass to Inject Spill Code (`compiler.c`)**

The `compile()` function must be updated to populate the IR array, call the allocator, and then write the physical machine bytecode. If an instruction uses a spilled register, the compiler must dynamically insert `LOAD` and `STORE` opcodes around it using a reserved temporary physical register.

```c
// Inside compiler.c
#include "compiler.h"
#include "reg_alloc.h"

// Reserve Physical Register 15 as a temporary scratch register for spill code swapping
#define SPILL_SCRATCH_REG 15

void compile(const char *input_file, const char *output_file) {
    IR_Instruction ir_stream[MAX_INSTRUCTIONS];
    int instr_count = 0;

    // ... Pass 1: find_label() logic ...[cite: 1]

    // Pass 2: Parse into IR (Instead of writing directly to file)
    // Assume logic here fills ir_stream based on parse_register() and parse_constant()[cite: 1]
    // Example: parse_register("x25") -> returns 25 (Virtual Register 25).

    // Pass 3: Register Allocation
    RegMapping alloc_map[MAX_VIRTUAL_REGS];
    allocate_registers(ir_stream, instr_count, alloc_map);

    // Pass 4: Code Emission & Spill Code Injection
    FILE *out = fopen(output_file, "wb");

    for (int i = 0; i < instr_count; i++) {
        IR_Instruction ir = ir_stream[i];

        uint8_t phys_dest = 0, phys_src1 = 0, phys_src2 = 0;

        // --- Handle SRC1 Spilling ---
        if (ir.src1_vreg != -1) {
            if (alloc_map[ir.src1_vreg].is_spilled) {
                // Emit: LOAD SPILL_SCRATCH_REG, spill_offset
                fputc(OP_LOAD, out);
                fputc(SPILL_SCRATCH_REG, out);
                fputc((alloc_map[ir.src1_vreg].spill_offset >> 8) & 0xFF, out);
                fputc(alloc_map[ir.src1_vreg].spill_offset & 0xFF, out);
                phys_src1 = SPILL_SCRATCH_REG;
            } else {
                phys_src1 = alloc_map[ir.src1_vreg].physical_reg;
            }
        }

        // --- Handle SRC2 Spilling ---
        // (Similar to SRC1, but you may need a second scratch register like Reg 14 if both src1 and src2 are spilled)

        // --- Map Destination ---
        if (ir.dest_vreg != -1) {
            if (alloc_map[ir.dest_vreg].is_spilled) {
                phys_dest = SPILL_SCRATCH_REG;
            } else {
                phys_dest = alloc_map[ir.dest_vreg].physical_reg;
            }
        }

        // Emit Actual Operation using Physical Registers[cite: 1]
        fputc(ir.opcode, out);
        fputc(phys_dest, out);
        fputc(phys_src1, out);
        fputc(phys_src2, out);

        // --- Handle DEST Spilling ---
        if (ir.dest_vreg != -1 && alloc_map[ir.dest_vreg].is_spilled) {
            // Emit: STORE SPILL_SCRATCH_REG, spill_offset
            fputc(OP_STORE, out);
            fputc(SPILL_SCRATCH_REG, out);
            fputc((alloc_map[ir.dest_vreg].spill_offset >> 8) & 0xFF, out);
            fputc(alloc_map[ir.dest_vreg].spill_offset & 0xFF, out);
        }
    }

    fclose(out);
}

```

### Verification & Testing

1. **Reduce Physical Limits:** Ensure `NUM_PHYSICAL_REGS` is set to a low number (e.g., 4 or 8) so that spills are forcibly triggered during testing.
2. **Create a High-Pressure Test (`tests/spill_test.txt`):** Write an assembly program that defines and uses more variables than the physical limit simultaneously.

```text
x1 = 10
x2 = 20
x3 = 30
x4 = 40
x5 = 50
x6 = x1 + x2
x7 = x3 + x4
x8 = x6 + x7 + x5
PRINT x8
HALT

```

3. **Execution Verification:**

- Check the compiled bytecode size (`program.byte`). It should be larger than the number of source lines because the compiler silently inserted `LOAD` and `STORE` opcodes to swap data in and out of the simulated physical memory.

- Run the compiled bytecode. The mathematical output must remain completely accurate despite the variables being routed through memory spill slots rather than living persistently in hardware registers.
