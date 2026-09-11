#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

extern unsigned char Instruction[256];
extern unsigned char Data[4096];

uint32_t readWord(int address);
void writeWord(int address, uint32_t value);

void initialise(char folder[]);
void finalize(char folder[]);

#endif
