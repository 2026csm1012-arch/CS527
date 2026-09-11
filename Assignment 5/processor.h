#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "memory.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

extern int32_t Register[NP][256];
extern int32_t VectorRegister[NP][32][8];
extern int PC[NP];
extern int Z[NP], N[NP], C[NP], V[NP];
extern int end_of_simulation[NP];
extern FILE *fd_log;

void reset(int proc_id);
void fetch(int proc_id, int *opcode, int *dest, int *src1, int *src2);
void decode(void);
void execute(int proc_id, int opcode, int dest, int src1, int src2);
void process_instructions(int proc_id, int instruction_count);
void processor_cleanup(void);

#endif