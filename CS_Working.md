# CS527 Lab 5 — Mini-Computer / OS Simulator
## Complete Project Status, Architecture, Data Flow, Components, and AI Handoff

> **Purpose of this file:** This README is the single source of truth for the current Lab 5 project.  
> Give this file to another AI together with the project source when you want it to understand what has already been implemented and what remains to be done.

---

# 1. Project Overview

This project is a **C-based mini-computer system simulator** for CS527 Lab 5.

The project models several layers of a computer system:

```text
User Program
     |
     v
Compiler / Assembler
     |
     v
Machine Bytecode
     |
     v
Operating System / Loader
     |
     +--------------------+
     |                    |
     v                    v
Process / PCB          MMU / Paging
     |                    |
     +---------+----------+
               |
               v
        Simulated Processor
        +----------------+
        | CPU 0          |
        | CPU 1          |
        | CPU 2          |
        | CPU 3          |
        +----------------+
               |
               v
        Physical Memory
               |
               v
          Output Data
```

The project currently contains:

- A two-pass compiler/assembler
- A bytecode instruction format
- Scalar CPU registers
- Vector/SIMD registers
- Program counter and condition flags
- Arithmetic and memory instructions
- Branch instructions
- Vector instructions
- Physical memory
- Logical-to-physical address translation
- Paging and page allocation
- Per-process page tables
- Process control blocks (PCB)
- Four simulated processors
- Process states
- Loader
- Waiting queue
- Round-Robin scheduling
- Time slicing
- Interactive OS shell
- Standalone/single-program execution
- Output memory dumps
- Simulation logging
- Multiple test programs

---

# 2. Current Implementation Status

## Overall status

The core Lab 5 simulator architecture is implemented.

### Completed

- [x] Project split into compiler, memory, processor, OS and main components
- [x] Makefile
- [x] C compilation with GCC
- [x] Compiler/assembler
- [x] Two-pass label handling
- [x] Scalar registers
- [x] Vector registers
- [x] Program counter
- [x] Arithmetic operations
- [x] Constant arithmetic operations
- [x] Register copy / constant loading
- [x] Scalar memory read/write
- [x] Vector memory read/write
- [x] Branch instructions
- [x] Condition flags
- [x] Physical memory
- [x] Paging configuration
- [x] Page allocation
- [x] Per-processor page tables
- [x] Logical-to-physical address translation
- [x] Process Control Block
- [x] Process states
- [x] Process loader
- [x] Four simulated processors
- [x] Waiting queue behavior
- [x] Round-Robin scheduling
- [x] Time slice of 10 instructions
- [x] Interactive shell
- [x] Standalone mode
- [x] Output memory generation
- [x] Simulation log
- [x] Function-level data-flow comments
- [x] Project documentation

### Important verification status

The project has been structured and documented for both:

1. **Single-processing / standalone execution**
2. **Multi-processing / OS scheduling**

The supplied project currently contains the source programs under `tests/`.

The separately mentioned files:

```text
commands_basic.txt
commands_parallel.txt
```

are **not present in the current project archive inspected for this README**. If these files exist elsewhere, they should be added to the project before claiming that those exact automated command-file tests are part of the repository.

---

# 3. Project Directory Structure

Current project structure:

```text
lab5_inspect/
│
├── Makefile
├── README.md
├── PROJECT_GUIDE.md
│
├── main.c
├── compiler.c
├── compiler.h
├── memory.c
├── memory.h
├── processor.c
├── processor.h
├── os.c
├── os.h
│
├── data.byte
│
└── tests/
    ├── complex_multiply.txt
    ├── data.byte
    ├── fir_filter.txt
    ├── fir_filter_data.byte
    ├── matrix_det.txt
    ├── sum_array.txt
    ├── sum_array_data.byte
    ├── sum_arrays_runtime.txt
    ├── sum_n_fixed.txt
    ├── test_basic.txt
    ├── test_basic_data.byte
    ├── vector_add.txt
    └── vector_add_data.byte
```

---

# 4. Components of the Project

## 4.1 `main.c`

### Responsibility

`main.c` is the entry point of the entire simulator.

It decides whether the simulator should run in:

- Standalone mode
- Multi-process OS mode

### Main functions

```c
main()
run_standalone()
run_os_mode()
```

### Data flow

```text
Command line
    |
    v
main()
    |
    +---- arguments supplied ----> run_standalone()
    |
    +---- no arguments ----------> run_os_mode()
```

### Significance

This provides two interfaces to the same simulator:

