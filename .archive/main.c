#include <stdio.h>
#include "processor.h"
#include "memory.h"
#include "compiler.h"

int main(int argc, char *argv[])
{

    char *program_file;
    if (argc >= 2)
    {
        // User provided a file
        program_file = argv[1];
    }
    else
    {
        // No file provided, use default
        program_file = "program.txt";
    }

    // call compiler and  generate a program.byte  file intermediate code  file that is used to read for the code.
    compile(program_file);
    // memory is initialisd and  reset the register and memory file,
    initialize();
    reset();

    // used  to work on instruction that is has its owns steps.
    while (!end_of_simulation)
    {
        fetch();
        decode();
        execute();
    }

    // write the register and data to file data.byte format that we want
    finalize();
    return 0;
}
