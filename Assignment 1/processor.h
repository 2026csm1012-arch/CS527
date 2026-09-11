#ifndef processor_h
#define processor_h

extern int Register[256];

extern int PC;
extern int opcode;
extern int dest;
extern int src1;
extern int src2;

extern int end_of_simulation;

void reset(void);
void fetch(void);
void decode(void);
void execute(void);

#endif

// registers are fo 256 bytes and declared as external variable which means they can be accessed from other files that include this header file.
//  variables needed program counter, opcode, destination register, source registers and end of simulation are declared as external variable
//  functions reset, fetch, decode and execute are declared to provide implementation for processor
//  end_of_simulation is used to check if the simulation has ended or not, and it is declared as an external variable so that it can be accessed from other files that include this header file.