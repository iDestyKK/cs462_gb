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

	//Adjacent IDs
	size_t dir_inx[4];
	size_t dir_inv[4];

	double sum;
};
const struct relaxation_function_class_s _relaxation_mpi;

//Helper Functions for sending and receiving data about the matrix entries
void append_vector(struct relaxation_mpi_hidden_params_s* rp, double* vec) {
	//0b0000NWSE
	AREA *cr = &rp->region[rp->rank];

	//Direction and length are defined in the vector
	unsigned char dir = (int)vec[1];
	unsigned int  len =      vec[0],
	              i   = 0,
	              j   = 2;
	
	//NOTE: These are flipped versions of the "generate_vector" sister function.
	switch (dir) {
		case 0:
			//South
			for (i = cr->x; i < cr->x + cr->w && vec[j] != -1; i++)
				rp->data[((cr->y + cr->h) * rp->w) + i] = vec[j++];
			break;
		case 1:
			//East
			for (i = cr->y; i < cr->y + cr->h && vec[j] != -1; i++)
				rp->data[(i * rp->w) + cr->x + cr->w] = vec[j++];
			break;
		case 2:
			//North
			for (i = cr->x; i < cr->x + cr->w && vec[j] != -1; i++)
				rp->data[((cr->y - 1) * rp->w) + i] = vec[j++];
			break;
		case 3:
			//West
			for (i = cr->y; i < cr->y + cr->h && vec[j] != -1; i++)
				rp->data[(i * rp->w) + (cr->x - 1)] = vec[j++];
			break;
	}
}

