DL_Model: This module has two components sim_dl.c, inference_call.py, and traning_model.py 
sim_dl.c is the module for data receiving from Stager Module. Then it calls inference_call.py which makes predictions for each of the sub-domains. Then based on predictions made it transfers the sub-domain data to the Visualization module.

training_model.py is for training the model. 

Compilation: Run Make to compile this module
