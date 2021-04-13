/************************************************************************
University of Leeds
School of Computing
COMP2932- Compiler Design and Construction
Lexer Module

I confirm that the following code has been developed and written by me and it is entirely the result of my own work.
I also confirm that I have not copied any parts of this program from another person or any other source or facilitated someone to copy this program from me.
I confirm that I will not publish the program online or share it with anyone without permission of the module leader.

Student Name: Razvan-Antonio Berbece
Student ID: 201301759
Email: sc19rab@leeds.ac.uk
Date Work Commenced: 15.02.2021
*************************************************************************/

#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#include <ctype.h>

#include "lexer.h"


// YOU CAN ADD YOUR OWN FUNCTIONS, DECLARATIONS AND VARIABLES HERE

// File reading related variables
FILE * fp;
char filename[512];
char * code;
int n = 0;
int emptyFileFlag = 0;

// Scanning and tokenising related variables
int lineNumber = -1;
int peekLineNumber = -1;
int charCounter = 0;

// Terminal consts
const char symbols[20] = "()[]{},;=.+-*/&|~<>";
// reserved keywords
const char keywords[22][20] = {
  "class",
  "constructor",
  "method",
  "function",
  "int",
  "boolean",
  "char",
  "void",
  "var",
  "static",
  "field",
  "let",
  "do",
  "if",
  "else",
  "while",
  "return",
  "true",
  "false",
  "null",
  "this"
};

// Util functions
int checkFilename(char * file_name) {
  // Check file format (.jack extension)
  char extension[strlen(file_name)];
  strcpy(extension, file_name);
  extension[strlen(file_name)] = '\0';
  int index = 0;
  for (int i = 0; i < strlen(file_name); ++i) {
    if (i > 0 && file_name[i] == '.') { // check extension
      for (int j = i; j < strlen(file_name); ++j) {
        extension[index++] = file_name[j];
      }
      extension[index] = 0;
      break;
    }
  }
  if (strcmp(extension, ".jack") == 0) { // has .jack extension
    return 1;
  }
  return 0;
}

