#include <shmem.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "jacobi.h"
#include "header.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

//Handy
#include "lib/handy/handy.h"
#include "lib/handy/types.h"

//CNDS (Clara Nguyen's Data Structures)
#include "lib/handy/cnds/cn_vec.h"

/**
 * This is a oshmem for an empty relaxation that can be used as an
 * exmaple for the homework.
 */

void cn_shvec_get(CN_VEC target, const CN_VEC source, int pe) {
	//Copy over data based on source vector
	//shrealloc calls "shmem_barrier_all()", which deadlocks this unless
	//the size of the two vectors are the same!

	shmem_getmem(
		target->data,
		source->data,
		cn_vec_size(target) * cn_vec_element_size(target),
		pe
	);
}

typedef struct area {
	unsigned int x, y, w, h;
	char colour;
	char send_dir; //0b0000NWSE
	CN_VEC dir_vec[4]; //4 Vectors per section. Will contain node information.
} AREA;

struct relaxation_oshmem_hidden_params_s {
	struct relaxation_params_s super;
	uint32_t idx,
			 id,
			 num_proc,
			 w, h,
			 max_w, max_h,
			 q, p, max_size;
	CN_VEC data;
	CN_VEC new;
	struct area* region; //Array of region details for the image

	//Adjacent IDs
	size_t dir_inx[4];
	size_t dir_inv[4];
};
const struct relaxation_function_class_s _relaxation_oshmem;

static struct relaxation_params_s*
oshmem_relaxation_init(struct hw_params_s* hw_params)
{
	struct relaxation_oshmem_hidden_params_s* rp;
	uint32_t np = hw_params->resolution + 2;

	rp = (struct relaxation_oshmem_hidden_params_s*)malloc(sizeof(struct relaxation_oshmem_hidden_params_s));
	if( NULL == rp ) {
		fprintf(stderr, "Cannot allocate memory for the relaxation structure\n");
		return NULL;
	}

	//Prepare all basic integer data
	rp->super.sizex = np;
	rp->super.sizey = np;
	rp->super.rel_class = &_relaxation_oshmem;

	rp->idx      = 0;
	rp->id       = shmem_my_pe();
	rp->num_proc = shmem_n_pes();
	rp->p        = hw_params->num_proc_p;
	rp->q        = rp->num_proc / rp->p;
	rp->w        = rp->super.sizex;
	rp->h        = rp->super.sizey;
	rp->max_w    = (rp->w / rp->q) + (rp->w % rp->q);
	rp->max_h    = (rp->h / rp->p) + (rp->h % rp->p);
	rp->max_size = MAX(rp->max_w, rp->max_h);

	//Now for the new part... Initialise the CN_VECs
	rp->data = cn_vec_init(double);
	cn_vec_resize(rp->data, np * np);
	rp->new = cn_vec_init(double);
	cn_vec_resize(rp->new, np * np);

	//Setup the data correctly into both matrices
	relaxation_matrix_set(hw_params, cn_vec_data(rp->data), np, 0, 0);
	relaxation_matrix_set(hw_params, cn_vec_data(rp->new ), np, 0, 0);

	//Now, let's initialise the regional data... this is pretty much the exact
	//same as in HW2...
	//Create Region Data
	rp->region = (AREA *) shmalloc(rp->q * rp->p * sizeof(struct area));
	uint32_t ii = 0;
	for (; ii < rp->q * rp->p; ii++) {
		unsigned int vx, vy, i, j;
		i = ii % rp->q;
		j = ii / rp->q;
		vx = (i == rp->q - 1) ? rp->max_w : rp->w / rp->q;
		vy = (j == rp->p - 1) ? rp->max_h : rp->h / rp->p;

		//Set up a region
		rp->region[ii].x = i * (rp->w / rp->q);
		rp->region[ii].y = j * (rp->h / rp->p);
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
		if (i < rp->q - 1) rp->region[ii].send_dir |= (1 << 3);

		//Set up vectors
		//These vectors will hold the "indexes" of what they should represent.
		CN_UINT ing = 0;
		for (; ing < 4; ing++)
			rp->region[ii].dir_vec[ing] = cn_vec_init(CN_UINT);

		//Now let's generate whatever is inside the vectors.
		//TODO: Implement this...
		//North
		CN_VEC ptr;
		if ((rp->region[ii].send_dir) & 1) {
			ptr = rp->region[ii].dir_vec[0];
		}

		//West
		if ((rp->region[ii].send_dir >> 1) & 1) {
			ptr = rp->region[ii].dir_vec[1];
		}

		//South
		if ((rp->region[ii].send_dir >> 2) & 1) {
			ptr = rp->region[ii].dir_vec[2];
		}

		//East
		if ((rp->region[ii].send_dir >> 3) & 1) {
			ptr = rp->region[ii].dir_vec[3];
		}

		printf("%d:%c%c%c%c\n",
			ii,
			(rp->region[ii].send_dir     ) & 1 ? '^' : ' ',
			(rp->region[ii].send_dir >> 1) & 1 ? '<' : ' ',
			(rp->region[ii].send_dir >> 2) & 1 ? 'v' : ' ',
			(rp->region[ii].send_dir >> 3) & 1 ? '>' : ' '
		);
	}

	//Ensure everything is initialised at the start before moving on.
	shmem_barrier_all();

	if( NULL == rp->data ) {
		fprintf(stderr, "Cannot allocate the memory for the matrices\n");
		goto fail_and_return;
	}

	return (struct relaxation_params_s*)rp;
fail_and_return:
	if( NULL != rp->data ) cn_vec_free(rp->data);
	free(rp);
	return NULL;
}

