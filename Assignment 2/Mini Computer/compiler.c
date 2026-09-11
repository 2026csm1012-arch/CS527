#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include "compiler.h"

#define MAX_LABELS 100
#define MAX_LINE 256
#define MAX_PATH 512

typedef struct {
    char name[50];
    int address;
} Label;

static Label labels[MAX_LABELS];
static int labelCount = 0;

static void removeComment(char *line)
{
    char *p = strchr(line, '%');
    if (p != NULL) *p = '\0';
}

static void trim(char *s)
{
    char *start = s;
    size_t len;
    while (isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

static int validRegister(int r)
{
    return r >= 0 && r <= 255;
}

static int validConstant(int v)
{
    return v >= 0 && v <= 255;
}

static int validLabelName(const char *name)
{
    size_t i;
    if (name[0] == '\0') return 0;
    for (i = 0; name[i] != '\0'; i++) {
        if (!isalnum((unsigned char)name[i])) return 0;
    }
    return 1;
}

static int findLabel(const char *name)
{
    int i;
    for (i = 0; i < labelCount; i++) {
        if (strcmp(labels[i].name, name) == 0) return labels[i].address;
    }
    return -1;
}

static int addLabel(const char *name, int address)
{
    if (!validLabelName(name)) return 0;
    if (labelCount >= MAX_LABELS) return 0;
    if (findLabel(name) >= 0) return 0;
    strcpy(labels[labelCount].name, name);
    labels[labelCount].address = address;
    labelCount++;
    return 1;
}

static int getBranchCode(const char *operation)
{
    if (strcmp(operation, "BEQ") == 0) return 0;
    if (strcmp(operation, "BNE") == 0) return 1;
    if (strcmp(operation, "BCS") == 0) return 2;
    if (strcmp(operation, "BCC") == 0) return 3;
    if (strcmp(operation, "BMI") == 0) return 4;
    if (strcmp(operation, "BPL") == 0) return 5;
    if (strcmp(operation, "BVS") == 0) return 6;
    if (strcmp(operation, "BVC") == 0) return 7;
    if (strcmp(operation, "BHI") == 0) return 8;
    if (strcmp(operation, "BLS") == 0) return 9;
    if (strcmp(operation, "BGE") == 0) return 10;
    if (strcmp(operation, "BLT") == 0) return 11;
    if (strcmp(operation, "BGT") == 0) return 12;
    if (strcmp(operation, "BLE") == 0) return 13;
    if (strcmp(operation, "BAL") == 0) return 14;
    return -1;
}

static int collectLabels(const char *folder)
{
    FILE *source;
    char path[MAX_PATH], line[MAX_LINE], name[50];
    int address = 0;

    snprintf(path, sizeof(path), "%sprogram.txt", folder);
    source = fopen(path, "r");
    if (!source) {
        printf("Error: Cannot open %s\n", path);
        return 0;
    }

    labelCount = 0;
    while (fgets(line, sizeof(line), source)) {
        removeComment(line);
        trim(line);
        if (line[0] == '\0') continue;

        if (line[0] == '.') {
            if (sscanf(line, ".%49s", name) != 1 || !validLabelName(name)) {
                printf("Error: Invalid label: %s\n", line);
                fclose(source);
                return 0;
            }
            if (!addLabel(name, address)) {
                printf("Error: Duplicate/invalid label: .%s\n", name);
                fclose(source);
                return 0;
            }
        } else {
            address += 4;
            if (address > 256) {
                printf("Error: Instruction memory exceeds 256 bytes.\n");
                fclose(source);
                return 0;
            }
        }
    }
    fclose(source);
    return 1;
}

static int writeBytecode(FILE *out, int a, int b, int c, int d)
{
    if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255)
        return 0;
    fprintf(out, "%02X %02X %02X %02X\n", a, b, c, d);
    return 1;
}

void compile(char folder[])
{
    FILE *source, *bytecode;
    char inputFile[MAX_PATH], outputFile[MAX_PATH], line[MAX_LINE];
    char operation[20], operand[50];
    int dest, src1, src2, value;
    int currentAddress = 0;
    int ok = 1;

    snprintf(inputFile, sizeof(inputFile), "%sprogram.txt", folder);
    snprintf(outputFile, sizeof(outputFile), "%sprogram.byte", folder);

    if (!collectLabels(folder)) return;

    source = fopen(inputFile, "r");
    bytecode = fopen(outputFile, "w");
    if (!source || !bytecode) {
        printf("Error: Cannot open compiler files.\n");
        if (source) fclose(source);
        if (bytecode) fclose(bytecode);
        return;
    }

    while (fgets(line, sizeof(line), source)) {
        removeComment(line);
        trim(line);
        if (line[0] == '\0') continue;
        if (line[0] == '.') continue;

        /* Legacy Lab 1 Read/Write. */
        if (sscanf(line, "Read x%d %d", &dest, &value) == 2 ||
            sscanf(line, "Read x%d, %d", &dest, &value) == 2) {
            ok = validRegister(dest) && validConstant(value) && writeBytecode(bytecode, 0x05, dest, value, 0);
        }
        else if (sscanf(line, "Write x%d %d", &dest, &value) == 2 ||
                 sscanf(line, "Write x%d, %d", &dest, &value) == 2) {
            ok = validRegister(dest) && validConstant(value) && writeBytecode(bytecode, 0x06, dest, value, 0);
        }
        /* Memory read: variable or constant address. */
        else if (sscanf(line, "x%d = [x%d]", &dest, &src1) == 2) {
            ok = validRegister(dest) && validRegister(src1) && writeBytecode(bytecode, 0x05, dest, src1, 0);
        }
        else if (sscanf(line, "x%d = [%d]", &dest, &value) == 2) {
            ok = validRegister(dest) && validConstant(value) && writeBytecode(bytecode, 0x0D, dest, 0, value);
        }
        /* Memory write: address in register, value in register/constant. */
        else if (sscanf(line, "[x%d] = x%d", &src1, &dest) == 2) {
            ok = validRegister(dest) && validRegister(src1) && writeBytecode(bytecode, 0x06, dest, src1, 0);
        }
        else if (sscanf(line, "[x%d] = %d", &src1, &value) == 2) {
            ok = validRegister(src1) && validConstant(value) && writeBytecode(bytecode, 0x0E, 0, src1, value);
        }
        /* Branch. */
        else if (sscanf(line, "%19s %49s", operation, operand) == 2) {
            int code = getBranchCode(operation);
            if (code >= 0) {
                int target = -1;
                int offset;
                if (operand[0] == '.') target = findLabel(operand + 1);
                if (target < 0) {
                    printf("Error: Undefined label %s\n", operand);
                    ok = 0;
                } else {
                    offset = (target - currentAddress) / 4;
                    if (offset < -128 || offset > 127) {
                        printf("Error: Branch offset out of range: %d\n", offset);
                        ok = 0;
                    } else {
                        ok = writeBytecode(bytecode, 0x10 + code, 0, 0, (unsigned char)offset);
                    }
                }
            } else {
                /* Fall through to arithmetic below. */
                goto not_branch;
            }
        }
        else {
not_branch:
            if (sscanf(line, "x%d = x%d + x%d", &dest, &src1, &src2) == 3)
                ok = validRegister(dest) && validRegister(src1) && validRegister(src2) && writeBytecode(bytecode, 0x01, dest, src1, src2);
            else if (sscanf(line, "x%d = x%d - x%d", &dest, &src1, &src2) == 3)
                ok = validRegister(dest) && validRegister(src1) && validRegister(src2) && writeBytecode(bytecode, 0x02, dest, src1, src2);
            else if (sscanf(line, "x%d = x%d * x%d", &dest, &src1, &src2) == 3)
                ok = validRegister(dest) && validRegister(src1) && validRegister(src2) && writeBytecode(bytecode, 0x03, dest, src1, src2);
            else if (sscanf(line, "x%d = x%d / x%d", &dest, &src1, &src2) == 3)
                ok = validRegister(dest) && validRegister(src1) && validRegister(src2) && writeBytecode(bytecode, 0x04, dest, src1, src2);
            else if (sscanf(line, "x%d = x%d + %d", &dest, &src1, &value) == 3)
                ok = validRegister(dest) && validRegister(src1) && validConstant(value) && writeBytecode(bytecode, 0x09, dest, src1, value);
            else if (sscanf(line, "x%d = x%d - %d", &dest, &src1, &value) == 3)
                ok = validRegister(dest) && validRegister(src1) && validConstant(value) && writeBytecode(bytecode, 0x0A, dest, src1, value);
            else if (sscanf(line, "x%d = x%d * %d", &dest, &src1, &value) == 3)
                ok = validRegister(dest) && validRegister(src1) && validConstant(value) && writeBytecode(bytecode, 0x0B, dest, src1, value);
            else if (sscanf(line, "x%d = x%d / %d", &dest, &src1, &value) == 3)
                ok = validRegister(dest) && validRegister(src1) && validConstant(value) && writeBytecode(bytecode, 0x0C, dest, src1, value);
            else if (sscanf(line, "x%d = %d", &dest, &value) == 2)
                ok = validRegister(dest) && validConstant(value) && writeBytecode(bytecode, 0x0F, dest, 0, value);
            else {
                printf("Error: Unknown instruction: %s\n", line);
                ok = 0;
            }
        }

        if (!ok) break;
        currentAddress += 4;
    }

    if (ok) writeBytecode(bytecode, 0, 0, 0, 0);
    fclose(source);
    fclose(bytecode);

    if (ok) printf("Compilation successful.\n");
    else printf("Compilation failed.\n");
}
