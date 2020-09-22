/*
 * HandyC - String Manipulation
 * 
 * Description:
 *     Functions to manipulate strings and write data to or read data from.
 * 
 * Author:
 *     Clara Van Nguyen
 */

#ifndef __HANDYC_STRMANIP_C_HAN__
#define __HANDYC_STRMANIP_C_HAN__

#include <stdio.h>
#include <stdlib.h>

#include "handy.h"
#include "types.h"

char*  fstrread(FILE*);
size_t fbufstr (FILE*, cn_byte*);
size_t fbufchar(FILE*, cn_byte*);

#endif
