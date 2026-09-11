#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 256
#define MAX_LABELS 64
#define MAX_LINE_LEN 256

/* Structure to store Label to Instruction Address mapping */
typedef struct
{
    char name[64];
    int instruction_index;
} Label;

/* Structure to hold a parsed 4-byte instruction */
typedef struct
{
    unsigned char opcode;
    unsigned char dest;
    unsigned char src1;
    unsigned char src2;
} InstructionByte;

/* Trim leading and trailing whitespace from a string */
static char *trim_whitespace(char *str)
{
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
    return str;
}

/* Remove comments starting with '%' or '//' */
/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Removes '%' or '//' comments before parsing; data flows from source line to comment-free assembly, preventing documentation from becoming instructions.
 */
static void strip_comments(char *line)
{
    for (int i = 0; line[i] != '\0'; i++)
    {
        if (line[i] == '%' || (line[i] == '/' && line[i + 1] == '/'))
        {
            line[i] = '\0';
            break;
        }
    }
}

/* Find label index in symbol table */
/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Looks up a label in the pass-1 symbol table; data flows from branch name to instruction index, allowing symbolic branches to become numeric targets.
 */
static int find_label(Label labels[], int label_count, const char *name)
{
    for (int i = 0; i < label_count; i++)
    {
        if (strcmp(labels[i].name, name) == 0)
        {
            return labels[i].instruction_index;
        }
    }
    return -1;
}

/* Parse register string (e.g. "x5" -> 5, "v3" -> 3) */
/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Validates x/v register syntax and extracts its number; data flows from text token to ISA register field, connecting assembly syntax to machine encoding.
 */
static bool parse_register(const char *token, char expected_prefix, int *reg_num)
{
    if (!token || token[0] != expected_prefix)
        return false;
    char *endptr;
    long val = strtol(token + 1, &endptr, 10);
    if (*endptr != '\0' && !isspace((unsigned char)*endptr))
        return false;
    if (expected_prefix == 'x' && (val < 0 || val > 255))
        return false;
    if (expected_prefix == 'v' && (val < 0 || val > 31))
        return false;
    *reg_num = (int)val;
    return true;
}

/* Parse a constant integer (decimal or hex like 0x10) */
/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Converts decimal/hex text into an integer; data flows from literal text to numeric operand, enabling constants in arithmetic, memory and branches.
 */
static bool parse_constant(const char *token, int *const_val)
{
    if (!token || *token == '\0')
        return false;
    char *endptr;
    long val = strtol(token, &endptr, 0);
    if (*endptr != '\0' && !isspace((unsigned char)*endptr))
        return false;
    *const_val = (int)val;
    return true;
}

/* Helper to get branch suffix opcode (0x10 + suffix_code) */
/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Maps a branch mnemonic to its ISA opcode; data flows from human-readable condition to encoded instruction byte.
 */
static int get_branch_opcode(const char *token)
{
    if (strcasecmp(token, "BEQ") == 0)
        return 0x10;
    if (strcasecmp(token, "BNE") == 0)
        return 0x11;
    if (strcasecmp(token, "BCS") == 0)
        return 0x12;
    if (strcasecmp(token, "BCC") == 0)
        return 0x13;
    if (strcasecmp(token, "BMI") == 0)
        return 0x14;
    if (strcasecmp(token, "BPL") == 0)
        return 0x15;
    if (strcasecmp(token, "BVS") == 0)
        return 0x16;
    if (strcasecmp(token, "BVC") == 0)
        return 0x17;
    if (strcasecmp(token, "BHI") == 0)
        return 0x18;
    if (strcasecmp(token, "BLS") == 0)
        return 0x19;
    if (strcasecmp(token, "BGE") == 0)
        return 0x1A;
    if (strcasecmp(token, "BLT") == 0)
        return 0x1B;
    if (strcasecmp(token, "BGT") == 0)
        return 0x1C;
    if (strcasecmp(token, "BLE") == 0)
        return 0x1D;
    if (strcasecmp(token, "BAL") == 0)
        return 0x1E;
    return -1;
}

/**
 * Main compilation logic:
 * Pass 1: Scan for labels and collect their instruction indices.
 * Pass 2: Parse instructions and generate 4-byte bytecodes.
 */
