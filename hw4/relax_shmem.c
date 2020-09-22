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
			 q, p, max_size, tick;
	CN_VEC data;
	CN_VEC new;
	CN_VEC sum;
	struct area* region; //Array of region details for the image
	
	double prev_residual;

	//Adjacent IDs
	int dir_inx[4];
	int dir_inv[4];
};
const struct relaxation_function_class_s _relaxation_oshmem;

void update_vectors(struct relaxation_oshmem_hidden_params_s* rp) {
	//TODO: Implement
	//Notice that, below in the init function, that dir_vec's size is dependent
	//on whether or not that edge can actually be reached. In other words, if
	//that node can't go above, then cr->dir_vec[0]'s size is 0, and so forth.
	
	AREA* cr = &rp->region[rp->id];
	CN_UINT i;
	double* data = cn_vec_array(rp->data, double);
	
	//North vector
	for (i = 0; i < cn_vec_size(cr->dir_vec[0]); i++) {
		cn_vec_get(cr->dir_vec[0], double, i) =
			data[(cr->y * rp->w) + (cr->x + i)];
	}

	//West Vector
	for (i = 0; i < cn_vec_size(cr->dir_vec[1]); i++) {
		cn_vec_get(cr->dir_vec[1], double, i) =
			data[((cr->y + i) * rp->w) + cr->x];
	}
	
	//South Vector
	for (i = 0; i < cn_vec_size(cr->dir_vec[2]); i++) {
		cn_vec_get(cr->dir_vec[2], double, i) =
			data[((cr->y + cr->h - 1) * rp->w) + (cr->x + i)];
	}
	
	//East Vector
	for (i = 0; i < cn_vec_size(cr->dir_vec[3]); i++) {
		cn_vec_get(cr->dir_vec[3], double, i) =
			data[((cr->y + i) * rp->w) + (cr->x + cr->w - 1)];
	}
}

