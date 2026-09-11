#include "processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static void micro_sleep(int usec) {
    if (usec >= 1000) Sleep(usec / 1000);
    else Sleep(1);
}
#else
#include <unistd.h>
static void micro_sleep(int usec) {
    usleep(usec);
}
#endif

/* Global processor state definitions for NP processors */
int32_t Register[NP][256];
int32_t VectorRegister[NP][32][8];
int PC[NP];
int Z[NP], N[NP], C[NP], V[NP];
int end_of_simulation[NP];
FILE *fd_log = NULL;

/**
 * Reset processor state: Initializes all registers, PC, and flags to 0.
 */
void reset(int proc_id) {
    if (proc_id < 0 || proc_id >= NP) return;

    memset(Register[proc_id], 0, sizeof(Register[proc_id]));
    memset(VectorRegister[proc_id], 0, sizeof(VectorRegister[proc_id]));
    PC[proc_id] = 0;
    Z[proc_id] = 0;
    N[proc_id] = 0;
    C[proc_id] = 0;
    V[proc_id] = 0;
    end_of_simulation[proc_id] = 0;

    if (!fd_log) {
        fd_log = fopen("simulation.log", "a");
    }
}

/**
 * Cleanup log file handle on system exit.
 */
void processor_cleanup(void) {
    if (fd_log) {
        fclose(fd_log);
        fd_log = NULL;
    }
}

/**
 * Fetch stage: Reads four consecutive bytes from Instruction memory at PC
 * and increments PC by 4.
 */
void fetch(int proc_id, int *opcode, int *dest, int *src1, int *src2) {
    if (proc_id < 0 || proc_id >= NP) return;

    if (PC[proc_id] + 3 >= INSTRUCTION_MEM_SIZE) {
        *opcode = 0;
        *dest = 0;
        *src1 = 0;
        *src2 = 0;
        return;
    }

    *opcode = (unsigned char)Instruction[proc_id][PC[proc_id]];
    *dest   = (unsigned char)Instruction[proc_id][PC[proc_id] + 1];
    *src1   = (unsigned char)Instruction[proc_id][PC[proc_id] + 2];
    *src2   = (unsigned char)Instruction[proc_id][PC[proc_id] + 3];

    PC[proc_id] += 4;
}

/**
 * Decode stage: Passthrough function as per lab specification.
 */
void decode(void) {
    /* No complex decode logic needed */
}

/**
 * Update condition flags Z, N, C, V based on addition.
 */
static void update_flags_add(int proc_id, int32_t a, int32_t b, int32_t res) {
    Z[proc_id] = (res == 0) ? 1 : 0;
    N[proc_id] = ((res >> 31) & 1) ? 1 : 0;

    uint32_t ua = (uint32_t)a;
    uint32_t ub = (uint32_t)b;
    uint32_t ures = (uint32_t)res;
    C[proc_id] = (ures < ua || ures < ub) ? 1 : 0;

    int sign_a = (a >> 31) & 1;
    int sign_b = (b >> 31) & 1;
    int sign_res = (res >> 31) & 1;
    V[proc_id] = (sign_a == sign_b && sign_res != sign_a) ? 1 : 0;
}

/**
 * Update condition flags Z, N, C, V based on subtraction.
 */
static void update_flags_sub(int proc_id, int32_t a, int32_t b, int32_t res) {
    Z[proc_id] = (res == 0) ? 1 : 0;
    N[proc_id] = ((res >> 31) & 1) ? 1 : 0;

    uint32_t ua = (uint32_t)a;
    uint32_t ub = (uint32_t)b;
    C[proc_id] = (ua >= ub) ? 1 : 0;

    int sign_a = (a >> 31) & 1;
    int sign_b = (b >> 31) & 1;
    int sign_res = (res >> 31) & 1;
    V[proc_id] = (sign_a != sign_b && sign_res == sign_b) ? 1 : 0;
}

/**
 * Check if branch condition is satisfied.
 */