int isKeyword(char * input) {
  for (int i = 0; i < 21; ++i) {
    if (strcmp(input, keywords[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

void printToken(Token t) {
  switch ((int) t.tp) {
  case 0:
    printf("< %s, %d, %s, RESWORD >\n", t.fl, t.ln, t.lx);
    break;
  case 1:
    printf("< %s, %d, %s, ID >\n", t.fl, t.ln, t.lx);
    break;
  case 2:
    printf("< %s, %d, %s, INT >\n", t.fl, t.ln, t.lx);
    break;
  case 3:
    printf("< %s, %d, %s, SYMBOL >\n", t.fl, t.ln, t.lx);
    break;
  case 4:
    printf("< %s, %d, %s, STRING >\n", t.fl, t.ln, t.lx);
    break;
  case 5:
    printf("< %s, %d, %s, EOFile >\n", t.fl, t.ln, t.lx);
    break;
  case 6:
    printf("< %s, %d, %s, ERR >\n", t.fl, t.ln, t.lx);
    break;
  default:
    break;
  }
}

// IMPLEMENT THE FOLLOWING functions
//***********************************

// Initialise the lexer to read from source file
// file_name is the name of the source file
// This requires opening the file and making any necessary initialisations of the lexer
// If an error occurs, the function should return 0
// if everything goes well the function should return 1
int InitLexer(char * file_name) {

  if (!checkFilename(file_name)) return 0; // filename invalid

  fp = fopen(file_name, "r");

  if (!fp) { // file couldn't be opened
    return 0;
  }

  strcpy(filename, file_name);
  filename[strlen(file_name)] = '\0';

  // init line numbering and char array
  lineNumber = 1;
  peekLineNumber = 1;

  // fixes for garbage values in n and flag (even if declared in global scope)
  n = 0;
  emptyFileFlag = 0;

  // get the total size of the file (in characters) for mem alloc
  fseek(fp, 0, SEEK_END);
  long long f_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  code = malloc(f_size + 1);

  // empty file
  if (f_size == 0) {
    emptyFileFlag = 1;
    return 1;
  }

  // copy all characters from the file to the code[] array
  int c;
  while ((c = fgetc(fp)) != -1) {
    code[n] = (char)c;
    n++;
  }

  // a valid c-string ends with the null character
  code[n] = '\0';

  return 1;
}

// Get the next token from the source file
Token GetNextToken() {

  Token t;

  // empty file token
  if (emptyFileFlag || charCounter == n) {
    // build token
    t.tp = EOFile;
    t.ln = lineNumber;
    strcpy(t.lx, "End of File\0");
    strcpy(t.fl, filename);
    
    charCounter = -1; // scanning complete
    return t;
  }

  for (int i = charCounter; i < n; i++) {

    // skip leading whitespaces
    while (i != n && (code[i] == ' ' || code[i] == '\n' || code[i] == '\r')) {
      if (code[i] == '\n') {
        lineNumber++;
        peekLineNumber = lineNumber;
      }
      i++;
      charCounter = i;
    }

    // skip comments
    if (i != n && code[i] == '/' && code[i + 1] == '/') { // full line comments //
      i += 2;
      while (i != n) {
        if (code[i] == '\n') { // end of comment
          lineNumber++;
          peekLineNumber = lineNumber;
          break;
        }
        i++;
      }
    }
    
    while (i != n && code[i] == '/' && code[i + 1] == '*') { // comments until closed /* */
        int closed = 0;
        i += 2;
        while (i != n) { // comment not closed
          if (code[i] == '\n') {
            lineNumber++;
            peekLineNumber = lineNumber;
          }
          i++;
          if (code[i] == '*') {
            if (code[i + 1] == '/') { // end of comment in (i + 1)
              closed = 1; // flag as closed comment
              i += 2; // move to symbol AFTER end of comment 
              charCounter = i;
              if (code[i] == '\n') {
                lineNumber++;
                peekLineNumber = lineNumber;
              }
              break;
            }
          }
        }
        if (!closed) { // EOF in comment
          // build token
          strcpy(t.lx, "Error: unexpected eof in comment");
          t.tp = ERR;
          t.ec = EofInCom;
          t.lx[strlen(t.lx)] = '\0';
          t.ln = lineNumber;
          strcpy(t.fl, filename);
          
          charCounter = i;
          return t;
        }
        charCounter = i;
    }

    // integers
    if (isdigit(code[i])) {
      int intIndexStart = i;
      int digitCounter = 1;
      int peek = i + 1;
      while (peek != n && isdigit(code[peek])) {
        digitCounter++;
        peek++;
      }
      i = peek;
      charCounter = i;
      // build resulting lexeme
      char integer[256];
      int index = 0;
      for (int j = intIndexStart; j < intIndexStart + digitCounter; ++j) {
        integer[index++] = code[j];
      }
      integer[index] = '\0';
      // build token
      strcpy(t.lx, integer);
      t.lx[strlen(t.lx)] = '\0';
      strcpy(t.fl, filename);
      t.tp = INT;
      t.ln = lineNumber;
      
      return t;
    }

    // alphanumericals
    if (i != n && (isalpha(code[i]) || code[i] == '_')) { // KEYWORD or ID
      int indexStart = i;
      int characterCounter = 1;
      while (i != n && (isalpha(code[i]) || isdigit(code[i]) || code[i] == '_')) {
        characterCounter++;
        i++;
        charCounter = i;
      }
      // build resulting lexeme
      char result[512];
      int index = 0;
      for (int j = indexStart; j < indexStart + characterCounter - 1; ++j) {
        result[index++] = code[j];
      }
      result[index] = '\0';
      // Check if reserved keyword or id
      if (isKeyword(result)) { // KEYWORD
        t.tp = RESWORD;
      } else { // ID
        t.tp = ID;
      }
      // build token
      strcpy(t.lx, result);
      t.lx[strlen(t.lx)] = '\0';
      t.ln = lineNumber;
      strcpy(t.fl, filename);
      
      return t;
    }

    // strings
    if (i != n && code[i] == '\"') {
      int indexStart = i; // string starts in indexStart + 1
      i++;
      if (i == n) { // EofInStr
        // build token
        strcpy(t.lx, "Error: unexpected eof in string constant");
        t.tp = ERR;
        t.ec = EofInStr;
        t.lx[strlen(t.lx)] = '\0';
        t.ln = lineNumber;
        strcpy(t.fl, filename);
        
        charCounter = i;
        return t;
      } else { // build string
        int characterCounter = 1;
        while (i != n && code[i] != '\"') {
          i++;
          charCounter = i;
          characterCounter++;
          if (code[i] == '\n') { // \n not permitted
            // build token
            strcpy(t.lx, "Error: new line in string constant");
            t.tp = ERR;
            t.ec = NewLnInStr;
            t.lx[strlen(t.lx)] = '\0';
            t.ln = lineNumber;
            strcpy(t.fl, filename);          
            
            charCounter = i;
            return t;
          } else if (code[i] == '\0' || i == n) {
            strcpy(t.lx, "Error: unexpected eof in string constant");
            t.tp = ERR;
            t.ec = EofInStr;
            t.lx[strlen(t.lx)] = '\0';
            t.ln = lineNumber;
            strcpy(t.fl, filename);
            
            charCounter = i;
            return t;
          }

          if (code[i] == '\"') {
            i++;
            charCounter = i;
            characterCounter++;
            break;
          }
        }
        // build resulting lexeme
        char result[512];
        int index = 0;
        for (int j = indexStart + 1; j < indexStart + characterCounter - 1; ++j) {
          result[index++] = code[j];
        }
        result[index] = '\0';
        strcpy(t.lx, result);
        t.tp = STRING;
        t.lx[strlen(t.lx)] = '\0';
        t.ln = lineNumber;
        strcpy(t.fl, filename);
        
        return t;
      }
    }

    // symbols
    if (i != n && strchr(symbols, (code[i]))) {
      // build token
      char symbolLxm[2];
      symbolLxm[0] = code[i];
      symbolLxm[1] = '\0';
      strcpy(t.lx, symbolLxm);
      t.lx[strlen(t.lx)] = '\0';
      t.tp = SYMBOL;
      strcpy(t.fl, filename);
      t.ln = lineNumber;
      charCounter = i + 1;
      
      return t;
    }

    // illegal symbols
    if (i != n && strchr("?$`!@£$^", (code[i]))) {
      // build token
      strcpy(t.lx, "Error: illegal symbol in source file\0");
      t.tp = ERR;
      t.ec = IllSym;
      strcpy(t.fl, filename);
      t.ln = lineNumber;
      charCounter = i + 1;
      
      return t;
    }

    // EOF reached
    if (i == n) {
      // build token
      t.tp = EOFile;
      t.ln = lineNumber;
      strcpy(t.lx, "End of File\0");
      strcpy(t.fl, filename);
      charCounter = i + 1; // scanning complete
      
      return t;
    }
  }
}

// peek (look) at the next token in the source file without removing it from the stream
Token PeekNextToken() {

  Token t;
  int lineNumberLocal = peekLineNumber;

  // empty file token
  if (emptyFileFlag || charCounter == n) {
    // build token
    t.tp = EOFile;
    t.ln = lineNumberLocal;
    strcpy(t.lx, "End of File\0");
    strcpy(t.fl, filename);
    
    return t;
  }

  for (int i = charCounter; i < n; i++) {

    // skip leading whitespaces
    while (i != n && (code[i] == ' ' || code[i] == '\n' || code[i] == '\r')) {
      if (code[i] == '\n') {
        lineNumberLocal++;
      }
      i++;
    }

    // skip comments
    if (i != n && code[i] == '/' && code[i + 1] == '/') { // full line comments //
      i += 2;
      while (i != n) {
        if (code[i] == '\n') { // end of comment
          lineNumberLocal++;
          break;
        }
        i++;
      }
    }
    
    while (i != n && code[i] == '/' && code[i + 1] == '*') { // comments until closed /* */
        int closed = 0;
        i += 2;
        while (i != n) { // comment not closed
          if (code[i] == '\n') {
            lineNumberLocal++;
          }
          i++;
          if (code[i] == '*') {
            if (code[i + 1] == '/') { // end of comment in (i + 1)
              closed = 1;
              i += 2;
              if (code[i] == '\n') {
                lineNumberLocal++;
              }
              break;
            }
          }
        }
        if (!closed) { // EOF in comment
          // build token
          strcpy(t.lx, "Error: unexpected eof in comment");
          t.tp = ERR;
          t.ec = EofInCom;
          t.lx[strlen(t.lx)] = '\0';
          t.ln = lineNumberLocal;
          strcpy(t.fl, filename);
          
          return t;
        }   
    }

    // integers
    if (isdigit(code[i])) {
      int intIndexStart = i;
      int digitCounter = 1;
      int peek = i + 1;
      while (peek != n && isdigit(code[peek])) {
        digitCounter++;
        peek++;
      }
      i = peek;
      // build resulting lexeme
      char integer[512];
      int index = 0;
      for (int j = intIndexStart; j < intIndexStart + digitCounter; ++j) {
        integer[index++] = code[j];
      }
      integer[index] = '\0';
      // build token
      strcpy(t.lx, integer);
      t.lx[strlen(t.lx)] = '\0';
      strcpy(t.fl, filename);
      t.tp = INT;
      t.ln = lineNumberLocal;
      
      return t;
    }

    // alphanumericals
    if (i != n && (isalpha(code[i]) || code[i] == '_')) { // KEYWORD or ID
      int indexStart = i;
      int characterCounter = 1;
      while (i != n && (isalpha(code[i]) || isdigit(code[i]) || code[i] == '_')) {
        characterCounter++;
        i++;
      }
      // build resulting lexeme
      char result[512];
      int index = 0;
      for (int j = indexStart; j < indexStart + characterCounter - 1; ++j) {
        result[index++] = code[j];
      }
      result[index] = '\0';
      // Check if reserved keyword or id
      if (isKeyword(result)) { // KEYWORD
        t.tp = RESWORD;
      } else { // ID
        t.tp = ID;
      }
      // build token
      strcpy(t.lx, result);
      t.lx[strlen(t.lx)] = '\0';
      t.ln = lineNumberLocal;
      strcpy(t.fl, filename);
      
      return t;
    }

    // strings
    if (i != n && code[i] == '\"') {
      int indexStart = i;
      i++;
      if (i == n) { // EofInStr
        // build token
        strcpy(t.lx, "Error: unexpected eof in string constant");
        t.tp = ERR;
        t.ec = EofInStr;
        t.lx[strlen(t.lx)] = '\0';
        t.ln = lineNumberLocal;
        strcpy(t.fl, filename);
        
        return t;
      } else { // build string
        int characterCounter = 1;
        while (i != n && code[i] != '\"') {
          i++;
          characterCounter++;
          if (code[i] == '\n') { // \n not permitted
            // build token
            strcpy(t.lx, "Error: new line in string constant");
            t.tp = ERR;
            t.ec = NewLnInStr;
            t.lx[strlen(t.lx)] = '\0';
            t.ln = lineNumberLocal;
            strcpy(t.fl, filename);          
            
            return t;
          } else if (code[i] == '\0' || i == n) {
            strcpy(t.lx, "Error: unexpected eof in string constant");
            t.tp = ERR;
            t.ec = EofInStr;
            t.lx[strlen(t.lx)] = '\0';
            t.ln = lineNumberLocal;
            strcpy(t.fl, filename);
            
            return t;
          }

          if (code[i] == '\"') {
            i++;
            characterCounter++;
            break;
          }
        }
        // build resulting lexeme
        char result[512];
        int index = 0;
        for (int j = indexStart + 1; j < indexStart + characterCounter - 1; ++j) {
          result[index++] = code[j];
        }
        result[index] = '\0';
        strcpy(t.lx, result);
        t.tp = STRING;
        t.lx[strlen(t.lx)] = '\0';
        t.ln = lineNumberLocal;
        strcpy(t.fl, filename);
        
        return t;
      }
    }

    // symbols
    if (i != n && strchr(symbols, (code[i]))) {
      // build token
      char symbolLxm[2];
      symbolLxm[0] = code[i];
      symbolLxm[1] = '\0';
      strcpy(t.lx, symbolLxm);
      t.tp = SYMBOL;
      strcpy(t.fl, filename);
      t.ln = lineNumberLocal;
      
      return t;
    }

    // illegal symbols
    if (i != n && strchr("?$`!@£$^", (code[i]))) {
      // build token
      strcpy(t.lx, "error: illegal symbol in source file\0");
      t.tp = ERR;
      t.ec = IllSym;
      strcpy(t.fl, filename);
      t.ln = lineNumberLocal;
      
      return t;
    }

    // EOF reached
    if (i == n) {
      // build token
      t.tp = EOFile;
      t.ln = lineNumberLocal;
      strcpy(t.lx, "End of File\0");
      strcpy(t.fl, filename);
      
      return t;
    }
  }
}

// clean out at end, e.g. close files, free memory, ... etc
int StopLexer() {
  fclose(fp);
  lineNumber = -1;
  peekLineNumber = -1;
  charCounter = 0;
  code = 0;
  filename[0] = '\0';
  n = 0;
  return 0;
}

// do not remove the next line
/*
#ifndef TEST
int main() {
  // implement your main function here
  // NOTE: the autograder will not use your main function

  // Init
  printf("\n");
  InitLexer("./SquareGame.jack");

  // Test PeekNextToken()
  // Token p = PeekNextToken();
  // printToken(p);

  // Test GetNextToken()
  int num = 0;
  Token t = GetNextToken();
  printToken(t);
  num++;
  while (t.tp != EOFile && t.tp != ERR) {
    t = GetNextToken();
    printToken(t);
    num++;
  }
  printf("%d\n", num);

  // Cleanup
  StopLexer();

  return 0;
}
// do not remove the next line
#endif
*/