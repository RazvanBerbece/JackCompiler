/************************************************************************
University of Leeds
School of Computing
COMP2932- Compiler Design and Construction
The Compiler Module

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

#include "compiler.h"

/** Global variables for compilation process **/
char files[512][128]; // holds all .jack filenames in current directory
int filesCounter; // indexing for files[]
int globalPass = 1;

extern int counter; // from parser.c
extern SymbolTable tables[512]; // from parser.c

int InitCompiler ()
{	
	filesCounter = 0;
	globalPass = 1;
	extern int counter;
	extern SymbolTable tables[512]; // from parser.c
	return 1;
}

ParserInfo compile (char* dir_name)
{
	ParserInfo p;

	// get all .jack files in given directory by executing unix command
	// redirect output to a textfile : output.txt in same directory as compiler.c
	char command[64];
	sprintf(command, "cd %s && ls | grep '\\.jack$' > ../output.txt", dir_name);
	system(command);

	// open file and read all filenames line by line
	FILE *fp;
	char fileName[256]; // acts as buffer for fgets

	fp = fopen("output.txt", "r");
	if (fp == NULL) {
		printf("Could not get all .jack files in current directory. :(\n");
		exit(-1);
	}

	// read filenames line by line and add them to the global array
	while(fgets(fileName, 512, fp)) {
		strcpy(files[filesCounter], fileName);
		filesCounter += 1;
	}
	fclose(fp);

	// PARSING PASS 0 - JACK LIBRARIES
	char standardLibsPaths[8][16] = {"./Array.jack\0", "./Keyboard.jack\0", "./Math.jack\0", "./Memory.jack\0", "./Output.jack\0", "./Screen.jack\0", "./String.jack\0", "./Sys.jack\0"};
	for (int i = 0; i < 8; ++i) {
		InitParser(standardLibsPaths[i]);
		ParserInfo sourceParseInfo = Parse(); // parse file
		if (sourceParseInfo.er != none) { // report on errors if any and exit if errors found
			p.er = sourceParseInfo.er;
			p.tk = sourceParseInfo.tk;
			StopParser();
			return p;
		}
		StopParser();
	}

	
	// parse all .jack source files 
	const int PASSES = 2;
	// pass 1 : gather symbols <- starts here on PASS = 1!
	// pass 2 : semantic analysis
	for (int j = 1; j <= PASSES; ++j) { globalPass = j;

		for (int i = 0; i < filesCounter; ++i) {

		// build complete path : ./dir_name/...
		char completePathToSource[512];
		strcpy(completePathToSource, "./");
		strcat(completePathToSource, dir_name);
		strcat(completePathToSource, "/");
		strncat(completePathToSource, files[i], strlen(files[i]) - 1);

		InitParser(completePathToSource); // init parser for file
		ParserInfo sourceParseInfo = Parse(); // parse file
		if (sourceParseInfo.er != none) { // report on errors if any and exit if errors found
			p.er = sourceParseInfo.er;
			p.tk = sourceParseInfo.tk;
			StopParser();
			return p;
		}
		StopParser();

		}

	}

	p.er = none;
	return p;
}

int StopCompiler ()
{	
	// set all filenames characters to NULL
	for (int i = 0; i < filesCounter; ++i) {
		memset(files[i], 0, sizeof files[i]);
	}
	for (int i = 0; i < counter; ++i) {
		FreeTable(tables[i]);
	}

	memset(tables, 0, sizeof tables);

	filesCounter = 0;
	globalPass = 1;
	counter = 0;
	return 1;
}

/*
#ifndef TEST_COMPILER
int main ()
{
	InitCompiler ();
	ParserInfo p = compile ("Square");
	// PrintError (p);
	StopCompiler ();
	return 1;
}
#endif
*/