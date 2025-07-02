#include "data_stager.h"
#include <math.h>
#include <adios2_c.h>
#include <dlfcn.h>
#include <mpi.h>
#include <string.h>
#include <time.h> 
//#include "decomp.h"
//#include "mpivars.h"// For timing
#define NUM_VARS 19
int  comm_size = 0;
char open_file[255] = "";
int  cts=0;
int lstupd = 0;
int level = 1, imp_count=0;
int initflag=0;
int arraysize=0;
float *prevsrc;
simulation_data sim;
simulation_metric met;
adios2_adios *adios;
adios2_io *io;
float **data_p;
//adios2_variable *var_g, *var_t;
adios2_variable *vars[NUM_VARS];
adios2_engine *engine;
adios2_step_status err;
double total_transfer_time = 0.0;
size_t shape[19], start[19], count[19];
//const char *var_name[NUM_VARS] = {
//    "WRFPARRY", "WRFUARRY", "WRFVARRY", "WRFWARRY", "WRFTARRY", "WRFQVARRY", "WRFPBARRY", "WRFPSFCARRY", "WRFSNHARRY", "WRFSNARRY", "WRFSNCARRY", "WRFRNCARRY", "WRFRNARRY", "WRFRNSARRY", //"WRFOLARRY", "WRFU10ARRY", "WRFV10ARRY", "WRFSSTARRY" };

int GetVarIndex(char *varname)
{
    if(strcmp(varname,"P") == 0)
        return 0;
    else if(strcmp(varname,"U") == 0)
        return 1;
    else if(strcmp(varname,"V") == 0)
        return 2; 
    else if(strcmp(varname,"W") == 0)
        return 3;
    else if(strcmp(varname,"T") == 0)
        return 4;  
    else if(strcmp(varname,"QVAPOR") == 0)
        return 5;
    else if(strcmp(varname,"PB") == 0)
        return 6; 
    else if(strcmp(varname,"PSFC") == 0)
        return 7;  
    else if(strcmp(varname,"SNOWH") == 0)
        return 8;  
    else if(strcmp(varname,"SNOW") == 0)
        return 9; 
    else if(strcmp(varname,"SNOWNC") == 0)
        return 10; 
    else if(strcmp(varname,"SNOWC") == 0)
        return 11;
    else if(strcmp(varname,"RAINNC") == 0)
        return 12;   
    else if(strcmp(varname,"RAINC") == 0)
        return 13; 
    else if(strcmp(varname,"RAINSH") == 0)
        return 14; 
    else if(strcmp(varname,"OLR") == 0)
        return 15;  
    else if(strcmp(varname,"U10") == 0)
        return 16;  
    else if(strcmp(varname,"V10") == 0)
        return 17;  
    else if(strcmp(varname,"SST") == 0)
        return 18;                                               
    else
        return -1;
}

void gather_decomp_1d(size_t *mysize, size_t *myshape, size_t *myoffset)
{
    int nproc, rank;
    size_t *sizes;
    int i;
    
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    sizes = malloc(sizeof(size_t) * (size_t)nproc);
    MPI_Allgather(mysize, 1, MPI_LONG_LONG, sizes, 1, MPI_LONG_LONG, MPI_COMM_WORLD);

    *myshape = 0;
    for (i = 0; i < nproc; i++)
    {
        *myshape += sizes[i];
    }
    *myoffset = 0;
    for (i = 0; i < rank; i++)
    {
        *myoffset += sizes[i];
    }

    free(sizes);
    return;
}

int stage_open(const char *fname, unsigned int flags, int *ncid)
{
    printf("DS:Function: %s\n", __func__);
    strcpy(open_file, fname);
    printf("DS:File name passed = %s, mode = %d\n", fname, flags);
   
    int (*orig_nc_open)(const char *, unsigned int, int *) = (int (*)(const char *, unsigned int, int *))dlsym(RTLD_NEXT, "nc_open");
    return orig_nc_open(fname, flags, ncid);
}

