# XEventNet: Extreme Weather Event Prediction using Convolutional Neural Networks and In Situ Visualization
This project is a work for the paper titled "XEventNet: Extreme Weather Event Prediction using Convolutional Neural Networks and In Situ Visualization". 
The project predicts 4 extreme events from WRF simulation data. The extreme events include landslides, cyclones, heavy rainfalls, and snowfalls. 
The extreme event data is then visualized by Visit.

# Requirements:
ADIOS2: https://github.com/ornladios/ADIOS2

Visit: https://visit-dav.github.io/visit-website/releases-as-tables/#series-33

WRF: https://github.com/wrf-model

TensorFlow : https://www.tensorflow.org/install

Python 3 : https://www.python.org/downloads/

# Installation: Instructions for installing each of the components are given in the respective links.
Install ADIOS2 from github source code

Install Visit version suitable for your system configuration (You can choose any version of Visit >= 3.0). Download the version given in the link for your operating system version.

Install TensorFlow 2, any version > 2.15

# Setting up paths for each component:
After installing all the above. Set the paths in the Makefiles for each of the components with details given below:

1.Stager:

Set the paths of ADIOS2 installation with the variable ADIOS2_DIR

Set the ADIOS2_CXXLIBS by setting the paths to the ADIOS2 C API and ADIOS2 C API with MPI Support.

2. Data_reader

Set the ADIOS2_CXXLIBS by setting the paths to the ADIOS2 C API and ADIOS2 C API with MPI Support.

Set the NUMPY_INC path to your numpy include path.

Set the PYTHON_INC and PYTHON_LIB paths to your python include and lib

3. Visualization
   
Set the ADIOS2_CXXLIBS by setting the paths to the ADIOS2 C API and ADIOS2 C API with MPI Support.

Set the NUMPY_INC path to your numpy include path.

Set the PYTHON_INC and PYTHON_LIB paths to your python include and lib.

# Compilation

The instructions to compile are present in the respective directories

# Execution:
1. Before execution, make sure you have installed Visit on the local system as well, where you plan to visualize the data.
   
2. Set up LD_PRELOAD = /path/compiled/code/of/Stager/code on the system where you are running the WRF simulation.
   
3. Execute the WRF simulation by commands like mpirun -n <number_of_processes> ./wrf.exe
 
4. Execute the Data_reader module mpirun -n <number_of_processes> ./sim_dl
 
5. Execute the Visualization module mpirun -n <number_of_processes> ./dl_vis
6. Once the visualization module is executing, do the below:
   
	a. Launch the visit gui from the node where you want to visualize the data.

	b. Using the open button from the GUI connect to the /.visit/simulations directory.

	c. There will be a file created with a name like "001718701078.WRFReader.sim2"

	d. Connect to the latest file created from that and select the variable from Pseudocolor to visualize the variable.
    
7. Based on the architecture available, change the number of processes for each of the components.
