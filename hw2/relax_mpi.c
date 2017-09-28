#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "jacobi.h"
#include "header.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct area {
	unsigned int x, y, w, h;
	char colour;
	char send_dir; //0b0000NWSE
} AREA;

struct relaxation_mpi_hidden_params_s {
	//Generic parametre
	struct relaxation_params_s super;
	double* data;
	double* tmp;
	uint32_t idx;

	uint32_t rank,       //Node ID
	         size,       //Number of nodes
	         w,          //Fast access to width
	         h,          //Fast access to height
			 max_w,      //Max width
			 max_h,      //Max height
	         q,          //
	         p,          //
	         max_size;   //Max size to allocate vectors (minus 2)

	struct area* region; //Array of region details for the image
	MPI_Datatype t1;     //Vector datatype
};
const struct relaxation_function_class_s _relaxation_mpi;

static struct relaxation_params_s*
mpi_relaxation_init(struct hw_params_s* hw_params)
{
    struct relaxation_mpi_hidden_params_s* rp;
    uint32_t np = hw_params->resolution + 2;

    rp = (struct relaxation_mpi_hidden_params_s*)malloc(sizeof(struct relaxation_mpi_hidden_params_s));

    if( NULL == rp ) {
        fprintf(stderr, "Cannot allocate memory for the relaxation structure\n");
        return NULL;
    }

	//MPI Parametre
	MPI_Comm_rank(MPI_COMM_WORLD, &rp->rank);
	MPI_Comm_size(MPI_COMM_WORLD, &rp->size);
    
	//Ordinary Parametre
    rp->super.sizex = np;
    rp->super.sizey = np;
    rp->super.rel_class = &_relaxation_mpi;
	rp->p = hw_params->num_proc_p;
	rp->q = rp->size / rp->p;
	rp->w = np;
	rp->h = np;
	rp->max_w = (rp->w / rp->q) + (rp->w % rp->q);
	rp->max_h = (rp->h / rp->p) + (rp->h % rp->p);
	
	//Construct the vector data type
	MPI_Type_vector(MAX(rp->max_w, rp->max_h) + 2, sizeof(double), 0, MPI_DOUBLE, &rp->t1);
	MPI_Type_commit(&rp->t1);

	//Create Region Data
	rp->region = (AREA *) malloc(rp->q * rp->p * sizeof(struct area));
	uint32_t ii = 0;
	for (; ii < rp->q * rp->p; ii++) {
		unsigned int vx, vy, i, j;
		i = ii % rp->p;
		j = ii / rp->p;
		vx = (i == rp->q - 1) ? rp->max_w : rp->w / rp->q;
		vy = (j == rp->p - 1) ? rp->max_h : rp->h / rp->p;

		//Set up a region
		rp->region[ii].x = i * (rp->w / rp->q);
		rp->region[ii].y = i * (rp->h / rp->p);
		rp->region[ii].w = vx;
		rp->region[ii].h = vy;
		if (i == 0)
			rp->region[ii].colour = j % 2;
		else
			rp->region[ii].colour = !rp->region[ii - 1].colour;

		//Bitmasking to tell if we can go in a certain direction
		rp->region[ii].send_dir = 0;
		if (j >         0) rp->region[ii].send_dir |= (1 << 0);
		if (i >         0) rp->region[ii].send_dir |= (1 << 1);
		if (j < rp->p - 1) rp->region[ii].send_dir |= (1 << 2);
		if (j < rp->q - 1) rp->region[ii].send_dir |= (1 << 3);
	}

    rp->idx = 0;
    rp->data = calloc(np*np, sizeof(double));
    rp->tmp  = calloc(np*np, sizeof(double));
	
    if( NULL == rp->data ) {
        fprintf(stderr, "Cannot allocate the memory for the matrices\n");
        goto fail_and_return;
    }

    return (struct relaxation_params_s*)rp;
 fail_and_return:
    if( NULL != rp->data ) free(rp->data);
    free(rp);
    return NULL;
}

static int mpi_relaxation_fini(relaxation_params_t** prp)
{
    struct relaxation_mpi_hidden_params_s* rp = (struct relaxation_mpi_hidden_params_s*)*prp;
    if( NULL != rp->data ) free(rp->data);
    free(rp);
    *prp = NULL;
    return 0;
}

static int mpi_relaxation_coarsen(relaxation_params_t* grp, double* dst, uint32_t dstx, uint32_t dsty)
{
    struct relaxation_mpi_hidden_params_s* rp = (struct relaxation_mpi_hidden_params_s*)grp;
    return coarsen(rp->data, rp->super.sizex, rp->super.sizey,
                   dst, dstx, dsty);
}

static int mpi_relaxation_print(relaxation_params_t* grp, FILE* fp)
{
    struct relaxation_mpi_hidden_params_s* rp = (struct relaxation_mpi_hidden_params_s*)grp;
    fprintf( fp, "\n# Iteration %d\n", rp->idx);
    print_matrix(fp, rp->data, rp->super.sizex, rp->super.sizey);
    fprintf( fp, "\n\n");
    return 0;
}

/**
 * One step of a simple mpi relaxation.
 */
static double mpi_relaxation_apply(relaxation_params_t* grp)
{
    struct relaxation_mpi_hidden_params_s* rp = (struct relaxation_mpi_hidden_params_s*)grp;
    fprintf(stdout, "This is only a mpi relaxation class. No computations are done!\n");
    rp->idx++;
    return 0.0;
}

static double* mpi_relaxation_get_data(relaxation_params_t* grp)
{
    return ((struct relaxation_mpi_hidden_params_s*)grp)->data;
}


const struct relaxation_function_class_s _relaxation_mpi =
    {
        .type = RELAXATION_JACOBI_MPI,
        ._init = mpi_relaxation_init,
        ._fini = mpi_relaxation_fini,
        ._coarsen = mpi_relaxation_coarsen,
        ._print = mpi_relaxation_print,
        ._relaxation = mpi_relaxation_apply,
        ._get_data = mpi_relaxation_get_data,
    };
