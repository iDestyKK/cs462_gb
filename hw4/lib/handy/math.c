/*
 * Handy - Math Library
 *
 * Description:
 *     Includes math functions.
 * 
 * Author:
 *     Clara Van Nguyen
 */

#include "math.h"

/*
   Interpolation

   Interpolates a and b with value c.
   e.g. interpolate(1, 4, 0.5) = 2.5
*/

int     interpolate_i (int     a, int     b, double c) { return a + (b - a) * c; }
cn_uint interpolate_ui(cn_uint a, cn_uint b, double c) { return a + (b - a) * c; }
double  interpolate_d (double  a, double  b, double c) { return a + (b - a) * c; }

/* 
	Minmax Boundaries
	
	Returns either a, b, or c depending on whether or not a > c or a < b.
	
	e.g. minmax(7 , 5, 10) = 7
	     minmax(3 , 5, 10) = 3
	     minmax(12, 5, 10) = 10
*/

int     minmax_i (int     a, int     b, int     c) { return (a > c) ? c : (a < b) ? b : a; }
cn_uint minmax_ui(cn_uint a, cn_uint b, cn_uint c) { return (a > c) ? c : (a < b) ? b : a; }
double  minmax_d (double  a, double  b, double  c) { return (a > c) ? c : (a < b) ? b : a; } 

cn_byte hamming_weight(cn_u64 val) {
	cn_u64 bb = val - ((val >> 1) & 0x5555555555555555ULL);
	bb = (bb & 0x3333333333333333ULL) + ((bb >> 2) & 0x3333333333333333ULL);
	bb = (bb + (bb >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
	return (bb * 0x0101010101010101ULL) >> 56;
}
