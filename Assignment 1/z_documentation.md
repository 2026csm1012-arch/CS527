# Mini Computer Simulation System

**Course:** CS LAB — Assignment 1  
**Project:** Mini Computer Architecture & Simulator  

---

## 📋 Overview

The **Mini Computer** project is a C-based simulation of a simplified hardware architecture and execution system. It models fundamental computing concepts, including CPU instruction processing, RAM memory management, assembly compilation, and program execution via a custom simulator executable.

---

## 📁 Project Directory Structure

```text
CS LAB/
└── Assignment 1/
    └── Mini Computer/
        ├── Makefile            # Build system rules and dependencies
        ├── main.c              # Main application entry point & setup
        ├── main.o              # Compiled object file for main module
        ├── processor.h         # Architecture declarations & register definitions
        ├── processor.c         # CPU execution loop & instruction decoding
        ├── processor.o         # Compiled object file for CPU module
        ├── memory.h            # Memory map and interface functions
        ├── memory.c            # RAM reading, writing, and storage allocation
        ├── memory.o            # Compiled object file for memory module
        ├── compiler.h          # Source code parser declarations
        ├── compiler.c          # Converts assembly code into binary bytecode
        ├── compiler.o          # Compiled object file for compiler module
        ├── program.txt         # Human-readable instruction source file
        ├── program.byte        # Output machine code / binary instructions
        ├── data.byte           # Pre-loaded RAM data payload
        └── simulator.exe       # Fully compiled simulation executable
```

---

## 🏛 Architecture & Design Components

### 1. Processor Module (`processor.h`, `processor.c`)
- Models CPU registers, Program Counter (PC), and flag registers.
- Handles the core instruction pipeline: **Fetch $\rightarrow$ Decode $\rightarrow$ Execute**.
- Implements arithmetic, logical, data transfer, and control flow instructions.

### 2. Memory Subsystem (`memory.h`, `memory.c`)
- Simulates byte-addressable system memory (RAM).
- Provides read and write functions with bounds checking.
- Pre-loads static data arrays from `data.byte` into active RAM blocks.

### 3. Compiler (`compiler.h`, `compiler.c`)
- Acts as an assembler for the system.
- Reads high-level or assembly source statements from `program.txt`.
- Emits executable binary bytecode saved into `program.byte`.

### 4. Controller & Driver (`main.c`)
- Coordinates component initialization.
- Triggers program compilation or loads pre-compiled bytecode.
- Controls execution clock cycles and system shutdown.

---

## 📊 Component Interaction Flow

```
+------------------+         +-------------------+         +------------------+
|   program.txt    | ------> |    compiler.c     | ------> |   program.byte   |
| (Assembly Code)  |         |    (Assembler)    |         |   (Machine Code) |
+------------------+         +-------------------+         +------------------+
                                                                    |
                                                                    v
+------------------+         +-------------------+         +------------------+
|    data.byte     | ------> |    memory.c       | <-----> |   processor.c    |
|   (Data Array)   |         |   (Simulated RAM) |         | (Execution Loop) |
+------------------+         +-------------------+         +------------------+
```

---

## 🛠 Compilation & Build Guide

### Prerequisites
- GCC / Clang C Compiler
- GNU Make Utility

### Build Instructions

To compile the entire simulator system, run:
```bash
make
```

To clean intermediate object files and executables:
```bash
make clean
```

### Running the Simulator

To start the simulator and run `program.byte`:
```bash
./simulator.exe
```

---

## 📄 File Formats Reference

| File Extension | Content Type | Description |
| :--- | :--- | :--- |
| `.c` / `.h` | C Source / Header | Architecture implementation and definitions |
| `.o` | Object File | Intermediate binary compilation targets |
| `.txt` | Plain Text | Source program text in assembly/pseudo-code format |
| `.byte` | Raw Binary | Processable bytecode and data blocks |
| `.exe` | Executable | Binary file to execute the hardware simulation |
