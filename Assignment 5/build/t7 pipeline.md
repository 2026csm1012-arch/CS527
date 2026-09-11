### Topic 7: Pipelined Processor Architecture

To upgrade the simulator to a pipelined architecture, you must replace the single-cycle loop—where `fetch`, `decode`, and `execute` run sequentially for a single instruction before moving to the next—with a 5-stage RISC pipeline (Fetch, Decode, Execute, Memory, Writeback). This requires state registers between stages and a hazard detection unit to handle data dependencies.

#### **Step 1: Define Pipeline Registers (`processor.h`)**

Create structures to hold the state of an instruction as it flows through the five stages.

```c
// Inside processor.h
#include <stdint.h>
#include <stdbool.h>

// Represents an instruction in transit
typedef struct {
    bool is_active;
    uint32_t pc;
    uint8_t opcode;
    uint8_t dest_reg;
    uint8_t src1_reg;
    uint8_t src2_reg;

    // Decoded values
    int32_t val1;
    int32_t val2;

    // Execution results
    int32_t alu_result;
    bool branch_taken;
    uint32_t branch_target;

    // Memory results
    int32_t mem_data;
} PipelineReg;

// Pipeline state for each of the 4 cores[cite: 1]
extern PipelineReg IF_ID[4];
extern PipelineReg ID_EX[4];
extern PipelineReg EX_MEM[4];
extern PipelineReg MEM_WB[4];

void init_pipeline(int core_id);
void flush_pipeline(int core_id);

```

#### **Step 2: Implement Hazard Detection (`processor.c`)**

Before an instruction can be decoded, the CPU must check if the registers it needs to read are currently being modified by older instructions still in the EX, MEM, or WB stages (Read-After-Write hazards).

```c
// Inside processor.c
#include "processor.h"

PipelineReg IF_ID[4], ID_EX[4], EX_MEM[4], MEM_WB[4];

// Clears all pipeline registers for a context switch or branch flush
void flush_pipeline(int core_id) {
    IF_ID[core_id].is_active = false;
    ID_EX[core_id].is_active = false;
    EX_MEM[core_id].is_active = false;
    MEM_WB[core_id].is_active = false;
}

// Checks if the Decode stage needs to stall due to a data dependency
bool check_data_hazard(int core_id, uint8_t src_reg) {
    if (!IF_ID[core_id].is_active) return false;

    // Check EX stage
    if (ID_EX[core_id].is_active && ID_EX[core_id].dest_reg == src_reg) return true;
    // Check MEM stage
    if (EX_MEM[core_id].is_active && EX_MEM[core_id].dest_reg == src_reg) return true;
    // Check WB stage
    if (MEM_WB[core_id].is_active && MEM_WB[core_id].dest_reg == src_reg) return true;

    return false;
}

```

#### **Step 3: Implement the 5 Pipeline Stages (`processor.c`)**

Break down your existing `fetch`, `decode`, and `execute` logic into discrete phases that operate on the `PipelineReg` structures from the bottom up (WB to IF).

