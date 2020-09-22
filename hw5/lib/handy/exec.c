/*
 * HandyC - Exec
 * 
 * Incorporates functions to aid in forking and executing programs under POSIX.
 * 
 * Clara Van Nguyen
 */

#include <stdio.h>
#include "exec.h"

CN_PROCESS cn_process_init(cn_uint argc, char** argv) {
    CN_PROCESS process = malloc(sizeof(struct cn_process));
    if (process == NULL)
        return NULL;
    
    //Default Values
    process->argv   = NULL;
    process->argc   = 0;
    process->mode   = 0;
    process->i_mode = 0;
    process->o_mode = 0;
    process->file_i = NULL;
    process->file_o = NULL;
    
    return process;
}
