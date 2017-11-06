/*
 * HandyTypes
 * 
 * Description:
 *     Supplies Data Types for HandyC functions.
 *
 * Author:
 *     Clara Van Nguyen
 */

#ifndef __HANDYTYPES_C_HAN__
#define __HANDYTYPES_C_HAN__

//Unsigned Data types
typedef unsigned int       cn_uint;
typedef unsigned char      cn_byte;
typedef unsigned long long cn_ull;
typedef unsigned long long cn_u64;

//Signed
typedef int                cn_int;
typedef long long          cn_ll;
typedef long long          cn_64;

//Enums
typedef enum {
    cn_false,
    cn_true
} cn_bool;

//All Caps versions
#define CN_UINT  cn_uint
#define CN_BYTE  cn_byte
#define CN_ULL   cn_ull
#define CN_U64   cn_u64

#define CN_INT   cn_int
#define CN_LL    cn_ll
#define CN_64    cn_64

#define CN_BOOL  cn_bool
#define CN_FALSE cn_false
#define CN_TRUE  cn_true

#endif
