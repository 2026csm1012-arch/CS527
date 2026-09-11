#include <stdio.h>
#include <string.h>

#include "memory.h"

//memory
unsigned char Instruction[256];

//data 
unsigned char Data[256];

void initialise(char folder[])
{
    FILE *programFile;
    FILE *dataFile;

    char programPath[200];
    char dataPath[200];

    int opcode;
    int dest;
    int src1;
    int src2;

    int index = 0;


     sprintf(programPath,"%sprogram.byte",folder);
        sprintf(dataPath,"%sdata.byte",folder);

    for(int i=0;i<256;i++)
    {
        Instruction[i]=0;


        Data[i]=0;
    }

    programFile=fopen(programPath,"r");

    if(programFile==NULL)
    {
        printf("Error : Cannot open %s\n",programPath);
        return;
    }

    while(fscanf(programFile,"%d %d %d %d",
                 &opcode,
                 &dest,
                 &src1,
                 &src2)==4)
    {
        Instruction[index++]=opcode;
        Instruction[index++]=dest;

        Instruction[index++]=src1;
        Instruction[index++]=src2;
    }

    fclose(programFile);

        dataFile=fopen(dataPath,"r");

    if(dataFile==NULL)
    {
        printf("Error : Cannot open %s\n",dataPath);
        return;
    }



    int address;

    int value;

    while(fscanf(dataFile,"%d %d",&address,&value)==2)
    {
        if(address>=0 && address<256)
        {
            Data[address]=value;
        }
    }

    fclose(dataFile);

    printf("Memory Initialized Successfully.\n");
}

void finalize(char folder[])
{
    FILE *dataFile;

    char dataPath[200];


        sprintf(dataPath,"%sdata.byte",folder);
        
        dataFile=fopen(dataPath,"w");

    if(dataFile==NULL)
    {
        printf("Cannot write %s\n",dataPath);
        return;
    }

    for(int i=0;i<256;i++)
    {
        fprintf(dataFile,"%d %d\n",i,Data[i]);
    }

    fclose(dataFile);

    printf("Memory Saved Successfully.\n");
}