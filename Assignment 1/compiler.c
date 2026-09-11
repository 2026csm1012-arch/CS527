#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "compiler.h"

void compile(char folder[])
{
    FILE *source;
    FILE *bytecode;

    char inputFile[200];
    char outputFile[200];

    char line[256];

    int dest;
    int src1;
    int src2;
    int value;
    int address;

    /* Create complete file paths */
    sprintf(inputFile, "%sprogram.txt", folder);
    sprintf(outputFile, "%sprogram.byte", folder);

    source = fopen(inputFile, "r");

    if(source == NULL)
    {
        printf("Error : Cannot open %s\n", inputFile);
        return;
    }

    bytecode = fopen(outputFile, "w");

    if(bytecode == NULL)
    {
        printf("Error : Cannot create %s\n", outputFile);
        fclose(source);
        return;
    }

    while(fgets(line, sizeof(line), source))
    {

        line[strcspn(line, "\n")] = '\0';
        
        //read
        if(sscanf(line, "Read x%d, %d", &dest, &address) == 2)
        {
            fprintf(bytecode, "5 %d %d 0\n",dest,address);
        }
     
        //write
        else if(sscanf(line, "Write x%d, %d", &dest, &address) == 2)
        {
            fprintf(bytecode, "6 %d %d 0\n",dest,address);
        }
       
        //transfer
        else if(sscanf(line, "x%d = %d",&dest,&value) == 2)
        {
            fprintf(bytecode,"7 %d %d 0\n",dest,value);
        }
       
        //add

        else if(sscanf(line,
                       "x%d = x%d + x%d",&dest,&src1,&src2) == 3)
        {
            fprintf(bytecode,"1 %d %d %d\n",dest,src1,src2);
        }
        //sub
        else if(sscanf(line,"x%d = x%d - x%d",&dest,&src1,&src2) == 3)
        {
            fprintf(bytecode,"2 %d %d %d\n",dest,src1,src2);
        }
    //    Multiplication
        else if(sscanf(line,"x%d = x%d * x%d",&dest,&src1,&src2) == 3)
        {
            fprintf(bytecode,"3 %d %d %d\n",dest,src1,src2);
        }

       //divide
        else if(sscanf(line,"x%d = x%d / x%d",&dest,&src1,&src2) == 3)
        {
            fprintf(bytecode,"4 %d %d %d\n",dest,src1,src2);
        }
        else
        {
            printf("Unknown Instruction : %s\n",line);
        }
    }

    fprintf(bytecode, "0 0 0 0\n");

    fclose(source);
    fclose(bytecode);

    printf("Compilation Successful.\n");
}