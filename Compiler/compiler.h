#ifndef COMPILER_H
#define COMPILER_H

//#define TEST_COMPILER    // uncomment to run the compiler autograder

#include "parser.h"
#include "symbols.h"

int outputVM(char *filename, char *code); // outputs the code to the output .vm file

int InitCompiler ();
ParserInfo compile (char* dir_name);
int StopCompiler();

#endif
