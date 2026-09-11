#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "memory.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Processor Register Files and State Variables for NP processors 
extern int32_t Register[NP][256];         // 256 32-bit Scalar Registers (x0 - x255)
extern int32_t VectorRegister[NP][32][8]; // 32 Vector Registers (v0 - v31), 8 elements each
extern int PC[NP];                        // Program Counter for each processor
extern int Z[NP], N[NP], C[NP], V[NP];    // Condition Flags
extern int end_of_simulation[NP];         // Simulation termination flag (1 = halted)
extern FILE *fd_log;                      // Shared log file for print instructions


// Reset processor state: zeroes registers, sets PC=0, clears flags,
// and opens the execution log file in append mode.
// proc_id Processor ID (0 to NP-1)

void reset(int proc_id);
//  Fetch stage: Reads 4 bytes from Instruction[proc_id][PC[proc_id]]
//  into opcode, dest, src1, and src2. Advances PC by 4.
 
void fetch(int proc_id, int *opcode, int *dest, int *src1, int *src2);

//  Decode stage: Passthrough function as per lab specification.
void decode(void);

//  Execute stage: Executes the instruction corresponding to opcode on processor proc_id.
void execute(int proc_id, int opcode, int dest, int src1, int src2);

//  Process a time-sliced batch of instructions on processor proc_id.
//  Executes up to instruction_count cycles, then sleeps for 10 microseconds.
void process_instructions(int proc_id, int instruction_count);

// Close log file and cleanup processor resources.
void processor_cleanup(void);

#endif 