```text
One program
    -> standalone execution

Multiple programs
    -> OS / scheduler execution
```

---

# 5. Compiler / Assembler

## Files

```text
compiler.c
compiler.h
```

## Purpose

The compiler converts an assembly-like source program into machine bytecode.

Example source:

```text
x1 = x2 + x3
```

becomes a four-byte instruction containing:

```text
opcode destination source1 source2
```

Conceptually:

```text
+--------+--------+--------+--------+
| opcode |  dest  |  src1  |  src2  |
+--------+--------+--------+--------+
```

Each instruction occupies four bytes.

---

# 6. Compiler Data Flow

```text
source program
     |
     v
read line
     |
     v
remove comments
     |
     v
parse instruction
     |
     +---- register?
     |
     +---- constant?
     |
     +---- memory?
     |
     +---- vector?
     |
     +---- branch?
     |
     v
encode opcode
     |
     v
write bytecode
```

---

# 7. Compiler Functions

## `strip_comments()`

### Working

Removes comments from a source line.

### Data flow

```text
raw source line
      |
      v
strip_comments()
      |
      v
clean source line
```

### Significance

Allows programmers to write readable assembly without comments becoming part of the instruction parser.

---

## `find_label()`

### Working

Searches the compiler's label table for a label name.

### Data flow

```text
label name
    |
    v
label table
    |
    v
instruction position
```

### Significance

Required for branch instructions.

---

## `parse_register()`

### Working

Checks whether a token represents a valid scalar or vector register and extracts its numeric index.

Examples:

```text
x1
x25
v0
v31
```

### Data flow

```text
"x5"
  |
  v
parse_register()
  |
  v
register number = 5
```

---

## `parse_constant()`

Converts a textual constant into an integer.

Example:

```text
-18
```

becomes:

```text
-18
```

This is needed for constant arithmetic and constant memory addresses.

---

## `get_branch_opcode()`

Maps branch mnemonics to their opcode.

Examples include:

```text
BEQ
BNE
BGE
BLT
BGT
BLE
BAL
```

---

## `compile()`

This is the main compiler function.

### Working

The compiler performs two logical passes:

### Pass 1

Find labels and determine their instruction positions.

```text
source
 |
 +--> labels
 |
 +--> instruction positions
```

### Pass 2

Generate actual bytecode.

```text
source + label table
        |
        v
encoded instructions
        |
        v
program.byte
```

### Significance

A two-pass design allows a branch to refer to a label that appears later in the source.

---

# 8. Memory System

## Files

```text
memory.c
memory.h
```

## Configuration

```text
MEMSIZE = 8192 bytes
PAGESIZE = 512 bytes
NUM_PHYSICAL_PAGES = 16
INSTRUCTION_MEM_SIZE = 1024 bytes
DATA_MEM_SIZE = 4096 bytes
NUM_LOGICAL_PAGES = 10
```

Therefore:

```text
Physical memory = 8192 bytes
Page size       = 512 bytes
Physical frames = 16
```

Logical memory per process:

```text
Code = 1024 bytes = 2 pages
Data = 4096 bytes = 8 pages

Total = 10 logical pages
```

---

# 9. Physical Memory Layout

Conceptually:

```text
Physical RAM
+--------------------------------+
| Frame 0                        |
| Reserved                       |
+--------------------------------+
| Frame 1                        |
+--------------------------------+
| Frame 2                        |
+--------------------------------+
| ...                            |
+--------------------------------+
| Frame 15                       |
+--------------------------------+
```

Frame 0 is reserved.

The remaining frames are available for process allocation.

---

# 10. Logical Memory

Each process sees its own logical address space.

Conceptually:

```text
Logical Page 0
    Code

Logical Page 1
    Code

Logical Page 2
    Data

Logical Page 3
    Data

...

Logical Page 9
    Data
```

This gives:

```text
2 code pages
+
8 data pages
=
10 logical pages
```

---

# 11. Memory Functions

## `memory_init()`

Initializes the physical memory array.

### Data flow

```text
program start
     |
     v
memory_init()
     |
     v
physical memory reset
```

---

## `mem_read_byte_phys()`

Reads one byte directly from a physical memory address.

Used after logical addresses have already been translated.

---

## `mem_write_byte_phys()`

Writes one byte to physical memory.

---

## `mem_read_32()`

Reads a 32-bit integer from logical memory.

### Data flow

```text
logical address
      |
      v
MMU translation
      |
      v
physical address
      |
      v
4 physical bytes
      |
      v
32-bit integer
```

