#include <stdio.h>
#include <string.h>
#include <VisItControlInterface_V2.h>
#include <VisItDataInterface_V2.h>
#include "SimulationExample.h"

#define SIM_STOPPED 0
#define SIM_RUNNING 1

#define VISIT_COMMAND_PROCESS 0
#define VISIT_COMMAND_SUCCESS 1
#define VISIT_COMMAND_FAILURE 2


typedef struct
{
 
    MPI_Comm par_comm;
    int     cycle;
    double  time;
    int     runMode;
    int     done;
    int     par_rank;
    int     par_size;
    int     echo;
    int     nvars; 
    int     nx;
    int     ny;
    int     nz;
    int     upd;
    int     lstupd;
    int     i_beg;
    int    *impindex;
    int     visflag;
    float  *simvar;
    float  **bsimvar;
    
    float   *prevdata;
} simulation_data;

const char *cmd_names[] = {"halt", "step", "run", "update"};
const char *var_names[] = {"P"};
const char *mesh_var[]  = {"WRF","x","y","z"};
const char *mesh_labels[] = {"west_east_stag", "south_north", "bottom_top"};
int y_beg[] = {0,334};
int var_index=0;
int iterr=0;

/* Data Access Function prototypes */
visit_handle SimGetMetaData(void *);
visit_handle SimGetMesh(int, const char *, void *);
visit_handle SimGetVariable(int, const char *, void *);
visit_handle SimGetDomainList(const char *, void *);

void copy_data(size_t *size, float *src, simulation_data *sim, int nranks, int myrank, int *cts,int index,int var_index)
{

 int dsize = size[1] * size[2] * size[3];
//  sim->simvar = (float *) malloc(dsize * sizeof(float));
  sim->par_rank = myrank;
  sim->par_size = nranks;
  sim->runMode = SIM_STOPPED;
  sim->visflag = 0;
  sim->done = 0;
  sim->time = 0.;
  sim->nx = size[3];
  sim->ny = size[2];
  sim->nz = size[1];
  sim->lstupd = 0;
 
 
 
  for(int i=0; i<dsize;i++)
    sim->bsimvar[index][i] = src[i]; 
   
  index++;   
   if(index == 10)
    {
     index=0;
    } 
    
     
}





//static int visit_broadcast_int_callback(int *value, int sender, void *cbdata)
/*static int visit_broadcast_int_callback(int *value, int sender)
{ 
    return MPI_Bcast(value, 1, MPI_INT, sender, MPI_COMM_WORLD);
}

//static int visit_broadcast_string_callback(char *str, int len, int sender, void *cbdata)
static int visit_broadcast_string_callback(char *str, int len, int sender)
{
    return MPI_Bcast(str, len, MPI_CHAR, sender, MPI_COMM_WORLD);
}


static void BroadcastSlaveCommand(int *command, simulation_data *sim)
{
    MPI_Bcast(command, 1, MPI_INT, 0, MPI_COMM_WORLD);
}


void SlaveProcessCallback(void *cbdata)
{
    simulation_data *sim = (simulation_data *)cbdata;
    int command = VISIT_COMMAND_PROCESS;
    BroadcastSlaveCommand(&command, sim);
}

*/

void copy_data_var(float **var,int nranks){}
void initilize_visit(simulation_data *sim)
{
  //  char *env = NULL;
    int nranks,myrank;
    
     MPI_Comm_size(MPI_COMM_WORLD,&nranks);
     MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
     printf("DS point 1 and my rank is %d\n",myrank);
     
     
     // VisItSetBroadcastIntFunction2(visit_broadcast_int_callback, (void*)&sim);
     // VisItSetBroadcastStringFunction2(visit_broadcast_string_callback, (void*)&sim);   
     //  VisItSetBroadcastIntFunction(visit_broadcast_int_callback);
     //  VisItSetBroadcastStringFunction(visit_broadcast_string_callback);
     //  VisItSetParallel(nranks > 1);
     //  VisItSetParallelRank(myrank);
          
  //   if(myrank == 0)
  //     env = VisItGetEnvironment();
  //       printf("DS point 4\n");
  //   VisItSetupEnvironment2(env);
     VisItSetupEnvironment();   
  //    if(env != NULL)
  //        free(env);
 
  //  printf("DS point 5\n");
    if(myrank == 0)
     {
        VisItInitializeSocketAndDumpSimFile( "WRF", "Demonstrates 3D Rectilinear mesh","/path/to/where/sim/was/started", NULL, NULL, SimulationFilename());
     }
     
}

