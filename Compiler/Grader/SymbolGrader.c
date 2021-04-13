
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "compiler.h"

#define JsonStrSize 5000
#define Presubmission 1

// remove this before releasing template
#define NumberTestFiles 20
char* JsonStr;

char* ErrorString (SyntaxErrors e);
void PrintError (ParserInfo pn);


//Token tokenArray[2024];
//int numberTokens;

char* testFiles[NumberTestFiles] = {
	"UNDECLAR_VAR",
	"REDECLAR_VAR" ,
	"UNDECLAR_SUB",
	"DECLARED_SUB",
	"DECLARED_VAR" ,
	"DECLAR_EXT_FUN",
	"UNDECLAR_EXT_FUN",
	"UNDECLAR_EXT_METH",
	"DECLAR_EXT_METH",
	"UNDECLAR_CLASS",
	"USES_MATH_LIB",
	"ERR_MATH_LIB",
	"Pong",
	"Square",
	"StringTest",
	"Square1",
	"Square2",
	"Square3",
	"ComplexArrays",
	"Pong1",
};

ParserInfo correctInfo [NumberTestFiles] = {
	{undecIdentifier, {ID, "t" , NoLexErr, 8} },
	{redecIdentifier, {ID , "v" , NoLexErr , 5} } ,
	{undecIdentifier, {ID , "Sub" , NoLexErr , 11} },
	{none},
	{none},
	{none},
	{undecIdentifier, {ID , "N" , NoLexErr , 7} },
	{undecIdentifier, {ID , "N" , NoLexErr , 9} },
	{none},
	{undecIdentifier, {ID , "T" , NoLexErr , 4} },
	{none},
	{undecIdentifier, {ID , "mult" , NoLexErr , 8} },
	{none},
	{none},
	{none},
	{undecIdentifier, {ID , "nev" , NoLexErr , 12} },
	{undecIdentifier, {ID , "moveRigh" , NoLexErr , 45} },
	{undecIdentifier, {ID , "squar" , NoLexErr , 35} },
	{none},
	{undecIdentifier, {ID , "bas" , NoLexErr , 26} },
};

int InitGraderString ()
{
	JsonStr = (char *) malloc (sizeof (char) * JsonStrSize);
	strcpy (JsonStr, "{\n");   // let's start, bismillah
	strcat (JsonStr, "\t\"output\": \"Graded by CAutoGrader\",\n");
	strcat (JsonStr, "\t\"std_visibility\": \"visible\",\n");
	strcat (JsonStr, "\t\"tests\":\n");  // let's start the tests
	strcat (JsonStr, "\t[\n");
	return 1;
}

int AddTestString (int score, int max_score, char* output, int is_final)
{
	char temp[320];

	strcat (JsonStr , "\t\t{\n");
	sprintf (temp, "%i", score);
	strcat (JsonStr , "\t\t\t\"score\": ");
	strcat (JsonStr , temp);
	strcat (JsonStr , ",\n");

	sprintf (temp, "%i", max_score);
	strcat (JsonStr , "\t\t\t\"max_score\": ");
	strcat (JsonStr , temp);
	strcat (JsonStr , ",\n");


	strcat (JsonStr , "\t\t\t\"output\": ");
	strcat (JsonStr , "\"");
	strcat (JsonStr , output);
	strcat (JsonStr , "\"");
	strcat (JsonStr , "\n");

	strcat (JsonStr , "\t\t}");
	if (!is_final)
		strcat (JsonStr, ",");
	strcat (JsonStr, "\n");

	return 1;
}

int CloseGraderString ()
{
	strcat (JsonStr, "	]\n"); // end of tests
	strcat (JsonStr, "}\n");
	return 1;
}

char* TokenTypeString (TokenType t)
{
	switch (t)
	{
		case RESWORD: return "RESWORD";
		case ID: return "ID";
		case INT: return "INT";
		case SYMBOL: return "SYMBOL";
		case STRING: return "STRING";
		case EOFile: return "EOFile";
		case ERR: return "ERR";
		default: return "Not a recognised token type";
	}

}

