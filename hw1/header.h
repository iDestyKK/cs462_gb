/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2016-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * @Author: George Bosilca
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef HW1_HEADER_HAS_BEEN_INCLUDED
#define HW1_HEADER_HAS_BEEN_INCLUDED

#include <stdio.h>
#include <stdint.h>

#define HW1_FLAG_GENERATE_HEAT_IMAGE  0x0001
#define HW1_FLAG_TIME_ALG             0x0002
#define HW1_FLAG_VERBOSE              0x0010
#define HW1_FLAG_VERY_VERBOSE         0x0020

typedef struct heat_source_s {
    float x;
    float y;
    double range;
    double temp;
} heat_source_t;

typedef struct hw1_params_s {
    uint32_t alg_type;
    uint32_t max_iterations;  /* leave to 0 to only stop on residual */
    uint32_t resolution;
    uint32_t num_threads;
    int32_t flags;
    char* input;
    uint32_t vis_res;  /* visual resolution if the data is dumped in a graphical format */
    int32_t vis_step;  /* number of iteration between generating the graphical heat map.
                        * Set to negative to only get 2 images, one for the initial stage
                        * and one for the final.
                        */
    char* vis_output;
    double *vis_data;
    uint32_t num_sources;
    heat_source_t* heat_sources;
} hw1_params_t;

int hw1_init(hw1_params_t* param, int32_t* argc, char* argv[]);
int hw1_fini(hw1_params_t* param);

/**
 * Transform, coarsen, a matrix by agragating several point
 * in 2D into a single element. This can be used to prepare
 * the output matrix for generating the graphical visualization.
 */
int coarsen(const double* src, uint32_t srcx, uint32_t srcy,
            double* dst, uint32_t dstx, uint32_t dsty);

/**
 * A function to setup the initial values for the boundaries based on the
 * known heat sources. It is generic for most of the relaxations, for as long
 * as the provided size accounts the boundaries.
 */
int relaxation_matrix_set(hw1_params_t* hw_params, double* mat, uint32_t np);

/**
 * The number of differewnt values of gray in the output image */
#define MAXVAL 255

/**
 * Dump a matrix into a gray ppm format.
 */
int dump_gray_image(char* template_name, int iter, double* data, uint32_t sizex, uint32_t sizey);

/**
 * Dump a matrix into a RGB ppm format.
 */
int dump_rgb_image(char* template_name, int iter, double* data, uint32_t sizex, uint32_t sizey);

/**
 * Time in microsecond from an initial date. Only the difference between
 * 2 such values is meaningful, as it represent the number of microseconds
 * between the 2 events.
 */
double wtime(void);

int print_matrix( FILE* fp, const double* mat, int sizex, int sizey );

#endif  /* HW1_HEADER_HAS_BEEN_INCLUDED */