/*
int ProcessVisItCommand(simulation_data *sim)
{
    int command = VISIT_COMMAND_PROCESS;
    if (sim->par_rank==0)
    {  
        int success = VisItProcessEngineCommand();

        if (success == VISIT_OKAY)
        {
            command = VISIT_COMMAND_SUCCESS;
            BroadcastSlaveCommand(&command, sim);
            return 1;
        }
        else
        {
            command = VISIT_COMMAND_FAILURE;
            BroadcastSlaveCommand(&command, sim);
            return 0;
        }
    }
    else
    {
        
        while (1)
        {
            BroadcastSlaveCommand(&command, sim);
            switch (command)
            {
            case VISIT_COMMAND_PROCESS:
                VisItProcessEngineCommand();
                break;
            case VISIT_COMMAND_SUCCESS:
                return 1;
            case VISIT_COMMAND_FAILURE:
                return 0;
            }
        }
    }
}

*/
void simulate_one_timestep(simulation_data *sim, int *cts)
{
  int t=0;
  t = t+10*100;
  sim->upd = t;
  if(sim->cycle != *cts)
   {
    sim->cycle = *cts;
    VisItTimeStepChanged();
    VisItUpdatePlots();
   }
  
}

void ControlCommandCallback(const char *cmd, const char *args, void *cbdata)
{
    simulation_data *sim = (simulation_data *)cbdata;

    if(strcmp(cmd, "halt") == 0)
        sim->runMode = SIM_STOPPED;
    else if(strcmp(cmd, "run") == 0)
        sim->runMode = SIM_RUNNING;
    else if(strcmp(cmd, "update") == 0)
    {
        VisItTimeStepChanged();
        VisItUpdatePlots();
    }
}

/* Called to handle case 3 from VisItDetectInput where we have console
 * input that needs to be processed in order to accomplish an action.
 */
void
ProcessConsoleCommand(simulation_data *sim)
{
    /* Read A Command */
    char cmd[1000];

    int iseof = (fgets(cmd, 1000, stdin) == NULL);
    if (iseof)
    {
        sprintf(cmd, "quit");
        printf("quit\n");
    }

    if (strlen(cmd)>0 && cmd[strlen(cmd)-1] == '\n')
        cmd[strlen(cmd)-1] = '\0';

    if(strcmp(cmd, "quit") == 0)
        sim->done = 1;
    else if(strcmp(cmd, "halt") == 0)
        sim->runMode = SIM_STOPPED;
  
    else if(strcmp(cmd, "run") == 0)
        sim->runMode = SIM_RUNNING;
    else if(strcmp(cmd, "update") == 0)
    {
        VisItTimeStepChanged();
        VisItUpdatePlots();
    }
}