/**
 * DATA FLOW / WORKING / SIGNIFICANCE:
 * Runs the two-pass compiler: source -> cleaned lines -> labels -> parsed instructions -> 4-byte bytecode file. It is the boundary between user assembly and the CPU.
 */
bool compile(const char *input_source_file, const char *output_byte_file)
{
    if (!input_source_file || !output_byte_file)
        return false;

    FILE *fin = fopen(input_source_file, "r");
    if (!fin)
    {
        printf("[Compiler] Error: Could not open source file '%s'\n", input_source_file);
        return false;
    }

    char raw_lines[MAX_LINES][MAX_LINE_LEN];
    int total_lines = 0;

    while (fgets(raw_lines[total_lines], MAX_LINE_LEN, fin) && total_lines < MAX_LINES)
    {
        total_lines++;
    }
    fclose(fin);

    Label labels[MAX_LABELS];
    int label_count = 0;
    int instruction_count = 0;

    /* =========================================================================
     * Pass 1: Symbol Table Generation (Find Labels)
     * -------------------------------------------------------------------------
     * We scan the entire source code first to find all labels (e.g. ".loop").
     * When we find a label, we record its name and which instruction it points to.
     * This is necessary because a branch might jump FORWARD to a label we
     * haven't parsed yet.
     * ========================================================================= */
    for (int i = 0; i < total_lines; i++)
    {
        char line_copy[MAX_LINE_LEN];
        strncpy(line_copy, raw_lines[i], MAX_LINE_LEN - 1);
        line_copy[MAX_LINE_LEN - 1] = '\0';

        strip_comments(line_copy);
        char *trimmed = trim_whitespace(line_copy);
        if (strlen(trimmed) == 0)
            continue;

        /* If line starts with '.', it is a label definition */
        if (trimmed[0] == '.')
        {
            if (label_count < MAX_LABELS)
            {
                strncpy(labels[label_count].name, trimmed, sizeof(labels[0].name) - 1);
                labels[label_count].name[sizeof(labels[0].name) - 1] = '\0';
                labels[label_count].instruction_index = instruction_count;
                label_count++;
            }
        }
        else
        {
            /* Non-label line represents an instruction */
            instruction_count++;
        }
    }
    InstructionByte code[MAX_LINES];
    int current_inst_idx = 0;

    /* =========================================================================
     * Pass 2: Instruction Parsing and Bytecode Generation
     * -------------------------------------------------------------------------
     * Now that we know where all labels are, we read the code again line by line.
     * We parse each human-readable assembly instruction and convert it into a
     * 4-byte machine code format (Opcode | Dest | Src1 | Src2).
     * ========================================================================= */
    for (int i = 0; i < total_lines; i++)
    {
        char line[MAX_LINE_LEN];
        strncpy(line, raw_lines[i], MAX_LINE_LEN - 1);
        line[MAX_LINE_LEN - 1] = '\0';

        strip_comments(line);
        char *trimmed = trim_whitespace(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '.')
            continue;

        InstructionByte inst = {0, 0, 0, 0};
        bool parsed = false;

        /* 1. Branch instructions: B{Suffix} <label> */
        char op_str[32], arg1[64];
        if (sscanf(trimmed, "%s %s", op_str, arg1) == 2)
        {
            int branch_op = get_branch_opcode(op_str);
            if (branch_op != -1)
            {
                int target_idx = find_label(labels, label_count, arg1);
                if (target_idx == -1)
                {
                    printf("[Compiler] Error on line %d: Undefined label '%s'\n", i + 1, arg1);
                    return false;
                }
                int offset = target_idx - current_inst_idx;
                inst.opcode = (unsigned char)branch_op;
                inst.dest = 0;
                inst.src1 = 0;
                inst.src2 = (unsigned char)(offset & 0xFF);
                parsed = true;
            }
        }

        /* 2. Print instruction: print <variable> */
        if (!parsed && (strncasecmp(trimmed, "print ", 6) == 0 || strncasecmp(trimmed, "print\t", 6) == 0))
        {
            int reg_num = 0;
            if (parse_register(trimmed + 6, 'x', &reg_num))
            {
                inst.opcode = 0x08;
                inst.dest = 0;
                inst.src1 = 0;
                inst.src2 = (unsigned char)reg_num;
                parsed = true;
            }
        }

        /* 3. Legacy Memory Read: Read <variable> <address> or Read <variable>, <address> */
        if (!parsed && strncasecmp(trimmed, "Read ", 5) == 0)
        {
            char reg_buf[32], addr_buf[32];

            // To simplify string parsing with sscanf, we copy the string and replace
            // any commas with spaces. This way, both "Read x1 10" and "Read x1, 10"
            // look identical to the parser.
            char clean_read[MAX_LINE_LEN];
            strcpy(clean_read, trimmed);
            for (char *p = clean_read; *p; p++)
                if (*p == ',')
                    *p = ' ';

            if (sscanf(clean_read, "%s %s %s", op_str, reg_buf, addr_buf) == 3)
            {
                int reg_num = 0, addr_val = 0;
                if (parse_register(reg_buf, 'x', &reg_num) && parse_constant(addr_buf, &addr_val))
                {
                    inst.opcode = 0x05;
                    inst.dest = (unsigned char)reg_num;
                    inst.src1 = 0;
                    inst.src2 = (unsigned char)(addr_val & 0xFF);
                    parsed = true;
                }
            }
        }

        /* 4. Legacy Memory Write: Write <variable> <address> or Write <variable>, <address> */
        if (!parsed && strncasecmp(trimmed, "Write ", 6) == 0)
        {
            char reg_buf[32], addr_buf[32];

            // Just like the read instruction, clean the input by replacing commas with spaces
            char clean_write[MAX_LINE_LEN];
            strcpy(clean_write, trimmed);
            for (char *p = clean_write; *p; p++)
                if (*p == ',')
                    *p = ' ';

            if (sscanf(clean_write, "%s %s %s", op_str, reg_buf, addr_buf) == 3)
            {
                int reg_num = 0, addr_val = 0;
                if (parse_register(reg_buf, 'x', &reg_num) && parse_constant(addr_buf, &addr_val))
                {
                    inst.opcode = 0x06;
                    inst.dest = (unsigned char)reg_num;
                    inst.src1 = (unsigned char)(addr_val & 0xFF);
                    inst.src2 = 0;
                    parsed = true;
                }
            }
        }

        /* 5. Bracket Memory Write: [addr] = value */
        if (!parsed && trimmed[0] == '[')
        {
            char *closing = strchr(trimmed, ']');
            char *equals = strchr(trimmed, '=');
            if (closing && equals && closing < equals)
            {
                *closing = '\0';
                char *addr_token = trim_whitespace(trimmed + 1);
                char *val_token = trim_whitespace(equals + 1);

                int dest_reg = 0, src_reg = 0, const_val = 0;

                /* Vector memory write: [x2] = v1 */
                if (parse_register(addr_token, 'x', &dest_reg) && parse_register(val_token, 'v', &src_reg))
                {
                    inst.opcode = 0x26;
                    inst.dest = (unsigned char)dest_reg;
                    inst.src1 = 0;
                    inst.src2 = (unsigned char)src_reg;
                    parsed = true;
                }
                /* Scalar memory write with register address: [x2] = x1 */
                else if (parse_register(addr_token, 'x', &dest_reg) && parse_register(val_token, 'x', &src_reg))
                {
                    inst.opcode = 0x06;
                    inst.dest = (unsigned char)dest_reg;
                    inst.src1 = 0;
                    inst.src2 = (unsigned char)src_reg;
                    parsed = true;
                }
                /* Scalar memory write with constant address: [0] = x1 */
                else if (parse_constant(addr_token, &const_val) && parse_register(val_token, 'x', &src_reg))
                {
                    inst.opcode = 0x0E;
                    inst.dest = (unsigned char)(const_val & 0xFF);
                    inst.src1 = 0;
                    inst.src2 = (unsigned char)src_reg;
                    parsed = true;
                }
            }
        }

        /* 6. Assignment expressions: Dest = Expression */
        if (!parsed)
        {
            char *equals = strchr(trimmed, '=');
            if (equals)
            {
                *equals = '\0';
                char *lhs = trim_whitespace(trimmed);
                char *rhs = trim_whitespace(equals + 1);

                int dest_int_reg = 0, dest_vec_reg = 0;
                bool is_dest_vec = parse_register(lhs, 'v', &dest_vec_reg);
                bool is_dest_int = parse_register(lhs, 'x', &dest_int_reg);

                /* 6a. Memory Read via Brackets: x1 = [x2] or v1 = [x2] or x1 = [0] */
                if (rhs[0] == '[' && rhs[strlen(rhs) - 1] == ']')
                {
                    rhs[strlen(rhs) - 1] = '\0';
                    char *addr_token = trim_whitespace(rhs + 1);
                    int addr_reg = 0, addr_const = 0;

                    if (is_dest_vec)
                    {
                        if (parse_register(addr_token, 'x', &addr_reg))
                        {
                            inst.opcode = 0x25; /* Vector memory read from register */
                            inst.dest = (unsigned char)dest_vec_reg;
                            inst.src1 = 0;
                            inst.src2 = (unsigned char)addr_reg;
                            parsed = true;
                        }
                        else if (parse_constant(addr_token, &addr_const))
                        {
                            inst.opcode = 0x2C; /* Vector memory read from constant */
                            inst.dest = (unsigned char)dest_vec_reg;
                            inst.src1 = 0;
                            inst.src2 = (unsigned char)(addr_const & 0xFF);
                            parsed = true;
                        }
                    }
                    else if (is_dest_int)
                    {
                        if (parse_register(addr_token, 'x', &addr_reg))
                        {
                            inst.opcode = 0x05; /* Integer memory read from register */
                            inst.dest = (unsigned char)dest_int_reg;
                            inst.src1 = 0;
                            inst.src2 = (unsigned char)addr_reg;
                            parsed = true;
                        }
                        else if (parse_constant(addr_token, &addr_const))
                        {
                            inst.opcode = 0x0C; /* Integer memory read from constant address */
                            inst.dest = (unsigned char)dest_int_reg;
                            inst.src1 = 0;
                            inst.src2 = (unsigned char)(addr_const & 0xFF);
                            parsed = true;
                        }
                    }
                }

                /* 6b. Binary Arithmetic: Dest = Op1 <op> Op2 */
                if (!parsed)
                {
                    char op_char = '\0';
                    char *op_pos = NULL;
                    /* Check for +, -, *, / operators */
                    if ((op_pos = strchr(rhs, '+')))
                        op_char = '+';
                    else if ((op_pos = strchr(rhs, '-')))
                        op_char = '-';
                    else if ((op_pos = strchr(rhs, '*')))
                        op_char = '*';
                    else if ((op_pos = strchr(rhs, '/')))
                        op_char = '/';

                    if (op_pos)
                    {
                        *op_pos = '\0';
                        char *op1_str = trim_whitespace(rhs);
                        char *op2_str = trim_whitespace(op_pos + 1);

                        /* Vector Arithmetic */
                        if (is_dest_vec)
                        {
                            int v_src1 = 0, v_src2 = 0, x_src2 = 0, c_src2 = 0;
                            if (parse_register(op1_str, 'v', &v_src1))
                            {
                                if (parse_register(op2_str, 'v', &v_src2))
                                {
                                    /* Vector op Vector */
                                    if (op_char == '+')
                                        inst.opcode = 0x21;
                                    else if (op_char == '-')
                                        inst.opcode = 0x22;
                                    else if (op_char == '*')
                                        inst.opcode = 0x23;

                                    inst.dest = (unsigned char)dest_vec_reg;
                                    inst.src1 = (unsigned char)v_src1;
                                    inst.src2 = (unsigned char)v_src2;
                                    parsed = true;
                                }
                                else if (parse_constant(op2_str, &c_src2))
                                {
                                    /* Vector op Constant */
                                    if (op_char == '+')
                                        inst.opcode = 0x29;
                                    else if (op_char == '-')
                                        inst.opcode = 0x2A;
                                    else if (op_char == '*')
                                        inst.opcode = 0x2B;

                                    inst.dest = (unsigned char)dest_vec_reg;
                                    inst.src1 = (unsigned char)v_src1;
                                    inst.src2 = (unsigned char)(c_src2 & 0xFF);
                                    parsed = true;
                                }
                                else if (parse_register(op2_str, 'x', &x_src2))
                                {
                                    /* Vector op Scalar register */
                                    if (op_char == '+')
                                        inst.opcode = 0x29;
                                    else if (op_char == '-')
                                        inst.opcode = 0x2A;
                                    else if (op_char == '*')
                                        inst.opcode = 0x2B;

                                    inst.dest = (unsigned char)dest_vec_reg;
                                    inst.src1 = (unsigned char)v_src1;
                                    inst.src2 = (unsigned char)x_src2;
                                    parsed = true;
                                }
                            }
                        }
                        /* Integer Arithmetic */
                        else if (is_dest_int)
                        {
                            int x_src1 = 0, x_src2 = 0, c_src2 = 0;
                            if (parse_register(op1_str, 'x', &x_src1))
                            {
                                if (parse_register(op2_str, 'x', &x_src2))
                                {
                                    /* Integer op Integer Register */
                                    if (op_char == '+')
                                        inst.opcode = 0x01;
                                    else if (op_char == '-')
                                        inst.opcode = 0x02;
                                    else if (op_char == '*')
                                        inst.opcode = 0x03;
                                    else if (op_char == '/')
                                        inst.opcode = 0x04;

                                    inst.dest = (unsigned char)dest_int_reg;
                                    inst.src1 = (unsigned char)x_src1;
                                    inst.src2 = (unsigned char)x_src2;
                                    parsed = true;
                                }
                                else if (parse_constant(op2_str, &c_src2))
                                {
                                    /* Integer op Constant */
                                    if (op_char == '+')
                                        inst.opcode = 0x09;
                                    else if (op_char == '-')
                                        inst.opcode = 0x0A;
                                    else if (op_char == '*')
                                        inst.opcode = 0x0B;
                                    else if (op_char == '/')
                                        inst.opcode = 0x0C;

                                    inst.dest = (unsigned char)dest_int_reg;
                                    inst.src1 = (unsigned char)x_src1;
                                    inst.src2 = (unsigned char)(c_src2 & 0xFF);
                                    parsed = true;
                                }
                            }
                        }
                    }
                }

                /* 6c. Data Movement: x1 = 10 or x1 = x2 */
                if (!parsed && is_dest_int)
                {
                    int src_reg = 0, const_val = 0;
                    if (parse_register(rhs, 'x', &src_reg))
                    {
                        inst.opcode = 0x07; /* Move from register */
                        inst.dest = (unsigned char)dest_int_reg;
                        inst.src1 = 0;
                        inst.src2 = (unsigned char)src_reg;
                        parsed = true;
                    }
                    else if (parse_constant(rhs, &const_val))
                    {
                        inst.opcode = 0x0F; /* Move from constant */
                        inst.dest = (unsigned char)dest_int_reg;
                        inst.src1 = 0;
                        inst.src2 = (unsigned char)(const_val & 0xFF);
                        parsed = true;
                    }
                }
            }
        }

        if (!parsed)
        {
            printf("[Compiler] Error: Failed to parse line %d: '%s'\n", i + 1, raw_lines[i]);
            return false;
        }

        code[current_inst_idx++] = inst;
    }

    /* Add Halt instruction (00 00 00 00) at the end of the program */
    InstructionByte halt_inst = {0x00, 0x00, 0x00, 0x00};
    code[current_inst_idx++] = halt_inst;

    /* Write generated bytecodes to output_byte_file in hex format */
    FILE *fout = fopen(output_byte_file, "w");
    if (!fout)
    {
        printf("[Compiler] Error: Could not open output file '%s'\n", output_byte_file);
        return false;
    }

    for (int i = 0; i < current_inst_idx; i++)
    {
        fprintf(fout, "%02X %02X %02X %02X\n",
                code[i].opcode, code[i].dest, code[i].src1, code[i].src2);
    }

    fclose(fout);
    printf("[Compiler] Successfully compiled '%s' -> '%s' (%d instructions)\n",
           input_source_file, output_byte_file, current_inst_idx);
    return true;
}
