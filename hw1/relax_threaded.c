/*
 * COSC 462 - HW1: Jacobi Pthreaded Implementation
 *
 * Description:
 *     Based on the professor's code for serial implementation, this is a
 *     variation that will use multiple threads to solve the problem. This will
 *     give around a linear speedup to the process.
 *
 * Approach:
 *     We will make N number of threads. And each of those threads will be able
 *     to modify a single row of entries. When the threads are done with the
 *     image processing, join and make the program render the image. Repeat
 *     until the process is done.
 *
 * Author:
 *     Clara Van Nguyen
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#include "jacobi.h"
#include "header.h"

struct relaxation_pthread_hidden_params_s {
	//Generic parametre
    struct relaxation_params_s super;
    double*  data;
	double*  tmp;
	double   sum;
    uint32_t idx;
	char*    processable;

	//Array of threads
	uint32_t num_t;
	uint32_t proc, proc2;
	pthread_t       * THRD_ARR;
	struct arg_pair * pairs;
	pthread_mutex_t   start, access, contin;
};
const struct relaxation_function_class_s _relaxation_pthread;

typedef struct arg_pair {
	struct relaxation_pthread_hidden_params_s* ptr;
	size_t val;
} ARG_PAIR;

void* __FUNC(void* para) {
	//Unpack your variables
	ARG_PAIR* parametre = (ARG_PAIR*)para;
	struct relaxation_pthread_hidden_params_s* rp = parametre->ptr;

	
	while (1) {
		pthread_mutex_lock(&rp->start);
		pthread_mutex_unlock(&rp->start);

		uint32_t i = parametre->val, j;
		double diff, sum = 0.0;

		for (; i < rp->super.sizey - 1; i += rp->num_t) {
			for (j = 1; j < rp->super.sizex - 1; j++) {
				rp->data[i * rp->super.sizex + j] = 0.25 * (
					rp->tmp[ i      * rp->super.sizex + (j - 1)] +
					rp->tmp[ i      * rp->super.sizex + (j + 1)] +
					rp->tmp[(i - 1) * rp->super.sizex +  j     ] +
					rp->tmp[(i + 1) * rp->super.sizex +  j     ]
				);
				
				diff = rp->data[i * rp->super.sizex + j] - rp->tmp[i * rp->super.sizex + j];
				sum += diff * diff;
			}
		}

		//Try to claim the mutex, and write to its sum.
		pthread_mutex_lock(&rp->access);
		rp->sum += sum;
		rp->proc++;
		pthread_mutex_unlock(&rp->access);

		pthread_mutex_lock(&rp->contin);
		rp->proc2++;
		pthread_mutex_unlock(&rp->contin);
	}
	return 0;
}

static struct relaxation_params_s*
pthread_relaxation_init(struct hw1_params_s* hw_params)
{
    struct relaxation_pthread_hidden_params_s* rp;
    uint32_t np = hw_params->resolution + 2;

	printf("%u\n", hw_params->num_threads);

    rp = (struct relaxation_pthread_hidden_params_s*)malloc(sizeof(struct relaxation_pthread_hidden_params_s));
    if( NULL == rp ) {
        fprintf(stderr, "Cannot allocate memory for the relaxation structure\n");
        return NULL;
    }

	rp->num_t           = hw_params->num_threads;
    rp->super.sizex     = np;
    rp->super.sizey     = np;
    rp->super.rel_class = &_relaxation_pthread;

	pthread_mutex_init(&rp->start , NULL);
	pthread_mutex_init(&rp->access, NULL);
	pthread_mutex_init(&rp->contin, NULL);
	pthread_mutex_lock(&rp->start);

    rp->idx = 0;
    rp->data        = calloc(np*np , sizeof(double));
    rp->tmp         = malloc(np*np * sizeof(double));
	rp->THRD_ARR    = (pthread_t*) malloc(sizeof(pthread_t      ) * hw_params->num_threads);
	rp->pairs       = (ARG_PAIR *) malloc(sizeof(struct arg_pair) * hw_params->num_threads);

	uint32_t iter = 0;
	for (; iter < hw_params->num_threads; iter++) {
		rp->pairs[iter].ptr = (void*)rp;
		rp->pairs[iter].val = iter + 1;
		pthread_create(&rp->THRD_ARR[iter], NULL, __FUNC, (void*)&rp->pairs[iter]);
	}
    if( NULL == rp->data || NULL == rp->tmp ) {
        fprintf(stderr, "Cannot allocate the memory for the matrices\n");
        goto fail_and_return;
    }
	relaxation_matrix_set(hw_params, rp->data, np);
	memcpy(rp->tmp, rp->data, np * np * sizeof(double));

    return (struct relaxation_params_s*)rp;
 fail_and_return:
    if( NULL != rp->data ) free(rp->data);
    if( NULL != rp->tmp  ) free(rp->tmp );
    free(rp);
    return NULL;
}

static int pthread_relaxation_fini(relaxation_params_t** prp)
{
    struct relaxation_pthread_hidden_params_s* rp = (struct relaxation_pthread_hidden_params_s*)*prp;
    if( NULL != rp->data ) free(rp->data);
    if( NULL != rp->tmp  ) free(rp->tmp );
    free(rp);
    *prp = NULL;
    return 0;
}

static int pthread_relaxation_coarsen(relaxation_params_t* grp, double* dst, uint32_t dstx, uint32_t dsty)
{
    struct relaxation_pthread_hidden_params_s* rp = (struct relaxation_pthread_hidden_params_s*)grp;
    return coarsen(rp->data, rp->super.sizex, rp->super.sizey,
                   dst, dstx, dsty);
}

static int pthread_relaxation_print(relaxation_params_t* grp, FILE* fp)
{
    struct relaxation_pthread_hidden_params_s* rp = (struct relaxation_pthread_hidden_params_s*)grp;
    fprintf( fp, "\n# Iteration %d\n", rp->idx);
    print_matrix(fp, rp->data, rp->super.sizex, rp->super.sizey);
    fprintf( fp, "\n\n");
    return 0;
}

/**
 * One step of a simple pthread relaxation.
 */
