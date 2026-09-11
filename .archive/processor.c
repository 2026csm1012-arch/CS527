#include "processor.h"
#include "memory.h"
#include <stdio.h>
#include <stdint.h>

// global register of  size 256*4byte. this will give us temporary storage that processor will workon
int registers[256];

// flags are associalted with every process  that is done by  it.
int z = 0;
int n = 0;
int c = 0;
int v = 0;

// program_counter is acctually pointer on iinstruction array  wwherewill be niext operation is stored.
int program_counter;
// these variables are associate with every instruction, or we can saty that is it a structure given to us.
int opcode, dest, src1, src2;
int end_of_simulation = 0;

// reset initialise all refgister to value zero that must be done because we want to sttart black wiith processor registers.
void reset()
{
    program_counter = 0;
    for (int i = 0; i < 256; i++)
    {
        registers[i] = 0;
    }
}

// fetch instruction that is starting from program counter that is globbaly we control

void fetch()
{
    printf("PC = %d\n", program_counter);

    opcode = instruction[program_counter];
    dest = instruction[program_counter + 1];
    src1 = instruction[program_counter + 2];
    src2 = instruction[program_counter + 3];

    printf("Instruction: %x %x %x %x\n",
           opcode, dest, src1, src2);

    program_counter += 4;
}

//  decode is generally done on hardware , so  we skip for now
void decode() {}

