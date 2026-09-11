#ifndef MEMORY_H
#define MEMORY_H

// main processor array is here that we can work with infiles.
extern unsigned char instruction[256];
extern char Data[4096];

// memory inistalise and finalise methodsare  here.
void initialize();
void finalize();
int read_int(int address);
void write_int(int address, int value);

#endif