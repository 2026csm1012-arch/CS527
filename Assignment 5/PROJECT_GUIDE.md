# CS527 Mini-Computer Simulator: Comprehensive System Guide (Lab 5)

Welcome to the **CS527 Mini-Computer Simulator** project documentation. This guide details the complete hardware-software architecture for **Lab 5**, including memory paging (logical-to-physical address translation), multi-core preemptive round-robin scheduling, vector SIMD arithmetic, and the complete suite of test programs.

---

## 1. High-Level Architecture Overview

The simulator models a multi-core computer system with an Operating System, Memory Management Unit (MMU), and SIMD vector processors:

1. **Compiler (`compiler.c` / `compiler.h`)**:
   - Two-pass assembler translating high-level assembly into 4-byte machine instructions:
     $$\text{Byte 0: Opcode} \quad|\quad \text{Byte 1: Dest} \quad|\quad \text{Byte 2: Src1} \quad|\quad \text{Byte 3: Src2}$$
   - Supports labels (`.label`), condition codes (`BEQ`, `BNE`, etc.), scalar & vector math, square bracket memory access, and `print`.
2. **Operating System & MMU (`os.c` / `os.h`)**:
   - **Page-based Memory Management**: Separates logical addresses from physical memory frames.
   - **Task Loader**: Allocates free physical frames, initializes page tables, and assigns processes to idle processor cores.
   - **Preemptive Round-Robin Scheduler**: Time-slices `NP = 4` cores (`TIME_SLICE = 10` instructions).
   - **Non-blocking Shell**: Asynchronously takes user commands (`$ <prog.txt>`, `$ status`, `$ exit`).
3. **Physical Memory Subsystem (`memory.c` / `memory.h`)**:
   - Unified byte-addressable physical RAM of `MEMSIZE = 8192` bytes divided into `PAGESIZE = 512` byte frames (`NUM_PHYSICAL_PAGES = 16`). Frame 0 is reserved.
   - All processes share a single default `data.byte` file for data memory initialization.
4. **Processor Engine (`processor.c` / `processor.h`)**:
   - `NP = 4` independent processor cores.
   - 256 32-bit scalar integer registers (`x0` - `x255`) per core.
   - 32 256-bit vector registers (`v0` - `v31`) per core (each holding 8 32-bit integers).
   - Condition flags (`Z`, `N`, `C`, `V`) updated on additions/subtractions.
   - Pipeline stages: `fetch()`, `decode()`, `execute()`.

### Lab 5 System Data Flow Diagram

```
+-------------------------------------------------------------------------+
|                    User / Interactive Shell & CLI                       |
|           $ tests/sum_n_fixed.txt                                       |
|           $ tests/complex_multiply.txt                                  |
|           $ tests/fir_filter.txt                                        |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
|                         Operating System (OS)                           |
|  * Loader: Compiles .txt -> .byte, allocates frames via getFreePage()   |
|  * Scheduler: Round-Robin time multiplexing (TIME_SLICE = 10 instrs)    |
|  * Process Table (PCB): State tracking (READY, WAITING, TERMINATED)     |
|  * Shell: Non-blocking STDIN input handling                             |
+-------------------------------------------------------------------------+
                                    |
            +-----------------------+-----------------------+
            |                                               |
            v (Processor 0)                                 v (Processor 1..NP-1)
+---------------------------------------+       +---------------------------------------+
|           Processor Core 0            |       |        Processor Cores 1..3           |
| * PC[0], x0..x255, v0..v31            |       | * PC[i], x0..x255, v0..v31            |
| * Flags: Z, N, C, V                   |       | * Flags: Z, N, C, V                   |
+---------------------------------------+       +---------------------------------------+
            |                                               |
            +-----------------------+-----------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
|                  Memory Management Unit (MMU)                           |
|  getPhysicallAddress(proc_id, isFetch, logical_address):                |
|  Logical Page -> PageTable[proc_id] -> Physical Frame * 512 + Offset    |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
|                    Shared Physical Memory Subsystem                     |
|           char memory[MEMSIZE = 8192] (16 frames of 512 bytes)          |
|                 Frame 0: Reserved (never allocated)                     |
|                 Frames 1..15: Dynamically allocated                     |
+-------------------------------------------------------------------------+
```

---

## 2. Logical vs. Physical Memory Architecture (Lab 5)

In Lab 5, processes never access physical memory directly:
1. **Logical Memory**:
   - Each process believes it has its own private address space starting at `0`.
   - Instruction Memory: Logical pages `0..1` (1024 bytes).
   - Data Memory: Logical pages `2..9` (4096 bytes).
   - Total logical pages per process: `NUM_LOGICAL_PAGES = 10`.