---

## `mem_write_32()`

Writes a 32-bit integer into logical memory.

### Data flow

```text
32-bit value
     |
     v
split into bytes
     |
     v
logical address
     |
     v
MMU
     |
     v
physical memory
```

---

# 12. MMU / Paging

The operating system maintains:

```c
pageTable[NP][NUM_LOGICAL_PAGES]
```

With:

```text
NP = 4
```

there are four simulated processors/process address spaces.

The page table maps:

```text
logical page -> physical frame
```

---

# 13. Logical-to-Physical Address Translation

For an instruction fetch:

```text
logical page = address / 512
```

For data memory:

```text
logical page = (address / 512) + 2
```

The physical address is conceptually:

```text
physical address =
    physical frame * 512
    +
    page offset
```

where:

```text
page offset = logical address % 512
```

### Example

If:

```text
logical address = 530
```

then:

```text
page = 530 / 512 = 1
offset = 530 % 512 = 18
```

The page table determines which physical frame contains logical page 1.

Then:

```text
physical address =
frame * 512 + 18
```

---

# 14. Operating System

## Files

```text
os.c
os.h
```

This is the main process-management layer.

It combines:

- Process management
- Loader
- Memory allocation
- MMU
- Scheduling
- Waiting queue
- Shell

---

# 15. Process Control Block

The project uses a PCB containing information such as:

```text
PID
Processor ID
Program source
Compiled bytecode
Data file
Output file
Process state
Data size
```

Conceptually:

```text
PCB
+----------------------+
| PID                  |
| Processor ID         |
| Program source       |
| Program bytecode     |
| Data file             |
| Output file           |
| State                 |
| Data size             |
+----------------------+
```

---

# 16. Process States

The project defines:

```text
PROC_UNUSED
PROC_WAITING
PROC_READY
PROC_RUNNING
PROC_TERMINATED
```

Typical lifecycle:

```text
UNUSED
  |
  v
loader()
  |
  v
READY
  |
  v
RUNNING
  |
  +------> RUNNING
  |
  v
TERMINATED
```

If no processor is available:

```text
loader()
   |
   v
WAITING
   |
   v
processor becomes free
   |
   v
READY
   |
   v
RUNNING
```

---

# 17. Loader

## `loader()`

The loader is responsible for bringing a submitted program into the simulated computer.

### Data flow

```text
program file
     |
     v
loader()
     |
     +--> compile if needed
     |
     +--> create PID
     |
     +--> create PCB
     |
     +--> allocate memory pages
     |
     +--> load program/data
     |
     +--> assign free processor
     |
     +--> or put process in waiting state
```

### Significance

The loader connects the user-facing program submission to the internal OS.

---

# 18. `os_initialize_memory()`

Loads program instructions and data into the process's logical memory.

### Data flow

```text
program.byte
     |
     v
allocate logical pages
     |
     v
page table
     |
     v
physical frames
     |
     v
physical memory
```

For data:

```text
data.byte
   |
   v
logical data memory
   |
   v
MMU
   |
   v
physical RAM
```

---

# 19. `os_finalize_memory()`

Called when a process finishes.

It:

1. Extracts the process data memory.
2. Writes it to an output file.
3. Releases the process's physical pages.

Data flow:

```text
physical RAM
     |
     v
logical data addresses
     |
     v
output_data
     |
     v
data_out_pidN.byte
```

Then:

```text
process pages
     |
     v
free page table mappings
     |
     v
frames available again
```

---

# 20. Processor / CPU

## Files

```text
processor.c
processor.h
```

The simulator contains:

```text
NP = 4
```

simulated processors.

These are **simulated CPU cores**, not four native operating-system processes.

---

# 21. CPU Register Architecture

Each processor has:

```text
256 scalar registers
```

represented by:

```c
Register[NP][256]
```

Each scalar register is:

```text
int32_t
```

---

# 22. Vector Register Architecture

Each processor has:

```text
32 vector registers
```

Each vector register contains:

```text
8 x 32-bit integers
```

represented by:

```c
VectorRegister[NP][32][8]
```

This supports SIMD-style operations.

Conceptually:

```text
v0 = [a0 a1 a2 a3 a4 a5 a6 a7]
v1 = [b0 b1 b2 b3 b4 b5 b6 b7]

v2 = v0 + v1

v2 = [
 a0+b0,
 a1+b1,
 ...
 a7+b7
]
```

---

# 23. Program Counter

Each processor has its own:

```c
PC[NP]
```

The PC identifies the next instruction to fetch.

