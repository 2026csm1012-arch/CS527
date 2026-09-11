#include "memory.h"
#include "os.h"
#include <stdio.h>
#include <string.h>

char memory[MEMSIZE];

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Clears the unified physical RAM; startup state flows to a deterministic zeroed memory image.
 */
void memory_init(void)
{
    memset(memory, 0, sizeof(memory));
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Reads one raw physical byte after bounds checking; physical address flows directly to RAM data.
 */
uint8_t mem_read_byte_phys(int phys_addr)
{
    if (phys_addr < 0 || phys_addr >= MEMSIZE)
        return 0;
    return (uint8_t)memory[phys_addr];
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Writes one raw byte after bounds checking; value flows directly into a physical RAM location.
 */
void mem_write_byte_phys(int phys_addr, uint8_t val)
{
    if (phys_addr < 0 || phys_addr >= MEMSIZE)
        return;
    memory[phys_addr] = (char)val;
}
/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Reads four logical data bytes through the MMU and rebuilds a little-endian int32; logical address flows through page translation to physical RAM and back to a CPU value.
 */

int32_t mem_read_32(int proc_id, uint32_t logical_addr)
{
    uint32_t b[4];
    for (int i = 0; i < 4; i++)
    {
        int pa = getPhysicallAddress(proc_id, 0, logical_addr + i);
        if (pa < 0 || pa >= MEMSIZE)
            return 0;
        b[i] = (uint8_t)memory[pa];
    }
    return (int32_t)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Splits an int32 into four little-endian bytes and writes them through the MMU; CPU value flows to process data memory.
 */
void mem_write_32(int proc_id, uint32_t logical_addr, int32_t value)
{
    for (int i = 0; i < 4; i++)
    {
        int pa = getPhysicallAddress(proc_id, 0, logical_addr + i);
        if (pa >= 0 && pa < MEMSIZE)
        {
            memory[pa] = (char)((value >> (i * 8)) & 0xFF);
        }
    }
}