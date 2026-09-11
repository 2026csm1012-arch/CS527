#include <stdio.h>
#include <stdint.h>
#include "memory.h"

unsigned char Instruction[256];
unsigned char Data[4096];

uint32_t readWord(int address)
{
    uint32_t value = 0;

    if (address < 0 || address + 3 >= 4096)
        return 0;

    value = ((uint32_t)Data[address] << 24);
    value = value | ((uint32_t)Data[address + 1] << 16);
    value = value | ((uint32_t)Data[address + 2] << 8);
    value = value | Data[address + 3];

    return value;
}

void writeWord(int address, uint32_t value)
{
    if (address < 0 || address + 3 >= 4096)
        return;

    Data[address] = (value >> 24) & 0xFF;
    Data[address + 1] = (value >> 16) & 0xFF;
    Data[address + 2] = (value >> 8) & 0xFF;
    Data[address + 3] = value & 0xFF;
}

void clearMemory(void)
{
    int i;

    for (i = 0; i < 256; i++)
        Instruction[i] = 0;

    for (i = 0; i < 4096; i++)
        Data[i] = 0;
}

void initialise(char folder[])
{
    FILE *programFile;
    FILE *dataFile;
    char programPath[200];
    char dataPath[200];

    unsigned int b1, b2, b3, b4;
    int index = 0;

    sprintf(programPath, "%sprogram.byte", folder);
    sprintf(dataPath, "%sdata.byte", folder);

    clearMemory();

    programFile = fopen(programPath, "r");

    if (programFile == NULL)
    {
        printf("Error: Cannot open %s\n", programPath);
        return;
    }

    /* Every line contains four hexadecimal bytes. */
    while (fscanf(programFile, "%x %x %x %x", &b1, &b2, &b3, &b4) == 4)
    {
        if (index + 3 < 256)
        {
            Instruction[index++] = b1;
            Instruction[index++] = b2;
            Instruction[index++] = b3;
            Instruction[index++] = b4;
        }
    }

    fclose(programFile);

    dataFile = fopen(dataPath, "r");

    if (dataFile == NULL)
    {
        printf("Error: Cannot open %s\n", dataPath);
        return;
    }

    index = 0;

    while (fscanf(dataFile, "%x %x %x %x", &b1, &b2, &b3, &b4) == 4)
    {
        if (index + 3 < 4096)
        {
            Data[index++] = b1;
            Data[index++] = b2;
            Data[index++] = b3;
            Data[index++] = b4;
        }
    }

    fclose(dataFile);

    printf("Instruction memory loaded.\n");
    printf("Data memory loaded.\n");
}

void finalize(char folder[])
{
    FILE *dataFile;
    char dataPath[200];
    int i;

    sprintf(dataPath, "%sdata.byte", folder);

    dataFile = fopen(dataPath, "w");

    if (dataFile == NULL)
    {
        printf("Error: Cannot write %s\n", dataPath);
        return;
    }

    /* Four bytes per line, two hexadecimal digits per byte. */
    for (i = 0; i < 4096; i += 4)
    {
        fprintf(dataFile, "%02X %02X %02X %02X\n",
                Data[i], Data[i + 1], Data[i + 2], Data[i + 3]);
    }

    fclose(dataFile);

    printf("Data memory saved.\n");
}
