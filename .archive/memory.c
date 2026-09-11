#include "memory.h"
#include <stdio.h>

// these are actually character , but we can say that these are continous string of characcter ,
//  whileindex are just position for  every byte instruction we are going  to store
unsigned char instruction[256];
char Data[4096];

// initalize intermediate file and data byte file, where all operation will be stored after.
// this open both  files and copy complete data from files to data registers andintruction array  that we  create in processor file.
void initialize()
{

    FILE *input = fopen("program.byte", "r");
    FILE *datafile = fopen("data.byte", "r");

    if (input == NULL || datafile == NULL)
    {
        printf("error while opening files\n");
        return;
    }

    int opcode, dest, src1, src2;
    int i = 0;
    // it can the line in a  given format that initalise in intruction array  whta is used by  processor file.
    while (fscanf(input, "%x %x %x %x", &opcode, &dest, &src1, &src2) == 4)
    {
        instruction[i] = opcode;
        instruction[i + 1] = dest;
        instruction[i + 2] = src1;
        instruction[i + 3] = src2;
        i += 4;
        
        printf("Loaded instruction bytes: %d\n", i);
    }

    //  similarly data file is  initiaised  with values of given files.
    i = 0;
    int v1, v2, v3, v4;
    while (fscanf(datafile, "%x %x %x %x", &v1, &v2, &v3, &v4) == 4)
    {
        Data[i] = v1;
        Data[i + 1] = v2;
        Data[i + 2] = v3;
        Data[i + 3] = v4;
        i += 4;
    }

    // filepoites are closed.
    fclose(input);
    fclose(datafile);
}

// it used to write the final output and  data from operation generated from processiojn files.
void finalize()
{

    // data. byte file is opened in write mode
    FILE *output = fopen("data.byte", "w");
    // error in opening file
    if (output == NULL)
    {
        printf("Error while writing file...");
        return;
    }

    // size for data array, and used to write data .byte file
    for (int i = 0; i < 4096; i += 4)
    {
        fprintf(output, "%x %x %x %x\n", (unsigned char)Data[i], (unsigned char)Data[i + 1],
        (unsigned char)Data[i + 2], (unsigned char)Data[i + 3]);
    }
    // closes the  file
    fclose(output);
}

// this is used read 32 bit  integer stored in little  endian
int read_int(int address)
{
    int value = 0;

    value = value | (unsigned char)Data[address];    // first 8  bits  from LSB
    value |= (unsigned char)Data[address + 1] << 8;  // 2ndchunk of 8  bit while reading biits from right to  left order
    value |= (unsigned char)Data[address + 2] << 16; // 3rd chunk
    value |= (unsigned char)Data[address + 3] << 24; // 4th chunk and MSB

    return value;
}

// it is used to write value of large inno.in little endian format, byte shiffting 8 bits at a time,
// and last 8bit are seperated from main value and a&& with FF so that it can  be stored in data[]
void write_int(int address, int value)
{
    Data[address] = value & 0xFF;
    Data[address + 1] = (value >> 8) & 0xFF;
    Data[address + 2] = (value >> 16) & 0xFF;
    Data[address + 3] = (value >> 24) & 0xFF;
}