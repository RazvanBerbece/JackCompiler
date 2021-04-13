
/************************************************************************
University of Leeds
School of Computing
COMP2932- Compiler Design and Construction
The Symbol Tables Module

I confirm that the following code has been developed and written by me and it is entirely the result of my own work.
I also confirm that I have not copied any parts of this program from another person or any other source or facilitated someone to copy this program from me.
I confirm that I will not publish the program online or share it with anyone without permission of the module leader.

Student Name: Razvan-Antonio Berbece
Student ID: 201301759
Email: sc19rab@leeds.ac.uk
Date Work Commenced: 19.03.2021
*************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "symbols.h"

/** Symbol Table Function Implementations **/

// init given symbol table with 0 for the number of symbols it contains
void InitSymbolTable(SymbolTable symTable) {
    symTable.count = 0;
    symTable.fieldCount = 0;
    symTable.staticCount = 0;
    strcpy(symTable.functionName, "");
    strcpy(symTable.className, "");
    strcpy(symTable.parentClass, "");
    symTable.isMethod = 0;
    symTable.numberOfLocalVariables = 0;
}

// adds symbol to table
void AddSymbol(SymbolTable *symTable, char *name, char *type, SymbolKind kind) {

    // init symbol
    Symbol newSymbol;
    strcpy(newSymbol.name, name);
    strcpy(newSymbol.type, type);
    newSymbol.kind = kind;

    // determine processing based on whether symbol is static or field
    if (kind == STATIC) {
        symTable -> symbols[symTable -> count].offset = symTable -> staticCount;
        symTable -> staticCount += 1;
    }
    else if (kind == FIELD) {
        symTable -> symbols[symTable -> count].offset = symTable -> fieldCount;
        symTable -> fieldCount += 1;
    }
    else {
        symTable -> symbols[symTable -> count].offset = symTable -> count;
    }

    // add symbol to linked list and increase the symbol counter for given table
    symTable -> symbols[symTable -> count].kind = newSymbol.kind;
    strcpy(symTable -> symbols[symTable -> count].name, newSymbol.name);
    strcpy(symTable -> symbols[symTable -> count].type, newSymbol.type);
    symTable -> count += 1;

    if (kind == VAR) {
        symTable -> numberOfLocalVariables += 1;
    }
 
}

// looks up symbol with given name in table, returns the index in table if found or -1 if not present
int FindSymbol(SymbolTable symTable, char *name) {
    for (int i = 0; i < symTable.count; ++i) {
        if (strcmp(symTable.symbols[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// prints the given symbol table
void PrintTable(SymbolTable symTable) {
    for (int i = 0; i < symTable.count; ++i) {
        printf(" Name: %s , Type: %s , Kind: %d , Offset: %d \n", symTable.symbols[i].name, symTable.symbols[i].type, symTable.symbols[i].kind, symTable.symbols[i].offset);
    }
}

// cleanup given table -- reset counters, memset whole array to null etc
void FreeTable(SymbolTable symTable) {
    for (int i = 0; i < symTable.count; ++i) {
        memset(symTable.symbols[i].name, '\0', sizeof symTable.symbols[i].name);
        memset(symTable.symbols[i].type, '\0', sizeof symTable.symbols[i].type);
    }
    memset(symTable.symbols, 0, sizeof symTable.symbols);
    memset(symTable.functionName, '\0', sizeof symTable.functionName);
    memset(symTable.className, '\0', sizeof symTable.className);
    memset(symTable.parentClass, '\0', sizeof symTable.parentClass);
    symTable.count = -1;
    symTable.isMethod = 0;
    symTable.numberOfLocalVariables = 0;
    symTable.fieldCount = 0;
    symTable.staticCount = 0;
}

// COMMENT THIS OUT BEFORE RUNNING COMPILE WITH SYMBOL TABLES
/*
int main() { // for testing purposes

    printf("\nTesting symbol table logic ... \n\n");

    // simulate how the symbol tables will work in the parser
    SymbolTable tables[128];
    int tablesCount = 0;

    // init symbol table on position tablesCount = 0 and increase tablesCount
    InitSymbolTable(tables[tablesCount]);
    tablesCount++;

    // add a field symbol to tables[0]
    char *varName = "testInt\0";
    AddSymbol(tables, varName, "int", FIELD);
    AddSymbol(tables, "testString", "string", FIELD);

    // check if symbol is in table[0]
    int found = FindSymbol(tables[0], varName);
    if (found >= 0) {
        printf("Symbol %s is in tables[0].table[%d] ! \n\n", varName, found);
    }
    else {
        printf("Symbol %s is NOT in tables[0] ! \n\n", varName);
    }

    // print table
    PrintTable(tables[0]);

    // cleanup
    FreeTable(tables[0]);

    return 0;

}
*/
// STOP COMMENT HERE