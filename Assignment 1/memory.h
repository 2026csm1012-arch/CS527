#ifndef MEMORY_H
#define MEMORY_H

extern unsigned char Instruction[256];
extern unsigned char Data[256];


void initialise(char folder[]);
void finalize(char folder[]);

#endif
// Memory_h is macro name for header file
// here if macro is not defined , then it define new one and include the content of header
// instruction and data are declared as external variables,
// which means they can be accessed from other files that include this header file.
// it includes func to provide implementation for  memory
