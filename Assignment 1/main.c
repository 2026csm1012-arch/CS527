#include <stdio.h>
#include <string.h>


#include "compiler.h"
#include "memory.h"
#include "processor.h"

int main(void)
{
    int choice;
    char testFolder[100];

    
    printf("1. Test 01 - Sum of 4 Numbers\n");
    printf("2. Test 02 - Complex Multiplication\n");
    printf("3. Test 03 - Determinant of 3x3 Matrix\n");
    printf("0. Exit\n\n");

    printf("Enter your choice: ");
    scanf("%d", &choice);

    switch(choice)
    {
        case 1:
            strcpy(testFolder, "Tests/test_01/");
            break;
        case 2:
            strcpy(testFolder, "Tests/test_02/");
            break;

        case 3:
            strcpy(testFolder, "Tests/test_03/");
            break;

        case 0:
            printf("Exiting Simulator...\n");
            return 0;

        default:
            printf("Invalid Choice!\n");
            return 0;
    }

    printf("\nLoading %s\n\n", testFolder);

    /* Compiler */
    compile(testFolder);

    /* Memory */
    initialise(testFolder);

    /* Processor */
    reset();

    while(!end_of_simulation)
    {
        fetch();
        decode();
        execute();
    }

    /* Save Updated Memory */
    finalize(testFolder);
    printf("Execution done.\n");

    return 0;
}