#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

/* Memory sizes as per CS527 Lab 3 specification:
 * - Instruction memory: 256 bytes (up to 64 4-byte instructions)
 * - Data memory: 4096 bytes
 */
#define INSTRUCTION_MEM_SIZE 256
#define DATA_MEM_SIZE        4096

/* Global memory arrays */
extern char Instruction[INSTRUCTION_MEM_SIZE];
extern char Data[DATA_MEM_SIZE];

bool memory_initialize(const char *program_byte_file, const char *data_byte_file);

bool memory_finalize(const char *output_data_file);

int32_t mem_read_32(uint32_t addr);

void mem_write_32(uint32_t addr, int32_t value);

void memory_clear(void);

#endif /* MEMORY_H */
