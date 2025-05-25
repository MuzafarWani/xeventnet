# Compiler
CXX = mpicc

# Source files
SRCS = dl_vis.c decomp.c mpivars.c

# Output executable
TARGET = dl_vis

# ADIOS2 paths - replace these with your actual paths
ADIOS2_DIR = /path/to/adios2/install

# libsim paths - replace these with your actual paths
LIBSIM_INC = -I/path/to/visit/include
LIBSIM_LIB = -L/path/to/visit/lib

# Python paths - replace these with your actual paths
PYTHON_INC = -I/path/to/python/include
PYTHON_LIB = -L/path/to/python/lib

# NumPy include path (based on actual output)
NUMPY_INC = -I/path/to/numpy/include

# Compilation and linking flags
ADIOS2_CXXFLAGS = -DADIOS2_USE_MPI -isystem $(ADIOS2_DIR)/include
ADIOS2_CXXLIBS = -Wl,-rpath,$(ADIOS2_DIR)/lib $(ADIOS2_DIR)/path/adios/c/shared/library $(ADIOS2_DIR)/path/adios/c_mpi/shared/library -Wl,-rpath-link,$(ADIOS2_DIR)/lib

LIBSIM_CXXFLAGS = $(LIBSIM_INC)
LIBSIM_CXXLIBS = -lsimV2 $(LIBSIM_LIB) -ldl

PYTHON_CXXFLAGS = $(PYTHON_INC) $(NUMPY_INC)
PYTHON_CXXLIBS = -lpython3.12 $(PYTHON_LIB)

# Additional flags and libraries
CXXFLAGS = -O2 -g $(ADIOS2_CXXFLAGS) $(LIBSIM_CXXFLAGS) $(PYTHON_CXXFLAGS)
LDFLAGS = $(ADIOS2_CXXLIBS) $(LIBSIM_CXXLIBS) $(PYTHON_CXXLIBS) -lm

# Include paths for additional headers
INCLUDES = -I.

# Main build target
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(SRCS) $(LDFLAGS)

# Clean up build files
clean:
	rm -f $(TARGET) *.o

.PHONY: clean

