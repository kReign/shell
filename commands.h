#ifndef COMMANDS_H
#define COMMANDS_H

/********************************************************************
// File: commands.h
// Author: Alex Charles
********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>
#include "envAndShVars.h"
#include "globalVars.h"
#include "externalCommands.h"

void f_exit(char** arg);
void f_set(char** arg);
void f_unset(char** arg);
void f_prt(char** arg);
void f_envset(char** arg);
void f_envunset(char** arg);
void f_envprt(char** arg);
void f_witch(char** arg);
void f_pwd(char** arg);
void f_cd(char** arg);
void f_lim(char** arg);
void f_exist(char** arg);
void f_jobs(char** arg);
void f_kill(char** arg);
void f_fg(char** arg);
void f_bg(char** arg);

// Definition for the function/command "hash" table.
const static struct {
	char* name;
	void (*func)(char**);
} function_hash [] = {
	{ "exit", 		&f_exit },	
	{ "set", 		&f_set },
	{ "unset", 		&f_unset },
	{ "prt", 		&f_prt },
	{ "envset", 	&f_envset },
	{ "envunset", 	&f_envunset },
	{ "envprt", 	&f_envprt },
	{ "witch", 		&f_witch },
	{ "pwd", 		&f_pwd },
	{ "cd", 		&f_cd },
	{ "lim",		&f_lim },
	{ "exist",		&f_exist },
	{ "jobs",		&f_jobs },
	{ "kill",		&f_kill },
	{ "fg",		&f_fg },
	{ "bg",		&f_bg }
};

/********************************************************************
// Called by main, given function name and the arguments for that 
// function as strings, it will call the function by matching it
// in funcion_hash above.
********************************************************************/
int callCommandFunction(char* cmdName, char** args)
{
	for (int i = 0; i < sizeof(function_hash) / sizeof(function_hash[0]); ++i)
	{
		// If we find the command, call the corresponding function
		// and let main know we were successful. 
		if (strcmp(cmdName, function_hash[i].name) == 0) {
			(*function_hash[i].func)(args);
			return 1;
		}
	}

	// If it's not built in, try to run an external command.
	if (runExternalCommand(cmdName, args))
	{
		return 1;
	}
	else 
	{
		return 2;
	}

	// If we didn't find the command, let main know so it can report
	// an error.
	return 0;
}

/********************************************************************
// Returns whether or not a command called cmd exists.
********************************************************************/
int builtinCommandExists(char* cmd)
{
	for (int i = 0; i < sizeof(function_hash) / sizeof(function_hash[0]); ++i)
	{
		if (strcmp(cmd, function_hash[i].name) == 0) {
			return 1;
		}
	}
	return 0;
}

/********************************************************************
// Exits the program.
********************************************************************/
void f_exit(char** arg)
{
	exit(EXIT_SUCCESS);
}

/********************************************************************
// Sets a shell (instance) variable.
********************************************************************/
void f_set(char** arg)
{
	char* variableName = arg[1];

	// Print usage if no arguments.
	if ( !(numArgs > 1) )
	{
		printf("Usage: set varname value\n");
		return;
	}

	// If invalid variable name, report error.
	if (!isValidVarName(variableName))
	{
		printf("Invalid variable name: %s\n", variableName);
		return;
	}

	// If no value arg, report error.
	char* val = arg[2];
	if ( !(numArgs > 2) )
	{
		printf("Usage: set varname value\n");
		return;
	}

	// If none of the other branches returned, we can set the variable.
	if ( !setVar(variableName, val, 1))
	{
		printf("%s: variable already exists\n", variableName);
	}
}

/********************************************************************
// Removes a shell (instance) variable.
********************************************************************/
void f_unset(char** arg)
{
	char* var = NULL;

	// Print usage if no arguments.
	if ( !arg[1] )
	{
		printf("Usage: unset varname\n");
		return;
	}

	char* variableName = arg[1];

	// If invalid variable name, report error.
	if (!isValidVarName(variableName))
	{
		printf("Invalid variable name: %s\n", variableName);
		return;
	}

	if ( !unsetVar(variableName))
	{
		printf("%s: variable does not exist\n", variableName);
	}
}

/********************************************************************
// Prints text and/or variables.
********************************************************************/
void f_prt(char** arg)
{
	if (!arg[1])
	{
		printf("Usage: prt value/$varname ...\n");
		return;
	}

	for (int i = 1; i < numArgs; ++i)
	{
		printf("%s ", arg[i]);
	}

	printf("\n");
	return;
}

/********************************************************************
// Sets an environment variable.
********************************************************************/
void f_envset(char** arg)
{
	// Print usage if no arguments.
	if ( !(numArgs > 2) )
	{
		printf("Usage: envset VARNAME value\n");
		return;
	}

	char* variableName = arg[1];
	char* value = arg[2];

	// If invalid variable name, report error.
	if (!isValidVarName(variableName))
	{
		printf("Invalid variable name: %s\n", variableName);
		return;
	}

	if ( !setEnvVar(variableName, value, 1))
	{
		printf("%s: environment variable already exists\n", variableName);
	}
}

