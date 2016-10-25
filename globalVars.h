#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

/********************************************************************
// File: globalVars.h
// Author: Alex Charles
********************************************************************/
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 256
#define MAX_NUM_VARS 512

#define JOB_RUNNING 0
#define JOB_SUSPENDED 1
#define JOB_FINISHED 2
#define JOB_KILLED 3

static int inputFD = NULL;

static int numArgs = 0;

static int cpuLim = -1;
static int memLim = -1;

extern char** environ;

#endif 