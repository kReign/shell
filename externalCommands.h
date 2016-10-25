#ifndef EXTERNAL_COMMANDS_H
#define EXTERNAL_COMMANDS_H

/********************************************************************
// File: externalCommands.h
// Author: Alex Charles
********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <sys/resource.h>

#define MAX_NUM_JOBS 10
#define PID_PLACEHOLDER 11

static void processEnded(int);
static void catchInterrupt(int);
void initExternalCommands();
void printJobStatus(int, int);
void listJobs();
void killJob(int);
void resumeProcess(int, int);
void addProcess(int, char*, int);
char* getFullPath(char*);
int findPipe(char**);
int forkAndExec(char*, char**);
int forkAndExecPipe(char*, char**, char*, char**);
int runExternalCommand(char*, char**);

static struct {
    char name[MAX_BUFFER_SIZE];
    int pid;
    int status;
} waitingProcesses [MAX_NUM_JOBS];

static int foregroundProcess = PID_PLACEHOLDER;

//*********************************************************************
// Initializes the handler for child process termination.
//********************************************************************/
static void processEnded(int signum)
{
    int rc, status;
    rc = waitpid(-1, &status, WNOHANG);

    // If a child process has ended and we caught the status
    // (typically, not foreground).
    if (rc != 0)
    {
        for (int i = 0; i < MAX_NUM_JOBS; ++i)
        {
            if (waitingProcesses[i].pid == rc)
            {
                waitingProcesses[i].status = JOB_FINISHED;
                printJobStatus(i, 0);
                waitingProcesses[i].pid = PID_PLACEHOLDER;
            }
        }
        printf("\n");
    }

    return;
}

//*********************************************************************
// Initializes the handler for suspending current job.
//********************************************************************/
static void catchInterrupt(int signum)
{
    if (foregroundProcess != PID_PLACEHOLDER 
        && waitingProcesses[foregroundProcess].status != JOB_SUSPENDED)
    {
        waitingProcesses[foregroundProcess].status = JOB_SUSPENDED;
        printJobStatus(foregroundProcess, 0);
        kill(waitingProcesses[foregroundProcess].pid, SIGTSTP);
        printf("\n");
    }
    //tcsetpgrp(inputFD, getpgrp());

    fflush(stdout);
}

//*********************************************************************
// Initializes handlers.
//********************************************************************/
void initExternalCommands()
{
    for (int i = 0; i < MAX_NUM_JOBS; ++i)
    {
        waitingProcesses[i].pid = PID_PLACEHOLDER;
        waitingProcesses[i].status = 0;
    }

    signal(SIGTSTP, catchInterrupt);
    signal(SIGCHLD, processEnded);
}

//*********************************************************************
// Prints the status of specified job.
//********************************************************************/
void printJobStatus(int job, int killed)
{
    if (killed)
    {
        waitingProcesses[job].status = JOB_KILLED;
    }

    char* susp;
    switch(waitingProcesses[job].status)
    {
        case JOB_RUNNING:
            susp = "Running";
            break;
        case JOB_SUSPENDED:
            susp = "Suspended";
            break;
        case JOB_FINISHED:
            susp = "Finished";
            break;
        case JOB_KILLED:
            susp = "Killed";
            break;
        default:
            break;
    }

    printf("\n [%d]\t%s  \t%s", job, susp, waitingProcesses[job].name);
}

//*********************************************************************
// Lists all current jobs.
//********************************************************************/
void listJobs()
{
    printf(" ID\tStatus\t\tCMD");
    for (int i = 0; i < MAX_NUM_JOBS; ++i)
    {
        if (waitingProcesses[i].pid != PID_PLACEHOLDER)
        {
            printJobStatus(i, 0);
        }
    }
    printf("\n");
}

