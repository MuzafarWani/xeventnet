#include <dlfcn.h>
#include <limits.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "mpi.h"


// Macro Definitions

#ifndef RUN_STANDALONE

    #define stage_open                 nc_open
    #define stage_put_vara_float       nc_put_vara_float
    #define stage_close                nc_close

#endif

// Global variables
extern int global_key;
extern int shm_id_key;


// User defined structures

typedef struct dimension_data
{
    int ndims;
    int shm_dimlen_id;
    size_t *dim_len;
}Dimdata;



#ifdef HPC
    #define STAGEDIOLOG "/scratch/deepakbh21/Framework_outfiles/stagedIOLog_%d"
    #define WRITELOG "/scratch/deepakbh21/Framework_outfiles/writeDataLog_%d"
#else
    #define STAGEDIOLOG "/home/muzafarwan/Build_WRF/WRF/test/em_real/StageIOLog_%d"
    #define WRITELOG "/home/muzafarwan/Build_WRF/WRF/test/em_real/writeDataLog_%d"
#endif

extern FILE *stagedIOLog, *writeDataLog;
// Function prototype


int stage_open(const char *fname, unsigned int flags, int *ncid);
int stage_put_vara_float(int , int, size_t *, size_t *, float *);
int GetVarIndex(char *varname);