int stage_put_vara_float(int ncid, int var_id, size_t *begin, size_t *size, float *src)
{
    printf("DS:Function: %s\n", __func__);
    char varname[255];
    int ndims=0;
    //int gres=0;
    int nranks, myrank;
    
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
        
    // Dlsym Function pointers
    int (*orig_nc_inq_varndims)(int, int, int*) = (int (*)(int, int, int*))dlsym(RTLD_NEXT, "nc_inq_varndims");
    int (*orig_nc_inq_varname)(int, int, char*) = (int (*)(int, int, char*))dlsym(RTLD_NEXT, "nc_inq_varname");
    int (*orig_nc_inq_dimlen)(int, int, int*) = (int (*)(int, int, int*))dlsym(RTLD_NEXT, "nc_inq_dimlen");
    int (*orig_nc_put_vara_float)(int, int, size_t *, size_t *, float *) = (int (*)(int, int, size_t *, size_t *, float *))dlsym(RTLD_NEXT, "nc_put_vara_float");
    int (*orig_nc_inq_var)(int, int, char*, int*, int*, int*, int*) = (int (*)(int, int, char*, int*, int*, int*, int*))dlsym(RTLD_NEXT, "nc_inq_var");

    orig_nc_inq_varndims(ncid, var_id, &ndims);
    orig_nc_inq_varname(ncid, var_id, varname);
    int var_index = GetVarIndex(varname);
    //Calculate variable size from the dimensions.
    if(ndims==2)
     {
       arraysize = size[1];
     }
    else  if(ndims==3)
      {
        arraysize = size[1]* size[2];
      }
    else if(ndims==4)
     { 
       arraysize=size[1]* size[2]* size[3];
     }
    
 
     if((cts==0) && (initflag == 0)) 
        { 
          
          // ADIOS initialization
          adios = adios2_init_mpi(MPI_COMM_WORLD); 
          io = adios2_declare_io(adios, "output"); 

          // Set the SST engine
          adios2_set_engine(io, "SST");
          data_p = (float **)malloc(NUM_VARS * sizeof(float *));
          initflag = initflag + 1;
          }  
        
      if((cts==0) && (initflag == 1))
          { 
            
            gather_decomp_1d(&count[var_index], &shape[var_index], &start[var_index]);
            vars[var_index] = adios2_define_variable(io, var_name[var_index], adios2_type_float, 1, &shape[var_index], &start[var_index], &count[var_index], adios2_constant_dims_true); 
          } 
        
        if((cts==0) && (initflag ==1)) 
          {
            // Open Adios engine after the variable creation
            engine = adios2_open(io, "adios2-wrf-array", adios2_mode_write);
          } 
      
         int *dimids = (int *) malloc(ndims * sizeof(int));
          orig_nc_inq_var(ncid, var_id, NULL, NULL, NULL, dimids, NULL);
         int *dimlens = (int *) malloc(ndims * sizeof(int));
          int i = 0;
         int dsize;
        while(i < ndims)
         {
            orig_nc_inq_dimlen(ncid, dimids[i], &dimlens[i]);
            i++;
         }
          dsize = size[1] * size[2] * size[3]; 

         if(cts==0)
          {
             data_p[var_index] = (float *)malloc(dsize * sizeof(float));
            for(int j=0; j<dsize; j++)
                 data_p[var_index][j] = src[j];
         }
        
        // Timing ADIOS data transfer
        double start_time = MPI_Wtime();
      if(var_index == 18)
         { 
        // ADIOS data transfer for T
          adios2_begin_step(engine, adios2_step_mode_append, 5.0f, &err);
          for (int i = 0; i < NUM_VARS; i++) 
            {
              adios2_put(engine, vars[i], data_p[i], adios2_mode_deferred);
            }
          adios2_end_step(engine);
         }
     
        double end_time = MPI_Wtime();
        double transfer_time = end_time - start_time;
        total_transfer_time += transfer_time;
        
        // Print timing information
        printf("Rank %d: ADIOS total data transfer till iteration %d took %f seconds\n", myrank, cts, total_transfer_time);
         
        cts=cts+1;
        if(cts == 191)
            adios2_close(engine);
      
   
    return orig_nc_put_vara_float(ncid, var_id, begin, size, src);
}

