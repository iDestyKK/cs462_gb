# Test Files for OpenSHMEM

## Synopsis
I like to play around with libraries to make sure I know what I'm doing before I attempt to try a homework assignment.
Because I feel that it is worth saving for future reference, this directory contains all of the testing I've done with OpenSHMEM that are aside from the actual homework assignment.

## Compilation
...Just run `make`.

## Execution
The procedure of executing an OpenMPI, OpenMP, or OpenSHMEM program isn't exactly obvious to the average user. They have to use `mpiexec`, `mpirun`, `oshrun`, etc in order to correctly execute the application with multiple instances. However, the executable will run just fine being executed as a single instance.

In order to execute these, use `oshrun` as follows:
```bash
oshrun -np ./EXECUTABLE [args]
```
