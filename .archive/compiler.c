#include <stdio.h>
#include <string.h>
#include "compiler.h"
#define MAX_LABELS 256

// this is structure  to store lable in single object or node we can say, and it store label name and its address it points to
typedef struct{
    char name[50];
    int address;
} Label;

// we have array of labels , it means we can store 100 labels in this array
Label labels[MAX_LABELS];
int label_count = 0;

// this method find array in label array.
int find_label(const char *name)
{
    for (int i = 0; i < label_count; i++)
    {
        if (strcmp(labels[i].name, name) == 0)
        {
            return labels[i].address;
        }
    }
    return -1;
}

// print label
/*
void print_label()
{
    for (int i = 0; i < label_count; i++)
    {
        printf("Label %s = %d\n",
               labels[i].name,
               labels[i].address);
    }
}
*/
// this compile method is used to compile all the instruction inprogram.txt file

void compile(char *program_file)
{
    printf("Compilation started\n");
    printf("pass 1 started\n");

    // oopen program file in read mode and not openedd it gives error.
    FILE *program = fopen(program_file, "r");
    if (program == NULL)
    {
        printf("Error opening program file...\n");
        return;
    }

    // line is  a string of 256 character, means it will fetch a line of 256 character only form program file and
    char line[256];
    // bytecode_address is just pointer to  fetch the next code intruction and
    //  it store the relative valuue  and label address while traversion for labels, it in first pass
    int bytecode_address = 0;

    // while loop run, till fgets fetch a line form file, and if there is no line it it will return null and it stop compiling.
    while (fgets(line, sizeof(line), program) != NULL)
    {
        // char pointeer points to the first occurance location of % character, in line,
        //  if comment pointer is not  null it will replace complete strnig with the \0 symbol
        char *comment = strchr(line, '%');
        if (comment != NULL)
        {
            *comment = '\0';
        }

        // it line is empty  or have string terminator symbol \0 in it  it will skip this line
        if (line[0] == '\n' || line[0] == '\0')
        {
            continue;
        }

        // if theline starts with . it  will start reading as it is a label.
        if (line[0] == '.')
        {
            sscanf(line, "%49s", labels[label_count].name);
            labels[label_count].address = bytecode_address;
            label_count++;
            continue;
        }
        bytecode_address += 4;
    }

    printf("pass 1 ended\n");

    printf("pass 2 started\n");

    // rewind method is used to push back pointer of file  to start of file.
    rewind(program);

    // this can print all labels
    // print_label();

    rewind(program);
    bytecode_address = 0;

    // it open file in write mode and checks for the pointer.
    FILE *text_output = fopen("program.byte", "w");
    if (text_output == NULL)
    {
        printf("error opening program.byte..");
        fclose(program);
        return;
    }
    // this will fetch line from file and work  on it line by line
    while (fgets(line, sizeof(line), program) != NULL)
    {
        if (line[0] == '.')
        {
            continue;
        }

        // all these are string, it confuse every time , with char data type.
        char operation[200];
        char variable[20];
        int address;

        // The sscanf function in C++ is used to read formatted input from a string, allowing the extraction of data according to a specified format. It takes a source string, a format string, and a variable number of additional arguments to store the extracted values.
        sscanf(line, "%s %s %d", operation, variable, &address);

        // try to  compare the operation string we  fetched from sscanf with  read
        // this is  legacy  read
        if (strcmp(operation, "Read") == 0)
        {
            // it is used to read that operation is read but its registers and  source  is needed.
            int register_number;
            sscanf(variable, "x%d", &register_number);
            // this bytecode is also declaredtosystematically  store and write to file
            char bytecode[4];
            bytecode[0] = 5;
            bytecode[1] = register_number;
            bytecode[2] = 0;
            bytecode[3] = address;

            // this  fprintf  is  actuaaly  used to store formated to a file
            write_bytecode(text_output, bytecode);
        }
        // thiss is  updated read and memory read
        else if (strchr(line, '[') != NULL && line[0] != '[')
        {
            char dest_v[20];
            char add_v[20];
            // this  expression used to  read valur of  formated string that is insquare brackets
            sscanf(line, "%s = [%[^]]]", dest_v, add_v);
            int dest;
            // here we extraced the strings
            sscanf(dest_v, "x%d", &dest);

            // here we actually distingush between mordern read and memory read allong side square brackets implemented new formated here
            char bytecode[4];
            if (add_v[0] == 'x')
            {
                int address;
                sscanf(add_v, "x%d", &address);
                bytecode[0] = 0x05;
                bytecode[1] = dest;
                bytecode[2] = 0;
                bytecode[3] = address;
            }
            else
            {
                int address;
                sscanf(add_v, "%d", &address);
                bytecode[0] = 0x0D;
                bytecode[1] = dest;
                bytecode[2] = 0;
                bytecode[3] = address;
            }

            // instruction  is written by  this  mehtod.
            write_bytecode(text_output, bytecode);
        }
        // here we are checking for legacy write  statement
        else if (strcmp(operation, "Write") == 0)
        {
            int register_number;
            sscanf(variable, "x%d", &register_number);
            // implemented new instruction format  that variable or source will be from operannd.
            char bytecode[4];
            bytecode[0] = 0x06;
            bytecode[1] = register_number;
            bytecode[2] = 0;
            bytecode[3] = address;

            // instruction  is written by  this  mehtod.
            write_bytecode(text_output, bytecode);
        }
        // itis used to wriite for memory write or mordern write to register
        else if (strchr(line, '[') && line[0] == '[')
        {
            char add_v[20];
            char src_v[20];
            //  it    scan for  specific pattern where memory will be assign value oof register, or register to memory write
            sscanf(line, "[%[^]]] = %s", add_v, src_v);
            int src;
            sscanf(src_v, "x%d", &src);
            char bytecode[4];
            if (add_v[0] == 'x')
            {
                int address;
                sscanf(add_v, "x%d", &address);
                bytecode[0] = 0x06;
                bytecode[1] = address;
                bytecode[2] = 0;
                bytecode[3] = src;
            }
            else
            {
                // this is a constant write to  memory
                int address;
                sscanf(add_v, "%d", &address);
                bytecode[0] = 0x0E;
                bytecode[1] = address;
                bytecode[2] = 0;
                bytecode[3] = src;
            }

            // instruction  is written by  this  mehtod.
            write_bytecode(text_output, bytecode);
        }
        // branch

        // BEQ
        else if (strcmp(operation, "BEQ") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;
            fprintf(text_output, "%x %x %x %x\n", 0x10, 0, 0, (unsigned char)offset);
        }

        // BNE
        else if (strcmp(operation, "BNE") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x11, 0, 0, (unsigned char)offset);
        }

        // BCS
        else if (strcmp(operation, "BCS") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x12, 0, 0, (unsigned char)offset);
        }

        // BCC
        else if (strcmp(operation, "BCC") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x13, 0, 0, (unsigned char)offset);
        }

        // BMI
        else if (strcmp(operation, "BMI") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x14, 0, 0, (unsigned char)offset);
        }

        // BPL
        else if (strcmp(operation, "BPL") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x15, 0, 0, (unsigned char)offset);
        }

        // BVS
        else if (strcmp(operation, "BVS") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x16, 0, 0, (unsigned char)offset);
        }

        // BVC
        else if (strcmp(operation, "BVC") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x17, 0, 0, (unsigned char)offset);
        }

        // BHI
        else if (strcmp(operation, "BHI") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x18, 0, 0, (unsigned char)offset);
        }

        // BLS
        else if (strcmp(operation, "BLS") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x19, 0, 0, (unsigned char)offset);
        }

        // BGE
        else if (strcmp(operation, "BGE") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x1A, 0, 0, (unsigned char)offset);
        }

        // BLT
        else if (strcmp(operation, "BLT") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x1B, 0, 0, (unsigned char)offset);
        }

        // BGT
        else if (strcmp(operation, "BGT") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x1C, 0, 0, (unsigned char)offset);
        }

        // BLE
        else if (strcmp(operation, "BLE") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x1D, 0, 0, (unsigned char)offset);
        }

        // BAL
        else if (strcmp(operation, "BAL") == 0)
        {
            char label[50];
            sscanf(line, "%s %s", operation, label);

            int target_address = find_label(label);

            if (target_address == -1)
            {
                printf("Error: label %s not found\n", label);
                continue;
            }
            int offset = (target_address - bytecode_address) / 4;

            fprintf(text_output, "%x %x %x %x\n", 0x1E, 0, 0, (unsigned char)offset);
        }

        // strchr find for character in line, it return first occurence in line else return NULL
        else if (strchr(line, '+') != NULL)
        {
            char dest_v[20];
            char src1_v[20];
            char src2_v[20];

            // this read the instruction struccture that how registers are  used to perform arithmatic  registers  and store it in destination register.
            sscanf(line, "%s = %s + %s", dest_v, src1_v, src2_v);

            int dest, src1, src2;
            char bytecode[4];
            sscanf(dest_v, "x%d", &dest);
            sscanf(src1_v, "x%d", &src1);

            if (src2_v[0] == 'x')
            {
                // this is used to format ths values of register so that processor can perform operation efficiently
                sscanf(src2_v, "x%d", &src2);
                bytecode[0] = 1;
                bytecode[1] = dest;
                bytecode[2] = src1;
                bytecode[3] = src2;
            }
            else
            {
                //  instruction read as constant is  being added to register value
                int constant;
                sscanf(src2_v, "%d", &constant);
                bytecode[0] = 9;
                bytecode[1] = dest;
                bytecode[2] = src1;
                bytecode[3] = constant;
            }
            // instruction  is written by  this  mehtod.
            write_bytecode(text_output, bytecode);
        }
        //  subtraction method is implemmented here
        else if (strchr(line, '-') != NULL)
        {
            char dest_v[20], src1_v[20], src2_v[20];

            sscanf(line, "%s = %s - %s", dest_v, src1_v, src2_v);

            int dest, src1, src2;
            char bytecode[4];
            int constant;
            sscanf(dest_v, "x%d", &dest);
            sscanf(src1_v, "x%d", &src1);

            if (src2_v[0] == 'x')
            {
                sscanf(src2_v, "x%d", &src2);
                bytecode[0] = 2;
                bytecode[1] = dest;
                bytecode[2] = src1;
                bytecode[3] = src2;
            }
            else
            {
                sscanf(src2_v, "x%d", &constant);
                bytecode[0] = 0x0A;
                bytecode[1] = dest;
                bytecode[2] = src1;
                bytecode[3] = constant;
            }

            // instruction  is written by  this  mehtod.
            write_bytecode(text_output, bytecode);
        }

        //  multiplicatin  method is  implemented  here
        else if (strchr(line, '*') != NULL)
        {

            compile_multiply(line, text_output);
            // char dest_v[20], src1_v[20], src2_v[20];
            // sscanf(line, "%s = %s * %s", dest_v, src1_v, src2_v);

            // int dest, src1, src2;
            // char bytecode[4];
            // int constant;
            // sscanf(dest_v, "x%d", &dest);
            // sscanf(src1_v, "x%d", &src1);

            // if (src2_v[0] == 'x')
            // {
            //     sscanf(src2_v, "x%d", &src2);
            //     bytecode[0] = 3;
            //     bytecode[1] = dest;
            //     bytecode[2] = src1;
            //     bytecode[3] = src2;
            // }
            // else
            // {
            //     sscanf(src2_v, "x%d", &constant);
            //     bytecode[0] = 0x0B;
            //     bytecode[1] = dest;
            //     bytecode[2] = src1;
            //     bytecode[3] = constant;
            // }

            // // instruction  is written by  this  mehtod.
            // fprintf(text_output, "%x %x %x %x\n", (unsigned char)bytecode[0], (unsigned char)bytecode[1], (unsigned char)bytecode[2], (unsigned char)bytecode[3]);
        }

        // division method is implemented  with  similar  patern reogination sscanf
        else if (strchr(line, '/') != NULL)
        {

            char dest_v[20], src1_v[20], src2_v[20];

            sscanf(line, "%s = %s / %s", dest_v, src1_v, src2_v);

            int dest, src1, src2;
            char bytecode[4];
            int constant;
            sscanf(dest_v, "x%d", &dest);
            sscanf(src1_v, "x%d", &src1);

            if (src2_v[0] == 'x')
            {
                sscanf(src2_v, "x%d", &src2);
                bytecode[0] = 4;
                bytecode[1] = dest;
                bytecode[2] = src1;
                bytecode[3] = src2;
            }
            else
            {
                sscanf(src2_v, "%d", &constant);
                bytecode[0] = 0x0C;
                bytecode[1] = dest;
                bytecode[2] = src1;
                bytecode[3] = constant;
            }
            // instruction  is written by  this  mehtod.
            write_bytecode(text_output, bytecode);
        }

        /// this  iis for datamovement where  address and mordern address formaat is there.
        else if (operation[0] == 'x')
        {

            int register_number;
            sscanf(operation, "x%d", &register_number);

            char bytecode[4];
            bytecode[0] = 0x0F;
            bytecode[1] = register_number;
            bytecode[2] = 0;
            bytecode[3] = address;

            // instruction  is written by  this  mehtod.
            write_bytecode(text_output, bytecode);
        }

        bytecode_address += 4;
    }

    printf("pass 2  ended\n");
    /// this is  used  to  denote program end, after that all file pointers for compiler are closed.
    fprintf(text_output, "0 0 0 0\n");
    fclose(program);
    fclose(text_output);

    printf("Compilation ended\n");
}

void compile_multiply(char *line, FILE *text_output)
{
    char dest_v[20];
    char src1_v[20];
    char src2_v[20];

    int dest;
    int src1;
    int src2;
    int constant;

    char bytecode[4];

    sscanf(line, "%s = %s * %s",
           dest_v,
           src1_v,
           src2_v);

    sscanf(dest_v, "x%d", &dest);
    sscanf(src1_v, "x%d", &src1);

    if (src2_v[0] == 'x')
    {
        sscanf(src2_v, "x%d", &src2);

        bytecode[0] = 3;
        bytecode[1] = dest;
        bytecode[2] = src1;
        bytecode[3] = src2;
    }
    else
    {
        sscanf(src2_v, "%d", &constant);

        bytecode[0] = 0x0B;
        bytecode[1] = dest;
        bytecode[2] = src1;
        bytecode[3] = constant;
    }
    write_bytecode(text_output, bytecode);
}

void write_bytecode(FILE *text_output, char bytecode[4])
{
    fprintf(text_output, "%x %x %x %x\n",
            (unsigned char)bytecode[0],
            (unsigned char)bytecode[1],
            (unsigned char)bytecode[2],
            (unsigned char)bytecode[3]);
}