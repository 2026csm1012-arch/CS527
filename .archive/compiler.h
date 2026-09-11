#ifndef COMPILER_H
#define COMPILER_H

void compile(char *program_file);
// void compile();

void compile_multiply(char *line, FILE *text_output);

void write_bytecode(FILE *text_output, char bytecode[4]);

#endif