2. **Physical Memory**:
   - Total size: `MEMSIZE = 8192` bytes (16 frames of 512 bytes).
   - `pageTable[NP][NUM_LOGICAL_PAGES]` records which physical frame holds each logical page.
   - Translation formula in `getPhysicallAddress()`:
     $$\text{Logical Page} = \begin{cases} \text{addr} / 512 & \text{if fetch} \\ (\text{addr} / 512) + 2 & \text{if data read/write} \end{cases}$$
     $$\text{Physical Address} = (\text{Physical Frame} \times 512) + (\text{addr} \pmod{512})$$

---

## 3. The 5 Required Lab Programs (`tests/`)

All 5 required programs specified in the Lab specification FAQs have been created and verified against the shared `data.byte`:

| Program | Source File | Description | Output Verification |
| :--- | :--- | :--- | :--- |
| **1. Sum of N (Fixed)** | `tests/sum_n_fixed.txt` | Sum of integers 1 to N where N=10 is fixed at compile time. | Calculates sum = 55 (`0x37`), prints and writes to address 200. |
| **2. Complex Multiplication** | `tests/complex_multiply.txt` | Multiplies two complex numbers $(A + Bi) \times (C + Di)$. Real = $AC - BD$, Imag = $AD + BC$. | Real = -5 (`0xFFFFFFFB`), Imag = 10 (`0x0A`). |
| **3. 3x3 Matrix Determinant** | `tests/matrix_det.txt` | Calculates $\det(M)$ for 3x3 matrix stored row-wise at address 4. | Calculates determinant = 0 (`0x0`). |
| **4. Sum of Two Arrays (Runtime N)** | `tests/sum_arrays_runtime.txt` | Parallel vector addition of Array A and B of size N (loaded dynamically from address 0). | SIMD adds 8 words per vector operation. |
| **5. Vector FIR Filter** | `tests/fir_filter.txt` | 8-tap Finite Impulse Response filter using vector SIMD multiplication and window sliding. | Generates $(N - 8)$ outputs starting at address `0x100`. |

---

## 4. Instruction Set Architecture (ISA) Reference

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
| **Scalar Memory**| `x1 = [x2]` | `0x05` | Read word from memory at register address |
| | `x1 = [10]` | `0x0C` | Read word from memory at constant address |
| | `[x2] = x1` | `0x06` | Write word to memory at register address |
| | `[10] = x1` | `0x0E` | Write word to memory at constant address |
| **Debug Print** | `print x1` | `0x08` | Output register to console & `simulation.log` |
| **Branches** | `B{Suffix} <label>`| `0x10` - `0x1E` | Branch (`BEQ`, `BNE`, `BGE`, `BAL`, etc.) |
| **Vector Math** | `v1 = v2 + v3` | `0x21` | 8-way parallel vector addition |
| | `v1 = v2 + 10` / `v1 + x1` | `0x29` | 8-way vector + scalar addition |
| | `v1 = v2 - v3` | `0x22` | 8-way parallel vector subtraction |
| | `v1 = v2 - 10` / `v1 - x1` | `0x2A` | 8-way vector - scalar subtraction |
| | `v1 = v2 * v3` | `0x23` | 8-way parallel vector multiplication |
| | `v1 = v2 * 10` / `v1 * x1` | `0x2B` | 8-way vector * scalar multiplication |
| **Vector Memory**| `v1 = [x2]` | `0x25` | Load 8 words with auto-increment (+32 bytes) |
| | `v1 = [10]` | `0x2C` | Load 8 words from constant address |
| | `[x2] = v1` | `0x26` | Store 8 words with auto-increment (+32 bytes) |
| | `[10] = v1` | `0x2E` | Store 8 words to constant address |

---

## 5. How to Build & Run

### 1. Build
```bash
make clean
make
```

### 2. Run in Standalone Mode (Single Process)
Run any program directly; the simulator will compile the source, load the default `data.byte`, execute to completion, save output to `data_out_pid1.byte`, and return to terminal:
```bash
./simulator.exe tests/sum_n_fixed.txt
./simulator.exe tests/complex_multiply.txt
./simulator.exe tests/matrix_det.txt
./simulator.exe tests/sum_arrays_runtime.txt
./simulator.exe tests/fir_filter.txt
```

### 3. Run in Multi-Processing OS Mode (Interactive Shell)
Launch the interactive Operating System:
```bash
./simulator.exe
```

Inside the interactive shell (`$`):
- **Submit tasks** (uses the shared `data.byte` automatically):
  ```
  $ tests/sum_n_fixed.txt
  $ tests/complex_multiply.txt
  $ tests/matrix_det.txt
  $ tests/sum_arrays_runtime.txt
  $ tests/fir_filter.txt
  ```
- **Inspect active processes**:
  ```
  $ status
  ```
- **Exit**:
  ```
  $ exit
  ```
All completed tasks will save their data memory dumps to `data_out_pid<PID>.byte` and print statements to `simulation.log`.
