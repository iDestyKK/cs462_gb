/*
 * Handy - Math Library
 *
 * Description:
 *     Includes math functions.
 * 
 * Author:
 *     Clara Van Nguyen
 */

#ifndef __HANDY_MATH_C_HAN__
#define __HANDY_MATH_C_HAN__

#include "types.h"

/*
   Interpolation

   Interpolates a and b with value c.

   e.g. interpolate(1, 4, 0.5) = 2.5
*/

int     interpolate_i (int    , int    , double);
cn_uint interpolate_ui(cn_uint, cn_uint, double);
double  interpolate_d (double , double , double);

#define interpolate(a, b, value) \
	(a + (b - a) * value)

/* 
	Minmax Boundaries

	Returns either a, b, or c depending on whether or not a > c or a < b.

	e.g. minmax(7 , 5, 10) = 7
	     minmax(3 , 5, 10) = 3
	     minmax(12, 5, 10) = 10
*/

int     minmax_i (int    , int    , int    ); 
cn_uint minmax_ui(cn_uint, cn_uint, cn_uint);
double  minmax_d (double , double , double );

#define minmax(value, low_bound, high_bound) \
	((value > high_bound) ? high_bound : (value < low_bound) ? low_bound : value)

cn_byte hamming_weight(cn_u64);

#endif