void controlloop(simulation_data *sim, int *cts, int var_i, int itr)
{    
    int blocking, visitstate = 0;
    var_index = var_i;
    iterr = itr;
   
 //   if (sim->par_rank == 0)
 //  {
        fprintf(stderr, "command> ");
        fflush(stderr);
 //   }

    do
    {
          if (sim->runMode == SIM_STOPPED) 
              blocking = 0;          
          else 
              blocking = 1;


        
     //   if(sim->par_rank == 0)
          visitstate = VisItDetectInput(blocking, fileno(stdin));
     //  if(sim->par_size > 1)
     //     MPI_Bcast(&visitstate, 1, MPI_INT, 0, MPI_COMM_WORLD);  
        
        if(visitstate >= -5 && visitstate <= -1)
        {
            fprintf(stderr, "Can't recover from error!\n");
        }
        else if(visitstate == 0)
        {
            /* There was no input from VisIt, return control to sim. */
            simulate_one_timestep(sim,cts);
        }
        else if(visitstate == 1)
        {    printf("Visit:Inside\n");
           
            if(VisItAttemptToCompleteConnection() == VISIT_OKAY)
            {   
                fprintf(stderr, "VisIt connected\n");
                printf("Visit:Inside1\n");
                VisItSetCommandCallback(ControlCommandCallback, (void*)sim);
              //  VisItSetSlaveProcessCallback2(SlaveProcessCallback, (void*)sim);
                  
                VisItSetGetMetaData(SimGetMetaData, (void*)sim);
                VisItSetGetMesh(SimGetMesh, (void*)sim);
                VisItSetGetVariable(SimGetVariable, (void*)sim);
             //   VisItSetGetDomainList(SimGetDomainList, (void*)sim);
              //  sim->visflag=1;
                printf("Visit : visflag =%d\n",sim->visflag);  
                
            }
            else
                fprintf(stderr, "VisIt did not connect\n");
        }
        else if(visitstate == 2)
        {
            
            if(VisItProcessEngineCommand() == VISIT_ERROR)
            {
                /* Disconnect on an error or closed connection. */
                VisItDisconnect();
                /* Start running again if VisIt closes. */
                sim->runMode = SIM_RUNNING;
            }
        }
        else if(visitstate == 3)
        {
            ProcessConsoleCommand(sim);
            fprintf(stderr, "command> ");
            fflush(stderr);
        }
        
        if(sim->visflag == 3)
          *cts = *cts + 1;
     // printf("Visit : visflag =%d\n",sim->visflag);  
    } while(sim->visflag < 3);

}

visit_handle
SimGetMetaData(void *cbdata)
{
    visit_handle md = VISIT_INVALID_HANDLE;
    simulation_data *sim = (simulation_data *)cbdata;

    /* Create metadata. */
    if(VisIt_SimulationMetaData_alloc(&md) == VISIT_OKAY)
    {
        int i;
        visit_handle mmd = VISIT_INVALID_HANDLE;
        visit_handle vmd = VISIT_INVALID_HANDLE;

        /* Set the simulation state. */
        VisIt_SimulationMetaData_setMode(md, (sim->runMode == SIM_STOPPED) ?
            VISIT_SIMMODE_STOPPED : VISIT_SIMMODE_RUNNING);
        VisIt_SimulationMetaData_setCycleTime(md, sim->cycle, sim->time);

        /* Add mesh metadata. */
        if(VisIt_MeshMetaData_alloc(&mmd) == VISIT_OKAY)
        {
            /* Set the mesh's properties.*/
            VisIt_MeshMetaData_setName(mmd, mesh_var[0]);
            VisIt_MeshMetaData_setMeshType(mmd, VISIT_MESHTYPE_RECTILINEAR);
            VisIt_MeshMetaData_setTopologicalDimension(mmd, 3);
            VisIt_MeshMetaData_setSpatialDimension(mmd, 3);
             VisIt_MeshMetaData_setNumDomains(mmd,1);
    
            VisIt_MeshMetaData_setXUnits(mmd, mesh_var[1]);  
            VisIt_MeshMetaData_setYUnits(mmd, mesh_var[2]);
            VisIt_MeshMetaData_setZUnits(mmd, mesh_var[3]);
            
            VisIt_MeshMetaData_setXLabel(mmd, mesh_labels[0]);
            VisIt_MeshMetaData_setYLabel(mmd, mesh_labels[1]);
            VisIt_MeshMetaData_setZLabel(mmd, mesh_labels[2]);
            

            VisIt_SimulationMetaData_addMesh(md, mmd);
        }
        
      

        /* Add variable metadata. */
        if(VisIt_VariableMetaData_alloc(&vmd) == VISIT_OKAY)
        {
            VisIt_VariableMetaData_setName(vmd, var_names[0]);
            VisIt_VariableMetaData_setMeshName(vmd, "WRF");
             VisIt_VariableMetaData_setUnits(vmd, "Pa");
            VisIt_VariableMetaData_setType(vmd, VISIT_VARTYPE_SCALAR);
            VisIt_VariableMetaData_setCentering(vmd, VISIT_VARCENTERING_ZONE);
            VisIt_SimulationMetaData_addVariable(md, vmd);
        }
      
        
        /* Add some commands. */
        for(i = 0; i < sizeof(cmd_names)/sizeof(const char *); ++i)
        {
            visit_handle cmd = VISIT_INVALID_HANDLE;
            if(VisIt_CommandMetaData_alloc(&cmd) == VISIT_OKAY)
            {
                VisIt_CommandMetaData_setName(cmd, cmd_names[i]);
                VisIt_SimulationMetaData_addGenericCommand(md, cmd);
            }
        }
    }
    
    sim->visflag = sim->visflag + 1; 
    printf("Visit: SimGetMetaData fine\n");
    return md;
}


