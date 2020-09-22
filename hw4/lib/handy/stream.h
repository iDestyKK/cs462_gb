/*
 * Handy - Streams
 *
 * Description:
 *     Implements file and string streams which can be used for reading
 *     data from files or char* NULL-terminated strings.
 * 
 * Author:
 *     Clara Van Nguyen
 */

#ifndef __HANDY_STREAM_C_HAN__
#define __HANDY_STREAM_C_HAN__

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "handy.h"
#include "types.h"

//File Stream struct
typedef struct cn_fstream {
	FILE* strloc;
	char* val;
} *CN_FSTREAM;

//Buffered File Stream struct
typedef struct cn_bfstream {
	char* str, //Entire string
	    * val;
	cn_uint pos, len;
} *CN_BFSTREAM;

//String Stream struct
typedef struct cn_sstream {
	char* str, //Entire String
	    * val; //Stream Value
	cn_uint pos, len;
} *CN_SSTREAM;

//File Stream
CN_FSTREAM cn_fstream_init(FILE*);
void  cn_fstream_next(CN_FSTREAM);
char* cn_fstream_get (CN_FSTREAM);
void  cn_fstream_free(CN_FSTREAM);

//Buffered File Stream
CN_BFSTREAM cn_bfstream_init(char*);
void  cn_bfstream_next(CN_BFSTREAM);
char* cn_bfstream_get (CN_BFSTREAM);
void  cn_bfstream_free(CN_BFSTREAM);

//String Stream
CN_SSTREAM cn_sstream_init(char*);
void  cn_sstream_next(CN_SSTREAM);
char* cn_sstream_get (CN_SSTREAM);
void  cn_sstream_free(CN_SSTREAM);

#endif