char* ErrorString (SyntaxErrors e)
{

	switch (e)
	{
		case none: return "no errors";
		case lexerErr: return "lexer error";
		case classExpected: return "keyword class expected";
		case idExpected: return "identifier expected";
		case openBraceExpected:	return "{ expected";
		case closeBraceExpected: return "} expected";
		case memberDeclarErr: return "class member declaration must begin with static, field, constructor , function , or method";
		case classVarErr: return "class variables must begin with field or static";
		case illegalType: return "a type must be int, char, boolean, or identifier";
		case semicolonExpected: return "; expected";
		case subroutineDeclarErr: return "subrouting declaration must begin with constructor, function, or method";
		case openParenExpected: return "( expected";
		case closeParenExpected: return ") expected";
		case closeBracketExpected: return "] expected";
		case equalExpected: return "= expected";
		case syntaxError: return "syntax error";
		// semantic errors
		case undecIdentifier: return "undeclared identifier";
		case redecIdentifier: return "redeclaration of identifier";
		default: return "not a valid error code";
	}
}

void PrintError (ParserInfo pn)
{
	if (pn.er == none)
		printf ("No errors\n");
	else
		printf ("Error in file %s line %i at or near %s: %s\n" , pn.tk.fl , pn.tk.ln , pn.tk.lx , ErrorString(pn.er));
}


void ShowInfo (ParserInfo pn)
{
	if (pn.er == none)
		printf ("none\n");
	else
		printf ("error type: %s, line: %i,token: %s, \n" , ErrorString(pn.er),  pn.tk.ln , pn.tk.lx  );
}


void PrintToken (Token t)
{
	printf ("<%s, %i, %s, %s>\n", t.fl, t.ln , t.lx, TokenTypeString (t.tp));
}


// test the parser
int t_compiler ()
{
	int m=20;
	char s[100];
	int fail = 0;


	printf ("\nTesting your compiler on various JACK programs (1/2 mark each)\n");
	for (int j = 0 ; j < NumberTestFiles ; j++) // for each test file
	{
		printf ("JACK Program %s:\n", testFiles[j]);
		InitCompiler ();
		fail = 0;
		ParserInfo p = compile (testFiles[j]);
		if (p.er != correctInfo[j].er)
			fail = 1;
		else if (correctInfo[j].er != none && (p.tk.tp != correctInfo[j].tk.tp || p.tk.ln != correctInfo[j].tk.ln || (correctInfo[j].er != lexerErr && strcmp (correctInfo[j].tk.lx , p.tk.lx))) )
			fail = 1;
		else
			printf ("\t$$ Great PASSED :-)\n");
		if (fail)
		{
			printf ("** Oops: your compiler returned the following info:\n");
			ShowInfo (p);
			printf ("It should have returned:\n");
			ShowInfo (correctInfo[j]);
			printf ("Sorry, -1 mark\n");
			m--;
		}
		//printf ("Calling your StopParser function\n");
		StopCompiler ();
	}

	m = (int) (m/2.0);
	sprintf (s,"%i/10 for the symbol table", m);
	if (!Presubmission)
		AddTestString (m, 10, s, 1);
	return m;
}


#ifdef TEST_COMPILER
int main (int argc, char* argv[])
{
	FILE* jsonFile;
	int tot = 0;


	if (!Presubmission)
		InitGraderString ();

	printf ("\t$$$ Checking your lexer, behold $$$\n");
	printf ("\t=========================================\n");
	printf ("Started ...\n");

	tot += t_compiler ();


	printf ("\n---------------------------------------------------\n");
	printf ("\t\tTotal mark = %i/10\n", tot);
	printf ("---------------------------------------------------\n\n");

	printf ("Finished\n");


	if (!Presubmission)  // create the results.json file
	{
		CloseGraderString ();
		//printf ("%s",JsonStr);
		//jsonFile = fopen ("/autograder/results/results.json", "w");
		//fprintf (jsonFile, "%s", JsonStr);
		//fclose (jsonFile);
	}

	return 0;
}
#endif
