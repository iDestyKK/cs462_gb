/*
 * Handy - File Management
 *
 * Description:
 *     Includes functions which involve file I/O and management.
 * 
 * Author:
 *     Clara Van Nguyen
 */

#ifndef __HANDY_FILEMGMT_C_HAN__
#define __HANDY_FILEMGMT_C_HAN__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "types.h"

cn_bool file_exists   (char*);
size_t  get_file_size (char*);

cn_byte array_to_file (char**, const char*);
char**  file_to_array (        const char*);

#endif