static int oshmem_relaxation_fini(relaxation_params_t** prp)
{
	struct relaxation_oshmem_hidden_params_s* rp = (struct relaxation_oshmem_hidden_params_s*)*prp;
	if( NULL != rp->data ) cn_vec_free(rp->data);
	if( NULL != rp->new  ) cn_vec_free(rp->new );

	shfree(rp->region);
	free(rp);
	*prp = NULL;
	return 0;
}

static int oshmem_relaxation_coarsen(relaxation_params_t* grp, double* dst, uint32_t dstx, uint32_t dsty)
{
	struct relaxation_oshmem_hidden_params_s* rp = (struct relaxation_oshmem_hidden_params_s*)grp;
	return coarsen((double*)rp->data->data, rp->super.sizex, rp->super.sizey,
			dst, dstx, dsty);
}

static int oshmem_relaxation_print(relaxation_params_t* grp, FILE* fp)
{
	struct relaxation_oshmem_hidden_params_s* rp = (struct relaxation_oshmem_hidden_params_s*)grp;
	fprintf( fp, "\n# Iteration %d\n", rp->idx);
	print_matrix(fp, (double*)rp->data->data, rp->super.sizex, rp->super.sizey);
	fprintf( fp, "\n\n");
	return 0;
}

/**
 * One step of a simple oshmem relaxation.
 */
static double oshmem_relaxation_apply(relaxation_params_t* grp)
{
	struct relaxation_oshmem_hidden_params_s* rp = (struct relaxation_oshmem_hidden_params_s*)grp;
	double diff, sum = 0.0, *new, *old;
	int i, j;

	new = (double *)cn_vec_data(rp->data);
	old = (double *)cn_vec_data(rp->new);
	/* The size[x,y] account for the boundary */
	for( i = 1; i < (rp->super.sizey-1); i++ ) {
		for( j = 1; j < (rp->super.sizex-1); j++ ) {
			new[i * rp->super.sizex + j] = 0.25 * (
					old[ i     * rp->super.sizex + (j-1) ] +  // left
					old[ i     * rp->super.sizex + (j+1) ] +  // right
					old[ (i-1) * rp->super.sizex + j     ] +  // top
					old[ (i+1) * rp->super.sizex + j     ]    // bottom
					);
			diff = new[i*rp->super.sizex+j] - old[i*rp->super.sizex+j];
			sum += diff * diff;
		}
	}
	memcpy(old, new, sizeof(double) * (rp->super.sizex * rp->super.sizey));
	rp->idx++;
	return sum;
}

static double* oshmem_relaxation_get_data(relaxation_params_t* grp)
{
	return (double*)((struct relaxation_oshmem_hidden_params_s*)grp)->data->data;
}


const struct relaxation_function_class_s _relaxation_oshmem =
{
	.type = RELAXATION_JACOBI_OSHMEM,
	._init = oshmem_relaxation_init,
	._fini = oshmem_relaxation_fini,
	._coarsen = oshmem_relaxation_coarsen,
	._print = oshmem_relaxation_print,
	._relaxation = oshmem_relaxation_apply,
	._get_data = oshmem_relaxation_get_data,
};