Typical cycle:

```text
PC
 |
 v
FETCH
 |
 v
DECODE
 |
 v
EXECUTE
 |
 v
PC updated
 |
 +----> next instruction
```

---

# 24. CPU Flags

Each processor maintains:

```text
Z = Zero
N = Negative
C = Carry
V = Overflow
```

These are used mainly by arithmetic and conditional branches.

Example:

```text
x1 = x2 - x3
```

can update:

```text
Z
N
C
V
```

Then:

```text
BEQ label
```

can test the zero condition.

---

# 25. Processor Functions

## `reset()`

Resets the state of a simulated processor.

Typical state reset includes:

```text
PC = 0
registers = 0
vector registers = reset
flags = reset
end_of_simulation = false
```

---

## `fetch()`

Fetches the next four-byte instruction.

### Data flow

```text
PC
 |
 v
logical instruction address
 |
 v
MMU
 |
 v
physical address
 |
 v
4 instruction bytes
 |
 v
opcode + dest + src1 + src2
```

---

## `decode()`

Represents the decode stage of the CPU pipeline.

The actual instruction fields are already obtained by the fetch/parsing path, while opcode-specific behavior is dispatched during execution.

---

## `execute()`

This is the central instruction execution function.

It receives:

```text
processor id
opcode
destination
source 1
source 2
```

and performs the requested operation.

It handles the simulator's scalar, vector, memory, branch and utility instructions.

---

## `process_instructions()`

Executes a bounded number of instructions for a processor.

This is especially important for the OS scheduler.

If:

```text
TIME_SLICE = 10
```

then the scheduler can call:

```text
process_instructions(proc_id, 10)
```

The process runs for up to 10 instructions before control returns to the scheduler.

### Data flow

```text
scheduler
    |
    v
process_instructions()
    |
    +--> fetch
    |
    +--> decode
    |
    +--> execute
    |
    +--> update PC
    |
    v
return control to scheduler
```

---

# 26. Instruction Set

The project supports the following broad categories.

## Control

```text
Halt
```

---

## Scalar arithmetic

```text
ADD
SUB
MUL
DIV
```

and constant forms:

```text
ADD constant
SUB constant
MUL constant
DIV constant
```

---

## Data movement

```text
register copy
load constant
```

---

## Scalar memory

```text
read from register address
read from constant address

write to register address
write to constant address
```

---

## Branches

Examples:

```text
BEQ
BNE
BGE
BLT
BGT
BLE
BAL
```

Branches use the processor flags/condition evaluation.

---

## Vector arithmetic

```text
vector + vector
vector - vector
vector * vector
```

and scalar/constant forms.

---

## Vector memory

Vector loads/stores operate on eight 32-bit values.

This provides SIMD-style processing.

---

## Print

The project supports printing a scalar register and logging the result.

---

# 27. Important Instruction Data Flow

## Scalar ADD

```text
x2 ----+
       |
       +----> ADD ----> x1
       |
x3 ----+
```

Internally:

```text
read Register[x2]
read Register[x3]
        |
        v
     addition
        |
        +--> update flags
        |
        v
write Register[x1]
```

---

## Scalar memory READ

```text
register/address
       |
       v
logical address
       |
       v
MMU
       |
       v
physical memory
       |
       v
32-bit value
       |
       v
destination register
```

---

## Scalar memory WRITE

```text
source register
       |
       v
32-bit value
       |
       v
logical address
       |
       v
MMU
       |
       v
physical memory
```

---

## Vector operation

```text
v1 ----+
       |
       +----> VECTOR ADD/MUL/etc. ----> v3
       |
v2 ----+
```

Eight elements are processed.

---

# 28. Scheduler

## Configuration

```text
MAX_PROCESSES = 64
TIME_SLICE = 10
NP = 4
```

Therefore the simulator can track up to 64 PCBs while four processors are available for execution.

---

# 29. Round-Robin Scheduling

The scheduler provides time slicing.

Conceptually:

```text
Round 1

CPU 0 -> PID 1 -> 10 instructions
CPU 1 -> PID 2 -> 10 instructions
CPU 2 -> PID 3 -> 10 instructions
CPU 3 -> PID 4 -> 10 instructions

              |
              v

Round 2

CPU 0 -> PID 1 -> next 10
CPU 1 -> PID 2 -> next 10
CPU 2 -> PID 3 -> next 10
CPU 3 -> PID 4 -> next 10
```

When a process terminates:

```text
PID finishes
    |
    v
write output
    |
    v
free pages
    |
    v
free processor
    |
    v
take process from WAITING queue
```

