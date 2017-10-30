/*
 * HandyC - Exec
 * 
 * Incorporates functions to aid in forking and executing programs under POSIX.
 * 
 * Clara Van Nguyen
 */

#ifndef __HANDYC_EXEC_C_HAN__
#define __HANDYC_EXEC_C_HAN__

#include "handy.h"
#include "types.h"

typedef enum cnp_mode {
    M_NONE, M_PIPE_OUT, M_PIPE_IN, M_PIPE, M_APPEND_PIPE, M_NOWAIT
} CNP_MODE;

#define M_ON 1;

typedef struct cn_process {
    char**  argv;   //Argument Value(s)
    cn_uint argc;   //Argument Count
    cn_byte mode,   //Current Mode
            i_mode, //In Mode
            o_mode; //Out Mode
    char*   file_i, //Input Path
        *   file_o; //Output Path
} *CN_PROCESS;

CN_PROCESS new_process(cn_uint, char**);



#endif
