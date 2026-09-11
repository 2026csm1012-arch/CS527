#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "memory.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Processor Register Files and State Variables as specified in Lab 3 */
extern int32_t Register[256];          /* 256 32-bit Scalar Registers (x0 - x255) */
extern int32_t VectorRegister[32][8];  /* 32 Vector Registers (v0 - v31), 8 elements each */
extern int PC, opcode, dest, src1, src2;
extern int Z, N, C, V;                 /* Condition Flags */
extern int end_of_simulation;          /* Simulation termination flag (1 = halted) */

void reset(void);

void fetch(void);

void decode(void);

void execute(void);

#endif /* PROCESSOR_H */