---

# 30. Waiting Queue

There are only four simulated processors.

If four programs are already running:

```text
PID 1 -> CPU 0
PID 2 -> CPU 1
PID 3 -> CPU 2
PID 4 -> CPU 3
```

and another program arrives:

```text
PID 5 -> WAITING
```

When one CPU becomes free:

```text
CPU 1
  |
  v
PID 2 terminates
  |
  v
CPU 1 becomes free
  |
  v
PID 5 leaves WAITING
  |
  v
PID 5 -> CPU 1
```

This demonstrates the OS process queue and resource allocation.

---

# 31. Important Clarification About "True Multi-Core"

The simulator has four **simulated processors**.

It demonstrates:

- Multiple PCBs
- Multiple processor slots
- Independent PC/register state
- Waiting processes
- Scheduling
- Time slicing
- Process completion
- Processor reuse

However, this implementation is a simulator. It does **not necessarily mean four native CPU threads are executing simultaneously**.

The scheduler calls simulated processor execution from the C program.

For a viva, describe it as:

> "A four-core simulated processor architecture with OS-level multi-process scheduling."

Do not claim that the C program creates four native parallel threads unless the implementation is later changed to use `pthread`, OpenMP, processes, or another native parallel mechanism.

---

# 32. Interactive Shell

`os.c` provides an interactive shell.

Conceptually:

```text
$ program.txt
$ status
$ exit
```

### Program submission

A program path is passed to:

```text
loader()
```

The OS then either:

```text
assigns a free processor
```

or:

```text
places the process in WAITING
```

---

# 33. `status`

The shell can display process information.

It is useful for checking:

```text
PID
processor
state
program
```

Example conceptual output:

```text
PID 1 | Proc 0 | RUNNING | tests/sum_n_fixed.txt
PID 2 | Proc 1 | RUNNING | tests/matrix_det.txt
PID 3 | Proc 2 | READY   | tests/fir_filter.txt
PID 5 | Proc -1 | WAITING | tests/vector_add.txt
```

---

# 34. Exit Behavior

The shell supports:

```text
exit
```

The intention is to stop accepting new work while allowing existing work to finish.

The OS continues scheduling active/waiting jobs until the system becomes inactive.

---

# 35. Single-Processing / Standalone Mode

Use this mode to run one program directly.

## Linux / WSL / Git Bash

```bash
./simulator tests/sum_n_fixed.txt
```

With a custom data file:

```bash
./simulator tests/fir_filter.txt tests/fir_filter_data.byte
```

## Windows PowerShell

```powershell
.\simulator.exe tests\sum_n_fixed.txt
```

With custom data:

```powershell
.\simulator.exe tests\fir_filter.txt tests\fir_filter_data.byte
```

### Standalone data flow

```text
command line
     |
     v
main()
     |
     v
run_standalone()
     |
     v
os_init()
     |
     v
loader()
     |
     v
memory allocation
     |
     v
processor execution
     |
     v
process termination
     |
     v
output memory
```

---

# 36. Multi-Processing / OS Mode

Start without arguments.

Linux:

```bash
./simulator
```

Windows PowerShell:

```powershell
.\simulator.exe
```

Then submit programs through the shell.

Example:

```text
$ tests/sum_n_fixed.txt
$ tests/complex_multiply.txt
$ tests/matrix_det.txt
$ tests/sum_arrays_runtime.txt
$ tests/fir_filter.txt
```

With four processors:

```text
PID 1 -> CPU 0
PID 2 -> CPU 1
PID 3 -> CPU 2
PID 4 -> CPU 3
PID 5 -> WAITING
```

---

# 37. Building the Project

From the project directory:

```bash
make clean
make
```

The Makefile currently uses:

```text
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -O2
TARGET = simulator
```

Build pipeline:

```text
main.c
compiler.c
processor.c
memory.c
os.c
   |
   v
object files
   |
   v
simulator
```

---

# 38. Windows PowerShell Important Note

Bash supports:

```bash
./simulator < commands_parallel.txt
```

PowerShell does **not** use `<` in the same way for stdin redirection.

In PowerShell use:

```powershell
Get-Content commands_parallel.txt | .\simulator.exe
```

Similarly:

```powershell
Get-Content commands_basic.txt | .\simulator.exe
```

Therefore, if working in Windows PowerShell, use:

```powershell
make clean
make
Get-Content commands_basic.txt | .\simulator.exe
Get-Content commands_parallel.txt | .\simulator.exe
```

