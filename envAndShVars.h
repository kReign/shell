#ifndef ENV_AND_SH_VARS_H
#define ENV_AND_SH_VARS_H

/********************************************************************
// File: envAndShVars.h
// Author: Alex Charles
********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "globalVars.h"

static int numShellVars = 0;
static struct {
    char name[MAX_BUFFER_SIZE];
    char value[MAX_BUFFER_SIZE];
} shellVars [MAX_NUM_VARS];

static int numEnvVars = 0;

/********************************************************************
// Initializes the environment variables from file.
********************************************************************/
void initEnvVars()
{
	clearenv();
	char cwd[MAX_BUFFER_SIZE];
	getcwd(cwd, sizeof(cwd));

	setenv("AOSPATH", "/bin:/usr/bin", 1);
	setenv("AOSCWD", cwd, 1);

    numEnvVars = 2;
}

/********************************************************************
// Prints all of the environment variables.
********************************************************************/
void printEnvVars()
{
	for (int i = 0; environ[i]; ++i)
	{
		printf("%s\n", environ[i]);
	}
}

/********************************************************************
// Sets an environment variable.
********************************************************************/
int setEnvVar(char* name, char* value, int overwrite)
{
	if (overwrite || !(getenv(name)))
	{		
		setenv(name, value, 1);		
		numEnvVars++;
		return 1;
	}	
	return 0;
}

/********************************************************************
// Unsets an environment variable called name.
********************************************************************/
int unsetEnvVar(char* name)
{
/*    for (int i = 0; i < numEnvVars; ++i)
    {
        if (strcmp(envVars[i].name, name) == 0)
        {
            strcpy(envVars[i].name, "\0");
            strcpy(envVars[i].value, "\0");
            return 1;
        }
    }
*/
	if (!unsetenv(name))
	{
		numEnvVars--;
		return 1;
	}
	
    return 0;
}

/********************************************************************
// Gets an environment variable called name.
********************************************************************/
char* getEnvVar(char* name)
{
    return getenv(name);
}

/********************************************************************
// Sets a shell (instance) variable.
********************************************************************/
int setVar(char* name, char* value, int overwrite)
{
    for (int i = 0; i < numShellVars; ++i)
    {
        if (strcmp(shellVars[i].name, name) == 0)
        {   
            if (!overwrite)
            {
                return 0;
            }
            else 
            {
                strcpy(shellVars[i].name, name);
                strcpy(shellVars[i].value, value);
                numShellVars++;
                return 1;
            }
        }
    }

    strcpy(shellVars[numShellVars].name, name);
    strcpy(shellVars[numShellVars].value, value);
    numShellVars++;

    return 1;
}

/********************************************************************
// Unsets a shell (instance) variable.
********************************************************************/
int unsetVar(char* name)
{
    for (int i = 0; i < numShellVars; ++i)
    {
        if (strcmp(shellVars[i].name, name) == 0)
        {
            strcpy(shellVars[i].name, "\0");
            strcpy(shellVars[i].value, "\0");
            return 1;
        }
    }

    return 0;
}

/********************************************************************
// Gets a shell variable called name.
********************************************************************/
char* getVar(char* name)
{
    for (int i = 0; i < numShellVars; ++i)
    {
        if (strcmp(shellVars[i].name, name) == 0)
        {
            return shellVars[i].value;
        }
    }

    return NULL;
}

/********************************************************************
// Returns whether or not the given name is a valid variable name.
********************************************************************/
int isValidVarName(char* name)
{
    // If first character is a number, it's not valid.
    if (name[0] >= '0' && name[0] <= '9')
    {
        return 0;  
    }

    for (int i = 0; name[i] != '\0'; ++i)
    {
        // Check if the current character is one of our 
        // valid characters for a variable name.
        if ( !((name[i] >= '0' && name[i] <= '9') 
            || (name[i] >= 'A' && name[i] <= 'Z') 
            || (name[i] >= 'a' && name[i] <= 'z') 
            || name[i] == '_') ) 
        {
            return 0;
        }
    }
    return 1;
}

/********************************************************************
// Takes an input string and replaces all variable names with the values
// of variables if they are found.
********************************************************************/
char* interpolateVars(char* line)
{
    char* lineCopy = strdup(line);

    char* result = malloc(MAX_BUFFER_SIZE);
    char* varValue = NULL;

    char* token = strtok(line, " \t\n/");

    // If the first character is 
    if (lineCopy && lineCopy[0] == '/')
    {
        strcat(result, "/");
    }

    while (token)
    {
        char lastCh = lineCopy[token - line - 1];
        if (lastCh == '/')
        {
            strcat(result, "/");
        }

        // Token is a variable
        if (token[0] == '$')
        {
            // Get variable value - check shell then environment
            varValue = getVar(&token[1]);
            if (!varValue)
            {
                varValue = getEnvVar(&token[1]);
                if (!varValue)
                {
                    return NULL;
                }
            }

            strcat(result, varValue);
        }

        // Token is not a variable
        else
        {
            strcat(result, token);
        }

        // Check character after the token and then add whitespace or '/'.
        char nextCh = lineCopy[token - line + strlen(token)];
        switch (nextCh)
        {
            case ' ':
            case '\t':
                strcat(result, " ");
                break;
            case '\n':
                break;
            case '/':
                strcat(result, "/");
                break;
        }

        token = strtok(NULL, " \t\n/");
    }

    free(lineCopy);

	strcat(result, "\n");
    return result;
}

//*********************************************************************
// Strips comments from input.
//********************************************************************/
char* cleanAndInterpolateInput(char* line)
{
	char* lineCopy = strdup(line);

	if (lineCopy[0] == '#' || lineCopy[0] == '\n')
	{
		return NULL;
	}

	char* token = strtok(lineCopy, "#\n");

	if (token) {
		char* l = interpolateVars(token);
		return l;
	}

	return NULL;
}

//*********************************************************************
// Takes a string and converts it into an array of strings.
//********************************************************************/ 
char** stringToArray(char* s)
{
	numArgs = 0;

	char** res = malloc(MAX_BUFFER_SIZE*sizeof(char*));
	for (int i = 0; i < MAX_BUFFER_SIZE; ++i)
	{
		res[i] = malloc(MAX_BUFFER_SIZE*sizeof(char));
	}

	char* tok = strtok(s, " \t\n");
	for (int i = 0; tok; ++i)
	{
		strcpy(res[i], tok);
		numArgs++;
		tok = strtok(NULL, " \t\n");
	}

    res[numArgs] = NULL;
    res[numArgs+1] = NULL;

	return res;
}

//*********************************************************************
// Takes an array of strings and converts it into a string.
//********************************************************************/ 
char* arrayToString(char** arr)
{
    char* res = malloc(MAX_BUFFER_SIZE*sizeof(char));
    strcpy(res, arr[0]);
    strcat(res, " ");

    int i = 1;
    while(!(arr[i] == NULL && arr[i+1] == NULL))
    {
        if (arr[i] == NULL)
        {
            strcat(res, "| ");
        }
        else
        {
            strcat(res, arr[i]);
            strcat(res, " "); 
        }
        i++;
    }

    return res;
}


#endif
