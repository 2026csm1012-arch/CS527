# CS527 Mini-Computer System Simulator (Labs 1 – 4)

A clean, modular, and beginner-friendly implementation of a multi-core mini-computer simulator system written in C.

---

## Architecture Overview

The system models a complete hardware-software computing stack:
1. **Compiler (`compiler.c`, `compiler.h`)**:
   - Parses high-level assembly language into 4-byte machine bytecodes (`<opcode> <dest> <src1> <src2>`).
   - Supports arithmetic (`+`, `-`, `*`, `/`), vector math, memory operations (`[addr]`), branch labels (`.loop`, `BEQ`, `BAL`), `print`, and comments (`%`).
2. **Processor (`processor.c`, `processor.h`)**:
   - Models `NP = 4` parallel processors.
   - 256 32-bit scalar registers (`x0` - `x255`) per processor.
   - 32 256-bit vector registers (`v0` - `v31`) per processor (each containing 8 32-bit integers).
   - Condition flags (`Z`, `N`, `C`, `V`) for conditional branching.
   - Fetch-Decode-Execute pipeline with time-sliced execution (`process_instructions`).
3. **Memory (`memory.c`, `memory.h`)**:
   - Byte-addressable Little-Endian memory system.
   - 256 bytes instruction memory and 4096 bytes data memory per processor.
   - Reads/writes in 4-byte space-separated hexadecimal format (`program.byte`, `data.byte`).
4. **Operating System Layer (`os.c`, `os.h`)**:
   - **Loader**: Loads and prepares tasks for execution, allocating free hardware processors or enqueuing into a waiting queue.
   - **Scheduler**: Preemptive Round-Robin scheduler executing processes in fixed time slices (`TIME_SLICE = 10` instructions).
   - **Shell**: Interactive non-blocking terminal CLI accepting user commands.

---

## File Structure

```
├── Makefile                # Build automation script
├── main.c                  # Program entry point (Standalone & OS modes)
├── compiler.h / compiler.c # Lexer, parser, and bytecode code generator
├── memory.h / memory.c     # Byte-addressable memory management
├── processor.h / processor.c # ISA execution engine, vector registers & flags
├── os.h / os.c             # Multi-processing OS, Round-Robin scheduler & Shell
├── tests/                  # Example test programs and data files
│   ├── sum_array.txt       # Sum of N numbers array example
│   ├── sum_array_data.byte # Data memory for sum_array
│   ├── vector_add.txt      # Parallel vector addition (8 elements)
│   ├── vector_add_data.byte# Data memory for vector_add
│   ├── fir_filter.txt      # 8-tap Vector FIR filter computation
│   ├── fir_filter_data.byte# Data memory for fir_filter
│   └── test_basic.txt      # Basic arithmetic & control flow tests
├── PROJECT_GUIDE.md        # Comprehensive Architecture & Step-by-Step Guide
└── README.md               # User manual
```

---

## Building the Simulator

Compile with `make` or directly using `gcc`:

```bash
make
```

---

## Running the Simulator

### 1. Multi-Processing OS Mode (Assignment 4)
Run without arguments (or with `make run`) to launch the OS environment with the interactive shell:

```bash
./simulator.exe
```

Inside the interactive shell:
- Submit programs: `$ tests/sum_array.txt tests/sum_array_data.byte`
- Submit multiple concurrent jobs:
  ```
  $ tests/vector_add.txt tests/vector_add_data.byte
  $ tests/fir_filter.txt tests/fir_filter_data.byte
  ```
- Check running tasks: `$ status`
- Exit shell and wait for background tasks: `$ exit`

### 2. Standalone Single Program Mode (Labs 1 / 2 / 3)
Run any test assembly program directly:

```bash
./simulator.exe tests/sum_array.txt tests/sum_array_data.byte
./simulator.exe tests/vector_add.txt tests/vector_add_data.byte
./simulator.exe tests/fir_filter.txt tests/fir_filter_data.byte
```
