#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "memory.h"
#include "processor.h"

int main(int argc, char **argv)
{
    char folder[200];
    int choice;

    if (argc > 1)
    {


        strcpy(folder, argv[1]);
    }
    else
    {
        printf("1. Test 01 - Sum of 4 Numbers\n");
        printf("2. Test 02 - Complex Multiplication\n");
        printf("3. Test 03 - Determinant of 3x3 Matrix\n");
        printf("4. Test 04 - Lab 2 Feature Demo\n");
        printf("0. Exit\n\n");

        printf("Enter your choice: ");
        scanf("%d", &choice);

        if (choice == 1)
            strcpy(folder, "tests/test_01/");
        else if (choice == 2)
            strcpy(folder, "tests/test_02/");
        else if (choice == 3)
            strcpy(folder, "tests/test_03/");
        else if (choice == 4)
            strcpy(folder, "tests/test_04/");
        else
        {
            printf("Exiting Simulator...\n");
            return 0;
        }
    }

    printf("\nUsing folder: %s\n\n", folder);

    compile(folder);
    initialise(folder);
    reset();

    while (!end_of_simulation)
    {
        fetch();
        decode();
        execute();
    }
    finalize(folder);

    printf("Simulation finished.\n");

    return 0;
}
