#include "decomp.h"
#include "mpivars.h"
#include <adios2_c.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>    
#include <Python.h>  
#include <numpy/arrayobject.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#define NUM_VARS 19

const char *var_names[NUM_VARS] = { "WRFPARRY", "WRFUARRY", "WRFVARRY", "WRFWARRY", "WRFTARRY", "WRFQVARRY", "WRFPBARRY", "WRFPSFCARRY", "WRFSNHARRY", "WRFSNARRY", "WRFSNCARRY", "WRFRNCARRY", "WRFRNARRY", "WRFRNSARRY", "WRFOLARRY", "WRFU10ARRY", "WRFV10ARRY", "WRFSSTARRY" };

// Global variables for Python function and module
PyObject *pFunc = NULL;
PyObject *pModule = NULL;

void initialize_python() {
    dlopen("/path/to/python/shared/library", RTLD_LAZY | RTLD_GLOBAL);
    // Initialize the Python Interpreter
    Py_Initialize();
   // printf("All Ok till here 4\n");
    // Add the current directory and numpy's installation path to Python's module search path
    PyRun_SimpleString("import sys; sys.path.append('.')");  
    PyRun_SimpleString("path/to/python/lib/packages"); //Path to the 

    // Import the Python module
    PyObject *pName = PyUnicode_FromString("inference_call");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        // Get the Python function
        pFunc = PyObject_GetAttrString(pModule, "predict_rainfall_from_gridded_data");

        if (!(pFunc && PyCallable_Check(pFunc))) {
            if (PyErr_Occurred()) PyErr_Print();
            Py_XDECREF(pFunc);
            pFunc = NULL;
        }
    } else {
        PyErr_Print();
    }
}

void finalize_python() {
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_Finalize();
}

int call_python_average(float **data_p, size_t *local_counts, int num_vars) 
{
    if (pFunc == NULL) {
        printf("Python function is not initialized properly\n");
        return;
    }

    // Import NumPy C API
    import_array();  // Required for NumPy C-API usage

    // Create Python list to hold NumPy arrays
    PyObject *pList = PyList_New(num_vars);

    for (int i = 0; i < num_vars; ++i) {
        npy_intp dims[1] = {local_counts[i]};  // Each variable is 1D

        // Create a NumPy array from the C float* data
        PyObject *np_array = PyArray_SimpleNewFromData(1, dims, NPY_FLOAT32, (void *)data_p[i]);

        if (!np_array) {
            PyErr_Print();
            printf("Failed to create NumPy array for variable %d\n", i);
            return;
        }

        // Set item in the list
        PyList_SetItem(pList, i, np_array); 
    }

    // Pack the list into a tuple
    PyObject *pArgs = PyTuple_Pack(1, pList);

    // Call the Python function
    PyObject *pResult = PyObject_CallObject(pFunc, pArgs);

    // Clean up
    Py_DECREF(pArgs);
    Py_DECREF(pList);

    if (pResult != NULL) {
        long result = PyLong_AsLong(pResult);  // Convert return PyObject* to long
        Py_DECREF(pResult);
        return (int)result;
    } else {
        PyErr_Print();
        printf("Python function call failed\n");
        return -1;
    }
}



void reader(adios2_adios *adios)
{   
    int step;  // Step counter
    adios2_variable *var_over;
    float **data = NULL;  // Pointer for the data
    data = (float **)malloc(NUM_VARS * sizeof(float *));
    float *t = NULL;
    const char *streamname = "adios2-wrf-array"; // Stream name for SST
    adios2_step_status err;  // ADIOS2 step status
    double total_transfer_time = 0.0;  // Total transfer time

    // MPI variables
    int nranks, myrank;
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    
    int overlay[nranks];
    size_t overlay_shape[1] = {nranks};
    size_t overlay_start[1] = {0};
    size_t overlay_count[1] = {nranks};

    // Declare ADIOS IO and set the engine to SST
    adios2_io *io = adios2_declare_io(adios, "input");
    adios2_set_engine(io, "SST");
    adios2_engine *engine = adios2_open(io, streamname, adios2_mode_read); // Open the engine for reading

    size_t global_shape[19], local_start[19], local_count[19];  // Arrays for global shape, local start, and local count

    step = 0;  // Initialize step counter

    // Loop through steps
    while (1)
    {
        
        adios2_begin_step(engine, adios2_step_mode_read, 100.0f, &err); // Start reading a step

        if (err == adios2_step_status_end_of_stream)
        {
            break; // End of stream reached
        }
        if (err != adios2_step_status_ok)
        {
            printf("Unexpected status when calling adios2_begin_step() for step %d and erro code is %d\n", step, err);
            break;
        }
        
        adios2_variable *vars[NUM_VARS] = {NULL};
        
        for (int i = 0; i < NUM_VARS; i++)
        {
            vars[i] = adios2_inquire_variable(io, var_names[i]);
            if (!vars[i])
            {
                printf("ERROR: Variable '%s' not found in step %d\n", var_names[i], step);
                break;
            }
        }

     
        if (step == 0)
        {
           for (int i = 0; i < NUM_VARS; i++)
             {
              adios2_variable_shape(&global_shape[i], vars[i]);
              decomp_1d(&global_shape[i], &local_start[i], &local_count[i]);
              data[i]=malloc(local_count[i] * sizeof(float));
             }
            //gather_decomp_1d(count, shape, start);
            var_over = adios2_define_variable(io, "overlay", adios2_type_int8_t, 1, overlay_shape, overlay_start, overlay_count, adios2_constant_dims_true);  
        }
        double start_time = MPI_Wtime(); // Start timing
        for (int i = 0; i < NUM_VARS; i++)
        {
          adios2_set_selection(vars[i], 1, &local_start[i], &local_count[i]);
          adios2_get(engine, vars[i], data[i], adios2_mode_deferred);
        }
        adios2_perform_gets(engine);

        adios2_end_step(engine); // End reading step
        double end_time = MPI_Wtime(); // End timing
        double transfer_time = end_time - start_time;
        total_transfer_time += transfer_time;
        printf("ADIOS total read time till iteration %d took %f seconds\n", step, total_transfer_time); 
     
        printf("Read 1 time step\n");

        step++;
        
        //Call Python function to calculate average
       int t = call_python_average(vars, local_count, 19);
       overlay[myrank] = t;
       if(t==1)
       {
         // Transfer variables to another stream for visualization
        adios2_begin_step(engine, adios2_step_mode_append, 100.0f, &err);
        adios2_put(engine, var_over, overlay, adios2_mode_deferred);

        for (int i = 0; i < 8; i++)
         {
            adios2_put(engine, vars, data[i], adios2_mode_deferred);
         }
        
       } 
       
      adios2_end_step(engine); 
      step++;
        
    }
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

