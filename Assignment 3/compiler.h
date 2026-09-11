#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>

/**
 * Compile a high-level assembly program file into a bytecode file ("program.byte").
 *
 * Supports:
 * - Basic arithmetic (+, -, *, /) on integer and vector registers
 * - Constant and variable operands (e.g. x1 = x2 + 10, v1 = v2 + v3)
 * - Memory operations with brackets: x1 = [x2], [x2] = x1, v1 = [x2], [x2] = v1
 * - Legacy memory syntax: Read x1, 0 and Write x1, 0
 * - Data movement: x1 = 0, x1 = x2
 * - Labels: .label_name
 * - Branch instructions: BEQ, BNE, BGE, BLT, BGT, BLE, BAL, etc.
 * - Print instruction: print x1
 * - Comments starting with %
 */
bool compile(const char *input_source_file, const char *output_byte_file);

#endif /* COMPILER_H */