static bool evaluate_branch_condition(int proc_id, int suffix_code) {
    int z = Z[proc_id];
    int n = N[proc_id];
    int c = C[proc_id];
    int v = V[proc_id];

    switch (suffix_code) {
        case 0x0: return (z == 1);                       /* EQ */
        case 0x1: return (z == 0);                       /* NE */
        case 0x2: return (c == 1);                       /* CS */
        case 0x3: return (c == 0);                       /* CC */
        case 0x4: return (n == 1);                       /* MI */
        case 0x5: return (n == 0);                       /* PL */
        case 0x6: return (v == 1);                       /* VS */
        case 0x7: return (v == 0);                       /* VC */
        case 0x8: return (c == 1 && z == 0);             /* HI */
        case 0x9: return (c == 0 || z == 1);             /* LS */
        case 0xA: return (n == v);                       /* GE */
        case 0xB: return (n != v);                       /* LT */
        case 0xC: return (z == 0 && n == v);             /* GT */
        case 0xD: return (z == 1 || n != v);             /* LE */
        case 0xE: return true;                           /* AL (Always) */
        default:  return false;
    }
}

/**
 * Execute stage: executes instruction on designated processor.
 */
void execute(int proc_id, int opcode, int dest, int src1, int src2) {
    if (proc_id < 0 || proc_id >= NP) return;

    /* Opcode 0x00: Halt / End of Program */
    if (opcode == 0x00) {
        end_of_simulation[proc_id] = 1;
        return;
    }

    switch (opcode) {
        case 0x01: { /* Add: Register + Register */
            int32_t a = Register[proc_id][src1];
            int32_t b = Register[proc_id][src2];
            int32_t res = a + b;
            Register[proc_id][dest] = res;
            update_flags_add(proc_id, a, b, res);
            break;
        }
        case 0x09: { /* Add: Register + Constant */
            int32_t a = Register[proc_id][src1];
            int32_t b = src2;
            int32_t res = a + b;
            Register[proc_id][dest] = res;
            update_flags_add(proc_id, a, b, res);
            break;
        }

        case 0x02: { /* Subtract: Register - Register */
            int32_t a = Register[proc_id][src1];
            int32_t b = Register[proc_id][src2];
            int32_t res = a - b;
            Register[proc_id][dest] = res;
            update_flags_sub(proc_id, a, b, res);
            break;
        }
        case 0x0A: { /* Subtract: Register - Constant */
            int32_t a = Register[proc_id][src1];
            int32_t b = src2;
            int32_t res = a - b;
            Register[proc_id][dest] = res;
            update_flags_sub(proc_id, a, b, res);
            break;
        }

        case 0x03: { /* Multiply: Register * Register */
            Register[proc_id][dest] = Register[proc_id][src1] * Register[proc_id][src2];
            break;
        }
        case 0x0B: { /* Multiply: Register * Constant */
            Register[proc_id][dest] = Register[proc_id][src1] * src2;
            break;
        }

        case 0x04: { /* Divide: Register / Register */
            if (Register[proc_id][src2] != 0) {
                Register[proc_id][dest] = Register[proc_id][src1] / Register[proc_id][src2];
            } else {
                printf("[Processor %d] Error: Division by zero\n", proc_id);
            }
            break;
        }
        case 0x0C: { /* Divide: Register / Constant or Mem Read Const */
            if (src1 != 0) {
                if (src2 != 0) {
                    Register[proc_id][dest] = Register[proc_id][src1] / src2;
                }
            } else {
                Register[proc_id][dest] = mem_read_32(proc_id, (uint32_t)src2);
            }
            break;
        }

        /* Integer Memory Read */
        case 0x05: { /* Read: dest = [Register[src2]] */
            uint32_t addr = (uint32_t)Register[proc_id][src2];
            Register[proc_id][dest] = mem_read_32(proc_id, addr);
            break;
        }
        case 0x0D: { /* Read: dest = [Constant address src2] */
            Register[proc_id][dest] = mem_read_32(proc_id, (uint32_t)src2);
            break;
        }

        /* Integer Memory Write */
        case 0x06: { /* Write: [Register[dest]] = Register[src2] */
            uint32_t addr = (uint32_t)Register[proc_id][dest];
            mem_write_32(proc_id, addr, Register[proc_id][src2]);
            break;
        }
        case 0x0E: { /* Write: [Constant address dest] = Register[src2] */
            mem_write_32(proc_id, (uint32_t)dest, Register[proc_id][src2]);
            break;
        }

        /* Data Movement */
        case 0x07: { /* Move from register */
            Register[proc_id][dest] = Register[proc_id][src2];
            break;
        }
        case 0x0F: { /* Move from constant */
            Register[proc_id][dest] = src2;
            break;
        }
        case 0x08: { /* print Register[src2] */
            int reg_num = src2;
            int32_t val = Register[proc_id][reg_num];
            printf("Process id %d: x%d : 0x%X\n", proc_id, reg_num, (uint32_t)val);
            if (fd_log) {
                fprintf(fd_log, "Process id %d: x%d : 0x%X\n", proc_id, reg_num, (uint32_t)val);
                fflush(fd_log);
            }
            break;
        }

        case 0x21: { /* Vector Add: Vector + Vector */
            for (int i = 0; i < 8; i++) {
                VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] +
                                                    VectorRegister[proc_id][src2][i];
            }
            break;
        }
        case 0x29: { /* Vector Add: Vector + Scalar */
            int32_t val = (src2 < 32 && Register[proc_id][src2] != 0) ?
                           Register[proc_id][src2] : src2;
            for (int i = 0; i < 8; i++) {
                VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] + val;
            }
            break;
        }

        case 0x22: { /* Vector Subtract: Vector - Vector */
            for (int i = 0; i < 8; i++) {
                VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] -
                                                    VectorRegister[proc_id][src2][i];
            }
            break;
        }
        case 0x2A: { /* Vector Subtract: Vector - Scalar */
            int32_t val = (src2 < 32 && Register[proc_id][src2] != 0) ?
                           Register[proc_id][src2] : src2;
            for (int i = 0; i < 8; i++) {
                VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] - val;
            }
            break;
        }

        case 0x23: { /* Vector Multiply: Vector * Vector */
            for (int i = 0; i < 8; i++) {
                VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] *
                                                    VectorRegister[proc_id][src2][i];
            }
            break;
        }
        case 0x2B: { /* Vector Multiply: Vector * Scalar */
            int32_t val = (src2 < 32 && Register[proc_id][src2] != 0) ?
                           Register[proc_id][src2] : src2;
            for (int i = 0; i < 8; i++) {
                VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] * val;
            }
            break;
        }

        case 0x25: { /* Vector Memory Read: v_dest = [Register[src2]] with post-increment */
            uint32_t base_addr = (uint32_t)Register[proc_id][src2];
            for (int i = 0; i < 8; i++) {
                VectorRegister[proc_id][dest][i] = mem_read_32(proc_id, base_addr + (i * 4));
            }
            Register[proc_id][src2] += 32;
            break;
        }
        case 0x2C: { /* Vector Memory Read: v_dest = [Constant src2] */
            uint32_t base_addr = (uint32_t)src2;
            for (int i = 0; i < 8; i++) {
                VectorRegister[proc_id][dest][i] = mem_read_32(proc_id, base_addr + (i * 4));
            }
            break;
        }

        case 0x26: { /* Vector Memory Write: [Register[dest]] = v_src2 with post-increment */
            uint32_t base_addr = (uint32_t)Register[proc_id][dest];
            for (int i = 0; i < 8; i++) {
                mem_write_32(proc_id, base_addr + (i * 4), VectorRegister[proc_id][src2][i]);
            }
            Register[proc_id][dest] += 32;
            break;
        }
        case 0x2E: { /* Vector Memory Write: [Constant dest] = v_src2 */
            uint32_t base_addr = (uint32_t)dest;
            for (int i = 0; i < 8; i++) {
                mem_write_32(proc_id, base_addr + (i * 4), VectorRegister[proc_id][src2][i]);
            }
            break;
        }

        default: {
            if (opcode >= 0x10 && opcode <= 0x1E) {
                int suffix_code = opcode - 0x10;
                if (evaluate_branch_condition(proc_id, suffix_code)) {
                    signed char offset = (signed char)src2;
                    int branch_address = PC[proc_id] - 4;
                    PC[proc_id] = branch_address + (offset * 4);
                }
            } else {
                printf("[Processor %d] Unknown opcode 0x%02X at PC 0x%X\n",
                       proc_id, opcode, PC[proc_id] - 4);
            }
            break;
        }
    }
}

/**
 * Process instructions for a given processor during its scheduled time slice.
 */
void process_instructions(int proc_id, int instruction_count) {
    if (proc_id < 0 || proc_id >= NP) return;

    for (int i = 0; i < instruction_count; i++) {
        if (end_of_simulation[proc_id]) {
            break;
        }

        int opcode = 0, dest = 0, src1 = 0, src2 = 0;
        fetch(proc_id, &opcode, &dest, &src1, &src2);
        decode();
        execute(proc_id, opcode, dest, src1, src2);
    }

    micro_sleep(10);
}
