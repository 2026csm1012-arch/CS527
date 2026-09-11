#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "memory.h"
#include "processor.h"

int main(int argc, char **argv) {
    const char *program_source = "program.txt";
    const char *data_file = "data.byte";

    /* Accept command-line arguments if provided: ./simulator [program.txt] [data.byte] */
    if (argc >= 2) {
        program_source = argv[1];
    }
    if (argc >= 3) {
        data_file = argv[2];
    }

    printf("====================================================\n");
    printf("  CS527 Mini-Computer System Simulator (Lab 3)     \n");
    printf("  Program: %s | Data: %s\n", program_source, data_file ? data_file : "(none)");
    printf("====================================================\n");

    /* 1. Compile assembly source into program.byte */
    const char *dot = strrchr(program_source, '.');
    const char *byte_file = program_source;

    if (dot && (strcmp(dot, ".txt") == 0 || strcmp(dot, ".asm") == 0)) {
        if (!compile(program_source, "program.byte")) {
            printf("[Main] Error: Compilation failed for '%s'\n", program_source);
            return 1;
        }
        byte_file = "program.byte";
    }

    /* 2. Initialize Memory */
    if (!memory_initialize(byte_file, data_file)) {
        printf("[Main] Error: Memory initialization failed\n");
        return 1;
    }

    /* 3. Reset Processor State */
    reset();

    /* 4. Execute Program */
    printf("[Main] Simulation running...\n");
    while (!end_of_simulation) {
        fetch();
        decode();
        execute();
    }

    /* 5. Finalize and Save Data Memory */
    memory_finalize("data_out.byte");
    printf("[Main] Simulation completed! Output written to 'data_out.byte'\n");
    printf("====================================================\n");

    return 0;
}
