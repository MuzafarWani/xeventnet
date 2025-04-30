#include <dlfcn.h>
#include <climits>
#include <semaphore>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cerrno>

#include "mpi.h"


// Macro Definitions

#ifndef RUN_STANDALONE

    #define stage_open                 nc_open
    #define stage_put_vara_float       nc_put_vara_float
    #define stage_close                nc_close

#endif



#ifdef HPC
    #define STAGEDIOLOG "/scratch/deepakbh21/Framework_outfiles/stagedIOLog_%d"
    #define WRITELOG "/scratch/deepakbh21/Framework_outfiles/writeDataLog_%d"
#else
    #define STAGEDIOLOG "/home/muzafarwan/Build_WRF/WRF/test/em_real/StageIOLog_%d"
    #define WRITELOG "/home/muzafarwan/Build_WRF/WRF/test/em_real/writeDataLog_%d"
#endif


// Function prototype

int stage_open(const char *fname, unsigned int flags, int *ncid);
int stage_put_vara_float(int , int, size_t *, size_t *, float *);
int GetVarIndex(char *varname);

