#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shmem.h>

static int* val;

main() {
	//Start N number of threads (determined by API)
	start_pes(0);

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
	int i = 1;
	if (id == 0) {
		for (; i < num_proc; i++)
			shmem_int_put(&val[0], &val[0], 1, i);
	}
	shmem_barrier_all();
	for (i = 0; i < 4; i++) {
		printf("%d - i[%d] = %d\n", id, i, val[i]);
	}
	shmem_barrier_all();
	shfree(val);
	if (id == 0) {
		printf("lol done\n");
	}
	return 0;
}