//*********************************************************************
// Kills a specified job.
//********************************************************************/
void killJob(int job)
{
    if (job >= MAX_NUM_JOBS)
    {
        printf("No processes with id %s\n", job);
        return;
    }

    int pid = waitingProcesses[job].pid;
    if (pid != PID_PLACEHOLDER)
    {
        waitingProcesses[job].status = JOB_KILLED;
        kill(pid, SIGKILL);
    }
    else {
        printf("No processes with id %s\n", job);
        return;
    }
}

//*********************************************************************
// Runs a suspended process in the foreground or background.
//********************************************************************/
void resumeProcess(int job, int fg)
{
    // If we are given -1 for job (no argument from user).
    int whichJob = (job == -1) ? foregroundProcess : job;

    if (whichJob != PID_PLACEHOLDER)
    {
        kill(waitingProcesses[whichJob].pid, SIGCONT);
        waitingProcesses[whichJob].status = JOB_RUNNING;
        printJobStatus(whichJob, 0);
        printf("\n");
        if (fg)
        {
            int status;
            int rc = waitpid(waitingProcesses[whichJob].pid, &status, WUNTRACED);
            if (WIFSTOPPED(status) != 1)
            {
                waitingProcesses[whichJob].pid = PID_PLACEHOLDER;
            }
        }
    }
    else
    {
        printf("No suspended process\n");
        return;
    }
}

//*********************************************************************
// Adds a process to the list of processes being tracked.
//********************************************************************/
void addProcess(int newPid, char* path, int fg)
{
    // Find where we put the new process in our current list.
    int spot = -1;
    for (int i = 0; i < MAX_NUM_JOBS; ++i)
    {
        if (waitingProcesses[i].pid == PID_PLACEHOLDER)
        {
            spot = i;
            break;
        }
    }

    // If there were no open spots.
    if (spot == -1)
    {
        printf("too many processes already running\n");
        return;
    }

    strcpy(waitingProcesses[spot].name, path);
    waitingProcesses[spot].pid = newPid;
    waitingProcesses[spot].status = JOB_RUNNING;

    // If the new process is a foreground process, we need to wait on it.
    if (fg)
    {
        foregroundProcess = spot;
        int status;
        int rc = waitpid(newPid, &status, WUNTRACED);
        if (WIFSTOPPED(status) != 1)
        {
            waitingProcesses[spot].pid = PID_PLACEHOLDER;
        }
        //tcsetpgrp(inputFD, getpgrp());
    }
    else
    {
        printf("[%d] %d\n", spot, path);
    }

    return;
}

//*********************************************************************
// Gets the full pathname for a file by searching AOSPATH.
//********************************************************************/
char* getFullPath(char* file)
{
    // Check if the file exists, otherwise check aospath for file 
    if ( (file[0] == '.' || file[0] == '/' || file[0] == '\\') 
        && access(file, F_OK) != -1)
    {
        return file;
    }
    else
    {
        char* p = getEnvVar("AOSPATH");
        if(!p)
        {
            printf("AOSPATH is not set\n");
            return NULL;
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
                strcat(path, file);

                // Check if the file exists 
                if (access(path, F_OK) != -1)
                {
                    return path;
                }

                token = strtok(NULL, ":");
            }
        }
    }

    return NULL;
}

//*********************************************************************
// Finds the location of a pipe in the argument array
// TODO: Expand to more than one pipe.
//********************************************************************/
int findPipe(char** args)
{
    for (int i = 0; i < numArgs; ++i)
    {
        if (strcmp(args[i], "|") == 0)
        {
            args[i] = NULL;
            return i+1;
        }
    }

    return -1;
}