/********************************************************************
// Unsets an environment variable.
********************************************************************/
void f_envunset(char** arg)
{
	char* var;

	// Print usage if no arguments.
	if ( !(var = strtok(NULL, " \t\n")) )
	{
		printf("Usage: envunset VARNAME\n");
		return;
	}

	char* variableName = interpolateVars(var);

	// If invalid variable name, report error.
	if (!isValidVarName(variableName))
	{
		printf("Invalid variable name: %s\n", variableName);
		return;
	}

	if ( !unsetEnvVar(variableName))
	{
		printf("%s: environment variable does not exist\n", variableName);
	}
}

/********************************************************************
// Prints all environment variables.
********************************************************************/
void f_envprt(char** arg)
{
	printEnvVars();
}

/********************************************************************
// Prints the location of the given program name.
********************************************************************/
void f_witch(char** arg)
{
	// Print usage if no arguments.
	if ( !(numArgs > 1) )
	{
		printf("Usage: witch program/command_name\n");
		return;
	}

	char* command = arg[1];

	if (builtinCommandExists(command))
	{
		printf("%s: built-in command\n", command);
		return;
	}
	else 
	{
		char* p = getEnvVar("AOSPATH");
		if(!p)
		{
			printf("AOSPATH is not set\n");
		}
		else
		{
			char* aospath = strdup(p);

			char* path;

			// Check each directory in AOSPATH for the file with
			// name "command".
			char* token = strtok(aospath, ":");
			while (token)
			{
				path = strdup(token);
				strcat(path, "/");
				strcat(path, command);

				// Check if the file exists 
				if (access(path, F_OK) != -1)
				{
					printf("%s\n", path);
					return;
				}

				token = strtok(NULL, ":");
			}
		}
	}
}

/********************************************************************
// Prints current working directory.
********************************************************************/
void f_pwd(char** arg)
{
	char cwd[MAX_BUFFER_SIZE];
   	getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
}

/********************************************************************
// Changes the current working directory - 
// also updates environment variable AOSCWD.
*******************************************************************/
void f_cd(char** arg) 
{
	// Print usage if no arguments.
	if ( !(numArgs > 1) )
	{
		printf("Usage: cd path\n");
		return;
	}

	char* path = arg[1];

	char changeTo[MAX_BUFFER_SIZE];
	
	// Check if the path is absolute, otherwise relative 
	if (path[0] == '/') 
	{
		strcpy(changeTo, path);
	} 
	else 
	{
		// Append the given path to the current path since we
		// were given a relative path. 
		getcwd(changeTo, sizeof(changeTo));
		strcat(changeTo, "/");
		strcat(changeTo, path);
	}

	// TODO: handle error codes correctly.
	if (chdir(changeTo) != 0)
	{
		printf("error\n");
		return;
	}

	char cwd[MAX_BUFFER_SIZE];
	getcwd(cwd, sizeof(cwd));

	// Print the new current path.
	// TODO: Check if I should do this.
	printf("%s\n", cwd);

	// Update the AOSCWD environment variable.
   	setEnvVar("AOSCWD", cwd, 1);

   	free(path);
}

/********************************************************************
// Limits the CPU time for any child processes.
********************************************************************/
void f_lim(char** arg)
{

	if (numArgs == 1)
	{
		if (cpuLim == -1)
		{
			printf("CPU Limit: Unlimited\n");
		}
		else
		{
			printf("CPU Limit: %ds\n", cpuLim);
		}

		if (memLim == -1)
		{
			printf("Memory Limit: Unlimited\n");
		}
		else
		{
			printf("Memory Limit: %dMB\n", memLim);
		}
		return;
	}

	// Print usage if no arguments.
	else if ( numArgs == 3 )
	{
		cpuLim = atoi(arg[1]);
		memLim = atoi(arg[2]);
		return;
	}
	else
	{
		printf("Usage: lim CPU MEM\n");
		return;
	}
	
}

/********************************************************************
// DEBUG check if a file exists
********************************************************************/
void f_exist(char** arg)
{
	// Print usage if no arguments.
	if ( !(numArgs > 1) )
	{
		printf("Usage: cd path\n");
		return;
	}

	char* file = arg[1];

	// Check if the file exists 
	if (access(file, F_OK) != -1)
	{
		printf("yes\n");
		return;
	}

	return;
}

/********************************************************************
// Lists all current jobs.
********************************************************************/
void f_jobs(char** arg)
{
	listJobs();
}

/********************************************************************
// Kills given job.
********************************************************************/
void f_kill(char** arg)
{
	// Print usage if no arguments.
	if ( !(numArgs > 1) )
	{
		printf("Usage: kill id\n");
		return;
	}

	killJob(atoi(arg[1]));
}

/********************************************************************
// Puts job into the foreground
********************************************************************/
void f_fg(char** arg)
{
	// Print usage if no arguments.
	if (numArgs > 2)
	{
		printf("Usage: fg (id)\n");
		return;
	}

	int a = (arg[1] == NULL) ? -1 : atoi(arg[1]);
	resumeProcess(a, 1);
}

/********************************************************************
// Puts job into the background
********************************************************************/
void f_bg(char** arg)
{
	// Print usage if no arguments.
	if (numArgs > 2)
	{
		printf("Usage: bg (id)\n");
		return;
	}

	int a = (arg[1] == NULL) ? -1 : atoi(arg[1]);
	resumeProcess(a, 0);
}

#endif
