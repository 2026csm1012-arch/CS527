#include <stdio.h>
#include <stdint.h>
#include "processor.h"
#include "memory.h"

uint32_t Register[256];

int PC;
int opcode;
int dest;
int src1;
int src2;

int end_of_simulation = 0;

int currentInstructionAddress;

int Z, N, C, V;

void reset(void)
{
    int i;

    for (i = 0; i < 256; i++)
        Register[i] = 0;

    PC = 0;
    opcode = 0;
    dest = 0;
    src1 = 0;
    src2 = 0;

    Z = 0;
    N = 0;
    C = 0;
    V = 0;

    end_of_simulation = 0;
}

void fetch(void)
{
    currentInstructionAddress = PC;

    opcode = Instruction[PC];
    dest = Instruction[PC + 1];
    src1 = Instruction[PC + 2];
    src2 = Instruction[PC + 3];

    PC = PC + 4;
}

void decode(void)
{
    /* No extra decoding is needed for this simple processor. */
}

void updateAddFlags(uint32_t a, uint32_t b, uint32_t result)
{
    uint64_t sum = (uint64_t)a + (uint64_t)b;

    Z = (result == 0);
    N = (result >> 31) & 1;
    C = (sum > 0xFFFFFFFF);

    V = (((a ^ b) & 0x80000000) == 0 &&
         ((a ^ result) & 0x80000000) != 0);
}

void updateSubFlags(uint32_t a, uint32_t b, uint32_t result)
{
    Z = (result == 0);
    N = (result >> 31) & 1;
    C = (a > b);

    V = (((a ^ b) & 0x80000000) != 0 &&
         ((b ^ result) & 0x80000000) != 0);
}

int branchTaken(int condition)
{
    switch (condition)
    {
        case 0:  return Z == 1;                 /* EQ */
        case 1:  return Z == 0;                 /* NE */
        case 2:  return C == 1;                 /* CS */
        case 3:  return C == 0;                 /* CC */
        case 4:  return N == 1;                 /* MI */
        case 5:  return N == 0;                 /* PL */
        case 6:  return V == 1;                 /* VS */
        case 7:  return V == 0;                 /* VC */
        case 8:  return C == 1 && Z == 0;       /* HI */
        case 9:  return C == 0 || Z == 1;       /* LS */
        case 10: return N == V;                 /* GE */
        case 11: return N != V;                 /* LT */
        case 12: return Z == 0 && N == V;       /* GT */
        case 13: return Z == 1 || N != V;       /* LE */
        case 14: return 1;                      /* AL */
    }

    return 0;
}

void execute(void)
{
    uint32_t a;
    uint32_t b;
    uint32_t result;
    int address;
    int condition;
    int offset;

    switch (opcode)
    {
        case 0x00:
            end_of_simulation = 1;
            break;

        /* Arithmetic using registers. */
        case 0x01:
            a = Register[src1];
            b = Register[src2];
            result = a + b;
            Register[dest] = result;
            updateAddFlags(a, b, result);
            break;

        case 0x02:
            a = Register[src1];
            b = Register[src2];
            result = a - b;
            Register[dest] = result;
            updateSubFlags(a, b, result);
            break;

        case 0x03:
            Register[dest] = Register[src1] * Register[src2];
            break;

        case 0x04:
            if (Register[src2] == 0)
            {
                printf("Division by zero.\n");
                end_of_simulation = 1;
            }
            else
            {
                Register[dest] = Register[src1] / Register[src2];
            }
            break;

        /* Legacy memory read/write. */
        case 0x05:
            address = (int)Register[src1];
            Register[dest] = readWord(address);
            break;

        case 0x06:
            address = (int)Register[src1];
            writeWord(address, Register[dest]);
            break;

        /* Legacy data movement. */
        case 0x07:
            Register[dest] = src1;
            break;

        /* Arithmetic using a constant. */
        case 0x09:
            a = Register[src1];
            b = src2;
            result = a + b;
            Register[dest] = result;
            updateAddFlags(a, b, result);
            break;

        case 0x0A:
            a = Register[src1];
            b = src2;
            result = a - b;
            Register[dest] = result;
            updateSubFlags(a, b, result);
            break;

        case 0x0B:
            Register[dest] = Register[src1] * src2;
            break;

        case 0x0C:
            if (src2 == 0)
            {
                printf("Division by zero.\n");
                end_of_simulation = 1;
            }
            else
            {
                Register[dest] = Register[src1] / src2;
            }
            break;

        /* Memory read with a constant address. */
        case 0x0D:
            address = src2;
            Register[dest] = readWord(address);
            break;

        /* Memory write with a constant value. */
        case 0x0E:
            address = (int)Register[src1];
            writeWord(address, src2);
            break;

        /* Data movement with a constant. */
        case 0x0F:
            Register[dest] = src2;
            break;

        /* Branch. */
        default:
            if (opcode >= 0x10 && opcode <= 0x1E)
            {
                condition = opcode - 0x10;

                if (branchTaken(condition))
                {
                    offset = (int8_t)src2;
                    PC = currentInstructionAddress + (offset * 4);
                }
            }
            else
            {
                printf("Invalid opcode: %02X\n", opcode);
                end_of_simulation = 1;
            }
            break;
    }
}