static double pthread_relaxation_apply(relaxation_params_t* grp)
{
    struct relaxation_pthread_hidden_params_s* rp = (struct relaxation_pthread_hidden_params_s*)grp;
	
	rp->sum  = 0.0;
	rp->proc = 0;
	uint32_t rem, i;
	
	//Let the threads do their work
	pthread_mutex_lock(&rp->contin);
	pthread_mutex_unlock(&rp->start);

	//Try to compute when the threads are done working.
	while (1) {
		pthread_mutex_lock(&rp->access);
		if (rp->proc >= rp->num_t) {
			pthread_mutex_lock(&rp->start);
			pthread_mutex_unlock(&rp->access);
			break;
		}
		pthread_mutex_unlock(&rp->access);
	}

	//Copy over memory.
	memcpy(rp->tmp, rp->data, rp->super.sizex * rp->super.sizey * sizeof(double));
	rp->proc2 = 0;
	pthread_mutex_unlock(&rp->contin);

	while (1) {
		pthread_mutex_lock(&rp->contin);
		if (rp->proc2 >= rp->num_t) {
			pthread_mutex_unlock(&rp->contin);
			break;
		}
		pthread_mutex_unlock(&rp->contin);
	}

    //fprintf(stdout, "This is only a pthread relaxation class. No computations are done!\n");
    rp->idx++;
    return rp->sum;
}

static double* pthread_relaxation_get_data(relaxation_params_t* grp)
{
    return ((struct relaxation_pthread_hidden_params_s*)grp)->data;
}


const struct relaxation_function_class_s _relaxation_pthread =
    {
        .type = RELAXATION_JACOBI_PTHREADS,
        ._init = pthread_relaxation_init,
        ._fini = pthread_relaxation_fini,
        ._coarsen = pthread_relaxation_coarsen,
        ._print = pthread_relaxation_print,
        ._relaxation = pthread_relaxation_apply,
        ._get_data = pthread_relaxation_get_data,
    };
