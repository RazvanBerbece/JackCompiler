#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"

// you can declare prototypes of parser functions below

/** ----------------- Parser Iteration Logic ----------------- **/
ParserInfo error(Token t, ParserInfo pi);
void OK(Token t);

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
ParserInfo operand(); // integerConstant | identifier [.identifier ] [ [ expression ] | (expressionList ) ] | (expression) | stringLiteral | true | false | null | this

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
    default:
      printf("Line %d : unrecognised error\n", t.ln);
      break;
  }
  return pi;
}

int InitParser (char* file_name)
{ 
	int lexerInitResult = InitLexer(file_name);
	if (!lexerInitResult) return 0;
	else return 1;
}

ParserInfo Parse ()
{
	ParserInfo pi;

	// implement the function
	pi = classDeclar();
  if (pi.er != none) {
    return error(pi.tk, pi);
  }

	pi.er = none;

	return pi;
}

int StopParser ()
{
	StopLexer();
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
    ParserInfo subroutineDeclarPi = subroutineDeclar();
    if (subroutineDeclarPi.er != none) {
      return error(subroutineDeclarPi.tk, subroutineDeclarPi);
    }
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
  // printParserStage("classVarDeclar", t);
  if (strcmp(t.lx, "static") == 0 || strcmp(t.lx, "field") == 0) {
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
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.er = lexerErr;
      pi.tk = t;
      return pi;
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
        // function signature
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
        // function signature
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
      t = PeekNextToken();
      if (lexerError(t)) {
        pi.er = lexerErr;
        pi.tk = t;
        return pi;
      }
      if (strcmp(t.lx, "[") == 0) { // const for indexed
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
  ParserInfo pi;
  Token t = GetNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("subroutineCall", t);
  if (t.tp == ID) {
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    // check for member access (ie: .[identifier])
    if (strcmp(t.lx, ".") == 0) {
      GetNextToken();
      t = PeekNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (t.tp == ID) {
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
  Token t = PeekNextToken();
  if (lexerError(t)) {
    pi.tk = t;
    pi.er = lexerErr;
    return pi;
  }
  // printParserStage("factor", t);
  if (strcmp(t.lx, "-") == 0 || strcmp(t.lx, "~") == 0) { // with preceding symbol
    GetNextToken();
    ParserInfo operandPi = operand();
    if (operandPi.er != none) {
      return error(operandPi.tk, operandPi);
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
    GetNextToken();
    pi.er = none;
    pi.tk = t;
    return pi;
  }
  else if (t.tp == ID) { // identifier [.identifier ] [ [ expression ] | ( expressionList ) ]
    GetNextToken();
    t = PeekNextToken();
    if (lexerError(t)) {
      pi.tk = t;
      pi.er = lexerErr;
      return pi;
    }
    if (strcmp(t.lx, ".") == 0) {
      GetNextToken();
      t = PeekNextToken();
      if (lexerError(t)) {
        pi.tk = t;
        pi.er = lexerErr;
        return pi;
      }
      if (t.tp == ID)
        ; // success, nothing here
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
    else if (strcmp(t.lx, "(") == 0) {
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

#ifndef TEST_PARSER
int main ()
{	
	/** Init Parser **/
	InitParser("./Main.jack");

	/** Parse **/
	Parse();

	/** Cleanup **/
	StopParser();

	return 1;
}
#endif
