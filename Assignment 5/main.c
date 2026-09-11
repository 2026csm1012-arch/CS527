#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "memory.h"
#include "processor.h"
#include "os.h"

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Single-program path: CLI program/data -> OS initialization -> loader -> scheduler until completion. It provides the simple single-processing/single-job execution mode.
 */

static int run_standalone(const char *prog_file, const char *data_file)
{
    printf("  CS527 Mini-Computer Simulator (Lab 5 Paged Architecture)\n");

    os_init();
    os_set_shell_active(false);
    loader(prog_file, data_file);

    while (os_is_active())
    {
        scheduler();
    }

    processor_cleanup();
    return 0;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Multi-process path: initialize the OS -> interactive shell + scheduler. Multiple submitted jobs are assigned to the four simulated cores or placed in the waiting queue.
 */
static int run_os_mode(void)
{
    printf("  CS527 Mini-Computer Simulator: OS Multi-Process (Lab 5)\n");
    printf("  Processors: %d | Time-Slice: %d | Frames: %d\n", NP, TIME_SLICE, NUM_PHYSICAL_PAGES);

    printf("Commands:\n");
    printf("  <program.txt> [data.byte] : Submit a new program to run\n");
    printf("  status                    : Display active processes\n");
    printf("  exit                      : Finish remaining jobs & exit\n");

    os_init();
    os_run();
    processor_cleanup();
    return 0;
}

/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Entry point: command-line arguments select standalone mode; no arguments select the interactive multi-process OS mode.
 */
int main(int argc, char **argv)
{
    if (argc >= 2)
    {
        const char *prog_file = argv[1];
        // Use default 'data.byte' if no data file is provided
        const char *data_file = (argc >= 3) ? argv[2] : "data.byte";
        return run_standalone(prog_file, data_file);
    }
    else
    {
        return run_os_mode();
    }
}