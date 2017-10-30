/*
 * HandyC - Memory Management
 * 
 * Description:
 *     Implements a map data structure to keep track of malloc calls and frees.
 *     This allows commands to exist which free all malloc'd space at will, or
 *     tell the user if there are other malloc calls which need to be freed.
 *
 *     As of now, the pointer to allocated memory is stored in a CN_List, but
 *     when CN_Maps are done being developed, it will then be stored in that as
 *     its lookup time is O(log n).
 *
 * Author:
 *     Clara Van Nguyen
 */

#ifndef __HANDY_LAZYMALLOC_C_HAN__
#define __HANDY_LAZYMALLOC_C_HAN__

#include "handy.h"
#include "types.h"
#include "cnds/cn_list.h"

//Global Variable for storing malloc calls
extern CN_LIST __malloc_list;

//Malloc Functions
void* lmalloc (size_t);
void* lcalloc (size_t, size_t);
void* lrealloc(void* , size_t);

//Free Functions
void lfree(void*);
void lfreeall();

//Malloc Information
cn_uint num_of_malloc();

//"free()" Shortcuts
void free_cstr_array(char**);

//Functions you won't use if you are sane
void __initialise_malloc_list();

#endif
