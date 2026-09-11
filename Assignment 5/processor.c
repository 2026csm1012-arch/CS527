#include "processor.h"
#include "os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Provides a tiny platform-specific delay; data flows from requested delay to the OS sleep API, making simulator time-slicing observable.
 */
static void micro_sleep(int usec)
{
    if (usec >= 1000)
        Sleep(usec / 1000);
    else
        Sleep(1);
}
#else
#include <unistd.h>

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Provides a tiny platform-specific delay; data flows from requested delay to the OS sleep API, making simulator time-slicing observable.
 */
static void micro_sleep(int usec)
{
    usleep(usec);
}
#endif

int32_t Register[NP][256];
int32_t VectorRegister[NP][32][8];
int PC[NP];
int Z[NP], N[NP], C[NP], V[NP];
int end_of_simulation[NP];
FILE *fd_log = NULL;

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Clears one core's scalar/vector registers, PC, flags and termination state; data flows from process assignment to a clean CPU context.
 */
void reset(int proc_id)
{
    if (proc_id < 0 || proc_id >= NP)
        return;

    memset(Register[proc_id], 0, sizeof(Register[proc_id]));
    memset(VectorRegister[proc_id], 0, sizeof(VectorRegister[proc_id]));
    PC[proc_id] = 0;
    Z[proc_id] = 0;
    N[proc_id] = 0;
    C[proc_id] = 0;
    V[proc_id] = 0;
    end_of_simulation[proc_id] = 0;

    if (!fd_log)
    {
        fd_log = fopen("simulation.log", "a");
    }
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Closes the execution log; data flows from simulator shutdown to released file resources, ensuring buffered logs are flushed.
 */
void processor_cleanup(void)
{
    if (fd_log)
    {
        fclose(fd_log);
        fd_log = NULL;
    }
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Reads the next 4-byte instruction using the MMU; data flows PC -> logical code addresses -> page-table translation -> physical bytes -> opcode/dest/src fields, then PC advances.
 */
void fetch(int proc_id, int *opcode, int *dest, int *src1, int *src2)
{
    if (proc_id < 0 || proc_id >= NP)
        return;

    // The Program Counter (PC) holds the logical address of the next instruction.
    // An instruction consists of 4 bytes: Opcode, Destination, Source 1, and Source 2.
    // We map these 4 consecutive logical addresses to physical memory addresses.
    int p0 = getPhysicallAddress(proc_id, 1, PC[proc_id]);
    int p1 = getPhysicallAddress(proc_id, 1, PC[proc_id] + 1);
    int p2 = getPhysicallAddress(proc_id, 1, PC[proc_id] + 2);
    int p3 = getPhysicallAddress(proc_id, 1, PC[proc_id] + 3);

    // If any byte falls into an invalid memory page, return an empty (Halt) instruction.
    if (p0 < 0 || p1 < 0 || p2 < 0 || p3 < 0)
    {
        *opcode = 0;
        *dest = 0;
        *src1 = 0;
        *src2 = 0;
        return;
    }

    // Read the 4 bytes from physical memory.
    *opcode = mem_read_byte_phys(p0);
    *dest = mem_read_byte_phys(p1);
    *src1 = mem_read_byte_phys(p2);
    *src2 = mem_read_byte_phys(p3);

    // Move the Program Counter to the next instruction (4 bytes forward).
    PC[proc_id] += 4;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Represents the decode pipeline stage; fetched instruction fields are already separated by fetch, so this function currently has no extra transformation.
 */
void decode(void) {}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Calculates Z/N/C/V after addition; arithmetic result flows into condition flags that later branch instructions consume.
 */
static void update_flags_add(int proc_id, int32_t a, int32_t b, int32_t res)
{
    // Z is 1 if the result is exactly 0
    Z[proc_id] = (res == 0) ? 1 : 0;

    // N is 1 if the result is negative
    N[proc_id] = (res < 0) ? 1 : 0;

    // C is 1 for addition if the unsigned result wrapped around (is less than a)
    uint32_t ua = (uint32_t)a, ures = (uint32_t)res;
    C[proc_id] = (ures < ua) ? 1 : 0;

    // V is 1 if both operands have the same sign, but the result has a different sign
    int sign_a = (a < 0) ? 1 : 0;
    int sign_b = (b < 0) ? 1 : 0;
    int sign_res = (res < 0) ? 1 : 0;
    V[proc_id] = (sign_a == sign_b && sign_res != sign_a) ? 1 : 0;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Calculates Z/N/C/V after subtraction; the resulting flags flow to conditional branches.
 */
static void update_flags_sub(int proc_id, int32_t a, int32_t b, int32_t res)
{
    // Z is 1 if the result is exactly 0
    Z[proc_id] = (res == 0) ? 1 : 0;

    // N is 1 if the result is negative
    N[proc_id] = (res < 0) ? 1 : 0;

    // C is 1 for subtraction if the first unsigned operand is greater than or equal to the second
    uint32_t ua = (uint32_t)a, ub = (uint32_t)b;
    C[proc_id] = (ua >= ub) ? 1 : 0;

    // V is 1 if operands have different signs, and the result's sign matches the second operand's original sign
    int sign_a = (a < 0) ? 1 : 0;
    int sign_b = (b < 0) ? 1 : 0;
    int sign_res = (res < 0) ? 1 : 0;
    V[proc_id] = (sign_a != sign_b && sign_res == sign_b) ? 1 : 0;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Evaluates an encoded branch condition against Z/N/C/V; flags flow to a Boolean branch decision.
 */
static bool evaluate_branch_condition(int proc_id, int suffix_code)
{
    int z = Z[proc_id], n = N[proc_id], c = C[proc_id], v = V[proc_id];
    switch (suffix_code)
    {
    case 0x0:
        return (z == 1);
    case 0x1:
        return (z == 0);
    case 0x2:
        return (c == 1);
    case 0x3:
        return (c == 0);
    case 0x4:
        return (n == 1);
    case 0x5:
        return (n == 0);
    case 0x6:
        return (v == 1);
    case 0x7:
        return (v == 0);
    case 0x8:
        return (c == 1 && z == 0);
    case 0x9:
        return (c == 0 || z == 1);
    case 0xA:
        return (n == v);
    case 0xB:
        return (n != v);
    case 0xC:
        return (z == 0 && n == v);
    case 0xD:
        return (z == 1 || n != v);
    case 0xE:
        return true;
    default:
        return false;
    }
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Dispatches one opcode to arithmetic, memory, vector, print or control-flow logic; instruction operands flow into registers/memory/PC and change simulated machine state.
 */
void execute(int proc_id, int opcode, int dest, int src1, int src2)
{
    if (proc_id < 0 || proc_id >= NP)
        return;
    if (opcode == 0x00)
    {
        end_of_simulation[proc_id] = 1;
        return;
    }

    switch (opcode)
    {
    // ==========================================
    // Scalar Arithmetic Operations
    // ==========================================
    case 0x01:
    { // Integer Add Register (x1 = x2 + x3)
        int32_t a = Register[proc_id][src1], b = Register[proc_id][src2];
        int32_t res = a + b;
        Register[proc_id][dest] = res;
        update_flags_add(proc_id, a, b, res);
        break;
    }
    case 0x09:
    { // Integer Add Constant (x1 = x2 + 10)
        int32_t a = Register[proc_id][src1], b = src2;
        int32_t res = a + b;
        Register[proc_id][dest] = res;
        update_flags_add(proc_id, a, b, res);
        break;
    }
    case 0x02:
    { // Integer Subtract Register (x1 = x2 - x3)
        int32_t a = Register[proc_id][src1], b = Register[proc_id][src2];
        int32_t res = a - b;
        Register[proc_id][dest] = res;
        update_flags_sub(proc_id, a, b, res);
        break;
    }
    case 0x0A:
    { // Integer Subtract Constant (x1 = x2 - 10)
        int32_t a = Register[proc_id][src1], b = src2;
        int32_t res = a - b;
        Register[proc_id][dest] = res;
        update_flags_sub(proc_id, a, b, res);
        break;
    }
    case 0x03: // Integer Multiply Register
        Register[proc_id][dest] = Register[proc_id][src1] * Register[proc_id][src2];
        break;
    case 0x0B: // Integer Multiply Constant
        Register[proc_id][dest] = Register[proc_id][src1] * src2;
        break;
    case 0x04: // Integer Divide Register
        if (Register[proc_id][src2] != 0)
            Register[proc_id][dest] = Register[proc_id][src1] / Register[proc_id][src2];
        break;
    case 0x0C: // Integer Divide Constant / Read Constant
        if (src1 != 0 && src2 != 0)
            Register[proc_id][dest] = Register[proc_id][src1] / src2;
        else
            Register[proc_id][dest] = mem_read_32(proc_id, (uint32_t)src2); // Overloaded: also reads from constant address
        break;

    // ==========================================
    // Scalar Memory & Data Operations
    // ==========================================
    case 0x05: // Read from Register Address (x1 = [x2])
        Register[proc_id][dest] = mem_read_32(proc_id, (uint32_t)Register[proc_id][src2]);
        break;
    case 0x0D: // Read from Constant Address (Alternative)
        Register[proc_id][dest] = mem_read_32(proc_id, (uint32_t)src2);
        break;
    case 0x06: // Write to Register Address ([x2] = x1)
        mem_write_32(proc_id, (uint32_t)Register[proc_id][dest], Register[proc_id][src2]);
        break;
    case 0x0E: // Write to Constant Address ([10] = x1)
        mem_write_32(proc_id, (uint32_t)dest, Register[proc_id][src2]);
        break;
    case 0x07: // Register Copy (x1 = x2)
        Register[proc_id][dest] = Register[proc_id][src2];
        break;
    case 0x0F: // Load Constant (x1 = 10)
        Register[proc_id][dest] = src2;
        break;
    case 0x08:
    { // Debug Print Operation (print x1)
        int32_t val = Register[proc_id][src2];
        printf("Process id: %d x%d : 0x%X\n", proc_id, src2, (uint32_t)val);
        if (fd_log)
        {
            fprintf(fd_log, "Process id: %d x%d : 0x%X\n", proc_id, src2, (uint32_t)val);
            fflush(fd_log);
        }
        break;
    }

        // ==========================================
        // Vector Arithmetic Operations (8 parallel ints)
        // ==========================================

    case 0x21: // Vector Add Vector (v1 = v2 + v3)
        for (int i = 0; i < 8; i++)
            VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] + VectorRegister[proc_id][src2][i];
        break;
    case 0x29:
    { // Vector Add Constant/Scalar (v1 = v2 + 10 / x1)
        int32_t val = (src2 < 32 && Register[proc_id][src2] != 0) ? Register[proc_id][src2] : src2;
        for (int i = 0; i < 8; i++)
            VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] + val;
        break;
    }
    case 0x22: // Vector Subtract Vector (v1 = v2 - v3)
        for (int i = 0; i < 8; i++)
            VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] - VectorRegister[proc_id][src2][i];
        break;
    case 0x2A:
    { // Vector Subtract Constant/Scalar (v1 = v2 - 10 / x1)
        int32_t val = (src2 < 32 && Register[proc_id][src2] != 0) ? Register[proc_id][src2] : src2;
        for (int i = 0; i < 8; i++)
            VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] - val;
        break;
    }
    case 0x23: // Vector Multiply Vector (v1 = v2 * v3)
        for (int i = 0; i < 8; i++)
            VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] * VectorRegister[proc_id][src2][i];
        break;
    case 0x2B:
    { // Vector Multiply Constant/Scalar (v1 = v2 * 10 / x1)
        int32_t val = (src2 < 32 && Register[proc_id][src2] != 0) ? Register[proc_id][src2] : src2;
        for (int i = 0; i < 8; i++)
            VectorRegister[proc_id][dest][i] = VectorRegister[proc_id][src1][i] * val;
        break;
    }

    // ==========================================
    // Vector Memory Operations
    // ==========================================
    case 0x25:
    { // Vector Read from Register Address (v1 = [x2])
        uint32_t base = (uint32_t)Register[proc_id][src2];
        for (int i = 0; i < 8; i++)
            VectorRegister[proc_id][dest][i] = mem_read_32(proc_id, base + (i * 4));
        // Auto-increment the address register by 32 bytes (8 integers * 4 bytes)
        Register[proc_id][src2] += 32;
        break;
    }
    case 0x2C:
    { // Vector Read from Constant Address (v1 = [10])
        uint32_t base = (uint32_t)src2;
        for (int i = 0; i < 8; i++)
            VectorRegister[proc_id][dest][i] = mem_read_32(proc_id, base + (i * 4));
        break;
    }
    case 0x26:
    { // Vector Write to Register Address ([x2] = v1)
        uint32_t base = (uint32_t)Register[proc_id][dest];
        for (int i = 0; i < 8; i++)
            mem_write_32(proc_id, base + (i * 4), VectorRegister[proc_id][src2][i]);
        // Auto-increment the address register by 32 bytes
        Register[proc_id][dest] += 32;
        break;
    }
    case 0x2E:
    { // Vector Write to Constant Address ([10] = v1)
        uint32_t base = (uint32_t)dest;
        for (int i = 0; i < 8; i++)
            mem_write_32(proc_id, base + (i * 4), VectorRegister[proc_id][src2][i]);
        break;
    }
    default:
        // ==========================================
        // Control Flow (Branching) Operations
        // ==========================================
        if (opcode >= 0x10 && opcode <= 0x1E)
        {
            int suffix_code = opcode - 0x10;
            if (evaluate_branch_condition(proc_id, suffix_code))
            {
                // src2 holds the branch offset. Since the offset is given as an 8-bit value,
                // we cast it to signed char to correctly handle negative (backward) branches.
                signed char offset = (signed char)src2;
                int branch_address = PC[proc_id] - 4; // PC was already incremented by fetch()
                PC[proc_id] = branch_address + (offset * 4);
            }
        }
        break;
    }
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Runs a core for a bounded instruction quantum using fetch -> decode -> execute; updated CPU state is returned to the scheduler after the time slice.
 */
void process_instructions(int proc_id, int instruction_count)
{
    if (proc_id < 0 || proc_id >= NP)
        return;
    for (int i = 0; i < instruction_count; i++)
    {
        if (end_of_simulation[proc_id])
            break;
        int opcode = 0, dest = 0, src1 = 0, src2 = 0;
        fetch(proc_id, &opcode, &dest, &src1, &src2);
        decode();
        execute(proc_id, opcode, dest, src1, src2);
    }
    micro_sleep(10);
}