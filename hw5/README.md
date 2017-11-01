Steps to add your algorithm into the provided skeleton. The relax_template.c is supposed to provide an example on how you can easily implement multiple relaxation algorithms using the same interface.

All the following steps assumes you have the most recent copy of the homework and that your shell is in the directory hw4.

### Adding a new algorithm

- check the defines in jacobi.h to find the one corresponding to the homework. For the OpenSHMEM homework, the identifier is RELAXATION\_JACOBI_OSHMEM. This is important as it will allow you to correctly add your algorithm.
- make a copy of relax\_template.c as relax\_bstudent.c (or any other name you prefer), and add it as a dependency to the libhw.a line in the Makefile.
- in relax\_bstudent.c replace all ```template``` by ```oshmem_jacobi``` (or another name of your liking).
- at the bottom of the file change the variable \_relaxation\_oshmem\_jacobi (due to the name change in the previous step) to have the field ```.type``` set to RELAXATION\_JACOBI_OSHMEM.
- in relaxation.c add the prototype of your algorithm and then add it to the \_relaxation\_classes. You should have something similar to:

extern const struct relaxation\_function\_class\_s \_relaxation\_oshmem\_jacobi;

const struct relaxation\_function\_class\_s const* \_relaxation\_classes[] =
    {&\_relaxation\_jacobi, &\_relaxation\_template, ..., &\_relaxation\_oshmem\_jacobi, NULL};

The .dat examples have been modified to use the OSHMEM algorithm, as a result your algorithm should be automatically called by the hw.c.

### Reacting to the configuration file

The elements in the configuration file (the .dat files) are fixed. The tester I provide will not understand anything that is not already there, and will therefore not be able to supply them to your algorithm. If you think there is a lack of support for some awesome feature, propose a patch via a pull request and we'll see.

In any case the configuration file is loaded by the ```read_input_file``` function into a well defined structure, ```hw_params_t```. This includes the problem size you are executing as well as the number of threads that will be used (in the ```num_threads```). Your algorithm is expected to take this number in to account and start the correct number of threads and put them all to work in the resolution of the Jacobi relaxation.

### Initialization/execution/finalization

Preparing the environment for MPI communications should be done only once in the initialization. This will basically identify the process placement in the grid of processes, set the boundaries of the data is will handle, and everything else you might consider necessary for the safe setup of your distributed relaxation algorithm. The ```_fini``` function should do all the opposite in order to leave the software infrastructure in a sane and clean state. Unlike in the pthread example, in a distributed environment all processes will call all the functions (including ```_init```, ```_fini```, ```_apply``` and ```_coarsen```). The ```_apply``` function will the called by all processes together and they will each compute the local relaxation and the residual. A global residual will be computed by all processes using a reduction operation, and this residual will be returned by each process ```_apply```. Similarly to the previous homework, the ```_apply``` function executes a single iteration of the Jacobi relaxation.

### Exposing data

Last but not least the ```_get_data``` is a critical function. It is supposed to return the pointer to the local matrix (in a distributed execution each process will return a pointer to it's local part of the entire data) that contains the most up-to-date version of the local data, i.e. the data as updated by the last iteration. This function is what my tester is using to extract the data from you algorithm and to validate it against a known set of data. It is assumed that this pointer points to the entire local data, including the ghost region around the perimeter of the local matrix tile.

### Avoiding conflicts

If you want to simplify your life and prevent your code from generating conflicts with my version, you should avoid as much as possible directly altering any of the provided files (that's why I suggested to make a copy of the template instead of modifying it directly). This way you will be able to easily pull any update on the COSC462 repository and integrate it seamlessly into your copy.
