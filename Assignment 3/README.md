# CS527 Mini-Computer System Simulator (Lab 3)

A clean, modular, and beginner-friendly implementation of a single-processor mini-computer simulator system supporting 32-bit integer ISA and 256-bit Vector SIMD operations written in C.

---

## Architecture Overview

1. **Compiler (`compiler.c`, `compiler.h`)**:
   - Compiles high-level assembly into 4-byte machine instructions (`<opcode> <dest> <src1> <src2>`).
   - Supports arithmetic (`+`, `-`, `*`, `/`), vector math, memory brackets (`[addr]`), branch labels (`.loop`, `BEQ`, `BAL`), and comments (`%`).
2. **Processor (`processor.c`, `processor.h`)**:
   - 256 32-bit scalar integer registers (`x0` - `x255`).
   - 32 256-bit vector registers (`v0` - `v31`), each holding 8 32-bit integers.
   - Condition flags (`Z`, `N`, `C`, `V`) for conditional branching.
   - Standard execution pipeline: `reset()`, `fetch()`, `decode()`, `execute()`.
3. **Memory (`memory.c`, `memory.h`)**:
   - Byte-addressable Little-Endian memory system.
   - 256 bytes instruction memory and 4096 bytes data memory.
   - Reads/writes in 4-byte space-separated hexadecimal format (`program.byte`, `data.byte`, `data_out.byte`).

---

## File Structure

```
├── Makefile                # Build automation script
├── main.c                  # Main entry point and simulation pipeline
├── compiler.h / compiler.c # Assembly-to-bytecode compiler
├── memory.h / memory.c     # Byte-addressable memory management
├── processor.h / processor.c # ISA execution engine, vector registers & flags
├── tests/                  # Example test programs and data files
│   ├── sum_array.txt       # Sum of N numbers array example
│   ├── sum_array_data.byte # Data memory for sum_array
│   ├── vector_add.txt      # Parallel vector addition (8 elements)
│   ├── vector_add_data.byte# Data memory for vector_add
│   ├── fir_filter.txt      # 8-tap Vector FIR filter computation
│   ├── fir_filter_data.byte# Data memory for fir_filter
│   └── test_basic.txt      # Basic arithmetic & control flow tests
└── README.md               # Documentation
```

---

## Building & Running

### 1. Build
```bash
make
```

### 2. Run
```bash
# Run default program.txt
make run

# Or pass custom program and data files
./simulator.exe program.txt
./simulator.exe tests/sum_array.txt tests/sum_array_data.byte
./simulator.exe tests/vector_add.txt tests/vector_add_data.byte
./simulator.exe tests/fir_filter.txt tests/fir_filter_data.byte
```

---

## Instruction Set & Opcode Reference

| Operation | Variable Operand 2 | Constant Operand 2 | Description |
| :--- | :--- | :--- | :--- |
| **Halt** | `0x00` | `0x00` | Ends simulation |
| **Add** | `0x01` (`x1 = x2 + x3`) | `0x09` (`x1 = x2 + 10`) | Scalar integer addition (updates Z, N, C, V) |
| **Subtract** | `0x02` (`x1 = x2 - x3`) | `0x0A` (`x1 = x2 - 10`) | Scalar integer subtraction (updates Z, N, C, V) |
| **Multiply** | `0x03` (`x1 = x2 * x3`) | `0x0B` (`x1 = x2 * 10`) | Scalar integer multiplication |
| **Divide** | `0x04` (`x1 = x2 / x3`) | `0x0C` (`x1 = x2 / 10`) | Scalar integer division |
| **Memory Read** | `0x05` (`x1 = [x2]`) | `0x0C` (`x1 = [10]`) | 32-bit integer read from memory |
| **Memory Write** | `0x06` (`[x2] = x1`) | `0x0E` (`[10] = x1`) | 32-bit integer write to memory |
| **Data Movement** | `0x07` (`x1 = x2`) | `0x0F` (`x1 = 10`) | Copy register or immediate value |
| **Print** | `0x08` (`print x1`) | `0x08` | Output register value to console |
| **Branch** | `0x10` - `0x1E` (`BEQ .label`, etc.) | - | Conditional/unconditional relative jump |
| **Vector Add** | `0x21` (`v1 = v2 + v3`) | `0x29` (`v1 = v2 + 10`) | Parallel 8-way integer addition |
| **Vector Subtract**| `0x22` (`v1 = v2 - v3`) | `0x2A` (`v1 = v2 - 10`) | Parallel 8-way integer subtraction |
| **Vector Multiply**| `0x23` (`v1 = v2 * v3`) | `0x2B` (`v1 = v2 * 10`) | Parallel 8-way integer multiplication |
| **Vector Mem Read**| `0x25` (`v1 = [x2]`) | `0x2C` (`v1 = [10]`) | Read 8 words from memory with auto-increment |
| **Vector Mem Write**| `0x26` (`[x2] = v1`) | `0x2E` (`[10] = v1`) | Write 8 words to memory with auto-increment |
