# CS527 Mini-Computer Simulator: Architecture & System Guide (Labs 1 – 4)

Welcome to the **CS527 Mini-Computer Simulator** project documentation. This guide explains how the entire system is built, how each component functions, and how they interact to compile and execute both scalar and vector assembly programs in a multi-core environment.

---

## 1. High-Level Architecture Overview

The simulator models a complete multi-processing computer system in software, featuring:
1. A **Compiler** that translates high-level assembly into machine bytecode.
2. A **Memory Subsystem** with separate 2D instruction and data memory across `NP = 4` parallel processors.
3. A **Multi-Core Processor** supporting 32-bit scalar ISA, 256-bit Vector SIMD operations, condition flags, and branching.
4. An **Operating System Layer** featuring an interactive non-blocking Shell, Task Loader, and Preemptive Round-Robin Scheduler.

### System Data Flow Diagram

```
+-------------------------------------------------------------+
|                      User / Interactive Shell               |
|            $ tests/sum_array.txt tests/sum_array_data.byte  |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                    OS Layer (os.c / os.h)                   |
|                                                             |
|  * Loader: Compiles .txt -> .byte, finds free core 0..NP-1  |
|  * Scheduler: Round-Robin time-slicing (10 instructions)    |
|  * Non-blocking Shell: Takes new commands asynchronously    |
+-------------------------------------------------------------+
          |                                        |
          v (Proc 0)                               v (Proc 1..NP-1)
+---------------------------+            +---------------------------+
| Instruction & Data Memory |            | Instruction & Data Memory |
|  Instruction[0][256]      |            |  Instruction[1..3][256]   |
|  Data[0][4096]            |            |  Data[1..3][4096]         |
+---------------------------+            +---------------------------+
          |                                        |
          v                                        v
+---------------------------+            +---------------------------+
|    Processor Core 0       |            |    Processor Cores 1..3   |
|  * PC[0]                  |            |  * PC[1..3]               |
|  * Register[0][256]       |            |  * Register[1..3][256]    |
|  * VectorRegister[0][32]  |            |  * VectorRegister[1..3]   |
|  * Flags: Z, N, C, V      |            |  * Flags: Z, N, C, V      |
+---------------------------+            +---------------------------+
```

---

## 2. Component-by-Component Breakdown

### A. The Orchestrator: `main.c`
`main.c` provides two execution modes:
1. **Interactive Multi-Processing OS Mode (`./simulator.exe`)**:
   - Initializes OS tables and queues (`os_init()`).
   - Starts the scheduling loop (`os_run()`), executing tasks on available cores while presenting the `$ ` interactive shell to the user.
2. **Standalone Single Program Mode (`./simulator.exe prog.txt [data.byte]`)**:
   - Compiles and runs a single program on Core 0 directly.

---

### B. The Operating System: `os.c` & `os.h`
The OS layer manages hardware resources and coordinates process execution:

1. **Process Control Block (PCB)**:
   - Tracks `pid`, allocated hardware `proc_id` (0 to `NP-1`, or `-1` if waiting), program source, bytecode, data files, and `state` (`PROC_READY`, `PROC_WAITING`, `PROC_TERMINATED`).
2. **Task Loader (`loader()`)**:
   - Compiles `.txt` assembly into a unique per-process `.byte` file.
   - Searches for an idle processor (`0..NP-1`).
   - If a processor is free, initializes memory and marks the process `PROC_READY`.
   - If all cores are busy, enqueues the process in the `PROC_WAITING` queue.
3. **Round-Robin Scheduler (`scheduler()`)**:
   - Cycles through all active cores and executes `TIME_SLICE = 10` instructions per core via `process_instructions(proc_id, 10)`.
   - When a core completes (`end_of_simulation[proc_id] == 1`), dumps its memory to `data_out_pid<pid>.byte`, frees the core, and immediately admits the next waiting task.
4. **Non-Blocking Shell (`shell()`)**:
   - Implements non-blocking STDIN character reading (`_kbhit()` on Windows, `select()` on Linux).
   - Allows users to enter commands like `$ prog.txt data.byte`, `$ status`, or `$ exit` without pausing active processes.

---

