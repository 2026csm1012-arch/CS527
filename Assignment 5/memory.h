#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#ifndef NP
#define NP 4
#endif

#define MEMSIZE 8192
#define PAGESIZE 512
#define NUM_PHYSICAL_PAGES (MEMSIZE / PAGESIZE)               /* 16 frames */
#define NUM_LOGICAL_PAGES  (1024 / PAGESIZE + 4096 / PAGESIZE) /* 2 code + 8 data = 10 pages */

#define INSTRUCTION_MEM_SIZE 1024
#define DATA_MEM_SIZE        4096

/* Unified Physical Memory */
extern char memory[MEMSIZE];

void memory_init(void);
int32_t mem_read_32(int proc_id, uint32_t logical_addr);
void mem_write_32(int proc_id, uint32_t logical_addr, int32_t value);
uint8_t mem_read_byte_phys(int phys_addr);
void mem_write_byte_phys(int phys_addr, uint8_t val);

#endif