The files `commands_basic.txt` and `commands_parallel.txt` are not included in the currently inspected project archive. Add them if they are part of the required submission/test setup.

---

# 39. Test Programs

The repository contains several test programs.

## `tests/sum_n_fixed.txt`

Purpose:

```text
Calculate 1 + 2 + ... + N
```

The documented test uses:

```text
N = 10
```

Expected mathematical result:

```text
55
```

---

## `tests/complex_multiply.txt`

Purpose:

Multiply:

```text
(A + Bi)(C + Di)
```

using:

```text
Real = AC - BD
Imag = AD + BC
```

The documented test expects:

```text
Real = -5
Imag = 10
```

---

## `tests/matrix_det.txt`

Purpose:

Calculate the determinant of a 3×3 matrix.

The documented example produces:

```text
determinant = 0
```

---

## `tests/sum_arrays_runtime.txt`

Purpose:

Demonstrate runtime-size array addition.

It also demonstrates vector/SIMD processing.

Conceptually:

```text
A[i] + B[i] = C[i]
```

with eight values handled per vector operation.

---

## `tests/fir_filter.txt`

Purpose:

Demonstrate a vectorized finite impulse response filter.

It uses:

```text
8-tap FIR
```

and vector processing.

---

## Other tests

The repository also contains:

```text
tests/test_basic.txt
tests/vector_add.txt
tests/sum_array.txt
```

These are useful for smaller functional checks and vector/data-memory behavior.

---

# 40. Generated Files

Depending on execution, the simulator can produce files such as:

```text
program.byte
prog_pid*.byte
data_out_pid*.byte
simulation.log
```

### `program.byte`

Compiled machine instructions.

### `prog_pid*.byte`

Process-specific compiled program representation where used by the OS path.

### `data_out_pid*.byte`

Final data-memory contents for a process.

### `simulation.log`

Console/print-related simulation logging.

---

# 41. Complete End-to-End Execution

## Case A — One program

```text
User
 |
 | run program
 v
main()
 |
 v
run_standalone()
 |
 v
os_init()
 |
 v
loader()
 |
 +--> compile()
 |
 +--> allocate pages
 |
 +--> page table
 |
 +--> load program
 |
 +--> load data
 |
 v
scheduler()
 |
 v
process_instructions()
 |
 +--> fetch()
 |
 +--> decode()
 |
 +--> execute()
 |
 +--> memory/MMU
 |
 v
Halt
 |
 v
os_finalize_memory()
 |
 v
data_out_pid1.byte
```

---

# 42. Complete End-to-End Multi-Process Flow

```text
                   USER
                    |
                    v
             Interactive shell
                    |
                    v
                loader()
                    |
          +---------+---------+
          |                   |
     free processor       no processor
          |                   |
          v                   v
      READY/RUNNING        WAITING
          |                   |
          +---------+---------+
                    |
                    v
               scheduler()
                    |
          +---------+---------+
          |         |         |
        CPU 0     CPU 1     CPU 2 ... CPU 3
          |         |         |
          +---------+---------+
                    |
                    v
             10-instruction
                time slice
                    |
                    v
             process executes
                    |
          +---------+---------+
          |                   |
      still running        finished
          |                   |
          v                   v
    next time slice      finalize output
                              |
                              v
                         free pages
                              |
                              v
                       free processor
                              |
                              v
                     schedule WAITING job
```

---

# 43. Function-Level Data Flow Summary

## Main

```text
main
 -> run_standalone / run_os_mode
```

## Compiler

```text
compile
 -> strip_comments
 -> parse_register
 -> parse_constant
 -> find_label
 -> get_branch_opcode
 -> write bytecode
```

## OS

```text
os_init
 -> memory/PCB/page state initialization

loader
 -> compile
 -> create process
 -> allocate pages
 -> load memory
 -> assign processor / waiting queue

scheduler
 -> execute time slice
 -> detect completion
 -> finalize process
 -> free pages
 -> schedule waiting process
```

## Processor

```text
process_instructions
 -> fetch
 -> decode
 -> execute
 -> update PC
```

## Memory

```text
mem_read_32 / mem_write_32
 -> logical address
 -> MMU
 -> physical frame
 -> physical memory
```

---

# 44. What Has Been Added for Code Understanding

The source files have been commented so that functions explain:

1. **Data flow**
2. **How the function works**
3. **Why the function is significant**

The comments are intentionally aimed at someone learning the project rather than only documenting syntax.

The most important conceptual chain is:

```text
Input
 ↓
Compiler
 ↓
Bytecode
 ↓
Loader
 ↓
PCB
 ↓
Page allocation
 ↓
Page table
 ↓
Processor
 ↓
Fetch
 ↓
Decode
 ↓
Execute
 ↓
Memory/MMU
 ↓
Scheduler
 ↓
Process completion
 ↓
Output
```

---

# 45. What Is Already Implemented vs. What Should Be Checked Next

## Already implemented

The project architecture contains the following major features:

```text
Compiler
Memory
MMU
Paging
CPU
Registers
Vector registers
Flags
Branches
Processes
PCB
Loader
Scheduler
Waiting queue
Interactive shell
Standalone mode
Multi-process mode
Output generation
Logging
Tests
Documentation
```

## Recommended next verification

Before final submission, verify these independently:

### 1. Build

```bash
make clean
make
```

### 2. Standalone

```bash
./simulator tests/sum_n_fixed.txt
```

### 3. More standalone programs

```bash
./simulator tests/complex_multiply.txt
./simulator tests/matrix_det.txt
./simulator tests/sum_arrays_runtime.txt
./simulator tests/fir_filter.txt
```

### 4. Interactive OS

```bash
./simulator
```

Then submit several programs.

### 5. Status

```text
status
```

### 6. Waiting queue

Submit more than four programs so that at least one process must wait.

### 7. Completion

Confirm that a waiting process moves onto a processor after an active process terminates.

### 8. Output

Check:

```text
data_out_pid*.byte
```

### 9. Logs

Check:

```text
simulation.log
```

### 10. Command-file tests

If required by the assignment, add:

```text
commands_basic.txt
commands_parallel.txt
```

and verify them separately.

---

# 46. Important Technical Caveats for the Next AI

The next AI should **not assume** the following without checking the actual source:

### Caveat 1 — Native parallelism

Four simulated processors do not automatically mean four native threads.

Check the implementation before describing it as actual OS-level parallel execution.

### Caveat 2 — Command files

The current archive inspected for this README does not contain:

```text
commands_basic.txt
commands_parallel.txt
```

Therefore those exact tests cannot be claimed as repository-contained tests until the files are added.

### Caveat 3 — ISA documentation

The exact opcode behavior must be taken from `compiler.c` and `processor.c`, not only from this README.

If changing an opcode, update both:

```text
compiler encoding
processor execution
```

### Caveat 4 — Memory

Do not bypass the MMU when implementing normal logical-memory operations.

The intended path is:

```text
logical address
 -> page table
 -> physical frame
 -> physical address
 -> physical RAM
```

### Caveat 5 — Process state

Any new scheduling behavior should preserve:

```text
WAITING
READY
RUNNING
TERMINATED
```

and processor ownership.

---

# 47. Recommended Way to Explain This Project in a Viva

A concise explanation:

> "This project is a C-based mini-computer and operating-system simulator. A user writes an assembly-like program, which is compiled into four-byte machine instructions. The OS loader creates a PCB, allocates paged memory and maps logical pages to physical frames using a page table. The program then executes on one of four simulated processors. Each processor has scalar and vector registers, a program counter and condition flags. The scheduler gives processes a ten-instruction time slice using Round-Robin scheduling. If all four processors are occupied, additional processes enter a waiting queue. When a process terminates, its memory is released and the scheduler can load a waiting process. Finally, the process's data memory is written to an output file."

---

# 48. Short Architecture Summary

```text
+--------------------------------------------------+
|                    USER                          |
+-------------------------+------------------------+
                          |
                          v
+--------------------------------------------------+
|              COMPILER / ASSEMBLER               |
|       Source -> Machine Bytecode                 |
+-------------------------+------------------------+
                          |
                          v
+--------------------------------------------------+
|                 OPERATING SYSTEM                 |
|                                                  |
|  Loader | PCB | Scheduler | Waiting Queue       |
|                                                  |
|  MMU | Page Tables | Physical Page Allocation    |
+-------------------------+------------------------+
                          |
                          v
+--------------------------------------------------+
|              4 SIMULATED PROCESSORS             |
|                                                  |
| CPU 0 | CPU 1 | CPU 2 | CPU 3                  |
|                                                  |
| Registers | Vector Registers | PC | Flags        |
+-------------------------+------------------------+
                          |
                          v
+--------------------------------------------------+
|                PHYSICAL MEMORY                  |
|                  8192 bytes                     |
|                 16 x 512 pages                  |
+--------------------------------------------------+
                          |
                          v
+--------------------------------------------------+
|                    OUTPUT                        |
|             data_out_pid*.byte                  |
+--------------------------------------------------+
```

