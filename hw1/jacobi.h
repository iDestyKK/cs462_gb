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
#ifndef JACOBI_H_HAS_BEEN_INCLUDED
#define JACOBI_H_HAS_BEEN_INCLUDED

#include <stdint.h>
#include <stdio.h>

/* No need to define them here, but they will need to be defined in the .c */
struct hw1_params_s;

struct relaxation_params_s;
typedef struct relaxation_params_s relaxation_params_t;

#define RELAXATION_JACOBI 0
struct relaxation_function_class_s {
    int type;
    int (*_init)(struct hw1_params_s*, relaxation_params_t*);
    int (*_fini)(relaxation_params_t**);
    int (*_coarsen)(relaxation_params_t*, double*, uint32_t, uint32_t);
    int (*_print)(relaxation_params_t*, FILE*);
    double (*_relaxation)(relaxation_params_t*);
    double* (*_get_data)(relaxation_params_t*);
};
struct relaxation_params_s {
    double* data[2];
    uint32_t idx;
    uint32_t sizex;  /* total size of the matrix including the read-only borders */
    uint32_t sizey;
    const struct relaxation_function_class_s* rel_class;
};

/**
 * Initialize all the internals necessary to run an iterative Jacobi process.
 * This includes allocating all memory and threads requireds, as well as all
 * the synchronization constructs. All these data structures will remains
 * valid for the entire duration of the algorithm, until the corrsponding call
 * jacobi_fini.
 */
relaxation_params_t* relaxation_init(struct hw1_params_s* hw_params);

/**
 * Finalize the Jacobi iterative process, release all resources allocated for
 * this process, and release all system level constructs (threads, ...). The
 * Jacobi plaholder will be released and will be set to NULL.
 */
static inline int relaxation_fini(relaxation_params_t** prp)
{
    return (*prp)->rel_class->_fini(prp);
}

/**
 * Helper for the coarseninh of the heat map, in order to prepare it
 * for saving.
 */
static inline int relaxation_coarsen(relaxation_params_t* rp, double* dat, uint32_t ncols, uint32_t nrows)
{
    return rp->rel_class->_coarsen(rp, dat, ncols, nrows);
}
/**
 * Dump the status of the relaxation, allowing external entities to
 * validate the data.
 */
static inline int relaxation_print(relaxation_params_t* rp, FILE* fp)
{
    return rp->rel_class->_print(rp, fp);
}

/**
 * Execute one iteration of the Jacobi algorithm, using as input the data
 * in the old data array (odat) and updating the new data array (ndat). This
 * function is supposed to be streamlined for performance, all additional
 * resources necessary should have been allocated during the jacobi_init, and
 * should be in the jacobi_t data structure.
 * The residual is returned.
 */
static inline double relaxation_apply(relaxation_params_t* rp)
{
    return rp->rel_class->_relaxation(rp);
}

/**
 * Return a pointer to the most up-to-data data. This data includes the
 * boundaries.
 */
static inline double* relaxation_get_data(relaxation_params_t* rp)
{
    return rp->rel_class->_get_data(rp);
}

#endif  /* JACOBI_H_HAS_BEEN_INCLUDED */
