#include <stdio.h>
#include <shmem.h>

static int* val;

main() {
	//Start N number of threads (determined by API)
	start_pes(4);

	int num_proc = shmem_n_pes(),
	    id       = shmem_my_pe();

	val = (int*) shmalloc(4 * sizeof(int));
	val[0] = 2 * id;
	val[1] = 1337;
	val[2] = 666;
	val[3] = 3567;

	if (id == 0) {
		val[0] = 678;
	}

	shmem_barrier_all();

	printf(
		"%d - %d\n",
		shmem_n_pes(),
		shmem_my_pe()
	);
	int i = 0;
	if (id != 0) {
		shmem_int_get(&val[0], &val[0], 1, 0);
	}
	for (; i < 4; i++) {
		printf("%d - i[%d] = %d\n", id, i, val[i]);
	}
	//shmem_barrier_all();
	shfree(val);
	return 0;
}
