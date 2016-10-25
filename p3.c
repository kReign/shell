/********************************************************************
// File: p1.c
// Author: Alex Charles
********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "commands.h"
#include "envAndShVars.h"
#include "globalVars.h"
#include "externalCommands.h"

int main(int argc, char* argv[])
{
	setbuf(stdout, NULL);
	
	// First, initialize environment.
	initEnvVars();

	// Initialize external commands.
	initExternalCommands();

	// Check for command line filename and set our input FILE.
	FILE* input = (argc > 1) ? fopen(argv[1], "r") : stdin;
	inputFD = fileno(input);

	// Set whether or not we display the prompt (isatty).
	char* prompt = (isatty(inputFD)) ? "asc4e_sh> " : "";

	// Print the inital prompt.
	printf("%s", prompt);

	char* line;
	char* cleanLine;
	size_t len = 0;
	char* cmdName;

	// Get a line from user and make sure it's not EOF.
	while (getline(&line, &len, input) != -1)
	{
		// Make a copy of the line to send to the command, since 
		// strtok modifies the original string.
		cleanLine = cleanAndInterpolateInput(line);
		char** args = stringToArray(cleanLine);	
	
		// Grab the command name from the line entered by user.
		// (Should be the first word in the line).
		if ( (numArgs > 0) )
		{
			if (!callCommandFunction(args[0], args)) 
			{
				printf("%s: command not found\n", args[0]);
			}
		}

		//checkCompleteProcesses();
		fflush(stdout);
		// Print the prompt for the next line.
		printf("%s", prompt);
	}

	printf("\n");

	return 0;
}
