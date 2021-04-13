#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "symbols.h"

// you can declare prototypes of parser functions below

// Semantic Analysis Variables
SymbolTable tables[512];
Symbol thisSymbol; // used to store the data needed for the this ref in method tables
int counter = 0; // used for symbol table array indexing
int previousClassCounter = 0; 

// Used for second pass
int initialCounter = 0;
int initialClassCounter = 0;
extern int globalPass; // from compiler.c
extern int standardPass; // from compiler.c

// used to count the number of nested scopes in which the parser is at a moment in time
// increases on scope entry, decreases on scope exit
// 1 = top class scope (memberDeclars)
// 2 (function) ... 3 (loop in function) ... etc
int scopeCounter = 0;

// testing variables
char fileName[128];

// Code Gen Variables
char vmCode[9999999999999]; // will hold the VM code generated while parsing and will be output to a .vm file
int lastArgsCounter = 0;
char subroutineCommands[99999];
char auxSubrCommands[99999];
int codegenIndex = 0;
int codegenIndexClass = 0; // will always be less than codegenIndex
int labelCounter = 0;
int codegenIndexStd = 0;
int codegenIndexClassStd = 0;

/** ----------------- Parser Iteration Logic ----------------- **/
ParserInfo error(Token t, ParserInfo pi);

/** ----------------- Code Gen Logic ----------------- **/
void addCommandToVM(char *vmCode, char *command); // adds a command to the vm code variable

/** ----------------- Grammar Rules ----------------- **/
/**************** CLASS GRAMMAR ****************/
ParserInfo classDeclar(); // classDeclar -> class identifier { {memberDeclar} }
ParserInfo memberDeclar(); // memberDeclar -> classVarDeclar | subroutineDeclar
ParserInfo classVarDeclar(); // (static | field) type identifier {, identifier} ;
ParserInfo type(); // type -> int | char | boolean | identifier
ParserInfo subroutineDeclar(); // (constructor | function | method) (type|void) identifier (paramList) subroutineBody
ParserInfo paramList(); // type identifier {, type identifier} | ε
ParserInfo subroutineBody(); // { {statement} }

/**************** STATEMENT GRAMMAR ****************/
ParserInfo statement(); // statement -> varDeclarStatement | letStatemnt | ifStatement | whileStatement | doStatement | returnStatemnt
ParserInfo varDeclarStatement(); // var type identifier { , identifier } ;
ParserInfo letStatemnt(); // let identifier [ [ expression ] ] = expression ;
ParserInfo ifStatement(); // if ( expression ) { {statement} } [else { {statement} }]
ParserInfo whileStatement(); // while ( expression ) { {statement} }
ParserInfo doStatement(); // do subroutineCall ;
ParserInfo subroutineCall(); // identifier [ . identifier ] ( expressionList )
ParserInfo expressionList(); // expression { , expression } | ε
ParserInfo returnStatemnt(); // return [ expression ] ;

/**************** EXPRESSION GRAMMAR ****************/
ParserInfo expression(); // relationalExpression { ( & | | ) relationalExpression }
ParserInfo relationalExpression(); // ArithmeticExpression { ( = | > | < ) ArithmeticExpression }
ParserInfo ArithmeticExpression(); // term { ( + | - ) term }
ParserInfo term(); // factor { ( * | / ) factor }
ParserInfo factor(); // ( - | ~ | ε ) operand
ParserInfo operand(); // integerConstant | identifier [.identifier ] [ [ expression ] | ( expressionList ) ] | (expression) | stringLiteral | true | false | null | this

/** ----------------- UTILS ----------------- **/
int lexerError(Token t) {
  if ((int)t.tp == 6) {
    return 1;
  }
  return 0;
}
void printParserStage(char *stage, Token t) {
  printf("%s, < line %d, lexeme %s >\n", stage, t.ln, t.lx);
}
ParserInfo error(Token t, ParserInfo pi) {
  switch (pi.er) {
    case classExpected:
      printf("Line %d: keyword class expected\n", t.ln);
      break;
    case idExpected:
      printf("Line %d: identifier expected\n", t.ln);
      break;
    case openBraceExpected:
      printf("Line %d: { expected\n", t.ln);
      break;
    case closeBraceExpected:
      printf("Line %d: } expected\n", t.ln);
      break;
    case memberDeclarErr:
      printf("Line %d: class member declaration must begin with static, field, constructor , function , or method\n", t.ln);
      break;
    case classVarErr:
      printf("Line %d : class variables must begin with field or static\n", t.ln);
      break;    
    case illegalType:
      printf("Line %d : a type must be int, char, boolean, or identifier\n", t.ln);
      break; 
    case semicolonExpected:
      printf("Line %d : ; expected\n", t.ln);
      break; 
    case subroutineDeclarErr:
      printf("Line %d : subrouting declaration must begin with constructor, function, or method\n", t.ln);
      break; 
    case openParenExpected:
      printf("Line %d : ( expected\n", t.ln);
      break; 
    case closeParenExpected:
      printf("Line %d : ) expected\n", t.ln);
      break;  
    case equalExpected:
      printf("Line %d : = expected\n", t.ln);
      break;   
    case syntaxError:
      printf("Line %d : syntax error\n", t.ln);
      break;
    case undecIdentifier:
      printf("Line %d : undeclared identifier\n", t.ln);
      break;
    case redecIdentifier:
      printf("Line %d : redeclared identifier\n", t.ln);
      break;
    default:
      printf("Line %d : unrecognised error\n", t.ln);
      break;
  }
  return pi;
}

/** ----------------- Code Gen Logic Implementation ----------------- **/
void addCommandToVM(char *vmCode, char *command) {
  if (!strlen(vmCode)) {
    memset(vmCode, '\0', 1024); // init code string
  }
  strcat(vmCode, command);

  if (vmCode[strlen(vmCode) - 1] == '\n') {
    return;
  }

  strcat(vmCode, "\n"); // add a newline (.vm aesthetic purposes) if it doesn't have one already
}

int InitParser (char* file_name)
{ 
  strcpy(fileName, file_name); // have a copy of the filename
  scopeCounter = 0;
	int lexerInitResult = InitLexer(file_name);
	if (!lexerInitResult) return 0;
	else return 1;
}

ParserInfo Parse ()
{

	ParserInfo pi;

  pi = classDeclar();
  if (pi.er != none) {
    return error(pi.tk, pi);
  }
  scopeCounter--; // exited class declar scope

	pi.er = none;
	return pi;

}

int StopParser ()
{
	StopLexer();
  memset(fileName, '\0', sizeof fileName);
  scopeCounter = 0;
	return 1;
}

