Stager Module: This transfers the data from WRF output netcdf functions to deep learning model. It intercepts the output functions of WRF. There is only module in the src/ named data_stager.c. ADIOS2 SST C functions are used for  data streaming from WRF to the DL model for predictions. The Makefile is for compilation of the module.

Compilation: Run Make to compile this module