### C. The Processor Engine: `processor.c` & `processor.h`
- **Register Files**:
  - `Register[NP][256]`: 256 32-bit scalar registers per core (`x0` - `x255`).
  - `VectorRegister[NP][32][8]`: 32 256-bit vector registers per core (`v0` - `v31`), each storing 8 32-bit integers.
- **Condition Flags**: `Z, N, C, V` updated on integer arithmetic to control branching.
- **Print Instruction**: Opcode `0x08` (`print x1`), logs `Process id <proc_id>: x<reg> : 0x<hex>` to `simulation.log` and console.
- **Real-Time Delays**: `process_instructions()` invokes `sleep(10 usec)` to simulate hardware timing.

---

### D. The Storage Subsystem: `memory.c` & `memory.h`
- `Instruction[NP][256]`: 256-byte instruction memory per processor.
- `Data[NP][4096]`: 4096-byte data memory per processor.
- Byte-addressable Little-Endian 32-bit integer read/write (`mem_read_32`, `mem_write_32`).

---

### E. The Compiler: `compiler.c` & `compiler.h`
- Two-pass assembler translating text instructions into 4-byte machine code lines:
  $$\text{Byte 0: Opcode} \quad|\quad \text{Byte 1: Dest} \quad|\quad \text{Byte 2: Src1} \quad|\quad \text{Byte 3: Src2}$$

---

## 3. Instruction Set Architecture (ISA) Reference

| Category | Instruction Syntax | Opcode (Hex) | Description |
| :--- | :--- | :--- | :--- |
| **Control** | `Halt` | `0x00` | Ends simulation for current core |
| **Scalar Math** | `x1 = x2 + x3` | `0x01` | Integer Add (updates Z, N, C, V) |
| | `x1 = x2 + 10` | `0x09` | Add Constant (updates Z, N, C, V) |
| | `x1 = x2 - x3` | `0x02` | Integer Subtract (updates Z, N, C, V) |
| | `x1 = x2 - 10` | `0x0A` | Subtract Constant (updates Z, N, C, V) |
| | `x1 = x2 * x3` | `0x03` | Integer Multiply |
| | `x1 = x2 * 10` | `0x0B` | Multiply Constant |
| | `x1 = x2 / x3` | `0x04` | Integer Divide |
| | `x1 = x2 / 10` | `0x0C` | Divide Constant |
| **Data Movement**| `x1 = x2` | `0x07` | Register Copy |
| | `x1 = 10` | `0x0F` | Load Constant |
| **Scalar Memory**| `x1 = [x2]` | `0x05` | Read word from RAM at address in register |
| | `x1 = [10]` | `0x0C` | Read word from RAM at constant address |
| | `[x2] = x1` | `0x06` | Write word to RAM at address in register |
| | `[10] = x1` | `0x0E` | Write word to RAM at constant address |
| **Debug Print** | `print x1` | `0x08` | Output register value to console & `simulation.log` |
| **Branches** | `B{Suffix} <label>`| `0x10` - `0x1E` | Conditional branch (`BEQ`, `BNE`, `BGE`, `BAL`, etc.) |
| **Vector Math** | `v1 = v2 + v3` | `0x21` | 8-way parallel vector addition |
| | `v1 = v2 + 10` | `0x29` | 8-way vector + scalar addition |
| | `v1 = v2 - v3` | `0x22` | 8-way parallel vector subtraction |
| | `v1 = v2 - 10` | `0x2A` | 8-way vector - scalar subtraction |
| | `v1 = v2 * v3` | `0x23` | 8-way parallel vector multiplication |
| | `v1 = v2 * 10` | `0x2B` | 8-way vector * scalar multiplication |
| **Vector Memory**| `v1 = [x2]` | `0x25` | Load 8 words from memory with auto-increment (+32) |
| | `[x2] = v1` | `0x26` | Store 8 words to memory with auto-increment (+32) |

---

## 4. How to Build & Run

```bash
# Build
make

# Launch Interactive Multi-Processing OS
./simulator.exe
# Inside the shell:
# $ tests/sum_array.txt tests/sum_array_data.byte
# $ tests/vector_add.txt tests/vector_add_data.byte
# $ status
# $ exit

# Run Standalone
./simulator.exe tests/sum_array.txt tests/sum_array_data.byte
```