//*********************************************************************
// Create another process and exec the given file with arguments
//********************************************************************/
int forkAndExec(char* path, char** args)
{
    int fg = 1;

    if (strcmp(args[numArgs-1], "&") == 0)
    {
        fg = 0;
        args[numArgs-1] == NULL;
    }

    int pid = fork();

    // Child process
    if (pid == 0)
    {
        // Set the CPU and Memory limits for the new process.
        struct rlimit cpu;
        cpu.rlim_cur = cpuLim;
        cpu.rlim_max = cpuLim;

        struct rlimit mem;
        mem.rlim_cur = memLim;
        mem.rlim_max = memLim;

        setrlimit(RLIMIT_CPU, &cpu);
        //setrlimit(RLIMIT_AS, &mem);

        signal(SIGTSTP, SIG_DFL);
        //signal(SIGCHLD, SIG_DFL);

        setpgid(0, 0);

        execv(path, args);
    }
    else if (pid > 0)
    {
        addProcess(pid, arrayToString(args), fg);
    }

    return 1;

}

//*********************************************************************
// Create another process and exec the given file with arguments
// Creates a pipe between two processes.
//********************************************************************/
int forkAndExecPipe(char* path1, char** args1, char* path2, char** args2)
{
    int fg = 1;

    if (strcmp(args1[numArgs-1], "&") == 0)
    {
        fg = 0;
        args1[numArgs-1] = NULL;
    }

    int fd[2];
    pipe(fd);

    int pid1 = fork();

    // Child process
    if (pid1 == 0)
    {
        // Set the CPU and Memory limits for the new process.
        struct rlimit cpu;
        cpu.rlim_cur = cpuLim;
        cpu.rlim_max = cpuLim;

        struct rlimit mem;
        mem.rlim_cur = memLim;
        mem.rlim_max = memLim;

        setrlimit(RLIMIT_CPU, &cpu);
        //setrlimit(RLIMIT_AS, &mem);

        // Reinitialize the environmental variables.
        initEnvVars();

        close(1);
        dup(fd[1]);
        close(fd[0]);
        close(fd[1]);

        signal(SIGTSTP, SIG_DFL);

        //printf("execing %s\n", path1);
        execv(path1, args1);
    }

    // Fork the second process
    int pid2 = fork();

    // Child process
    if (pid2 == 0)
    {
        // Set the CPU and Memory limits for the new process.
        struct rlimit cpu;
        cpu.rlim_cur = cpuLim;
        cpu.rlim_max = cpuLim;

        struct rlimit mem;
        mem.rlim_cur = memLim;
        mem.rlim_max = memLim;

        setrlimit(RLIMIT_CPU, &cpu);
        //setrlimit(RLIMIT_AS, &mem);

        // Reinitialize the environmental variables.
        initEnvVars();

        close(0);
        dup(fd[0]);
        close(fd[0]);
        close(fd[1]);

        signal(SIGTSTP, SIG_DFL);

        execv(path2, args2);
    }

    if (pid2 > 0)
    {
        close(fd[0]);
        close(fd[1]);

        addProcess(pid2, arrayToString(args1), fg);
    }

    return 1;
}

//*********************************************************************
// Runs an external program.
// TODO: Cut this down......
//********************************************************************/
int runExternalCommand(char* file, char** args)
{
    // Get the location of the beginning of the second filename
    // if there is a pipe.
    int splitIndex = findPipe(args);

    // If there was no pipe, fork and exec like normal.
    if (splitIndex == -1)
    {
        char* path = getFullPath(file);
        if (path != NULL)
        {
            forkAndExec(path, args);
        }

        // The file was not found
        else 
        {
            printf("%s: command not found\n", file);
        }
    }

    // Otherwise, we did find a pipe so we need to create the pipe and 
    // fork both programs.
    else 
    {
        char* firstPath = getFullPath(args[0]);
        char* secondPath = getFullPath(args[splitIndex]);

        // First program and second program were both found.
        if (firstPath != NULL && secondPath != NULL)
        {
            forkAndExecPipe(firstPath, args, secondPath, &args[splitIndex]);

            return 1;
        }

        // If one of the two programs was not found
        else
        {
            if (firstPath == NULL)
            {
                printf("%s: command not found\n", args[0]);
            }
            if (secondPath == NULL)
            {
                printf("%s: command not found\n", args[splitIndex]);
            }
        }
    }
    return 0;
}

#endif 