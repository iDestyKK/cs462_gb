/*
 * OpenSHMEM + Handy/CNDS Library
 *
 * Description:
 *     Handy is a custom (incomplete) library that I wrote myself which has
 *     functions and data stuctures that I constantly reuse when coding in C.
 *     However, what about using it with OpenSHMEM? Yes it is possible to use
 *     Handy with OpenSHMEM. By simply switching all memory functions out in
 *     CNDS for the respective OpenSHMEM functions, CNDS works with OpenSHMEM
 *     and allows users to allocate dynamic memory structures that the library
 *     will handle for you, including CN_VECs, which is what this example is
 *     for.
 *
 * Author:
 *     Clara Van Nguyen
 */

#include <stdio.h>
#include <shmem.h>

//Now let's try this with my libraries
//Handy
#include "../lib/handy/handy.h"
#include "../lib/handy/types.h"

//CNDS (Clara Nguyen's Data Structures)
//NOTE: In makefile, include "cn_shvec.c" instead of "ch_vec.c".
#include "../lib/handy/cnds/cn_vec.h"

//Custom function to copy a vector over from one thread to another
void cn_shvec_get(CN_VEC target, const CN_VEC source, int pe) {
	//Copy over data based on source vector
	//SCRAPPED: shrealloc calls "shmem_barrier_all()" which deadlocks this
	//if the size is different! So we will make sure the sizes are the same.
	//
	//shmem_int_get(&target->elem_size, &source->elem_size, 1, pe);
	//shmem_int_get(&target->size, &source->size, 1, pe);
	//shmem_getmem (&target->data, &source->data, target->elem_size, pe);
	
	//shmem_getmem(&target, &source, sizeof(struct cn_vec), pe);

	//Resize the vector accordingly
	//cn_vec_resize(target, target->size);

	//Now, copy over ALL data.
	shmem_getmem(
		target->data,
		source->data,
		cn_vec_size(target) * cn_vec_element_size(target),
		pe
	);
}

main() {
	start_pes(4);

	int num_proc = shmem_n_pes(),
	    id       = shmem_my_pe();

	CN_UINT i = 0;

	//Initialise CN_VEC
	CN_VEC vec = cn_vec_init(int);

	//Set its size to 16.
	cn_vec_resize(vec, 16);
	printf("%d/%d - 0x%016x\n", id + 1, num_proc, vec);
	shmem_barrier_all();
	
	//Set up the variables in the very first instance only.
	if (id == 0) {
		for (i = 0; i < cn_vec_size(vec); i++) {
			cn_vec_get(vec, int, i) = (i + 1) * 2;
		}
	}

	//Resize to 32
	cn_vec_resize(vec, 32);
	shmem_barrier_all();
	
	//Get the variables from vector in all but first instance
	if (id != 0)
		cn_shvec_get(vec, vec, 0);
	
	//Print out all values
	for (i = 0; i < cn_vec_size(vec); i++) {
		printf("vec[%d, %d] = %d\n", id, i, cn_vec_get(vec, int, i));
	}

	shmem_barrier_all();

	//Free CN_VEC
	cn_vec_free(vec);
}
