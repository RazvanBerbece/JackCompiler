

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"

#define JsonStrSize 5000
#define Presubmission 1

// remove this before releasing template
#define NumberTestFiles 20

char* JsonStr;

Token tokenArray[2024];
int numberTokens;

char* testFiles[NumberTestFiles] = {
	"Main.jack",
	"Ball.jack" ,
	"Fraction.jack",
	"List.jack",
	"Square.jack" ,
	"SquareGame.jack",
	"classExpected.jack" ,
	"closeBracketExpected.jack",
	"closeParenExpected.jack",
	"closeParenExpected1.jack",
	"equalExpected.jack",
	"idExpected.jack",
	"idExpected2.jack",
	"openBraceExpected.jack",
	"openParenExpected.jack",
	"semicolonExpected.jack",
	"syntaxError.jack",
	"syntaxError1.jack",
	"syntaxError2.jack",
	"NewLineInStr.jack",
};

ParserInfo correctInfo [NumberTestFiles] = {
	{none},
	{none},
	{none},
	{none},
	{none},
	{none},
	{classExpected , {ID , "clas" , NoLexErr , 7} } ,
	{closeBracketExpected , {SYMBOL , ";" , NoLexErr , 19} } ,
	{closeParenExpected , {SYMBOL , "{" , NoLexErr , 19} } ,
	{closeParenExpected , {SYMBOL , ";" , NoLexErr , 43} } ,
	{equalExpected , {INT , "0" , NoLexErr , 27} } ,
	{idExpected , {SYMBOL , "{" , NoLexErr , 11} } ,
	{idExpected , {SYMBOL , "." , NoLexErr , 59} } ,
	{openBraceExpected , {RESWORD , "field" , NoLexErr , 8} } ,
	{openParenExpected , {RESWORD , "false" , NoLexErr , 52} } ,
	{semicolonExpected , {SYMBOL , "}" , NoLexErr , 27} } ,
	{closeParenExpected , {ID , "dx" , NoLexErr , 96} } ,
	{syntaxError , {SYMBOL , ";" , NoLexErr , 61} } ,
	{semicolonExpected , {SYMBOL , ")" , NoLexErr , 91} } ,
	{lexerErr, {ERR , "" , NewLnInStr , 1} }
};

int InitGraderString ()
{
	JsonStr = (char *) malloc (sizeof (char) * JsonStrSize);
	strcpy (JsonStr, "{\n");   // let's start, bismillah
	strcat (JsonStr, "\t\"output\": \"Graded by CAutoGrader\",\n");
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
		default: return "not a valid error code";
	}
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
int t_parser ()
{
	int m=20;
	char s[100];
	int fail = 0;


	printf ("\nTesting your parser on various source files (1 mark each)\n");
	for (int j = 0 ; j < NumberTestFiles ; j++) // for each test file
	{
		printf ("File %s \n", testFiles[j]);
		InitLexer (testFiles[j]);
		fail = 0;
		ParserInfo p = Parse ();
		if (p.er != correctInfo[j].er)
			fail = 1;
		else if (correctInfo[j].er != none && (p.tk.tp != correctInfo[j].tk.tp || p.tk.ln != correctInfo[j].tk.ln || (correctInfo[j].er != lexerErr && strcmp (correctInfo[j].tk.lx , p.tk.lx))) )
			fail = 1;
		else
			printf ("	PASSED\n");
		if (fail)
		{
			printf ("** Oops: your parser returned the following info:\n");
			ShowInfo (p);
			printf ("It should have returned:\n");
			ShowInfo (correctInfo[j]);
			printf ("Sorry, -1 mark\n");
			m--;
		}
		//printf ("Calling your StopParser function\n");
		StopParser ();
	}


	sprintf (s,"%i/20 for the parser", m);
	if (!Presubmission)
		AddTestString (m, 20, s, 1);
	return m;
}


#ifdef TEST_PARSER
int main (int argc, char* argv[])
{
	FILE* jsonFile;
	int tot = 0;


	if (!Presubmission)
		InitGraderString ();

	printf ("\t$$$ Checking your lexer, behold $$$\n");
	printf ("\t=========================================\n");
	printf ("Started ...\n");

	tot += t_parser ();

	if (Presubmission)
	{
		printf ("\n---------------------------------------------------\n");
		printf ("\t\tTotal mark = %i/20\n", tot);
		printf ("---------------------------------------------------\n\n");
	}
	printf ("Finished\n");


	if (!Presubmission)  // create the results.json file
	{
		CloseGraderString ();
		//printf ("%s",JsonStr);
		jsonFile = fopen ("/autograder/results/results.json", "w");
		fprintf (jsonFile, "%s", JsonStr);
		fclose (jsonFile);
	}


	return 0;
}
#endif
