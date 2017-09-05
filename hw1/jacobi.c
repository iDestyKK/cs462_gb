#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "jacobi.h"
#include "header.h"

static int jacobi_relaxation_fini(relaxation_params_t** rp)
{
    if( NULL != (*rp)->data[0] ) free((*rp)->data[0]);
    if( NULL != (*rp)->data[1] ) free((*rp)->data[1]);
    free(*rp); *rp = NULL;
    return 0;
}

static int jacobi_relaxation_coarsen(relaxation_params_t* rp, double* dst, uint32_t dstx, uint32_t dsty)
{
    return coarsen(rp->data[(rp->idx + 1) % 2], rp->sizex, rp->sizey,
                   dst, dstx, dsty);
}

static int jacobi_relaxation_print(relaxation_params_t* rp, FILE* fp)
{
    fprintf( fp, "\n# Iteration %d\n", rp->idx);
    print_matrix(fp, rp->data[(rp->idx + 1) % 2], rp->sizex, rp->sizey);
    fprintf( fp, "\n\n");
    return 0;
}

/**
 * One step of a simple Jacobi relaxation.
 */
static double jacobi_relaxation_apply(relaxation_params_t* rp)
{
    double diff, sum = 0.0, *n, *o;
    int i, j;

    n = rp->data[(rp->idx + 0) % 2];
    o = rp->data[(rp->idx + 1) % 2];
    /* The size[x,y] account for the boundary */
    for( i = 1; i < (rp->sizex-1); i++ ) {
        for( j = 1; j < (rp->sizey-1); j++ ) {
            n[i*rp->sizex+j] = 0.25 * (o[ i     * rp->sizex + (j-1) ]+  // left
                                       o[ i     * rp->sizex + (j+1) ]+  // right
                                       o[ (i-1) * rp->sizex + j     ]+  // top
                                       o[ (i+1) * rp->sizex + j     ]); // bottom
            diff = n[i*rp->sizex+j] - o[i*rp->sizex+j];
            sum += diff * diff;
        }
    }
    rp->idx++;
    return sum;
}

static double* jacobi_relaxation_get_data(relaxation_params_t* rp)
{
    return rp->data[(rp->idx + 1) % 2];
}

static int jacobi_relaxation_init(struct hw1_params_s* hw_params, relaxation_params_t* rp)
{
    uint32_t i, j, np = hw_params->resolution + 2;

    rp->idx = 0;
    rp->sizex = np;
    rp->sizey = np;
    rp->data[0] = calloc(np*np, sizeof(double));
    rp->data[1] = (double*)malloc(np*np*sizeof(double));
    if( (NULL == rp->data[0]) && (NULL == rp->data[1]) ) {
        fprintf(stderr, "Cannot allocate the memory for the matrices\n");
        goto fail_and_return;
    }
    double dist, *mat = rp->data[0];
    for( i = 0; i < hw_params->num_sources; i++ ) {
        /**
         * The heat dissipate linearly in cercles around the central
         * point up to the defined range. It only affects the interface
         * between the mediums, so it only has an impact on the boundaries.
         */
        for( j = 1; j < np-1; j++ ) {  /* initialize the top row */
            dist = sqrt( pow((double)j/(double)(np-1) - 
                             hw_params->heat_sources[i].x, 2) +
                         pow(hw_params->heat_sources[i].y, 2));
            if( dist <= hw_params->heat_sources[i].range ) {
                mat[j] += ((hw_params->heat_sources[i].range - dist) /
                           hw_params->heat_sources[i].range *
                           hw_params->heat_sources[i].temp);
            }
        }
        for( j = 1; j < np-1; j++ ) {  /* initialize the bottom row */
            dist = sqrt( pow((double)j/(double)(np-1) - 
                             hw_params->heat_sources[i].x, 2) +
                         pow(1-hw_params->heat_sources[i].y, 2));
            if( dist <= hw_params->heat_sources[i].range ) {
                mat[(np-1)*np+j] += ((hw_params->heat_sources[i].range - dist) /
                                     hw_params->heat_sources[i].range *
                                     hw_params->heat_sources[i].temp);
            }
        }
        for( j = 1; j < np-1; j++ ) {  /* left-most column */
            dist = sqrt( pow(hw_params->heat_sources[i].x, 2) +
                         pow((double)j/(double)(np-1) -
                             hw_params->heat_sources[i].y, 2));
            if( dist <= hw_params->heat_sources[i].range ) {
                mat[j*np] += ((hw_params->heat_sources[i].range - dist) /
                              hw_params->heat_sources[i].range *
                              hw_params->heat_sources[i].temp);
            }
        }
        for( j = 1; j < np-1; j++ ) {  /* right-most column */
            dist = sqrt( pow(1-hw_params->heat_sources[i].x, 2) +
                         pow((double)j/(double)(np-1) -
                             hw_params->heat_sources[i].y, 2));
            if( dist <= hw_params->heat_sources[i].range ) {
                mat[j*np+(np-1)] += ((hw_params->heat_sources[i].range - dist) /
                                     hw_params->heat_sources[i].range *
                                     hw_params->heat_sources[i].temp);
            }
        }
    }
    /* Copy the boundary conditions on all matrices */
    memcpy(rp->data[1], rp->data[0], np*np*sizeof(double));

    return 0;
 fail_and_return:
    if( NULL != rp->data[0] ) free(rp->data[0]);
    if( NULL != rp->data[1] ) free(rp->data[1]);
    return -1;
}

const struct relaxation_function_class_s _relaxation_classes [] =
    {
        {
            .type = RELAXATION_JACOBI,
            ._init = jacobi_relaxation_init,
            ._fini = jacobi_relaxation_fini,
            ._coarsen = jacobi_relaxation_coarsen,
            ._print = jacobi_relaxation_print,
            ._relaxation = jacobi_relaxation_apply,
            ._get_data = jacobi_relaxation_get_data,
        },
    };

relaxation_params_t* relaxation_init(struct hw1_params_s* hw_params)
{
    relaxation_params_t* rp = malloc(sizeof(relaxation_params_t));
    if( NULL == rp ) {
        fprintf(stderr, "Cannot allocate memory for the relaxation structure\n");
        return NULL;
    }
    rp->rel_class = &_relaxation_classes[RELAXATION_JACOBI];
    if( 0 != rp->rel_class->_init(hw_params, rp) ) {
        free(rp);
        return NULL;
    }
    return rp;
}

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
