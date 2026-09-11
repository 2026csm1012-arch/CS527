#ifndef PROCESSOR_H
#define PROCESSOR_H

// this h files are blue prints for the .c files ther we can get that function iis declared and it exists butdont know where it exists, so wedefine the funcition in .c file.
extern int registers[256];
extern int program_counter;
extern int end_of_simulation;

// all global variable are  defined here
extern int opcode;
extern int dest;
extern int src1;
extern int src2;

// theseare the function declared but  we need to define them in file, it just works line interface in java
void reset();
void fetch();
void decode();
void execute();

#endif