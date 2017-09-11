Steps to add your algorithm into the provided skeleton. The relax_template.c is supposed to provide an example on how you can easily implement multiple relaxation algorithms using the same interface.

All the following steps assumes you have the most recent copy of the homework and that your shell is in the directory hw1.

### Adding a new algorithm

- check the defines in jacobi.h to find the one corresponding to the homework. For the pthread homework, the identifier is RELAXATION\_JACOBI_PTHREADS. This is important as it will allow you to correctly add your algorithm.
- make a copy of relax\_template.c as relax\_bstudent.c (or any other name you prefer), and add it as a dependency to the libhw1.a line in the Makefile.
- in relax\_bstudent.c replace all ```template``` by ```pthread_jacobi``` (or another name of your liking).
- at the bottom of the file change the variable \_relaxation_pthread_jacobi (because of the name change in the previous step) to have the field ```.type``` set to RELAXATION\_JACOBI_PTHREADS.

From now on if you modify the .dat examples and replace the line ```alg 0``` by ```alg 2``` your algorithm will be called.

### Reacting to the configuration file

The elements in the configuration file (the .dat files) are fixed. The tester I provide will not understand anything that is not already there, and will therefore not be able to supply them to your algorithm. If you think there is a lack of support for some awesome feature, propose a patch via a pull request and we'll see.

In any case the configuration file is loaded by the ```read_input_file``` function into a well defined structure, ```hw1_params_t```. This includes the problem size you are executing as well as the number of threads that will be used (in the ```num_threads```). Your algorithm is expected to take this number in to account and start the correct number of threads and put them all to work in the resolution of the Jacobi relaxation.

### Initialization/execution/finalization

Starting threads is an expensive operation, one that you don't want to have in the part of the algorithm that is timed. To solve this problem you should start your threads in the initialization function (```pthread_jacobi_relaxation_init```), and turn them off in the finalization (```pthread_jacobi_relaxation_fini```). The function ```pthread_jacobi_relaxation_apply``` is there to execute a single iteration of the multi-threaded Jacobi relaxation. This is the core of your homework, the function that will define the grade.

### Exposing data

Last but not least the ```pthread_jacobi_relaxation_get_data``` is a critical function. It is supposed to return the pointer to the matrix that contains the most up-to-date version of the data, i.e. the data as updated by the last iteration. This function is what my tester is using to extract the data from you algorithm and to validate it against a known set of data.

### Avoiding conflicts

If you want to simplify your life and prevent your code from generating conflicts with my version, you should avoid as much as possible directly altering my files (that's why I suggested to make a copy of the template instead of modifying it directly). This way you will be able to easily pull any update on the COSC462 repository and integrate it seamlessly into your copy.