```c
// Inside processor.c

void stage_writeback(int core_id) {
    if (!MEM_WB[core_id].is_active) return;

    uint8_t op = MEM_WB[core_id].opcode;
    // If it's an ALU operation or LOAD, write the result to the destination register[cite: 1]
    if (op == OP_ADD || op == OP_SUB || op == OP_MUL || op == OP_LOAD) {
        Register[core_id][MEM_WB[core_id].dest_reg] = MEM_WB[core_id].mem_data;
    }
}

void stage_memory(int core_id) {
    // Pass data forward
    MEM_WB[core_id] = EX_MEM[core_id];
    if (!EX_MEM[core_id].is_active) return;

    uint8_t op = EX_MEM[core_id].opcode;
    if (op == OP_LOAD) {
        MEM_WB[core_id].mem_data = mem_read_32(EX_MEM[core_id].alu_result); // Requires MMU translation[cite: 1]
    }
    else if (op == OP_STORE) {
        mem_write_32(EX_MEM[core_id].alu_result, EX_MEM[core_id].val2);
        MEM_WB[core_id].mem_data = 0; // Stores don't write to registers
    }
    else {
        // Pass ALU result through for non-memory operations
        MEM_WB[core_id].mem_data = EX_MEM[core_id].alu_result;
    }
}

void stage_execute(int core_id) {
    EX_MEM[core_id] = ID_EX[core_id];
    if (!ID_EX[core_id].is_active) return;

    uint8_t op = ID_EX[core_id].opcode;

    // ALU Operations
    if (op == OP_ADD) EX_MEM[core_id].alu_result = ID_EX[core_id].val1 + ID_EX[core_id].val2;
    if (op == OP_SUB) EX_MEM[core_id].alu_result = ID_EX[core_id].val1 - ID_EX[core_id].val2;
    // ... [Implement other ALU ops and update Z, N, C, V flags here][cite: 1]

    // Branch Operations
    EX_MEM[core_id].branch_taken = false;
    if (op == OP_BEQ && Z[core_id] == 1) { // Example: Branch if Equal (Zero flag set)[cite: 1]
        EX_MEM[core_id].branch_taken = true;
        EX_MEM[core_id].branch_target = ID_EX[core_id].val1; // Assume val1 holds target address
    }
}

void stage_decode(int core_id, bool *stall_flag) {
    if (!IF_ID[core_id].is_active) {
        ID_EX[core_id].is_active = false;
        return;
    }

    uint8_t op = IF_ID[core_id].opcode;
    uint8_t src1 = IF_ID[core_id].src1_reg;
    uint8_t src2 = IF_ID[core_id].src2_reg;

    // Hazard Detection
    if (check_data_hazard(core_id, src1) || check_data_hazard(core_id, src2)) {
        *stall_flag = true;
        ID_EX[core_id].is_active = false; // Inject bubble
        return;
    }

    // Pass data to EX
    ID_EX[core_id] = IF_ID[core_id];
    ID_EX[core_id].val1 = Register[core_id][src1];
    ID_EX[core_id].val2 = Register[core_id][src2];
}

void stage_fetch(int core_id, bool stall_flag) {
    if (stall_flag) return; // Do not fetch if pipeline is stalled

    uint32_t current_pc = PC[core_id];

    // Fetch 4 bytes from memory using MMU translation[cite: 1]
    uint32_t instruction = fetch(current_pc);

    if (instruction == OP_HALT) { // Assume 0x00 is HALT or similar[cite: 1]
        IF_ID[core_id].is_active = false;
        return;
    }

    IF_ID[core_id].is_active = true;
    IF_ID[core_id].pc = current_pc;
    IF_ID[core_id].opcode = (instruction >> 24) & 0xFF;
    IF_ID[core_id].dest_reg = (instruction >> 16) & 0xFF;
    IF_ID[core_id].src1_reg = (instruction >> 8) & 0xFF;
    IF_ID[core_id].src2_reg = instruction & 0xFF;

    PC[core_id] += 4; // Advance PC by 4 bytes[cite: 1]
}

```

#### **Step 4: Update the Processor Execution Loop (`processor.c`)**

Replace the original `process_instructions` loop with the pipeline clock cycle simulator.

```c
// Inside processor.c
void process_instructions(int core_id, int num_cycles) {
    PCB *current_pcb = get_pcb_on_core(core_id);
    if (!current_pcb) return;

    for (int cycle = 0; cycle < num_cycles; cycle++) {
        bool stall_flag = false;

        // Stages must be processed in reverse order (WB -> IF) to allow data to flow
        // forward without overwriting the next stage in the same clock cycle.
        stage_writeback(core_id);
        stage_memory(core_id);
        stage_execute(core_id);

        // Check for Control Hazards (Branches)
        if (EX_MEM[core_id].is_active && EX_MEM[core_id].branch_taken) {
            PC[core_id] = EX_MEM[core_id].branch_target; // Update PC[cite: 1]

            // Flush instructions currently in IF and ID
            IF_ID[core_id].is_active = false;
            ID_EX[core_id].is_active = false;

            // Do not fetch or decode this cycle
            continue;
        }

        stage_decode(core_id, &stall_flag);
        stage_fetch(core_id, stall_flag);

        // Determine if program has finished (pipeline is completely empty)
        if (!IF_ID[core_id].is_active && !ID_EX[core_id].is_active &&
            !EX_MEM[core_id].is_active && !MEM_WB[core_id].is_active) {

            current_pcb->state = PROC_TERMINATED; // Mark PCB as finished[cite: 1]
            break;
        }
    }
}

```

### Verification & Testing

1. **Compilation Check:** Run `make clean && make`.

2. **Data Hazard Test:** Create a small assembly program where an instruction immediately uses the result of the previous instruction:

```text
ADD x1, x2, x3
SUB x4, x1, x5

```

If hazard detection works, the `SUB` instruction will stall in the Decode stage for 3 cycles until the `ADD` writes `x1` back to the register file during the Writeback stage. 3. **Control Hazard Test:** Run `tests/sum_n_fixed.txt`. This program loops heavily. Every time the loop branches back to the start, the pipeline should flush `IF` and `ID`, costing a 2-cycle penalty. Ensure the final mathematical sum is exactly `55`, proving that the flushed instructions did not accidentally corrupt the registers.
