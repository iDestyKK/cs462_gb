#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "jacobi.h"
#include "header.h"

extern const struct relaxation_function_class_s _relaxation_jacobi;
extern const struct relaxation_function_class_s _relaxation_template;
extern const struct relaxation_function_class_s _relaxation_pthread;
extern const struct relaxation_function_class_s _relaxation_mpi;

const struct relaxation_function_class_s * const _relaxation_classes[] =
    {&_relaxation_jacobi, &_relaxation_template, &_relaxation_pthread, &_relaxation_mpi, NULL};

relaxation_params_t* relaxation_init(struct hw_params_s* hw_params)
{
    relaxation_params_t* rp = NULL;

    for( int i = 0; NULL != _relaxation_classes[i]; i++ ) {
        if( _relaxation_classes[i]->type != hw_params->alg_type )
            continue;
        if( NULL == (rp =  _relaxation_classes[i]->_init(hw_params)) ) {
            return NULL;
        }
    }
    return rp;
}

#if 0
/**
 * One step of a red-black solver.
 *
 * The difference with a Jacobi is that the updates are happening
 * in place.
 */
double redblack_relaxation(relaxation_params_t* rp,
                           double* o,
                           uint32_t sizex,
                           uint32_t sizey)
{
    double nval, diff, sum = 0.0;
    int i, j;

    for( i = 1; i < sizex; i++ ) {
        for( j = 1; j < sizex; j++ ) {
            nval = 0.25 * (o[ i    *sizey + (j-1) ]+  // left
                           o[ i    *sizey + (j+1) ]+  // right
                           o[ (i-1)*sizey + j     ]+  // top
                           o[ (i+1)*sizey + j     ]); // bottom
            diff = nval - o[i*sizey+j];
            sum += diff * diff;
            o[i*sizey+j] = nval;
        }
    }
    return sum;
}
#endif