/** Implementations of Prototypes **/
ParserInfo classDeclar() {
  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("classDeclar", t);
  if (strcmp(t.lx, "class") == 0) {
    t = GetNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (t.tp == ID) {

      // init symbol table for class scope - index 1 if on reading pass
      if (globalPass == 1) {

        scopeCounter++;
        counter++;
        previousClassCounter = counter;

        codegenIndexStd++;
        codegenIndexClassStd = codegenIndexStd;

        InitSymbolTable(tables[counter]);
        strcpy(tables[counter].className, t.lx);

        initialCounter = 60; // will only be accessed during second pass
        initialClassCounter = 60; // class on second pass

        codegenIndex = 60; // used during code gen pass on source
        codegenIndexClass = 60;

      }
      else if (globalPass == 2) {
        initialCounter++; // will only be accessed during second pass
        initialClassCounter = initialCounter; // class on second pass
      }
      else if (globalPass == 3) { // on pass 3
        codegenIndex++; // for codegen the tables will start processing from this index
        codegenIndexClass = codegenIndex;
      }

      // init 'this' symbol (arg) for later use
      strcpy(thisSymbol.name, "this");
      strcpy(thisSymbol.type, t.lx);
      thisSymbol.kind = ARGUMENT;
      thisSymbol.offset = 0;

      t = GetNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (strcmp(t.lx, "{") == 0) {
        t = PeekNextToken();
        if (lexerError(t)) {
          pi.tk = t;
          pi.er = lexerErr;
          return pi;
        }
        while (strcmp(t.lx, "static") == 0 || strcmp(t.lx, "field") == 0 || strcmp(t.lx, "constructor") == 0 || strcmp(t.lx, "function") == 0 || strcmp(t.lx, "method") == 0) {
          ParserInfo memberDeclarPi = memberDeclar();
          if (memberDeclarPi.er != none) {
            return error(t, memberDeclarPi);
          }
          t = PeekNextToken();
          if (lexerError(t)) {
            pi.tk = t;
            pi.er = lexerErr;
            return pi;
          }
          if (strcmp(t.lx, ";") == 0) {
            GetNextToken();
            t = PeekNextToken();
          }
        }
        t = PeekNextToken();
        if (strcmp(t.lx, "}") != 0) {
          pi.er = closeBraceExpected;
          pi.tk = t;
          return pi;
        }
        else {
          // printParserStage("END OF PARSING", t);
          pi.tk = t;
          pi.er = none;
          return pi;
        }
      }
      else {
        pi.tk = t;
        pi.er = openBraceExpected;
        return pi;
      }
    }
    else {
    pi.tk = t;
    pi.er = idExpected;
    return pi;
    }
  }
  else {
    pi.tk = t;
    pi.er = classExpected;
    return pi;
  }
}
ParserInfo memberDeclar() {
  ParserInfo pi;
  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("memberDeclar", t);
  if ((strcmp(t.lx, "static") == 0 || strcmp(t.lx, "field") == 0) && t.tp == RESWORD) {
    ParserInfo classVarDeclarPi = classVarDeclar();
    if (classVarDeclarPi.er != none) {
      return error(classVarDeclarPi.tk, classVarDeclarPi);
    }
    return classVarDeclarPi;
  }
  else if ((strcmp(t.lx, "constructor") == 0 || strcmp(t.lx, "function") == 0 || strcmp(t.lx, "method") == 0) && t.tp == RESWORD) {

    // add function vm code here
    if (globalPass == 3 || standardPass == 1) {
      strcpy(subroutineCommands, "");
      strcpy(auxSubrCommands, ""); 
      strcpy(subroutineCommands, "function ");
    }

    ParserInfo subroutineDeclarPi = subroutineDeclar();
    if (subroutineDeclarPi.er != none) {
      return error(subroutineDeclarPi.tk, subroutineDeclarPi);
    }

    if (globalPass == 3 || standardPass == 1) {

      // add counter of local variables to vm command
      char localCounter[200];
      strcpy(localCounter, "");
      if (standardPass == 1) {
        sprintf(localCounter, "%d\n", tables[codegenIndexStd].numberOfLocalVariables);
      }
      else {
        sprintf(localCounter, "%d\n", tables[codegenIndex].numberOfLocalVariables);
      }
      strcat(subroutineCommands, localCounter);

      strcat(subroutineCommands, auxSubrCommands);

      addCommandToVM(vmCode, subroutineCommands);

      strcpy(subroutineCommands, ""); // clean subroutine buffer
      strcpy(auxSubrCommands, ""); // clean aux subroutine buffer

    }

    scopeCounter--; // exit this scope
    return subroutineDeclarPi;
  }
  else {
    pi.tk = t;
    pi.er = memberDeclarErr;
    return pi;
  }
}
ParserInfo type() {
  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.er = lexerErr;
    pi.tk = t;
    return pi;
  }
  // printParserStage("type", t);
  if (strcmp(t.lx, "int") == 0 && t.tp == RESWORD) {
    pi.er = none;
    pi.tk = t;
    return pi;
  }
  else if (strcmp(t.lx, "char") == 0 && t.tp == RESWORD) {
    pi.er = none;
    pi.tk = t;
    return pi;
  }
  else if (strcmp(t.lx, "boolean") == 0 && t.tp == RESWORD) {
    pi.er = none;
    pi.tk = t;
    return pi;
  }
  else if (t.tp == ID) {
    if (globalPass == 2) { // semantic check
      int ok = 0;
      for (int i = 0; i <= counter; ++i) {
        if (strcmp(tables[i].functionName, t.lx) == 0 || strcmp(tables[i].className, t.lx) == 0) {
          ok = 1;
          break;
        }
      }
      if (!ok) {
          pi.er = undecIdentifier;
          pi.tk = t;
          return pi;
        } 
    }
    pi.er = none;
    pi.tk = t;
    return pi;
  }
  else {
    pi.er = illegalType;
    pi.tk = t;
    return pi;
  }
}
ParserInfo classVarDeclar() {

  ParserInfo pi;
  Token t = GetNextToken();

  if (lexerError(t)) {
    pi.er = lexerErr;
    pi.tk = t;
    return pi;
  }

  // store whether static or field
  char initLexeme[128];
  strcpy(initLexeme, t.lx);
  int isStatic = 0; // 1 if true -- memorising answer so we don't have to compute strcmp at each step of an id 

  if (strcmp(t.lx, "static") == 0) {
    isStatic = 1;
  }
  
  // printParserStage("classVarDeclar", t);
  if (isStatic || strcmp(t.lx, "field") == 0) {
    ParserInfo typePi = type();
    if (typePi.er != none) {
      return error(typePi.tk, typePi);
    }
    t = GetNextToken();
    if (lexerError(t)) {
      pi.er = lexerErr;
      pi.tk = t;
      return pi;
    }
    if (t.tp == ID) {

      // add symbol to table here if not declared
      if (globalPass == 1) {
        if (FindSymbol(tables[previousClassCounter], t.lx) >= 0) { // redeclared field | static
          pi.tk = t;
          pi.er = redecIdentifier;
          return pi;
        }
        else {
          if (isStatic) {
            AddSymbol(&tables[previousClassCounter], t.lx, typePi.tk.lx, STATIC);
          }
          else {
            AddSymbol(&tables[previousClassCounter], t.lx, typePi.tk.lx, FIELD);
          }
        }
      }

      t = PeekNextToken();
      if (lexerError(t)) {
        pi.er = lexerErr;
        pi.tk = t;
        return pi;
      }
      if (strcmp(t.lx, ";") == 0) {
        GetNextToken();
        pi.er = none;
        pi.tk = t;
        return pi;
      }
      while (strcmp(t.lx, ",") == 0) { // if there is a parameter list
        GetNextToken();
        t = PeekNextToken();
        if (lexerError(t)) {
          pi.er = lexerErr;
          pi.tk = t;
          return pi;
        }
        if (t.tp == ID) { // found ID

          // add symbol to table here
          if (globalPass == 1) {
            if (FindSymbol(tables[previousClassCounter], t.lx) >= 0) { // redeclared field | static
              pi.tk = t;
              pi.er = redecIdentifier;
              return pi;
            }
            else {
              if (isStatic) {
                AddSymbol(&tables[previousClassCounter], t.lx, typePi.tk.lx, STATIC);
              }
              else {
                AddSymbol(&tables[previousClassCounter], t.lx, typePi.tk.lx, FIELD);
              }
            }
          }

          GetNextToken();
          t = PeekNextToken();
          if (lexerError(t)) {
            pi.er = lexerErr;
            pi.tk = t;
            return pi;
          }
          if (strcmp(t.lx, ";") == 0) { // end of identifiers list
            pi.er = none;
            pi.tk = t;
            return pi;
          }
          else if (strcmp(t.lx, ",") == 0)
            ; // nothing here, continue loop
          else { // ID in id list not followed by , or ;
            pi.er = syntaxError;
            pi.tk = t;
            return pi;
          }
        }
        else {
          pi.er = idExpected;
          pi.tk = t;
          return pi;
        }
      }
    }
    else {
      pi.er = idExpected;
      pi.tk = t;
      return pi;
    }
  }
  else {
    pi.er = classVarErr;
    pi.tk = t;
    return pi;
  }
}
ParserInfo subroutineDeclar() {
  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.er = lexerErr;
    pi.tk = t;
    return pi;
  }
  // printParserStage("subroutineDeclar", t);
  if (strcmp("constructor", t.lx) == 0 || strcmp("function", t.lx) == 0 || strcmp("method", t.lx) == 0) {

    int subroutineIsMethod = (strcmp("method", t.lx) == 0) ? 1 : 0;

    t = PeekNextToken();
    if (lexerError(t)) {
      pi.er = lexerErr;
      pi.tk = t;
      return pi;
    }

    if (globalPass == 1) {

      previousClassCounter++; // enter function/method/constructor scope
      counter++; // increase index on method, constructor, function encounter

      if (standardPass == 1) {
        codegenIndexStd++;
      }

      if (subroutineIsMethod) { // only add this to methods ?
        AddSymbol(&tables[counter], "this\0", thisSymbol.type, thisSymbol.kind); // add 'this' symbol
      }

    }
    else if (globalPass == 2) {
      initialCounter++;
    }
    else if (globalPass == 3) {
      codegenIndex++;
    }

    if (strcmp("void", t.lx) == 0) { // void type subroutine
      t = GetNextToken(); // on Peek (= void) here
      t = GetNextToken(); // on (Peek + 1) here
      if (lexerError(t)) {
        pi.er = lexerErr;
        pi.tk = t;
        return pi;
      }
      if (t.tp == ID) {

        if (globalPass == 1) {
          // init table with more useful data for code gen phase
          tables[counter].isMethod = subroutineIsMethod;
          tables[counter].numberOfLocalVariables = 0;
          strcpy(tables[counter].functionName, t.lx);
          strcpy(tables[counter].parentClass, thisSymbol.type);
        }

        if (globalPass == 3 || standardPass == 1) { // code gen for function declar

          if (subroutineIsMethod) {
            // set 'this' to point to the passed obj if method
            strcat(auxSubrCommands, " push argument 0\n");
            strcat(auxSubrCommands, " pop pointer 0\n");
          }

          // add function name to subroutine specific commands
          strcat(subroutineCommands, thisSymbol.type);
          strcat(subroutineCommands, ".");
          strcat(subroutineCommands, t.lx);
          strcat(subroutineCommands, " ");

        }

        t = GetNextToken();
        if (lexerError(t)) {
          pi.er = lexerErr;
          pi.tk = t;
          return pi;
        }
        if (strcmp(t.lx, "(") == 0) {
          ParserInfo paramListPi = paramList();
          if (paramListPi.er != none) {
            return error(paramListPi.tk, paramListPi);
          }
          t = GetNextToken();
          if (lexerError(t)) {
            pi.er = lexerErr;
            pi.tk = t;
            return pi;
          }
          if (strcmp(t.lx, ")") == 0) {
            t = PeekNextToken(); // should be {
            if (lexerError(t)) {
              pi.er = lexerErr;
              pi.tk = t;
              return pi;
            }
            ParserInfo subroutineBodyPi = subroutineBody();
            if (subroutineBodyPi.er != none) {
              return error(subroutineBodyPi.tk, subroutineBodyPi);
            }

            pi.er = none;
            pi.tk = t;
            return pi;
          }
          else {
            pi.er = closeParenExpected;
            pi.tk = t;
            return pi;
          }
        }
        else {
          pi.er = openParenExpected;
          pi.tk = t;
          return pi;
        }
      }
      else {
        pi.er = idExpected;
        pi.tk = t;
        return pi;
      }
    }
    else { // type other than void
      ParserInfo typePi = type();
      if (typePi.er != none) {
        return error(typePi.tk, typePi);
      }
      t = GetNextToken();
      if (lexerError(t)) {
        pi.er = lexerErr;
        pi.tk = t;
        return pi;
      }
      if (t.tp == ID) {

        if (globalPass == 1) {
          // init table with more useful data for code gen phase
          tables[counter].isMethod = subroutineIsMethod;
          tables[counter].numberOfLocalVariables = 0;
          strcpy(tables[counter].functionName, t.lx);
          strcpy(tables[counter].parentClass, thisSymbol.type);
        }
        
        if (globalPass == 3 || standardPass == 1) { // code gen for function declar

          if (subroutineIsMethod) {
            // set 'this' to point to the passed obj if method
            strcat(auxSubrCommands, " push argument 0\n");
            strcat(auxSubrCommands, " pop pointer 0\n");
          }
          
          // add function name to subroutine specific commands
          strcat(subroutineCommands, thisSymbol.type);
          strcat(subroutineCommands, ".");
          strcat(subroutineCommands, t.lx);
          strcat(subroutineCommands, " ");

        }

        t = GetNextToken();
        if (lexerError(t)) {
          pi.er = lexerErr;
          pi.tk = t;
          return pi;
        }
        if (strcmp(t.lx, "(") == 0 && t.tp == SYMBOL) {
          ParserInfo paramListPi = paramList();
          if (paramListPi.er != none) {
            return error(paramListPi.tk, paramListPi);
          }
          t = GetNextToken();
          if (lexerError(t)) {
            pi.er = lexerErr;
            pi.tk = t;
            return pi;
          }
          if (strcmp(t.lx, ")") == 0) {
            ParserInfo subroutineBodyPi = subroutineBody();
            if (subroutineBodyPi.er != none) {
              return error(subroutineBodyPi.tk, subroutineBodyPi);
            }
            pi.er = none;
            pi.tk = t;
            return pi;
          }
          else {
            pi.er = closeParenExpected;
            pi.tk = t;
            return pi;
          }
        }
        else {
          pi.er = openParenExpected;
          pi.tk = t;
          return pi;
        }
      }
      else {
        pi.er = idExpected;
        pi.tk = t;
        return pi;
      }
    }
  }
  else {
    pi.er = subroutineDeclarErr;
    pi.tk = t;
    return pi;
  }
}
ParserInfo paramList() {
  ParserInfo pi;
  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.er = lexerErr;
    pi.tk = t;
    return pi;
  }

  // printParserStage("paramList", t);
  if (strcmp(t.lx, "int") == 0 || strcmp(t.lx, "boolean") == 0 || strcmp(t.lx, "char") == 0 || t.tp == ID) { // has arguments
    ParserInfo typePi = type();
    if (typePi.er != none) {
      return error(typePi.tk, typePi);
    }
    t = GetNextToken(); // on type + 1 = ID
    if (lexerError(t)) {
      pi.er = lexerErr;
      pi.tk = t;
      return pi;
    }
    if (t.tp == ID) {

      // add symbol here -- in paramList -> ARGUMENT
      if (globalPass == 1) {
        AddSymbol(&tables[counter], t.lx, typePi.tk.lx, ARGUMENT);
      }

      t = PeekNextToken();
      if (lexerError(t)) {
        pi.er = lexerErr;
        pi.tk = t;
        return pi;
      }
      while (strcmp(t.lx, ",") == 0) {
        GetNextToken(); // eat token
        ParserInfo typeListPi = type();
        if (typeListPi.er != none) {
          return error(t, typeListPi);
        }
        t = PeekNextToken();
        if (lexerError(t)) {
          pi.er = lexerErr;
          pi.tk = t;
          return pi;
        }     
        if (t.tp == ID) {
          
          // add symbol here -- in paramList -> ARGUMENT
          if (globalPass == 1) {
            if (FindSymbol(tables[counter], t.lx) >= 0) {
              pi.er = redecIdentifier;
              pi.tk = t;
              return pi;
            }
            else {
              AddSymbol(&tables[counter], t.lx, typeListPi.tk.lx, ARGUMENT);
            }
          }

          GetNextToken();
          t = PeekNextToken();
          if (lexerError(t)) {
            pi.er = lexerErr;
            pi.tk = t;
            return pi;
          }
        }
        else {
          pi.er = idExpected;
          pi.tk = t;
          return pi;
        }   
      }
      pi.er = none;
      pi.tk = t;
      return pi;
    }
    else {
      pi.er = idExpected;
      pi.tk = t;
      return pi;
    }
  }
  // reaches this for empty paramList
  pi.er = none;
  pi.tk = t;
  return pi;
}
ParserInfo subroutineBody() {
  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.er = lexerErr;
    pi.tk = t;
    return pi;
  }
  // printParserStage("subroutineBody", t);
  if (strcmp(t.lx, "{") == 0) {
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.er = lexerErr;
      pi.tk = t;
      return pi;
    }
    while (strcmp(t.lx, "var") == 0 || strcmp(t.lx, "let") == 0 || strcmp(t.lx, "if") == 0 || strcmp(t.lx, "while") == 0 || strcmp(t.lx, "do") == 0 || strcmp(t.lx, "return") == 0) {
      ParserInfo statementPi = statement();
      if (statementPi.er != none) {
        return error(t, statementPi);
      }
      t = PeekNextToken();
      if (lexerError(t)) {
        pi.er = lexerErr;
        pi.tk = t;
        return pi;
      }
    }
    t = GetNextToken();
    if (strcmp(t.lx, "}") == 0) {
      pi.er = none;
      pi.tk = t;
      return pi;
    }
    else {
      pi.er = closeBraceExpected;
      pi.tk = t;
      return pi;
    }
  }
  else {
    pi.er = openBraceExpected;
    pi.tk = t;
    return pi;
  }
}
ParserInfo statement() {
  ParserInfo pi;
  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.er = lexerErr;
    pi.tk = t;
    return pi;
  }
  // printParserStage("statement", t);
  if (strcmp(t.lx, "var") == 0) { // varDeclarStatement
    pi = varDeclarStatement();
    if (pi.er != none) {
      return error(pi.tk, pi);
    }
    return pi;
  }
  else if (strcmp(t.lx, "let") == 0) { // letStatement
    pi = letStatemnt();
    if (pi.er != none) {
      return error(pi.tk, pi);
    }
    return pi;
  }
  else if (strcmp(t.lx, "if") == 0) { // ifStatement
    pi = ifStatement();
    if (pi.er != none) {
      return error(pi.tk, pi);
    }
    return pi;
  }
  else if (strcmp(t.lx, "while") == 0) { // whileStatement
    pi = whileStatement();
    if (pi.er != none) {
      return error(pi.tk, pi);
    }
    return pi;
  }
  else if (strcmp(t.lx, "do") == 0) { // doStatement
    pi = doStatement();
    if (pi.er != none) {
      return error(pi.tk, pi);
    }
    return pi;
  }
  else if (strcmp(t.lx, "return") == 0) { // returnStatemnt
    pi = returnStatemnt();
    if (pi.er != none) {
      return error(pi.tk, pi);
    }
    return pi;
  }
  else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }
}
ParserInfo varDeclarStatement() {
  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("varDeclarStatement", t);
  if (strcmp(t.lx, "var") == 0) {
    ParserInfo typePi = type();
    if (typePi.er != none) {
      return error(t, typePi);
    }
    t = GetNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (t.tp == ID) {

      // add symbol here -- varDeclar -> VAR
      if (globalPass == 1) {
        if (FindSymbol(tables[counter], t.lx) >= 0) { // redeclared field | static
          pi.tk = t;
          pi.er = redecIdentifier;
          return pi;
        }
        else {
          AddSymbol(&tables[counter], t.lx, typePi.tk.lx, VAR);
        }
      }

      t = PeekNextToken();
      if (lexerError(t)) {
        pi.er = lexerErr;
        pi.tk = t;
        return pi;
      }
      // end of declaration
      if (strcmp(t.lx, ";") == 0) {
        GetNextToken();
        pi.er = none;
        pi.tk = t;
        return pi;
      }
      // check for list of var declarations
      while (strcmp(t.lx, ",") == 0) { // if there is a parameter list
        GetNextToken();
        t = PeekNextToken();
        if (lexerError(t)) {
          pi.er = lexerErr;
          pi.tk = t;
          return pi;
        }
        if (t.tp == ID) { // found ID

          // add symbol here -- in varDeclar -> VAR
          if (globalPass == 1) {
            if (FindSymbol(tables[counter], t.lx) >= 0) { // redeclared field | static
              pi.tk = t;
              pi.er = redecIdentifier;
              return pi;
            }
            else {
              AddSymbol(&tables[counter], t.lx, typePi.tk.lx, VAR);
            }
          }

          GetNextToken();
          t = PeekNextToken();
          if (lexerError(t)) {
            pi.er = lexerErr;
            pi.tk = t;
            return pi;
          }
          if (strcmp(t.lx, ";") == 0) { // end of identifiers list
            GetNextToken();
            pi.er = none;
            pi.tk = t;
            return pi;
          }
          else if (strcmp(t.lx, ",") == 0)
            ; // nothing here, continue loop
          else { // ID in id list not followed by , or ;
            pi.er = syntaxError;
            pi.tk = t;
            return pi;
          }
        }
        else {
          pi.er = idExpected;
          pi.tk = t;
          return pi;
        }
      }    
    }
    else {
      pi.er = idExpected;
      pi.tk = t;
      return pi;
    }
  }
  else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }
}
ParserInfo letStatemnt() {

  Token lht;
  int isIndexed = 0;

  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("letStatement", t);
  if (strcmp(t.lx, "let") == 0) {
    t = GetNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (t.tp == ID) {

      // check if variable is declared here (if not => error) (in a let statement)
      if (globalPass == 2) {
        if (FindSymbol(tables[initialCounter], t.lx) < 0 && FindSymbol(tables[initialClassCounter], t.lx) < 0) {
          pi.tk = t;
          pi.er = undecIdentifier;
          return pi;
        }
      }

      lht = t;

      t = PeekNextToken();
      if (lexerError(t)) {
        pi.er = lexerErr;
        pi.tk = t;
        return pi;
      }
      if (strcmp(t.lx, "[") == 0) { // const for indexed

        // code gen for indexed access - variable (before indexing)
        if (globalPass == 3 || standardPass == 1) {

          isIndexed = 1;

          char lhbi[200];
          strcpy(lhbi, "");

          int resultMethod;
          int resultClass;
          if (standardPass == 1) {
            resultMethod = FindSymbol(tables[codegenIndexStd], lht.lx);
            resultClass = FindSymbol(tables[codegenIndexClassStd], lht.lx);
          }
          else {
            resultMethod = FindSymbol(tables[codegenIndex], lht.lx);
            resultClass = FindSymbol(tables[codegenIndexClass], lht.lx);
          }

          if (resultMethod >= 0) { // var is declared in method
            if (standardPass == 1) {
              if (tables[codegenIndexStd].symbols[resultMethod].kind == ARGUMENT) {
                sprintf(lhbi, " push argument %d\n", tables[codegenIndexStd].symbols[resultMethod].offset); 
              }
              else if (tables[codegenIndexStd].symbols[resultMethod].kind == VAR) {
                sprintf(lhbi, " push local %d\n", tables[codegenIndexStd].symbols[resultMethod].offset); 
              }
            }
            else {
              if (tables[codegenIndex].symbols[resultMethod].kind == ARGUMENT) {
                sprintf(lhbi, " push argument %d\n", tables[codegenIndex].symbols[resultMethod].offset); 
              }
              else if (tables[codegenIndex].symbols[resultMethod].kind == VAR) {
                sprintf(lhbi, " push local %d\n", tables[codegenIndex].symbols[resultMethod].offset); 
              }
            }
          }
          else if (resultClass >= 0) { // var is declared in class
            if (standardPass == 1) {
              if (tables[codegenIndexClassStd].symbols[resultMethod].kind == FIELD) {
                sprintf(lhbi, " push this %d\n", tables[codegenIndexClassStd].symbols[resultMethod].offset); 
              }
              else if (tables[codegenIndexClassStd].symbols[resultMethod].kind == STATIC) {
                sprintf(lhbi, " push static %d\n", tables[codegenIndexClassStd].symbols[resultMethod].offset); 
              }
            }
            else {
              if (tables[codegenIndexClass].symbols[resultMethod].kind == FIELD) {
                sprintf(lhbi, " push this %d\n", tables[codegenIndexClass].symbols[resultMethod].offset); 
              }
              else if (tables[codegenIndexClass].symbols[resultMethod].kind == STATIC) {
                sprintf(lhbi, " push static %d\n", tables[codegenIndexClass].symbols[resultMethod].offset); 
              }
            }
          }

          strcat(auxSubrCommands, lhbi);

        }

        GetNextToken();

        ParserInfo expressionPi = expression();
        if (expressionPi.er != none) {
          return error(expressionPi.tk, expressionPi);
        }
        t = GetNextToken(); // should be ]
        if (lexerError(t)) {
          pi.tk = t;
          pi.er = lexerErr;
          return pi;
        }
        if (strcmp(t.lx, "]") != 0) {
          pi.er = closeBracketExpected;
          pi.tk = t;
          return pi;
        }
        
        // code gen for indexed access - variable (add indexing)
        if (globalPass == 3 || standardPass == 1) {
          strcat(auxSubrCommands, " add\n");
          strcat(auxSubrCommands, " pop pointer 1\n");
        }

        t = PeekNextToken(); // should be =
        if (lexerError(t)) {
          pi.tk = t;
          pi.er = lexerErr;
          return pi;
        }
      }
      if (strcmp(t.lx, "=") == 0) { // executes after checking for index
        GetNextToken();
        ParserInfo expressionPi = expression();
        if (expressionPi.er != none) {
          return error(expressionPi.tk, expressionPi);
        }
        t = GetNextToken();
        if (lexerError(t)) {
          pi.tk = t;
          pi.er = lexerErr;
          return pi;
        }
        if (strcmp(t.lx, ";") == 0) { // finished declaration

          // generate vm code for let assignment
          if (globalPass == 3 || standardPass == 1) {
            // vm let assignment rhs code
            char lh[200];
            strcpy(lh, "");

            int resultMethod;
            int resultClass;
            if (standardPass == 1) {
              resultMethod = FindSymbol(tables[codegenIndexStd], lht.lx);
              resultClass = FindSymbol(tables[codegenIndexClassStd], lht.lx);
            }
            else {
              resultMethod = FindSymbol(tables[codegenIndex], lht.lx);
              resultClass = FindSymbol(tables[codegenIndexClass], lht.lx);
            }

            if (isIndexed) { // pointer
              strcat(auxSubrCommands, " pop that 0\n");
            }
            else { // other
              if (resultMethod >= 0) { // var is declared in method
                if (standardPass == 1) {
                  if (tables[codegenIndexStd].symbols[resultMethod].kind == VAR) {
                    sprintf(lh, " pop local %d\n", tables[codegenIndexStd].symbols[resultMethod].offset); 
                  }
                  else if (tables[codegenIndexStd].symbols[resultMethod].kind == ARGUMENT) {
                    sprintf(lh, " pop argument %d\n", tables[codegenIndexStd].symbols[resultMethod].offset); 
                  }
                }
                else {
                  if (tables[codegenIndex].symbols[resultMethod].kind == VAR) {
                    sprintf(lh, " pop local %d\n", tables[codegenIndex].symbols[resultMethod].offset); 
                  }
                  else if (tables[codegenIndex].symbols[resultMethod].kind == ARGUMENT) {
                    sprintf(lh, " pop argument %d\n", tables[codegenIndex].symbols[resultMethod].offset); 
                  }
                }
              }
              else if (resultClass >= 0) { // var is declared in class
                if (tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[resultClass].kind == STATIC) {
                  sprintf(lh, " pop static %d\n", tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[resultClass].offset); 
                }
                else if (tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[resultClass].kind == FIELD)
                  sprintf(lh, " pop this %d\n", tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[resultClass].offset); 
              }
            }

            strcat(auxSubrCommands, lh);
          }

          pi.er = none;
          pi.tk = t;
          return pi;
        }
        else {
          pi.er = semicolonExpected;
          pi.tk = t;
          return pi;
        }
      }
      else { // not =
        pi.er = equalExpected;
        pi.tk = t;
        return pi;
      } 
    }
    else {
      pi.er = idExpected;
      pi.tk = t;
      return pi;
    }
  }
  else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }
}
ParserInfo ifStatement() {
  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("ifStatement", t);
  if (strcmp(t.lx, "if") == 0) {
    t = GetNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (strcmp(t.lx, "(") == 0) {
      ParserInfo expressionPi = expression();
      if (expressionPi.er != none) {
        return error(expressionPi.tk, expressionPi);
      }
      t = GetNextToken(); // should be )
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (strcmp(t.lx, ")") != 0) {
        pi.er = closeParenExpected;
        pi.tk = t;
        return pi;
      }

      // vm code gen for if -- HEADER
      if (globalPass == 3 || standardPass == 1) {
        if (strlen(auxSubrCommands)) {

          strcat(auxSubrCommands, " not\n");

          labelCounter++;

          char ifGotoCommand[200];
          strcpy(ifGotoCommand, "");

          sprintf(ifGotoCommand, " if-goto L%d\n", labelCounter);
          strcat(auxSubrCommands, ifGotoCommand);

        }
     }

      t = GetNextToken();
      // begin if body
      if (strcmp(t.lx, "{") == 0) {
        t = PeekNextToken(); 
        if (lexerError(t)) {
          pi.er = lexerErr;
          pi.tk = t;
          return pi;
        }
        while (strcmp(t.lx, "var") == 0 || strcmp(t.lx, "let") == 0 || strcmp(t.lx, "if") == 0 || strcmp(t.lx, "while") == 0 || strcmp(t.lx, "do") == 0 || strcmp(t.lx, "return") == 0) {
          ParserInfo statementPi = statement();
          if (statementPi.er != none) {
            return error(statementPi.tk, statementPi);
          }
          t = PeekNextToken();
          if (lexerError(t)) {
            pi.er = lexerErr;
            pi.tk = t;
            return pi;
          }
        }

        // vm code gen for if -- END OF PRIMARY EXEC IF CONDITION IS MET
        if (globalPass == 3 || standardPass == 1) {
          if (strlen(auxSubrCommands)) {

            char ifGotoCommand[200];
            strcpy(ifGotoCommand, "");

            sprintf(ifGotoCommand, " goto L%d\n", labelCounter + 1);
            strcat(auxSubrCommands, ifGotoCommand);

            strcpy(ifGotoCommand, "");

            sprintf(ifGotoCommand, "label L%d\n", labelCounter);
            strcat(auxSubrCommands, ifGotoCommand);

          }
        }

        t = GetNextToken();
        if (lexerError(t)) {
          pi.tk = t;
          pi.er = lexerErr;
          return pi;
        }
        // end of if body
        if (strcmp(t.lx, "}") == 0) {
          // test for else body
          t = PeekNextToken();
          if (lexerError(t)) {
            pi.er = lexerErr;
            pi.tk = t;
            return pi;
          }
          if (strcmp(t.lx, "else") == 0) {

            // vm code gen for if -- ELSE
            if (globalPass == 3 || standardPass == 1) {
            if (strlen(auxSubrCommands)) {

              char ifGotoCommand[200];
              strcpy(ifGotoCommand, "");

              sprintf(ifGotoCommand, "label L%d\n", labelCounter + 1);
              strcat(auxSubrCommands, ifGotoCommand);

              labelCounter++;

            }
          }

            GetNextToken();

            t = GetNextToken(); // should be {
            if (lexerError(t)) {
              pi.er = lexerErr;
              pi.tk = t;
              return pi;
            }
            if (strcmp(t.lx, "{") == 0) {
              t = PeekNextToken(); // statement parsing
              if (lexerError(t)) {
                pi.er = lexerErr;
                pi.tk = t;
                return pi;
              }
              while (strcmp(t.lx, "var") == 0 || strcmp(t.lx, "let") == 0 || strcmp(t.lx, "if") == 0 || strcmp(t.lx, "while") == 0 || strcmp(t.lx, "do") == 0 || strcmp(t.lx, "return") == 0) {
                ParserInfo statementPi = statement();
                if (statementPi.er != none) {
                  return error(statementPi.tk, statementPi);
                }
                t = PeekNextToken();
                if (lexerError(t)) {
                  pi.er = lexerErr;
                  pi.tk = t;
                  return pi;
                }
              }
              t = GetNextToken(); // end of else body
              if (lexerError(t)) {
                pi.tk = t;
                pi.er = lexerErr;
                return pi;
              }
              if (strcmp(t.lx, "}") == 0) {
                pi.er = none;
                pi.tk = t;
                return pi;
              }
              else {
                pi.er = closeBraceExpected;
                pi.tk = t;
                return pi;
              }
            }
            else {
              pi.er = openBraceExpected;
              pi.tk = t;
              return pi;
            }
          }

          // vm code gen for if -- ENDIF
          if (globalPass == 3 || standardPass == 1) {
            if (strlen(auxSubrCommands)) {

              char ifGotoCommand[200];
              strcpy(ifGotoCommand, "");

              sprintf(ifGotoCommand, "label L%d\n", labelCounter + 1);
              strcat(auxSubrCommands, ifGotoCommand);

              labelCounter++;

            }
          }

          pi.er = none;
          pi.tk = t;
          return pi;
        }
        else {
          pi.er = closeBraceExpected;
          pi.tk = t;
          return pi;
        }  
      }
      else {
        pi.er = openBraceExpected;
        pi.tk = t;
        return pi;
      }
    }
    else {
      pi.er = openParenExpected;
      pi.tk = t;
      return pi;
    }
  }
  else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }
}
ParserInfo whileStatement() {

  ParserInfo pi;
  int labelCounterLocal;

  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("whileStatement", t);
  if (strcmp(t.lx, "while") == 0) {
    t = GetNextToken(); // should be (
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (strcmp(t.lx, "(") == 0) {

      // vm code gen for while loop -- HEADER
      if (globalPass == 3 || standardPass == 1) {
        if (strlen(auxSubrCommands)) {

          labelCounter++;
          labelCounterLocal = labelCounter;

          char whileCommand[200];
          strcpy(whileCommand, "");

          sprintf(whileCommand, "label L%d\n", labelCounter);

          strcat(auxSubrCommands, whileCommand);

        }
      }

      ParserInfo expressionPi = expression();
      if (expressionPi.er != none) {
        return error(expressionPi.tk, expressionPi);
      }
      t = GetNextToken(); // should be )
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (strcmp(t.lx, ")") == 0) {

        // vm code gen for while loop -- AFTER !(cond) PUSH
        if (globalPass == 3 || standardPass == 1) {
          if (strlen(auxSubrCommands)) {

            strcat(auxSubrCommands, " not\n");

            char whileCommand[200];
            strcpy(whileCommand, "");

            sprintf(whileCommand, " if-goto L%d\n", labelCounter + 1);
            strcat(auxSubrCommands, whileCommand);

            labelCounter++;

          }
        }

        t = GetNextToken(); // should be {
        if (lexerError(t)) {
          pi.tk = t;
          pi.er = lexerErr;
          return pi;
        }
        if (strcmp(t.lx, "{") == 0) {
          t = PeekNextToken();
          if (lexerError(t)) {
            pi.tk = t;
            pi.er = lexerErr;
            return pi;
          }
          while (strcmp(t.lx, "var") == 0 || strcmp(t.lx, "let") == 0 || strcmp(t.lx, "if") == 0 || strcmp(t.lx, "while") == 0 || strcmp(t.lx, "do") == 0 || strcmp(t.lx, "return") == 0) {
            ParserInfo statementPi = statement();
            if (statementPi.er != none) {
              return error(statementPi.tk, statementPi);
            }
            t = PeekNextToken();
            if (lexerError(t)) {
              pi.er = lexerErr;
              pi.tk = t;
              return pi;
            }
          }
          t = GetNextToken(); // should be }
          if (lexerError(t)) {
            pi.tk = t;
            pi.er = lexerErr;
            return pi;
          }
          if (strcmp(t.lx, "}") == 0) {

            // vm code gen for while loop
            if (globalPass == 3 || standardPass == 1) {
              if (strlen(auxSubrCommands)) {

                char whileCommand[200];
                strcpy(whileCommand, "");

                sprintf(whileCommand, " goto L%d\n", labelCounterLocal);
                strcat(auxSubrCommands, whileCommand);

                sprintf(whileCommand, "label L%d\n", labelCounterLocal + 1);
                strcat(auxSubrCommands, whileCommand);

                labelCounter++;

              }
            }

            pi.er = none;
            pi.tk = t;
            return pi;
          }
          else {
            pi.er = closeBraceExpected;
            pi.tk = t;
            return pi;
          }
        }
        else {
          pi.er = openBraceExpected;
          pi.tk = t;
          return pi;
        }
      }
      else {
        pi.er = closeParenExpected;
        pi.tk = t;
        return pi;
      }
    }
    else {
      pi.er = openParenExpected;
      pi.tk = t;
      return pi;
    }
  }
}
ParserInfo doStatement() {
  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("doStatement", t);
  if (strcmp(t.lx, "do") == 0) {
    t = PeekNextToken();
    if (t.tp != ID) {
      pi.er = idExpected;
      pi.tk = t;
      return pi;
    }
    ParserInfo subroutineCallPi = subroutineCall();
    if (subroutineCallPi.er != none) {
      return error(subroutineCallPi.tk, subroutineCallPi);
    }
    t = GetNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (strcmp(t.lx, ";") == 0) { // end subroutine call
      pi.er = none;
      pi.tk = t;
      return pi;
    }
    else {
      pi.er = semicolonExpected;
      pi.tk = t;
      return pi;
    }
  }
  else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }
}
ParserInfo subroutineCall() {

  char subroutineFormat[200];
  strcpy(subroutineFormat, "");
  int isMethod = 0;

  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("subroutineCall", t);
  if (t.tp == ID) {

    if (globalPass == 2) { // semantic check

      if (FindSymbol(tables[initialCounter], t.lx) < 0 && FindSymbol(tables[initialClassCounter], t.lx) < 0) {
        int ok = 0;
        for (int i = 0; i <= counter; ++i) {
          if (i == initialCounter || i == initialClassCounter) continue;
          if (strcmp(tables[i].functionName, t.lx) == 0 || strcmp(tables[i].className, t.lx) == 0) { // declared subroutine / class
            ok = 1;
            break;
          }
        }
        if (!ok) {
          pi.er = undecIdentifier;
          pi.tk = t;
          return pi;
        }
      }
      else
        ;
    }

    if (globalPass == 3 || standardPass == 1) { // check whether the called subroutine is in parent class or in other class

      int methodResult;
      int classResult;
      if (standardPass == 1) {
        methodResult = FindSymbol(tables[codegenIndexStd], t.lx);
        classResult = FindSymbol(tables[codegenIndexClassStd], t.lx);
      }
      else {
        methodResult = FindSymbol(tables[codegenIndex], t.lx);
        classResult = FindSymbol(tables[codegenIndexClass], t.lx);
      }
      int filledBeginningSymbol = 0;
      int isSameClass = 0;

      if (methodResult >= 0) { // beginning symbol is local var or argument
        if (tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[methodResult].kind == ARGUMENT) {
          sprintf(subroutineFormat, " push argument %d\n", tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[methodResult].offset);
          strcat(auxSubrCommands, subroutineFormat);
        }
        else if (tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[methodResult].kind == VAR) {
          sprintf(subroutineFormat, " push local %d\n", tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[methodResult].offset);
          strcat(auxSubrCommands, subroutineFormat);
        }
        sprintf(subroutineFormat, " call %s", t.lx); // this ref
        filledBeginningSymbol = 1;
      }
      else if (classResult >= 0) { // beginning symbol is a class level var
        sprintf(subroutineFormat, " push this %d\n", tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[classResult].offset);
        strcat(auxSubrCommands, subroutineFormat);
        sprintf(subroutineFormat, " call %s", t.lx); // this ref
        filledBeginningSymbol = 1;
      }

      for (int i = 0; i <= counter; ++i) {
        if (strcmp(tables[i].functionName, t.lx) == 0) { // declared subroutine / class
          if (strlen(auxSubrCommands)) {
            if (strcmp(thisSymbol.type, tables[i].parentClass) == 0) {
              isMethod = tables[i].isMethod;
              if (strcmp(tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].className, tables[i].parentClass) == 0) {
                strcat(auxSubrCommands, " push pointer 0\n"); // this ref
              }
              if (!filledBeginningSymbol) {
                sprintf(subroutineFormat, " call %s", tables[i].parentClass); // this ref
              }
              strcat(subroutineFormat, ".");
              strcat(subroutineFormat, t.lx);
            }
          }
          break;
        }
        else if (strcmp(tables[i].parentClass, t.lx) == 0) {
          if (strlen(auxSubrCommands)) {
            isMethod = tables[i].isMethod;
            if (!filledBeginningSymbol) {
              sprintf(subroutineFormat, " call %s", tables[i].parentClass); // this ref
            }
          }
          break;
        }
      }
    }

    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    // check for member access (ie: .[identifier])
    if (strcmp(t.lx, ".") == 0) {

      if (globalPass == 3 || standardPass == 1) {
        if (strlen(auxSubrCommands)) {
          strcat(subroutineFormat, ".");
        }
      }

      GetNextToken();
      t = PeekNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (t.tp == ID) {

        if (globalPass == 3 || standardPass == 1) {
          if (strlen(auxSubrCommands)) {
            strcat(subroutineFormat, t.lx);

            // find out if this subroutine is a method
            for (int i = 0; i <= counter; ++i) {
              if (strcmp(tables[i].functionName, t.lx) == 0 && tables[i].isMethod) {
                isMethod = 1;
              }
            }
          }
        }

        if (globalPass == 2) { // semantic check
          int ok = 0;
          for (int i = 0; i <= counter; ++i) {
            if (strcmp(tables[i].functionName, t.lx) == 0) { // undeclared var, subroutine, class
              ok = 1;
              break;
            }
          }
          if (!ok) {
            pi.er = undecIdentifier;
            pi.tk = t;
            return pi;
          } 
        }
        GetNextToken();
      }
      else {
        pi.er = idExpected;
        pi.tk = t;
        return pi;
      }
    }

    t = GetNextToken(); // should be (
    if (strcmp(t.lx, "(") == 0) {
      ParserInfo expressionListPi = expressionList();
      if (expressionListPi.er != none) {
        return error(expressionListPi.tk, expressionListPi);
      }
      t = GetNextToken(); // should be )
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (strcmp(t.lx, ")") == 0) {

        // add number of args to command and add command to vm code
        if (globalPass == 3 || standardPass == 1) {
          if (strlen(auxSubrCommands)) {

            char argsCounter[200];
            strcpy(argsCounter, "");

            if (isMethod) { // if method, push this as well
              lastArgsCounter += 1; 
            }

            sprintf(argsCounter, " %d\n", lastArgsCounter);
            strcat(subroutineFormat, argsCounter);
            strcat(auxSubrCommands, subroutineFormat);

            lastArgsCounter = 0; // reset args counter
          }
        }

        pi.er = none;
        pi.tk = t;
        return pi;
      }
      else {
        pi.er = closeParenExpected;
        pi.tk = t;
        return pi;
      }
    }
    else {
      pi.er = openParenExpected;
      pi.tk = t;
      return pi;
    }
  }
}
ParserInfo expressionList() {
  ParserInfo pi;
  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("expressionList", t);
  if (strcmp(t.lx, "-") == 0 ||
    strcmp(t.lx, "~") == 0 ||
    t.tp == INT ||
    t.tp == STRING ||
    strcmp(t.lx, "true") == 0 ||
    strcmp(t.lx, "false") == 0 ||
    strcmp(t.lx, "null") == 0 ||
    strcmp(t.lx, "this") == 0 ||
    t.tp == ID ||
    strcmp(t.lx, "(") == 0) 
  {
    // expression() here
    ParserInfo expressionPi = expression();
    if (expressionPi.er != none) {
      error(expressionPi.tk, expressionPi);
    }

    // additional argument
    if (globalPass == 3 || standardPass == 1) {
      lastArgsCounter++;
    }

    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    while (strcmp(t.lx, ",") == 0) {
      GetNextToken();
      t = PeekNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      ParserInfo nestedExpressionPi = expression();
      if (nestedExpressionPi.er != none) {
        return error(nestedExpressionPi.tk, nestedExpressionPi);
      }
      
      // additional argument
      if (globalPass == 3 || standardPass == 1) {
        lastArgsCounter++;
      }

      t = PeekNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
    }
  }
  pi.er = none;
  pi.tk = t;
  return pi;
}
ParserInfo returnStatemnt() {
  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("returnStatement", t);
  if (strcmp(t.lx, "return") == 0) {
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (strcmp(t.lx, "-") == 0 ||
    strcmp(t.lx, "~") == 0 ||
    t.tp == INT ||
    t.tp == STRING ||
    strcmp(t.lx, "true") == 0 ||
    strcmp(t.lx, "false") == 0 ||
    strcmp(t.lx, "null") == 0 ||
    strcmp(t.lx, "this") == 0 ||
    t.tp == ID)
    {
      ParserInfo expressionPi = expression();
      if (expressionPi.er != none) {
        return error(expressionPi.tk, expressionPi);
      }
      t = GetNextToken();
      if (strcmp(t.lx, ";") == 0) {

        // return vm command -- with expression here
        if (globalPass == 3 || standardPass == 1) {
          if (strlen(auxSubrCommands)) {
            strcat(auxSubrCommands, " return\n");
          }
        }

        pi.er = none;
        pi.tk = t;
        return pi;
      }
      else {
        pi.er = semicolonExpected;
        pi.tk = t;
        return pi;
      }
    }
    else if (strcmp(t.lx, ";") == 0) {

      // return vm command
      if (globalPass == 3 || standardPass == 1) {
        if (strlen(auxSubrCommands)) {
          strcat(auxSubrCommands, " push constant 0\n");
          strcat(auxSubrCommands, " return\n");
        }
      }

      t = GetNextToken();
      pi.er = none;
      pi.tk = t;
      return pi;
    }
    else {
      pi.er = semicolonExpected;
      pi.tk = t;
      return pi;
    }
  }
  else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }
}
ParserInfo expression() {
  ParserInfo pi;
  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("expression", t);
  ParserInfo relationalExpressionPi = relationalExpression();
  if (relationalExpressionPi.er != none) {
    return error(relationalExpressionPi.tk, relationalExpressionPi);
  }
  t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  while (strcmp(t.lx, "&") == 0 || strcmp(t.lx, "|") == 0) {

    // add vm code
    if (globalPass == 3 || standardPass == 1) {
      if (strcmp(t.lx, "&") == 0) {
        if (strlen(auxSubrCommands)) {
          strcat(auxSubrCommands, " and\n");
        }
        else {
          addCommandToVM(vmCode, "and");
        }
      }
      else if (strcmp(t.lx, "|") == 0) {
        if (strlen(auxSubrCommands)) {
          strcat(auxSubrCommands, " or\n");
        }
        else {
          addCommandToVM(vmCode, "or");
        }
      }
    }

    GetNextToken();
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    ParserInfo chainedRelExprPi = relationalExpression();
    if (chainedRelExprPi.er != none) {
      return error(chainedRelExprPi.tk, chainedRelExprPi);
    }
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
  }
  pi.er = none;
  pi.tk = t;
  return pi;
}
ParserInfo relationalExpression() {

  ParserInfo pi;
  Token savedToken;

  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("relationalExpression", t);
  ParserInfo arithmeticPi = ArithmeticExpression();
  if (arithmeticPi.er != none) {
    return error(arithmeticPi.tk, arithmeticPi);
  }
  t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  while (strcmp(t.lx, "=") == 0 || strcmp(t.lx, ">") == 0 || strcmp(t.lx, "<") == 0) {

    savedToken = t;

    GetNextToken();
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }

    ParserInfo arithmeticChainedPi = ArithmeticExpression();
    if (arithmeticChainedPi.er != none) {
      return error(arithmeticChainedPi.tk, arithmeticChainedPi);
    }

    // add vm code
    if (globalPass == 3 || standardPass == 1) {
      if (strlen(auxSubrCommands)) {
        if (strcmp(savedToken.lx, "=") == 0) {
          strcat(auxSubrCommands, " eq\n");
        }
        else if (strcmp(savedToken.lx, ">") == 0) {
          strcat(auxSubrCommands, " gt\n");
        }
        else if (strcmp(savedToken.lx, "<") == 0) {
          strcat(auxSubrCommands, " lt\n");
        }
      }
      else {
        if (strcmp(savedToken.lx, "=") == 0) {
          addCommandToVM(vmCode, "eq");
        }
        else if (strcmp(savedToken.lx, ">") == 0) {
          addCommandToVM(vmCode, "gt");
        }
        else if (strcmp(savedToken.lx, "<") == 0) {
          addCommandToVM(vmCode, "lt");
        }
      }
    }

    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
  }
  pi.er = none;
  pi.tk = t;
  return pi;
}
ParserInfo ArithmeticExpression() {

  ParserInfo pi;
  Token savedToken;

  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("ArithmeticExpression", t);
  ParserInfo termPi = term();
  if (termPi.er != none) {
    return error(termPi.tk, termPi);
  }
  t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  while (strcmp(t.lx, "+") == 0 || strcmp(t.lx, "-") == 0) {

    savedToken = t;

    GetNextToken();
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }

    ParserInfo termChainedPi = term();
    if (termChainedPi.er != none) {
      return error(termChainedPi.tk, termChainedPi);
    }

  
    // add vm code
    if (globalPass == 3 || standardPass == 1) {
      if (strlen(auxSubrCommands)) {
        if (strcmp(savedToken.lx, "+") == 0) {
          strcat(auxSubrCommands, " add\n");
        }
        else if (strcmp(savedToken.lx, "-") == 0) {
          strcat(auxSubrCommands, " sub\n");
        }
      }
      else {
        if (strcmp(savedToken.lx, "+") == 0) {
          addCommandToVM(vmCode, "add");
        }
        else if (strcmp(savedToken.lx, "-") == 0) {
          addCommandToVM(vmCode, "sub");
        }
      }
    }

    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
  }
  pi.er = none;
  pi.tk = t;
  return pi;
}
ParserInfo term() {

  ParserInfo pi;
  Token savedToken;

  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("term", t);
  ParserInfo factorPi = factor();
  if (factorPi.er != none) {
    return error(factorPi.tk, factorPi);
  }
  t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  while (strcmp(t.lx, "*") == 0 || strcmp(t.lx, "/") == 0) {

    savedToken = t;

    GetNextToken();
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }

    ParserInfo factorChainedPi = factor();
    if (factorChainedPi.er != none) {
      return error(factorChainedPi.tk, factorChainedPi);
    }

    // vm code gen
    if (globalPass == 3 || standardPass == 1) {
      if (strlen(auxSubrCommands)) {
        if (strcmp(savedToken.lx, "*") == 0) {
          strcat(auxSubrCommands, " call Math.multiply 2\n");
        }
        else if (strcmp(savedToken.lx, "/") == 0) {
          strcat(auxSubrCommands, " call Math.divide 2\n");
        }
      }
      else {
        if (strcmp(savedToken.lx, "*") == 0) {
          addCommandToVM(vmCode, "call Math.multiply 2");
        }
        else if (strcmp(savedToken.lx, "/") == 0) {
          addCommandToVM(vmCode, "call Math.divide 2");
        }
      }
    }

    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
  }
  pi.er = none;
  pi.tk = t;
  return pi;
}
ParserInfo factor() {

  ParserInfo pi;
  Token savedToken;

  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("factor", t);
  if (strcmp(t.lx, "-") == 0 || strcmp(t.lx, "~") == 0) { // with preceding symbol

    savedToken = t;
    GetNextToken();

    ParserInfo operandPi = operand();
    if (operandPi.er != none) {
      return error(operandPi.tk, operandPi);
    }

    // add vm code
    if (globalPass == 3 || standardPass == 1) {
      if (strlen(auxSubrCommands)) {
        if (strcmp(savedToken.lx, "-") == 0) {
          strcat(auxSubrCommands, " neg\n");
        }
        else if (strcmp(savedToken.lx, "~") == 0) {
          strcat(auxSubrCommands, " not\n");
        }
      }
      else {
        if (strcmp(savedToken.lx, "-") == 0) {
          addCommandToVM(vmCode, "neg");
        }
        else if (strcmp(savedToken.lx, "~") == 0) {
          addCommandToVM(vmCode, "not");
        }
      }
    }

    pi.er = none;
    pi.tk = t;
    return pi;
  }
  // without preceding symbol
  ParserInfo operandPi = operand();
  if (operandPi.er != none) {
    return error(operandPi.tk, operandPi);
  }
  pi.er = none;
  pi.tk = t;
  return pi;
}
ParserInfo operand() {

  ParserInfo pi;
  int isSubroutineCall = 0; 
  int isIndexedOperand = 0;

  // Tokens for code gen -- operands
  int hasDot = 0;
  Token initialID;
  Token secondaryID;

  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("operand", t);
  if (strcmp(t.lx, "(") == 0) { // (expression)
    GetNextToken();
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    ParserInfo expressionPi = expression();
    if (expressionPi.er != none) {
      return error(expressionPi.tk, expressionPi);
    }
    t = GetNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (strcmp(t.lx, ")") == 0) { // end of (expression)
      pi.er = none;
      pi.tk = t;
      return pi;
    }
    else {
      pi.er = closeParenExpected;
      pi.tk = t;
      return pi;
    }
  }
  else if (t.tp == INT || t.tp == STRING || strcmp(t.lx, "true") == 0 || strcmp(t.lx, "false") == 0 || strcmp(t.lx, "null") == 0 || strcmp(t.lx, "this") == 0) {

    // add vm code
    if (globalPass == 3 || standardPass == 1) {

      char constantCommand[200];
      strcpy(constantCommand, "");
      
      // constants are added to constant segment
      if (t.tp == INT) {
        sprintf(constantCommand, " push constant %s\n", t.lx);
        strcat(auxSubrCommands, constantCommand);
      }
      else if (strcmp(t.lx, "true") == 0 || strcmp(t.lx, "false") == 0) {
        sprintf(constantCommand, " push constant %d\n", (strcmp(t.lx, "true") == 0 ? 1 : 0));
        strcat(auxSubrCommands, constantCommand);
      }
      else if (strcmp(t.lx, "null") == 0) {
        sprintf(constantCommand, " push constant 0\n");
        strcat(auxSubrCommands, constantCommand);
      }
      else if (t.tp == STRING) {

        // allocate memory for string
        int stringConstantLength = strlen(t.lx);
        sprintf(constantCommand, " push constant %d\n", stringConstantLength);
        strcat(auxSubrCommands, constantCommand);
        strcat(auxSubrCommands, " call String.new 1\n");
        strcat(auxSubrCommands, " pop temp 0\n"); // save result of .new in temp

        // create string in memory
        for (int i = 0; i < strlen(t.lx); ++i) {
          char stringCommand[200];
          strcat(auxSubrCommands, " push temp 0\n");
          sprintf(stringCommand, " push constant %c\n", t.lx[i]);
          strcat(auxSubrCommands, stringCommand);
          strcat(auxSubrCommands, " call String.appendChar 2\n");
        }

        // push ref of newly allocated string
        strcat(auxSubrCommands, " push temp 0\n");

      }
      else if (strcmp(t.lx, "this") == 0) { // this operand vm code gen here
        sprintf(constantCommand, " push pointer 0\n");
        strcat(auxSubrCommands, constantCommand);     
      }

    }

    GetNextToken();
    pi.er = none;
    pi.tk = t;
    return pi;
  }
  else if (t.tp == ID) { // identifier [.identifier ] [ [ expression ] | ( expressionList ) ]
    
    if (globalPass == 2) { // semantic check
      if (FindSymbol(tables[initialCounter], t.lx) < 0 && FindSymbol(tables[initialClassCounter], t.lx) < 0) { // undeclared var, subroutine, class in current class scope
        // check for standard lib ID + previous tables
        int ok = 0;
        for (int i = 0; i <= counter; ++i) {
          if (strcmp(tables[i].className, t.lx) == 0) { // declared stdlib subroutine / class
            ok = 1;
            break;
          }
        }
        if (!ok) {
          pi.er = undecIdentifier;
          pi.tk = t;
          return pi;
        }

      }
    }

    // generate vm code 
    if (globalPass == 3 || standardPass == 1) {

      initialID = t;

      char constantCommand[200];
      strcpy(constantCommand, "");

      // find offset and segment of token
      int localIndex = FindSymbol(tables[standardPass == 1 ? codegenIndexStd : codegenIndex], initialID.lx);
      if (localIndex >= 0) { // in local scope
        if (tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[localIndex].kind == VAR) {
          sprintf(constantCommand, " push local %d\n", tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[localIndex].offset);
        }
        else if (tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[localIndex].kind == ARGUMENT) {
          sprintf(constantCommand, " push argument %d\n", tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[localIndex].offset);
        }
      }
      else {
        int classIndex = FindSymbol(tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass], initialID.lx);
        if (classIndex >= 0) { // in parent class scope
          if (tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[classIndex].kind == STATIC) {
            sprintf(constantCommand, " push static %d\n", tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[classIndex].offset);
          }
          else if (tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[classIndex].kind == FIELD) {
            sprintf(constantCommand, " push this %d\n", tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[classIndex].offset);
          }
        }
        else { // in any other scope (?)
          // TODO
          // sprintf(constantCommand, " pop pointer 1\n");
          // strcat(auxSubrCommands, constantCommand);

        }
      }

      strcat(auxSubrCommands, constantCommand);

    }
    
    GetNextToken();
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (strcmp(t.lx, ".") == 0) {

      GetNextToken();
      hasDot = 1;

      t = PeekNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (t.tp == ID) { // success, check id
        if (globalPass == 2) {
          if (FindSymbol(tables[initialCounter], t.lx) < 0 && FindSymbol(tables[initialClassCounter], t.lx) < 0) { // undeclared var, subroutine, class
            
            // check for standard lib ID
            int ok = 0;
            for (int i = 0; i <= counter; ++i) {
              if (strcmp(tables[i].functionName, t.lx) == 0) { // declared stdlib subroutine / class
                ok = 1;
                break;
              }
            }
            if (!ok) {
              pi.er = undecIdentifier;
              pi.tk = t;
              return pi;
            }

          }
        } 

        // check whether the ID is a variable or a subroutine
        if (globalPass == 3 || standardPass == 1) {
          secondaryID = t;
        }

      }
      else {
        pi.er = idExpected;
        pi.tk = t;
        return pi;
      }
      GetNextToken(); // on ID
    }
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (strcmp(t.lx, "[") == 0) { // [ expression ]

      GetNextToken();
      isIndexedOperand = 1;

      t = PeekNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }

      // code gen for indexed access - variable (before indexing)
      if (globalPass == 3 || standardPass == 1) {

        char lhbi[200];
        strcpy(lhbi, "");

        int resultMethod = FindSymbol(tables[standardPass == 1 ? codegenIndexStd : codegenIndex], secondaryID.lx);
        int resultClass = FindSymbol(tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass], secondaryID.lx);

        if (resultMethod >= 0) { // var is declared in method
          sprintf(lhbi, " push local %d\n", tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[resultMethod].offset); 
        }
        else if (resultClass >= 0) { // var is declared in class
          sprintf(lhbi, " push this %d\n", tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[resultClass].offset); 
        }

        strcat(auxSubrCommands, lhbi);

      }

      ParserInfo expressionPi = expression();
      if (expressionPi.er != none) {
        return error(expressionPi.tk, expressionPi);
      }

      // code gen for indexed access - variable (add indexing)
      if (globalPass == 3 || standardPass == 1) {
        strcat(auxSubrCommands, " add\n");
        strcat(auxSubrCommands, " pop pointer 1\n");
        strcat(auxSubrCommands, " push that 0\n");
      }

      // code gen for indexed access - variable (before indexing)
      if (globalPass == 3 || standardPass == 1) {

        char indexedOperandCommand[200];
        strcpy(indexedOperandCommand, "");

        int resultMethod = FindSymbol(tables[standardPass == 1 ? codegenIndexStd : codegenIndex], secondaryID.lx);
        int resultClass = FindSymbol(tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass], secondaryID.lx);

        if (resultMethod >= 0) { // var is declared in method
          sprintf(indexedOperandCommand, " push local %d\n", tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[resultMethod].offset); 
        }
        else if (resultClass >= 0) { // var is declared in class
          sprintf(indexedOperandCommand, " push this %d\n", tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[resultClass].offset); 
        }

        strcat(auxSubrCommands, indexedOperandCommand);

      }      

      t = GetNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (strcmp(t.lx, "]") == 0) {
        pi.er = none;
        pi.tk = t;
        return pi;
      }
      else {
        pi.er = closeBracketExpected;
        pi.tk = t;
        return pi;
      }
    }
    else if (strcmp(t.lx, "(") == 0) { // ( expressionList )

      GetNextToken();

      t = PeekNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }

      ParserInfo expressionListPi = expressionList();
      if (expressionListPi.er != none) {
        return error(expressionListPi.tk, expressionListPi);
      }

      // vm code gen for subroutine call operand 
      if (globalPass == 3 || standardPass == 1) {
        
        int isMethod = 0;
        char subroutineFormat[200];
        strcpy(subroutineFormat, "");

        int methodResult = FindSymbol(tables[standardPass == 1 ? codegenIndexStd : codegenIndex], initialID.lx);
        int classResult = FindSymbol(tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass], initialID.lx);
        int filledBeginningSymbol = 0;

        if (methodResult >= 0) { // beginning symbol is local var or argument
          if (tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[methodResult].kind == ARGUMENT) {
            sprintf(subroutineFormat, " push argument %d\n", tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[methodResult].offset);
            strcat(auxSubrCommands, subroutineFormat);
          }
          else if (tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[methodResult].kind == VAR) {
            sprintf(subroutineFormat, " push local %d\n", tables[standardPass == 1 ? codegenIndexStd : codegenIndex].symbols[methodResult].offset);
            strcat(auxSubrCommands, subroutineFormat);
          }
          sprintf(subroutineFormat, " call %s", initialID.lx); // this ref
          filledBeginningSymbol = 1;
        }
        else if (classResult >= 0) { // beginning symbol is a class level var
          sprintf(subroutineFormat, " push this %d\n", tables[standardPass == 1 ? codegenIndexClassStd : codegenIndexClass].symbols[classResult].offset);
          strcat(auxSubrCommands, subroutineFormat);
          sprintf(subroutineFormat, " call %s", initialID.lx); // this ref
          filledBeginningSymbol = 1;
        }

        for (int i = 0; i <= counter; ++i) {
          if (strcmp(tables[i].functionName, secondaryID.lx) == 0 && strcmp(initialID.lx, tables[i].parentClass) == 0) { // declared subroutine / class
            isMethod = tables[i].isMethod;
            if (!filledBeginningSymbol) {
              if (isMethod) {
                lastArgsCounter++;
              }
              sprintf(subroutineFormat, " call %s.%s %d\n", tables[i].parentClass, tables[i].functionName, lastArgsCounter); // this ref
            }
            strcat(auxSubrCommands, subroutineFormat);
            break;
          }
        } 

        lastArgsCounter = 0;   
      }

      t = GetNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (strcmp(t.lx, ")") == 0) {
        pi.er = none;
        pi.tk = t;
        return pi;
      }
      else {
        pi.er = closeParenExpected;
        pi.tk = t;
        return pi;
      }
    }

    pi.er = none;
    pi.tk = t;
    return pi;
  }
  else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }
}

// do not remove the next line
/*
#ifndef TEST_PARSER
int main ()
{	
	//init Parser 
	InitParser("./Main.jack");

	// parser
	Parse();

	// cleanup
	StopParser();

	return 1;
}
#endif
*/