---

# 49. AI Handoff — Give This to the Next AI

Use the following context when asking another AI to continue the project:

```text
I am working on CS527 Lab 5, a C-based mini-computer/OS simulator.

The project is already implemented to the following major level:

1. Compiler/assembler:
   - Assembly-like source language
   - Two-pass compilation
   - Labels
   - Branch instructions
   - Scalar and vector instructions
   - Constants
   - Memory operations
   - Print

2. Processor:
   - 4 simulated processors
   - 256 int32 scalar registers per processor
   - 32 vector registers per processor
   - 8 int32 lanes per vector register
   - Program counter
   - Z/N/C/V flags
   - Fetch/decode/execute structure
   - Bounded instruction execution for scheduling

3. Memory:
   - 8192-byte physical memory
   - 512-byte pages
   - 16 physical frames
   - Frame 0 reserved
   - 1024-byte code area
   - 4096-byte data area
   - 10 logical pages per process
   - Logical-to-physical translation through page tables

4. OS:
   - PCB
   - PID
   - Process states
   - Loader
   - Page allocation
   - Per-processor/process page tables
   - Waiting queue
   - Round-Robin scheduling
   - TIME_SLICE = 10 instructions
   - Four processor slots
   - Interactive shell
   - Process finalization
   - Output data files
   - Simulation logging

5. Execution modes:
   - Standalone mode:
       ./simulator tests/program.txt
   - OS/multi-process mode:
       ./simulator
   - On Windows PowerShell use:
       .\simulator.exe ...
       Get-Content commands.txt | .\simulator.exe

6. Test programs exist in tests/:
   - sum_n_fixed.txt
   - complex_multiply.txt
   - matrix_det.txt
   - sum_arrays_runtime.txt
   - fir_filter.txt
   - test_basic.txt
   - vector_add.txt
   - sum_array.txt

7. Important caveat:
   - The current repository archive does not contain commands_basic.txt or commands_parallel.txt.
   - Four processors are simulated by the program; do not claim native four-thread execution unless the source is changed to use native parallelism.

8. Before changing code:
   - Inspect compiler.c and processor.c for the exact opcode mapping.
   - Preserve the logical-address -> page-table -> physical-address memory path.
   - Preserve PCB/process-state/scheduler behavior.
   - Keep standalone and OS modes working.
   - Run make clean && make after modifications.
   - Test at least one standalone program and the multi-process path.

9. The project source has been commented function-by-function with:
   - data flow
   - working
   - significance

The complete project status and architecture are documented in README.md.
```

---

# 50. Final Project Status

## Architecture

**Implemented:** Yes.

## Compiler

**Implemented:** Yes.

## CPU simulation

**Implemented:** Yes.

## Vector/SIMD support

**Implemented:** Yes.

## Paging/MMU

**Implemented:** Yes.

## Process management

**Implemented:** Yes.

## Four simulated processors

**Implemented:** Yes.

## Waiting queue

**Implemented:** Yes.

## Round-Robin scheduler

**Implemented:** Yes.

## Standalone execution

**Implemented:** Yes.

## Interactive multi-process execution

**Implemented:** Yes.

## Function-level explanatory comments

**Added:** Yes.

## Project documentation

**Added:** Yes.

## Exact `commands_basic.txt` / `commands_parallel.txt` test files

**Not present in the inspected archive:** These should be added/verified if they are required for the final submission.

---

# 51. Final Mental Model

If you remember only one thing, remember this:

```text
                PROGRAM
                   |
                   v
              COMPILER
                   |
                   v
               BYTECODE
                   |
                   v
                LOADER
                   |
             creates PCB
                   |
                   v
          allocate physical pages
                   |
                   v
              PAGE TABLE
                   |
                   v
              PROCESSOR
                   |
          +--------+--------+
          |        |        |
         CPU0     CPU1     CPU2 ... CPU3
          |
          v
       FETCH
          |
          v
       DECODE
          |
          v
       EXECUTE
          |
          +----> Registers
          |
          +----> Vector Registers
          |
          +----> Memory
                    |
                    v
                   MMU
                    |
                    v
              Physical RAM

        Scheduler controls all processes
                    |
                    v
          10 instructions/time slice
                    |
                    v
              process finishes
                    |
                    v
             output + free pages
                    |
                    v
            waiting process starts
```

**This is the complete current conceptual state of the Lab 5 project.**