//Get mesh structure
visit_handle
SimGetMesh(int domain, const char *name, void *cbdata)
{
    
    simulation_data *sim = (simulation_data *)cbdata;
     float rmesh_x[sim->nx+1];
     float rmesh_y[sim->ny+1];
     float rmesh_z[sim->nz+1];
  //   int rank = sim->par_rank;
    
    for(int j=0; j<(sim->nx+1); j++)
          rmesh_x[j]=j;
        
     for(int j=0; j<(sim->ny+1); j++)
      {  // rmesh_y[j]= j + y_beg[rank];
         rmesh_y[j]= j;
       }
        
    for(int i=0;i<sim->nz+1;i++)
         rmesh_z[i]=i;    
     
    visit_handle h = VISIT_INVALID_HANDLE;
    
    
    if(strcmp(name, "WRF") == 0)
    {
        if(VisIt_RectilinearMesh_alloc(&h) != VISIT_ERROR)
        {
            visit_handle hx, hy, hz;

            VisIt_VariableData_alloc(&hx);
            VisIt_VariableData_alloc(&hy);
            VisIt_VariableData_alloc(&hz);
            VisIt_VariableData_setDataF(hx, VISIT_OWNER_COPY, 1, sim->nx+1, rmesh_x);
            VisIt_VariableData_setDataF(hy, VISIT_OWNER_COPY, 1, sim->ny+1, rmesh_y);
            VisIt_VariableData_setDataF(hz, VISIT_OWNER_COPY, 1, sim->nz+1, rmesh_z);
            VisIt_RectilinearMesh_setCoordsXYZ(h, hx, hy, hz);
        }
    }
    printf("Visit: SimGetMesh fine\n");
    sim->visflag = sim->visflag + 1;
    return h;
}

//Get varaible data
visit_handle
SimGetVariable(int domain, const char *name, void *cbdata)
{
    
    visit_handle h = VISIT_INVALID_HANDLE;
    
    simulation_data *sim = (simulation_data *)cbdata;
   
    if(strcmp(name, "P") == 0)
    {
        if(VisIt_VariableData_alloc(&h) != VISIT_ERROR)
            {
             VisIt_VariableData_setDataF(h, VISIT_OWNER_SIM, 1,sim->nx*sim->ny*sim->nz,(float *)sim->simvar);
              
            }
    }
    sim->visflag = sim->visflag + 1;
    printf("Visit: SimGetVariable fine\n");
    return h;
}

//Get the domain list

visit_handle
SimGetDomainList(const char *name, void *cbdata)
{
    visit_handle h = VISIT_INVALID_HANDLE;
    //int par_rank, par_size;
    
    //MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
    //MPI_Comm_size(MPI_COMM_WORLD, &par_size);
    
    if(VisIt_DomainList_alloc(&h) != VISIT_ERROR)
    {
        visit_handle hdl;
        int *iptr = NULL;
         simulation_data *sim = (simulation_data *)cbdata;
 
        iptr = (int *)malloc(sizeof(int));
        *iptr = sim->par_rank;

        VisIt_VariableData_alloc(&hdl);
        VisIt_VariableData_setDataI(hdl, VISIT_OWNER_SIM, 1, 1, iptr);
        VisIt_DomainList_setDomains(h, sim->par_size, hdl);
    }
    printf("Visit: DomainList fine\n");
    return h;
    return h;
}