// this  will execute  all intruction and operation on memory  and registers, but basically  every ting is mapped to registers,
// this method is called in while loop, so that every instruction is executed.
void execute()
{

    // this work on basics of opcode and according to opcode every thinf is perofrmed and that iis stored.
    switch (opcode)
    {

    case 0:
    {
        end_of_simulation = 1;
        break;
    }
    case 1:
    {
        // uint32_t and int32_t are strictly telling processor to consider it as 32 bit or 64 bit int according to that all flags are set and operation is determined where to store how to store.
        int32_t a = (int32_t)registers[src1];
        int32_t b = (int32_t)registers[src2];

        // here strict 64 bit unsigned integer is used, to checck for the overflow flag and , first 64 bit addition is  donem andthen it is  mapped to 32 bit int.
        uint64_t unsigned_result = (uint64_t)(uint32_t)a + (uint64_t)(uint32_t)b;
        int32_t result = (int32_t)(uint32_t)unsigned_result;
        registers[dest] = (int32_t)result;

        //  flagvalues are set here
        z = (registers[dest] == 0);
        n = (registers[dest] < 0);
        c = (unsigned_result > UINT32_MAX);
        v = ((a > 0 && b > 0 && result < 0) || (a < 0 && b < 0 && result >= 0));

        break;
    }
    case 2:
    {
        // strict size mapping is done for subtraction of 2 interger and after  that mapping of 32 bit  intefger is  doneon integer adn result is checked if flagnee to  beset or  not.
        int32_t a = (int32_t)registers[src1];
        int32_t b = (int32_t)registers[src2];
        uint32_t unsigned_result = (uint32_t)a - (uint32_t)b;
        int32_t result = (int32_t)unsigned_result;
        registers[dest] = result;

        z = (registers[dest] == 0);
        n = (registers[dest] < 0);
        c = ((uint32_t)a >= (uint32_t)b);
        v = ((a > 0 && b < 0 && result < 0) || (a < 0 && b > 0 && result >= 0));

        printf("SUB: x%d = %d - %d = %d\n",
               dest,
               a,
               b,
               result);

        printf("FLAGS: Z=%d N=%d C=%d V=%d\n",
               z, n, c, v);
        break;
    }
    case 3:
    {
        registers[dest] = registers[src1] * registers[src2];

        z = (registers[dest] == 0);
        n = (registers[dest] < 0);

        break;
    }
    case 4:
    {
        if (registers[src2] == 0)
        {
            printf("error, denominator cant be zero");
            end_of_simulation = 1;
        }
        else
        {
            registers[dest] = registers[src1] / registers[src2];
        }
        break;
    }
    // read write and data movement in this part for legacy read and write of  instructions.
    case 5:
    {
        registers[dest] = read_int(registers[src2]);
        break;
    }
    case 6:
    {
        write_int(registers[dest], registers[src2]);
        break;
    }
    case 7:
    {
        registers[dest] = src1;
        break;
    }

    // only  sub and addition are updating the flfags for now, so both opcode  for register movement and constant arithmatic, we update flags.for now
    case 9:
    {
        uint32_t a = (uint32_t)registers[src1];
        uint32_t b = (uint32_t)src2;

        uint64_t result = (uint64_t)a + (uint64_t)b;

        registers[dest] = (int32_t)result;

        z = (registers[dest] == 0);
        n = (registers[dest] < 0);
        c = (result > UINT32_MAX);
        v = ((a > 0 && b > 0 && result < 0) || (a < 0 && b < 0 && result >= 0));

        break;
    }

    // subtraction case with constant
    case 0x0A:
    {
        int32_t a = (int32_t)registers[src1];
        int32_t b = (int32_t)src2;
        uint32_t unsigned_result = (uint32_t)a - (uint32_t)b;
        int32_t result = (int32_t)unsigned_result;
        registers[dest] = (int32_t)result;

        z = (result == 0);
        n = (result < 0);
        c = ((uint32_t)a >= (uint32_t)b);
        v = ((a >= 0 && b < 0 && result < 0) || (a < 0 && b >= 0 && result >= 0));
        printf("SUB: x%d = %d - %d = %d\n",
               dest,
               a,
               b,
               result);

        printf("FLAGS: Z=%d N=%d C=%d V=%d\n",
               z, n, c, v);
        break;
    }
    // multiplication with constant
    case 0x0B:
    {
        registers[dest] = registers[src1] * src2;
        break;
    }

    // divisibilty with a constant and we update the value if denominator is not 0
    case 0x0C:
    {
        if (src2 == 0)
        {
            printf("Error: division by zero\n");
            end_of_simulation = 1;
            break;
        }
        registers[dest] = registers[src1] / src2;
        break;
    }

    // this is constant read and write for constant, along side the data transfer of 0f  symbol.
    case 0x0D:
    {
        registers[dest] = read_int(src2);
        break;
    }
    case 0x0E:
    {
        write_int(src2, registers[dest]);
        break;
    }
    case 0x0F:
    {
        registers[dest] = src2;
        break;
    }

    case 0x10: // BEQ
    {
        if (z == 1)
        {
            int8_t offset = (int8_t)src2;
            program_counter += (offset - 1) * 4;
        }
        break;
    }

    case 0x11: // BNE
    {
        if (z == 0)
        {
            int8_t offset = (int8_t)src2;
            program_counter += (offset - 1) * 4;
        }
        break;
    }

    case 0x12: // BCS
    {
        if (c == 1)
        {
            int8_t offset = (int8_t)src2;
            program_counter += (offset - 1) * 4;
        }
        break;
    }

    case 0x13: // BCC
    {
        if (c == 0)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x14: // BMI
    {
        if (n == 1)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x15: // BPL
    {
        if (n == 0)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x16: // BVS
    {
        if (v == 1)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x17: // BVC
    {
        if (v == 0)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x18: // BHI
    {
        if (c == 1 && z == 0)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x19: // BLS
    {
        if (c == 0 || z == 1)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x1A: // BGE
    {
        if (n == v)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x1B: // BLT
    {
        if (n != v)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x1C: // BGT
    {
        if (z == 0 && n == v)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x1D: // BLE
    {
        if (z == 1 || n != v)
        {
            int8_t offset = (int8_t)src2;
            program_counter += offset * 4;
        }
        break;
    }

    case 0x1E: // BAL
    {
        int8_t offset = (int8_t)src2;
        program_counter += (offset * 4) - 4;
        break;
    }
    default:
        printf("Unknown opcode: %d\n", opcode);
        end_of_simulation = 1;
        break;
    }

    printf("instruction complete\n");

    // this prints out thevalue of flags for every instruction in execution.
    printf("Z=%d N=%d C=%d V=%d\n", z, n, c, v);
}
