#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>

extern uint32_t Register[256];

extern int PC;
extern int opcode;
extern int dest;
extern int src1;
extern int src2;

extern int end_of_simulation;

extern int Z;
extern int N;
extern int C;
extern int V;

void reset(void);
void fetch(void);
void decode(void);
void execute(void);

#endif
