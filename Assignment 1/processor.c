#include <stdio.h>
#include "processor.h"
#include "memory.h"

int Register[256];

int PC;
int opcode;
int dest;
int src1;
int src2;

int end_of_simulation = 0;

//reseting register
void reset(void)
{
    int i;

    for (i = 0; i < 256; i++)
    {
        Register[i] = 0;
    }

    PC = 0;
    end_of_simulation = 0;
}

//instructtion fetching
void fetch(void)
{
    opcode = Instruction[PC];
    dest = Instruction[PC + 1];
    src1 = Instruction[PC + 2];
    src2 = Instruction[PC + 3];

    printf("PC: %d, Opcode: %d, Dest: %d, Src1: %d, Src2: %d\n", PC, opcode, dest, src1, src2);

    PC = PC + 4;
}

void decode(void)
{}

void execute(void)
{
    switch (opcode)
    {
    case 0:
        end_of_simulation = 1;
        break;

    case 1:
        Register[dest] = Register[src1] + Register[src2];
        break;

    case 2:
        Register[dest] = Register[src1] - Register[src2];
        break;

    case 3:
        Register[dest] = Register[src1] * Register[src2];
        break;

    case 4:
        if (Register[src2] != 0)
        {
            Register[dest] = Register[src1] / Register[src2];
        }
        else
        {
            printf("Division by zero!\n");
        }
        break;

    case 5:
        Register[dest] = Data[src1];
        printf("read %d from address %d\n", Data[src1], src1);
        break;

    case 6:
        Data[src1] = Register[dest];
        printf("write %d to address %d\n", Register[dest], src1);
        break;

    case 7:
        Register[dest] = src1;
        break;

    default:
        printf("Invalid Opcode : %d\n", opcode);
        end_of_simulation = 1;
        break;
    }
}