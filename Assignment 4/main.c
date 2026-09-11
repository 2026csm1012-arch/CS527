#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "memory.h"
#include "processor.h"
#include "os.h"

static int run_standalone(const char *prog_file, const char *data_file) {
   
    printf("  CS527 Mini-Computer Simulator\n");

    char byte_file[128];
    const char *dot = strrchr(prog_file, '.');

    if (dot && (strcmp(dot, ".txt") == 0 || strcmp(dot, ".asm") == 0)) {
        strcpy(byte_file, "program.byte");
        if (!compile(prog_file, byte_file)) {
            printf("[Main] Error: Compilation failed for '%s'\n", prog_file);
            return 1;
        }
    } else {
        strncpy(byte_file, prog_file, sizeof(byte_file) - 1);
    }

    int proc_id = 0;

    if (!memory_initialize(proc_id, byte_file, data_file)) {
        printf("[Main] Error: Memory initialization failed\n");
        return 1;
    }

    reset(proc_id);

    printf("[Main] Executing program on Processor %d...\n", proc_id);

    while (!end_of_simulation[proc_id]) {
        int opcode = 0, dest = 0, src1 = 0, src2 = 0;
        fetch(proc_id, &opcode, &dest, &src1, &src2);
        decode();
        execute(proc_id, opcode, dest, src1, src2);
    }

    memory_finalize(proc_id, "data_out.byte");
    printf("[Main] Execution finished! Final data memory written to 'data_out.byte'\n");
    
    processor_cleanup();
    return 0;
}

static int run_os_mode(void) {
    printf("  CS527 Mini-Computer Simulator: OS Multi-Process   \n");
    printf("  Processors: %d | Time-Slice: %d instructions       \n", NP, TIME_SLICE);

    printf("Commands:\n");
    printf("  <program.txt> [data.byte] : Submit a new program to run\n");
    printf("  status                    : Display active processes\n");
    printf("  exit                      : Finish remaining jobs & exit\n");

    os_init();
    os_run();
    processor_cleanup();
    return 0;
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        const char *prog_file = argv[1];
        const char *data_file = (argc >= 3) ? argv[2] : NULL;
        return run_standalone(prog_file, data_file);
    } else {
        return run_os_mode();
    }
}
