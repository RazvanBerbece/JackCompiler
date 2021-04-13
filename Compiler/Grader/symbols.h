#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "lexer.h"
#include "parser.h"

// define your own types and function prototypes for the symbol table(s) module below

// represents the symbol types which will be added in the symbol table
typedef enum {STATIC, FIELD, ARGUMENT, VAR} SymbolKind;

// represents a symbol
typedef struct {
    char name[128]; // the string representation of the symbol
    char type[128]; // the type of the symbol (eg: int, class, etc.)
    SymbolKind kind; // the kind of symbol (class-scope: static, field | method-scope: argument, var)
    int offset; // offset of symbol
} Symbol;

// represents the symbol table (the parser will use an indexed array of SymbolTable)
typedef struct {
    Symbol symbols[256]; // list of symbols
    int count; // number of symbols in table
    char functionName[128]; // has length > 0 if it's a function table
    char className[128]; // has length > 0 if it's a class table
} SymbolTable;

void InitSymbolTable(SymbolTable symTable);
void AddSymbol(SymbolTable *symTable, char *name, char *type, SymbolKind kind);
int FindSymbol(SymbolTable table, char *name);
void PrintTable(SymbolTable table);
void FreeTable(SymbolTable table);

#endif

