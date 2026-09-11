#include "processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global processor state definitions */
int32_t Register[256];
int32_t VectorRegister[32][8];
int PC = 0;
int opcode = 0, dest = 0, src1 = 0, src2 = 0;
int Z = 0, N = 0, C = 0, V = 0;
int end_of_simulation = 0;

/**
 * Reset processor state: Initializes all registers, PC, and flags to 0.
 */
void reset(void) {
    memset(Register, 0, sizeof(Register));
    memset(VectorRegister, 0, sizeof(VectorRegister));
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

/**
 * Fetch stage: Reads four consecutive bytes from Instruction memory at PC
 * into opcode, dest, src1, and src2. Advances PC by 4.
 */
void fetch(void) {
    if (PC + 3 >= INSTRUCTION_MEM_SIZE) {
        opcode = 0;
        dest = 0;
        src1 = 0;
        src2 = 0;
        return;
    }

    opcode = (unsigned char)Instruction[PC];
    dest   = (unsigned char)Instruction[PC + 1];
    src1   = (unsigned char)Instruction[PC + 2];
    src2   = (unsigned char)Instruction[PC + 3];

    /* Advance Program Counter */
    PC += 4;
}

/**
 * Decode stage: Void and empty function as specified in Lab 3.
 */
void decode(void) {
    /* Fixed 4-byte instruction format requires no separate decode table */
}

/**
 * Update condition flags Z, N, C, V based on addition.
 */
static void update_flags_add(int32_t a, int32_t b, int32_t res) {
    Z = (res == 0) ? 1 : 0;
    N = ((res >> 31) & 1) ? 1 : 0;

    uint32_t ua = (uint32_t)a;
    uint32_t ub = (uint32_t)b;
    uint32_t ures = (uint32_t)res;
    C = (ures < ua || ures < ub) ? 1 : 0;

    int sign_a = (a >> 31) & 1;
    int sign_b = (b >> 31) & 1;
    int sign_res = (res >> 31) & 1;
    V = (sign_a == sign_b && sign_res != sign_a) ? 1 : 0;
}

/**
 * Update condition flags Z, N, C, V based on subtraction.
 */
static void update_flags_sub(int32_t a, int32_t b, int32_t res) {
    Z = (res == 0) ? 1 : 0;
    N = ((res >> 31) & 1) ? 1 : 0;

    uint32_t ua = (uint32_t)a;
    uint32_t ub = (uint32_t)b;
    C = (ua >= ub) ? 1 : 0;

    int sign_a = (a >> 31) & 1;
    int sign_b = (b >> 31) & 1;
    int sign_res = (res >> 31) & 1;
    V = (sign_a != sign_b && sign_res == sign_b) ? 1 : 0;
}

/**
 * Check if branch condition is satisfied.
 */
static bool evaluate_branch_condition(int suffix_code) {
    switch (suffix_code) {
        case 0x0: return (Z == 1);                       /* EQ */
        case 0x1: return (Z == 0);                       /* NE */
        case 0x2: return (C == 1);                       /* CS */
        case 0x3: return (C == 0);                       /* CC */
        case 0x4: return (N == 1);                       /* MI */
        case 0x5: return (N == 0);                       /* PL */
        case 0x6: return (V == 1);                       /* VS */
        case 0x7: return (V == 0);                       /* VC */
        case 0x8: return (C == 1 && Z == 0);             /* HI */
        case 0x9: return (C == 0 || Z == 1);             /* LS */
        case 0xA: return (N == V);                       /* GE */
        case 0xB: return (N != V);                       /* LT */
        case 0xC: return (Z == 0 && N == V);             /* GT */
        case 0xD: return (Z == 1 || N != V);             /* LE */
        case 0xE: return true;                           /* AL (Always) */
        default:  return false;
    }
}

/**
 * Execute stage: executes the fetched instruction.
 */
void execute(void) {
    /* Opcode 0x00: Halt / End of Program */
    if (opcode == 0x00) {
        end_of_simulation = 1;
        return;
    }

    switch (opcode) {

        case 0x01: { /* Add: Register + Register */
            int32_t a = Register[src1];
            int32_t b = Register[src2];
            int32_t res = a + b;
            Register[dest] = res;
            update_flags_add(a, b, res);
            break;
        }
        case 0x09: { /* Add: Register + Constant */
            int32_t a = Register[src1];
            int32_t b = src2;
            int32_t res = a + b;
            Register[dest] = res;
            update_flags_add(a, b, res);
            break;
        }

        case 0x02: { /* Subtract: Register - Register */
            int32_t a = Register[src1];
            int32_t b = Register[src2];
            int32_t res = a - b;
            Register[dest] = res;
            update_flags_sub(a, b, res);
            break;
        }
        case 0x0A: { /* Subtract: Register - Constant */
            int32_t a = Register[src1];
            int32_t b = src2;
            int32_t res = a - b;
            Register[dest] = res;
            update_flags_sub(a, b, res);
            break;
        }

        case 0x03: { /* Multiply: Register * Register */
            Register[dest] = Register[src1] * Register[src2];
            break;
        }
        case 0x0B: { /* Multiply: Register * Constant */
            Register[dest] = Register[src1] * src2;
            break;
        }

        case 0x04: { /* Divide: Register / Register */
            if (Register[src2] != 0) {
                Register[dest] = Register[src1] / Register[src2];
            } else {
                printf("[Processor] Error: Division by zero\n");
            }
            break;
        }
        case 0x0C: { /* Divide: Register / Constant or Mem Read Const */
            if (src1 != 0) {
                if (src2 != 0) {
                    Register[dest] = Register[src1] / src2;
                }
            } else {
                Register[dest] = mem_read_32((uint32_t)src2);
            }
            break;
        }

        /* Integer Memory Read */
        case 0x05: { /* Read: dest = [Register[src2]] */
            Register[dest] = mem_read_32((uint32_t)Register[src2]);
            break;
        }
        case 0x0D: { /* Read: dest = [Constant src2] */
            Register[dest] = mem_read_32((uint32_t)src2);
            break;
        }

        /* Integer Memory Write */
        case 0x06: { /* Write: [Register[dest]] = Register[src2] */
            mem_write_32((uint32_t)Register[dest], Register[src2]);
            break;
        }
        case 0x0E: { /* Write: [Constant dest] = Register[src2] */
            mem_write_32((uint32_t)dest, Register[src2]);
            break;
        }

        /* Data Movement */
        case 0x07: { /* Move from register */
            Register[dest] = Register[src2];
            break;
        }
        case 0x0F: { /* Move from constant */
            Register[dest] = src2;
            break;
        }

        /* Print Instruction */
        case 0x08: { /* print Register[src2] */
            printf("Register x%d = 0x%X (%d)\n", src2, (uint32_t)Register[src2], Register[src2]);
            break;
        }

        case 0x21: { /* Vector Add: Vector + Vector */
            for (int i = 0; i < 8; i++) {
                VectorRegister[dest][i] = VectorRegister[src1][i] + VectorRegister[src2][i];
            }
            break;
        }
        case 0x29: { /* Vector Add: Vector + Scalar (Constant or Register) */
            int32_t val = (src2 < 32 && Register[src2] != 0) ? Register[src2] : src2;
            for (int i = 0; i < 8; i++) {
                VectorRegister[dest][i] = VectorRegister[src1][i] + val;
            }
            break;
        }

        case 0x22: { /* Vector Subtract: Vector - Vector */
            for (int i = 0; i < 8; i++) {
                VectorRegister[dest][i] = VectorRegister[src1][i] - VectorRegister[src2][i];
            }
            break;
        }
        case 0x2A: { /* Vector Subtract: Vector - Scalar */
            int32_t val = (src2 < 32 && Register[src2] != 0) ? Register[src2] : src2;
            for (int i = 0; i < 8; i++) {
                VectorRegister[dest][i] = VectorRegister[src1][i] - val;
            }
            break;
        }

        case 0x23: { /* Vector Multiply: Vector * Vector */
            for (int i = 0; i < 8; i++) {
                VectorRegister[dest][i] = VectorRegister[src1][i] * VectorRegister[src2][i];
            }
            break;
        }
        case 0x2B: { /* Vector Multiply: Vector * Scalar */
            int32_t val = (src2 < 32 && Register[src2] != 0) ? Register[src2] : src2;
            for (int i = 0; i < 8; i++) {
                VectorRegister[dest][i] = VectorRegister[src1][i] * val;
            }
            break;
        }

        case 0x25: { /* Vector Memory Read: v_dest = [Register[src2]] with post-increment */
            uint32_t base_addr = (uint32_t)Register[src2];
            for (int i = 0; i < 8; i++) {
                VectorRegister[dest][i] = mem_read_32(base_addr + (i * 4));
            }
            Register[src2] += 32;
            break;
        }
        case 0x2C: { /* Vector Memory Read: v_dest = [Constant src2] */
            uint32_t base_addr = (uint32_t)src2;
            for (int i = 0; i < 8; i++) {
                VectorRegister[dest][i] = mem_read_32(base_addr + (i * 4));
            }
            break;
        }

        case 0x26: { /* Vector Memory Write: [Register[dest]] = v_src2 with post-increment */
            uint32_t base_addr = (uint32_t)Register[dest];
            for (int i = 0; i < 8; i++) {
                mem_write_32(base_addr + (i * 4), VectorRegister[src2][i]);
            }
            Register[dest] += 32;
            break;
        }
        case 0x2E: { /* Vector Memory Write: [Constant dest] = v_src2 */
            uint32_t base_addr = (uint32_t)dest;
            for (int i = 0; i < 8; i++) {
                mem_write_32(base_addr + (i * 4), VectorRegister[src2][i]);
            }
            break;
        }

        default: {
            if (opcode >= 0x10 && opcode <= 0x1E) {
                int suffix_code = opcode - 0x10;
                if (evaluate_branch_condition(suffix_code)) {
                    signed char offset = (signed char)src2;
                    int branch_address = PC - 4;
                    PC = branch_address + (offset * 4);
                }
            } else {
                printf("[Processor] Unknown opcode 0x%02X at PC 0x%X\n", opcode, PC - 4);
            }
            break;
        }
    }
}
