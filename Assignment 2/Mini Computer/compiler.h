#ifndef COMPILER_H
#define COMPILER_H

void compile(char folder[]);

#endif

// Compiler_h is macro name for header file
// here if macro is not defined , then it define new one and include the content of header file.
//  if alreaddy  defined then it will skip creation of new macrofile
// #endif is used to end the conditional directive started by #ifndef