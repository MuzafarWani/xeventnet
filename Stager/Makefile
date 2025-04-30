CC=mpicc
LIB=lib
SRC=src
HEADER=$(SRC)/headers
OBJ=obj

LOGGER=0    
LOGGER ?= 0

STANDALONE_LIB=0
STANDALONE_LIB ?= 0

ADIOS2_DIR = /path/to/adios2/install

# Use the output of adios2-config
ADIOS2_CXXFLAGS = -DADIOS2_USE_MPI -isystem $(ADIOS2_DIR)/include
ADIOS2_CXXLIBS = -Wl,-rpath,$(ADIOS2_DIR)/lib $(ADIOS2_DIR)/lib/ADIOS2_C_API  $(ADIOS2_DIR)/lib/ADIOS2_C_API_with MPI_Support -Wl,-rpath-link,$(ADIOS2_DIR)/lib

CPPFLAGS=-I$(SRC) -I$(HEADER) $(ADIOS2_CXXFLAGS)

CFLAGS=-D_GNU_SOURCE
ifeq ($(LOGGER),1)
    CFLAGS += -DLOGGER_H -DDEBUGGING_MODE
endif

ifeq ($(STANDALONE_LIB),1)
    CFLAGS += -DRUN_STANDALONE
    OUTFILE=standalone_stage_manager.so
    all: $(LIB) $(LIB)/$(OUTFILE)
endif

ifeq ($(STANDALONE_LIB),0)
    OUTFILE=stage_manager.so
    all: $(LIB) $(LIB)/$(OUTFILE)
endif

$(LIB):
	mkdir -p $(LIB) $(OBJ)

$(LIB)/$(OUTFILE): $(OBJ)/sm.o $(HEADER)/data_stager.h
	$(CC) -Wall -g -O0 -Wunreachable-code $(CFLAGS) $(CPPFLAGS) -shared -fopenmp -fPIC -o $(LIB)/$(OUTFILE) $(OBJ)/sm.o -ldl $(ADIOS2_CXXLIBS)

$(OBJ)/sm.o: $(SRC)/data_stager.c $(HEADER)/data_stager.h
	$(CC) -c -Wall -g -O0 -Wunreachable-code -Werror -fpic -fopenmp $(CFLAGS) $(CPPFLAGS) $(SRC)/data_stager.c -o $(OBJ)/sm.o 

clean:
	rm -rf $(OBJ)/*