void sync_vectors(struct relaxation_oshmem_hidden_params_s* rp) {
	//TODO: Implement
	CN_UINT i, j;
	AREA* cr,
	    * rr = &rp->region[rp->id];
	double* data = cn_vec_array(rp->data, double);
	double* vec;

	for (j = 0; j < 4; j++) {
		if (rp->dir_inx[j] == -1)
			continue;
		cr = &rp->region[rp->dir_inx[j]];


		switch (j) {
			case 0:
				//North
				cn_shvec_get(cr->dir_vec[2], cr->dir_vec[2], rp->dir_inx[j]);
				vec = cn_vec_array(cr->dir_vec[2], double);
				
				for (i = 0; i < cn_vec_size(cr->dir_vec[2]); i++)
					data[((cr->y + cr->h - 1) * rp->w) + (cr->x + i)] = vec[i];
				break;
			case 1:
				//West
				cn_shvec_get(cr->dir_vec[3], cr->dir_vec[3], rp->dir_inx[j]);
				vec = cn_vec_array(cr->dir_vec[3], double);
				
				for (i = 0; i < cn_vec_size(cr->dir_vec[3]); i++)
					data[((cr->y + i) * rp->w) + (cr->x + cr->w - 1)] = vec[i];
				break;
			case 2:
				//South
				cn_shvec_get(cr->dir_vec[0], cr->dir_vec[0], rp->dir_inx[j]);
				vec = cn_vec_array(cr->dir_vec[0], double);

				for (i = 0; i < cn_vec_size(cr->dir_vec[0]); i++)
					data[(cr->y * rp->w) + (cr->x + i)] = vec[i];
				break;
			case 3:
				//East
				cn_shvec_get(cr->dir_vec[1], cr->dir_vec[1], rp->dir_inx[j]);
				vec = cn_vec_array(cr->dir_vec[1], double);

				for (i = 0; i < cn_vec_size(cr->dir_vec[1]); i++)
					data[((cr->y + i) * rp->w) + cr->x] = vec[i];
				break;
		}
	}
}

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
	rp->tick     = 0;

	//Now for the new part... Initialise the CN_VECs
	rp->data = cn_vec_init(double);
	rp->new  = cn_vec_init(double);
	rp->sum  = cn_vec_init(double);

	cn_vec_resize(rp->data, np * np);
	cn_vec_resize(rp->new , np * np);
	cn_vec_resize(rp->sum , rp->q * rp->p);

	//Setup the data correctly into both matrices
	relaxation_matrix_set(hw_params, cn_vec_data(rp->data), np, 0, 0);
	relaxation_matrix_set(hw_params, cn_vec_data(rp->new ), np, 0, 0);

	//This is the fun part. Calculate what nodes are adjacent.
	rp->dir_inx[0] = rp->id - rp->q;
	rp->dir_inx[1] = rp->id - 1;
	rp->dir_inx[2] = rp->id + rp->q;
	rp->dir_inx[3] = rp->id + 1;


	/*printf("%d: %d %d %d %d\n",
		rp->id,
		rp->dir_inx[0],
		rp->dir_inx[1],
		rp->dir_inx[2],
		rp->dir_inx[3]
	);*/

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
		//These vectors will hold the data of what to send over.
		
		//Our awesome algorithm will involve simply copying the vectors over and then
		//setting the values. This will enforce that the nodes get the data and that
		//they are synced properly.

		CN_UINT ing = 0;
		for (; ing < 4; ing++) {
			rp->region[ii].dir_vec[ing] = cn_vec_init(double);
			if ((rp->region[ii].send_dir >> ing) & 1) {
				//Get the size
				CN_UINT sz;
				switch (ing) {
					//Abuse of Switch Statements
					case 0: 
						sz = rp->region[ii].w;
						break;
					case 1: 
						sz = rp->region[ii].h;
						break;
					case 2:
						sz = rp->region[ii].w;
						break;
					case 3:
						sz = rp->region[ii].h;
						break;
					default:
						sz = 0;
						break;
				}

				cn_vec_resize(rp->region[ii].dir_vec[ing], sz);
			}
		}

		//DEBUG: Check whether adjacent exists
		/*printf("%d:%c%c%c%c\n",
			ii,
			(rp->region[ii].send_dir     ) & 1 ? '^' : ' ',
			(rp->region[ii].send_dir >> 1) & 1 ? '<' : ' ',
			(rp->region[ii].send_dir >> 2) & 1 ? 'v' : ' ',
			(rp->region[ii].send_dir >> 3) & 1 ? '>' : ' '
		);*/
	}

	//Enforce that they actually make sense...
	if (rp->dir_inx[0] < 0                      ) rp->dir_inx[0] = -1;
	if (rp->dir_inx[2] >= rp->q * rp->p         ) rp->dir_inx[2] = -1;
	if (rp->dir_inx[1] / rp->q != rp->id / rp->q) rp->dir_inx[1] = -1;
	if (rp->dir_inx[3] / rp->q != rp->id / rp->q) rp->dir_inx[3] = -1;

	//Inverse lookup too
	rp->dir_inv[0] = rp->dir_inx[2];
	rp->dir_inv[1] = rp->dir_inx[3];
	rp->dir_inv[2] = rp->dir_inx[0];
	rp->dir_inv[3] = rp->dir_inx[1];

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

	//Go into each region and delete the vectors for the edges.
	CN_UINT i = 0, j;
	for (; i < rp->q * rp->p; i++) {
		for (j = 0; j < 4; j++)
			cn_vec_free(rp->region[i].dir_vec[j]);
	}

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
	//Initialise variables
	struct relaxation_oshmem_hidden_params_s* rp = (struct relaxation_oshmem_hidden_params_s*)grp;
	double diff, sum = 0.0, *new, *old;
	int i, j;

	AREA* cr = &rp->region[rp->id];

	old = (double *)cn_vec_data(rp->data);
	new = (double *)cn_vec_data(rp->new);

	//Write edges to CN_VECs and send them over to adjacent nodes.
	update_vectors(rp);
	shmem_barrier_all();

	sync_vectors(rp);

	//Process Jacobi Matrix
	for (i = MAX(1, cr->y); i < MIN(cr->y + cr->h, rp->h - 1); i++) {
		for (j = MAX(1, cr->x); j < MIN(cr->x + cr->w, rp->w - 1); j++) {
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

	//Swap the new and old matrix pointers
	double* tmp = rp->new->data;
	rp->new->data = rp->data->data;
	rp->data->data = tmp;
	cn_vec_get(rp->sum, double, rp->id) = sum;

	shmem_barrier_all();

	//Manage how the sum is handles over the nodes
	//Pass all sums to the first node into vector.
	//Add all of the sums up.
	if (rp->id == 0) {
		CN_UINT pf = 1;
		double lel = 0.0;
		
		for (; pf < cn_vec_size(rp->sum); pf++) {
			lel = shmem_double_g(&cn_vec_get(rp->sum, double, pf), pf);
			sum += lel;
		}
		cn_vec_get(rp->sum, double, 0) = sum;
	}

	shmem_barrier_all();

	//Update the other nodes
	if (rp->id != 0) {
		shmem_double_get(
			&cn_vec_get(rp->sum, double, 0),
			&cn_vec_get(rp->sum, double, 0),
			1,
			0
		);
		sum = cn_vec_get(rp->sum, double, 0);
	}

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
