#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global memory arrays as specified in Lab 3 */
char Instruction[INSTRUCTION_MEM_SIZE];
char Data[DATA_MEM_SIZE];

/**
 * Clear/Reset memory arrays to 0.
 */
void memory_clear(void) {
    memset(Instruction, 0, sizeof(Instruction));
    memset(Data, 0, sizeof(Data));
}

/**
 * Helper function to load hex bytes separated by spaces into destination buffer.
 */
static bool load_hex_bytes_to_buffer(const char *filename, char *buffer, size_t max_size) {
    if (!filename || strlen(filename) == 0) {
        return false;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("[Memory] Warning: Could not open file '%s'\n", filename);
        return false;
    }

    size_t byte_count = 0;
    unsigned int hex_val = 0;

    while (fscanf(fp, "%x", &hex_val) == 1) {
        if (byte_count < max_size) {
            buffer[byte_count] = (char)(hex_val & 0xFF);
            byte_count++;
        } else {
            printf("[Memory] Warning: File '%s' exceeded memory size limit (%zu bytes)\n",
                   filename, max_size);
            break;
        }
    }

    fclose(fp);
    return true;
}

/**
 * Initialize instruction and data memory.
 */
bool memory_initialize(const char *program_byte_file, const char *data_byte_file) {
    memory_clear();

    if (program_byte_file && strlen(program_byte_file) > 0) {
        if (!load_hex_bytes_to_buffer(program_byte_file, Instruction, INSTRUCTION_MEM_SIZE)) {
            printf("[Memory] Error: Failed to load instruction memory from '%s'\n", program_byte_file);
            return false;
        }
    }

    if (data_byte_file && strlen(data_byte_file) > 0) {
        load_hex_bytes_to_buffer(data_byte_file, Data, DATA_MEM_SIZE);
    }

    return true;
}

/**
 * Save data memory to text file in 4-byte hex format.
 */
bool memory_finalize(const char *output_data_file) {
    if (!output_data_file) return false;

    FILE *fp = fopen(output_data_file, "w");
    if (!fp) {
        printf("[Memory] Error: Could not open output file '%s' for writing\n", output_data_file);
        return false;
    }

    size_t last_nonzero = 0;
    for (size_t i = 0; i < DATA_MEM_SIZE; i++) {
        if (Data[i] != 0) {
            last_nonzero = i;
        }
    }

    size_t dump_size = ((last_nonzero / 4) + 1) * 4;
    if (dump_size < 16) dump_size = 16;
    if (dump_size > DATA_MEM_SIZE) dump_size = DATA_MEM_SIZE;

    for (size_t i = 0; i < dump_size; i += 4) {
        fprintf(fp, "%02X %02X %02X %02X\n",
                (unsigned char)Data[i],
                (unsigned char)Data[i + 1],
                (unsigned char)Data[i + 2],
                (unsigned char)Data[i + 3]);
    }

    fclose(fp);
    return true;
}

/**
 * Read a 32-bit integer from byte-addressable memory (Little-Endian).
 */
int32_t mem_read_32(uint32_t addr) {
    if (addr + 3 >= DATA_MEM_SIZE) {
        printf("[Memory] Warning: Read out of bounds at address 0x%X\n", addr);
        return 0;
    }

    uint32_t b0 = (uint8_t)Data[addr];
    uint32_t b1 = (uint8_t)Data[addr + 1];
    uint32_t b2 = (uint8_t)Data[addr + 2];
    uint32_t b3 = (uint8_t)Data[addr + 3];

    return (int32_t)(b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

/**
 * Write a 32-bit integer to byte-addressable memory (Little-Endian).
 */
void mem_write_32(uint32_t addr, int32_t value) {
    if (addr + 3 >= DATA_MEM_SIZE) {
        printf("[Memory] Warning: Write out of bounds at address 0x%X\n", addr);
        return;
    }

    Data[addr]     = (char)(value & 0xFF);
    Data[addr + 1] = (char)((value >> 8) & 0xFF);
    Data[addr + 2] = (char)((value >> 16) & 0xFF);
    Data[addr + 3] = (char)((value >> 24) & 0xFF);
}
