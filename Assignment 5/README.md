# CS527 Mini-Computer System Simulator (Lab 5)

A complete, modular, and beginner-friendly implementation of a multi-core mini-computer simulator system written in C.

---

## Lab 5 Features Overview

1. **Compiler (`compiler.c` / `compiler.h`)**:
   - Two-pass assembler translating high-level assembly into 4-byte machine code (`<opcode> <dest> <src1> <src2>`).
   - Supports arithmetic (`+`, `-`, `*`, `/`), vector operations, bracketed memory operations (`[addr]`), branch labels (`.loop`, `BEQ`, `BAL`), `print`, and comments (`%`).
2. **Processor (`processor.c` / `processor.h`)**:
   - Models `NP = 4` parallel processor cores.
   - 256 32-bit scalar registers (`x0` - `x255`) per core.
   - 32 256-bit vector registers (`v0` - `v31`) per core (each containing 8 32-bit integers).
   - Condition flags (`Z`, `N`, `C`, `V`) for conditional branching.
   - Fetch-Decode-Execute pipeline with time-sliced execution (`process_instructions`).
3. **Memory & MMU (`memory.c` / `memory.h`, `os.c`)**:
   - Byte-addressable physical RAM of 8192 bytes divided into 16 frames of 512 bytes (`#define PAGESIZE 512`, `#define MEMSIZE 8192`).
   - Frame 0 is reserved.
   - Dynamic logical-to-physical memory address translation via per-process page tables (`pageTable[NP][NUM_LOGICAL_PAGES]`).
   - All processes share a single default `data.byte` file for data memory initialization.
4. **Operating System Layer (`os.c` / `os.h`)**:
   - **Loader**: Automatically compiles `.txt` source files into bytecode, allocates physical frames via `getFreePage()`, and assigns free processor cores.
   - **Scheduler**: Preemptive Round-Robin scheduler executing processes in fixed time slices (`TIME_SLICE = 10` instructions).
   - **Shell**: Interactive non-blocking terminal CLI accepting user commands.

---

## Repository Structure

```
├── Makefile                    # Build script (make, make clean, make run)
├── main.c                      # Entry point (Standalone & Interactive OS modes)
├── compiler.h / compiler.c     # Lexer, parser, two-pass bytecode generator
├── memory.h / memory.c         # Physical memory operations & storage
├── processor.h / processor.c   # Processor cores, ISA execution, vector registers & flags
├── os.h / os.c                 # OS kernel, page tables, MMU, Round-Robin scheduler, Shell
├── data.byte                   # Unified shared data memory file
├── PROJECT_GUIDE.md            # In-depth architectural documentation
├── README.md                   # Quickstart guide and manual
└── tests/                      # Suite of Lab 5 test programs
    ├── data.byte               # Backup of unified data memory file
    ├── sum_n_fixed.txt         # Program 1: Sum of N where N is fixed (compile time)
    ├── complex_multiply.txt    # Program 2: Multiply two complex numbers
    ├── matrix_det.txt          # Program 3: Determinant of a 3x3 matrix
    ├── sum_arrays_runtime.txt  # Program 4: Sum of two arrays of size N (runtime, vector)
    ├── sum_array.txt           # Alias of Program 4
    ├── fir_filter.txt          # Program 5: 8-tap Vector FIR filter with dynamic N
    └── test_basic.txt          # Basic test for arithmetic and branches
```

---

## Building the Simulator

Compile with `make`:

```bash
make clean
make
```

---

## Running the 5 Required Lab 5 Programs

### 1. Standalone Execution Mode
You can run any test program directly from the command line without having to pass a separate data file (the simulator uses `data.byte` by default):

```bash
# 1. Sum of numbers 1 to 10 (fixed at compile time):
./simulator.exe tests/sum_n_fixed.txt

# 2. Multiplication of two complex numbers:
./simulator.exe tests/complex_multiply.txt

# 3. Determinant of a 3x3 matrix:
./simulator.exe tests/matrix_det.txt

# 4. Vector addition of two arrays A and B of runtime size N:
./simulator.exe tests/sum_arrays_runtime.txt

# 5. 8-tap Vector FIR filter response:
./simulator.exe tests/fir_filter.txt
```

Each run will output register print values to the console and `simulation.log`, and save the finalized data memory to `data_out_pid1.byte`.

---

### 2. Multi-Processing OS Interactive Shell Mode
Run `./simulator.exe` without arguments to launch the multi-processing operating system:

```bash
./simulator.exe
```

Inside the interactive shell:
- **Submit programs** (multiple tasks will be multiplexed across all 4 processor cores):
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
