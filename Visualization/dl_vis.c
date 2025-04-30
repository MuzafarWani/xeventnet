#include "decomp.h"  // Custom header for decomposition functions
#include "mpivars.h" // Custom header for MPI variables
#include <adios2_c.h> // ADIOS2 C API
#include <stddef.h>   // For size_t
#include <stdio.h>    // For printf
#include <stdlib.h>   // For malloc, free
#include "libsim_n_w.c"
#include <unistd.h> 
void reader(adios2_adios *adios)
{
    int step = 0;
    double total_transfer_time = 0.0;
    int nranks, myrank;
    int ts=8;
    int num_vars = 8;
   
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    int t[nranks];

    size_t global_shape[8], local_start[8], local_count[8];
    size_t global_shape1[0], local_start1[0], local_count1[0];
    
    global_shape1[0]=nranks; local_start1[0]=0; local_count1[0]=nranks;

    float **g = (float **)malloc(8 * sizeof(float *));
    if (!g)
    {
        printf("ERROR: Failed to allocate memory for variable pointers\n");
        return;
    }

    // Open transfer stream
    const char *streamname = "adios2-wrf-array-transfer";
    adios2_io *io = adios2_declare_io(adios, "transfer");
    adios2_set_engine(io, "SST");
    adios2_engine *engine = adios2_open(io, streamname, adios2_mode_read);

    adios2_step_status err;

    while (1)
    {
        double start_time = MPI_Wtime();
        adios2_begin_step(engine, adios2_step_mode_read, 100.0, &err);

        if (err == adios2_step_status_end_of_stream)
            break;

        if (err != adios2_step_status_ok)
        {
            printf("Unexpected status when calling adios2_begin_step() for step %d\n", step);
            break;
        }
        
        adios2_variable *var_over = adios2_inquire_variable(io, "overlay");
        adios2_variable **vars = malloc(num_vars * sizeof(adios2_variable*));
        
        

        for (int i = 0; i < 8; i++)
        {
            vars[i] = adios2_inquire_variable(io, var_names[i]);
            if (vars[i] == NULL)
            {
                printf("ERROR: Variable '%s' not found in step %d\n", var_names[i], step);
                continue;
            }

            if (step == 0)
            {
                adios2_variable_shape(&global_shape1[0], var_over);
                adios2_variable_shape(&global_shape[i], vars[i]);
                decomp_1d(&global_shape1[i], local_start1[0], local_count1[0]);
                decomp_1d(&global_shape[i], local_start[i], local_count[i]);
                g[i] = (float *)malloc(local_count[i] * sizeof(float));
                adios2_set_selection(var_over, 1, local_start1, local_count1);
                adios2_get(engine, var_over, t, adios2_mode_deferred);
                
                if (!g[i])
                {
                    printf("ERROR: Memory allocation failed for variable '%s'\n", var_names[i]);
                    continue;
                }

                printf("Read plan for '%s': rank = %d global shape = %zu local count = %zu offset = %zu\n",
                       var_names[i], myrank, global_shape[i], local_count[i], local_start[i]);
            }

            adios2_set_selection(vars[i], 1, local_start[i], local_count[i]);
            adios2_get(engine, vars[i], g[i], adios2_mode_deferred);
        }
     
        adios2_perform_gets(engine);
        adios2_end_step(engine);
        
       // if(t[myrank]==1)
       // {
       //  copy_data_var(vars,nranks);
       // }
        
       controlloop(g, &step, ts, step);
         

        double end_time = MPI_Wtime();
        total_transfer_time += (end_time - start_time);
        

        printf("Transfer read step %d complete, total time so far: %f seconds\n", step, total_transfer_time);
        step++;
    }

    // Clean up memory
    for (int i = 0; i < 8; i++)
    {
        if (g[i]) free(g[i]);
    }
    free(g);

    adios2_close(engine);
}

int main(int argc, char *argv[]) {
#if ADIOS2_USE_MPI
    init_mpi(123, argc, argv);
#endif

    {
#if ADIOS2_USE_MPI
        adios2_adios *adios = adios2_init_mpi(MPI_COMM_WORLD);
#else
        adios2_adios *adios = adios2_init();
#endif
        reader(adios);
        adios2_finalize(adios);
    }

#if ADIOS2_USE_MPI
    finalize_mpi();
#endif

    return 0;
}