double* generate_vector(struct relaxation_mpi_hidden_params_s* rp, int dir) {
	//0b0000NWSE
	//Store the vector size and direction before the data
	double* vec = (double *)malloc(sizeof(double) * (rp->max_size + 2));
	AREA *cr = &rp->region[rp->rank];

	vec[0] = ((dir % 2) == 0) ? cr->w : cr->h;
	vec[1] = dir;
	
	unsigned int i, j = 2;
	switch (dir) {
		case 0:
			//North
			for (i = cr->x; i < cr->x + cr->w; i++)
				vec[j++] = rp->data[(cr->y * rp->w) + i];
			break;
		case 1:
			//West
			for (i = cr->y; i < cr->y + cr->h; i++)
				vec[j++] = rp->data[(i * rp->w) + cr->x];
			break;
		case 2:
			//South
			for (i = cr->x; i < cr->x + cr->w; i++)
				vec[j++] = rp->data[((cr->y + cr->h - 1) * rp->w) + i];
			break;
		case 3:
			//East
			for (i = cr->y; i < cr->y + cr->h; i++)
				vec[j++] = rp->data[(i * rp->w) + (cr->x + cr->w - 1)];
			break;
	}

	//Make a "-1" terminator (just in case)
	if (j != rp->max_size + 2)
		vec[j++] = -1;
	
	return vec;
}

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
	rp->max_size = MAX(rp->max_w, rp->max_h);
	
	//Construct the vector data type
	MPI_Type_vector(rp->max_size + 2, 1, 1, MPI_DOUBLE, &rp->t1);
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

		/*printf("%d:%c%c%c%c\n",
			ii,
			(rp->region[ii].send_dir     ) & 1 ? '^' : ' ',
			(rp->region[ii].send_dir >> 1) & 1 ? '<' : ' ',
			(rp->region[ii].send_dir >> 2) & 1 ? 'v' : ' ',
			(rp->region[ii].send_dir >> 3) & 1 ? '>' : ' '
		);*/
	}

    rp->idx = 0;
    rp->data = calloc(np*np, sizeof(double));
    rp->tmp  = calloc(np*np, sizeof(double));
    relaxation_matrix_set(hw_params, rp->data, np);
	memcpy(rp->tmp, rp->data, np * np * sizeof(double));

	rp->dir_inx[0] = rp->rank - rp->q;
	rp->dir_inx[1] = rp->rank - 1;
	rp->dir_inx[2] = rp->rank + rp->q;
	rp->dir_inx[3] = rp->rank + 1;

	rp->dir_inv[0] = rp->dir_inx[2];
	rp->dir_inv[1] = rp->dir_inx[3];
	rp->dir_inv[2] = rp->dir_inx[0];
	rp->dir_inv[3] = rp->dir_inx[1];
	
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
	MPI_Barrier(MPI_COMM_WORLD);

	unsigned char jk;
	uint32_t i, j;
	AREA* cr =  &rp->region[rp->rank];

	//Send first. Update the edges of all nodes.
	if (cr->colour == 0) {
		for (jk = 0; jk < 4; jk++) {
			if (!(cr->send_dir & (1 << jk)))
				continue;

			double* vec = generate_vector(rp, jk);

			MPI_Request req;
			MPI_Status  sta;
			MPI_Isend(vec, 1, rp->t1, rp->dir_inx[jk], 0, MPI_COMM_WORLD, &req);
			MPI_Wait(&req, &sta);
			//printf("Send:%d: %lg %lg ", rp->rank, vec[0], vec[1]);
			//for (i = 0; i < vec[0]; i++)
			//	printf("%lg ", vec[2 + i]);
			//printf("\nlolsent\n");
			free(vec);
		}

		for (jk = 0; jk < 4; jk++) {
			//Invert the send_dir bits
			char send_d = ((cr->send_dir & 0x3) << 2) + (cr->send_dir >> 2);

			if (!(send_d & (1 << jk)))
				continue;
			double* vec = (double*) malloc(sizeof(double) * (rp->max_size + 2));
			MPI_Request req;
			MPI_Status  sta;
			MPI_Irecv(vec, 1, rp->t1, rp->dir_inv[jk], 0, MPI_COMM_WORLD, &req);
			MPI_Wait(&req, &sta);
			//MPI_Recv(vec, 1, t1, dir_inv[jk], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("Recv:%d: %lg %lg ", rp->rank, vec[0], vec[1]);
			//for (i = 0; i < vec[0]; i++) {
			//	printf("%lg ", vec[2 + i]);
			//}
			//printf("\nloltrue\n");
			append_vector(rp, vec);
			free(vec);
		}
	}
	else {
		for (jk = 0; jk < 4; jk++) {
			//Invert the send_dir bits
			char send_d = ((cr->send_dir & 0x3) << 2) + (cr->send_dir >> 2);

			if (!(send_d & (1 << jk)))
				continue;
			double* vec = (double*) malloc(sizeof(double) * (rp->max_size + 2));
			MPI_Request req;
			MPI_Status  sta;
			MPI_Irecv(vec, 1, rp->t1, rp->dir_inv[jk], 0, MPI_COMM_WORLD, &req);
			MPI_Wait(&req, &sta);
			//MPI_Recv(vec, 1, t1, dir_inv[jk], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("Recv:%d: %lg %lg ", rp->rank, vec[0], vec[1]);
			//for (i = 0; i < vec[0]; i++) {
			//	printf("%lg ", vec[2 + i]);
			//}
			//printf("\nloltrue\n");
			append_vector(rp, vec);
			free(vec);
		}
		for (jk = 0; jk < 4; jk++) {
			if (!(cr->send_dir & (1 << jk)))
				continue;

			double* vec = generate_vector(rp, jk);

			MPI_Request req;
			MPI_Status  sta;
			MPI_Isend(vec, 1, rp->t1, rp->dir_inx[jk], 0, MPI_COMM_WORLD, &req);
			MPI_Wait(&req, &sta);
			//printf("Send:%d: %lg %lg ", rp->rank, vec[0], vec[1]);
			//for (i = 0; i < vec[0]; i++)
			//	printf("%lg ", vec[2 + i]);
			//printf("\nlolsent\n");
			free(vec);
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);

	//Calculate data
	double diff;
	rp->sum = 0.0;
	for (i = MAX(1, cr->y); i < MIN(cr->y + cr->h, rp->h - 1); i++) {
		for (j = MAX(1, cr->x); j < MIN(cr->x + cr->w, rp->w - 1); j++) {
			rp->tmp[i * rp->super.sizex + j] = 0.25 * (
				rp->data[ i      * rp->w + (j - 1)] +
				rp->data[ i      * rp->w + (j + 1)] +
				rp->data[(i - 1) * rp->w +  j     ] +
				rp->data[(i + 1) * rp->w +  j     ]
			);
			
			diff = rp->tmp[i * rp->w + j] - rp->data[i * rp->w + j];
			rp->sum += diff * diff;
		}
	}

	//Copy memory over
	double* tmp = rp->tmp;
	rp->tmp = rp->data;
	rp->data = tmp;

	//Send Results to the first node and let it total them up.
	if (rp->rank != 0)
		MPI_Send(&rp->sum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
	else {
		for (i = 0; i < rp->size - 1; i++) {
			double nsum = 0.0;
			MPI_Recv(&nsum, 1, MPI_DOUBLE, i + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			rp->sum += nsum;
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	
	//Send those results to the others.
	if (rp->rank == 0)
		for (i = 1; i < rp->size; i++)
			MPI_Send(&rp->sum, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
	else
		MPI_Recv(&rp->sum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	MPI_Barrier(MPI_COMM_WORLD);

    rp->idx++;

    return rp->sum;
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
