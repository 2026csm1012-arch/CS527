#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

/* Number of hardware processors supported in Lab 4 */
#ifndef NP
#define NP 4
#endif

/* Memory sizes as per CS527 specification:
 * - Instruction memory: 256 bytes per processor (up to 64 4-byte instructions)
 * - Data memory: 4096 bytes per processor
 */
#define INSTRUCTION_MEM_SIZE 256
#define DATA_MEM_SIZE        4096

/* Global 2D memory arrays for NP processors */
extern char Instruction[NP][INSTRUCTION_MEM_SIZE];
extern char Data[NP][DATA_MEM_SIZE];

bool memory_initialize(int proc_id, const char *program_byte_file, const char *data_byte_file);
bool memory_finalize(int proc_id, const char *output_data_file);
int32_t mem_read_32(int proc_id, uint32_t addr);
void mem_write_32(int proc_id, uint32_t addr, int32_t value);
void memory_clear(int proc_id);

#endif 
