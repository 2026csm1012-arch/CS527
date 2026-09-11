#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global 2D memory arrays for NP processors */
char Instruction[NP][INSTRUCTION_MEM_SIZE];
char Data[NP][DATA_MEM_SIZE];


void memory_clear(int proc_id) {
    if (proc_id < 0 || proc_id >= NP) return;
    memset(Instruction[proc_id], 0, sizeof(Instruction[proc_id]));
    memset(Data[proc_id], 0, sizeof(Data[proc_id]));
}

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
            printf("[Memory] Warning: File '%s' exceeded memory limit (%zu bytes)\n",
                   filename, max_size);
            break;
        }
    }

    fclose(fp);
    return true;
}

bool memory_initialize(int proc_id, const char *program_byte_file, const char *data_byte_file) {
    if (proc_id < 0 || proc_id >= NP) {
        printf("[Memory] Error: Invalid processor ID %d\n", proc_id);
        return false;
    }

    memory_clear(proc_id);

    if (program_byte_file && strlen(program_byte_file) > 0) {
        if (!load_hex_bytes_to_buffer(program_byte_file, Instruction[proc_id], INSTRUCTION_MEM_SIZE)) {
            printf("[Memory] Error: Failed to load instruction memory from '%s'\n", program_byte_file);
            return false;
        }
    }

    if (data_byte_file && strlen(data_byte_file) > 0) {
        load_hex_bytes_to_buffer(data_byte_file, Data[proc_id], DATA_MEM_SIZE);
    }

    return true;
}

bool memory_finalize(int proc_id, const char *output_data_file) {
    if (proc_id < 0 || proc_id >= NP || !output_data_file) {
        return false;
    }

    FILE *fp = fopen(output_data_file, "w");
    if (!fp) {
        printf("[Memory] Error: Could not open output file '%s' for writing\n", output_data_file);
        return false;
    }

    size_t last_nonzero = 0;
    for (size_t i = 0; i < DATA_MEM_SIZE; i++) {
        if (Data[proc_id][i] != 0) {
            last_nonzero = i;
        }
    }

    size_t dump_size = ((last_nonzero / 4) + 1) * 4;
    if (dump_size < 16) dump_size = 16;
    if (dump_size > DATA_MEM_SIZE) dump_size = DATA_MEM_SIZE;

    for (size_t i = 0; i < dump_size; i += 4) {
        fprintf(fp, "%02X %02X %02X %02X\n",
                (unsigned char)Data[proc_id][i],
                (unsigned char)Data[proc_id][i + 1],
                (unsigned char)Data[proc_id][i + 2],
                (unsigned char)Data[proc_id][i + 3]);
    }

    fclose(fp);
    return true;
}

int32_t mem_read_32(int proc_id, uint32_t addr) {
    if (proc_id < 0 || proc_id >= NP) return 0;
    if (addr + 3 >= DATA_MEM_SIZE) {
        printf("[Memory] Warning: Read out of bounds at address 0x%X (proc %d)\n", addr, proc_id);
        return 0;
    }

    uint32_t b0 = (uint8_t)Data[proc_id][addr];
    uint32_t b1 = (uint8_t)Data[proc_id][addr + 1];
    uint32_t b2 = (uint8_t)Data[proc_id][addr + 2];
    uint32_t b3 = (uint8_t)Data[proc_id][addr + 3];

    return (int32_t)(b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

void mem_write_32(int proc_id, uint32_t addr, int32_t value) {
    if (proc_id < 0 || proc_id >= NP) return;
    if (addr + 3 >= DATA_MEM_SIZE) {
        printf("[Memory] Warning: Write out of bounds at address 0x%X (proc %d)\n", addr, proc_id);
        return;
    }

    Data[proc_id][addr]     = (char)(value & 0xFF);
    Data[proc_id][addr + 1] = (char)((value >> 8) & 0xFF);
    Data[proc_id][addr + 2] = (char)((value >> 16) & 0xFF);
    Data[proc_id][addr + 3] = (char)((value >> 24) & 0xFF);
}
