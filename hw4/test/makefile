CC=oshcc
CFLAGS=--std=gnu89 -g

all: shmem_test_put shmem_test_get

shmem_test_put: shmem_test_put.o
	$(CC) $(CFLAGS) -o $@ $^

shmem_test_get: shmem_test_get.o
	$(CC) $(CFLAGS) -o $@ $^

shmem_test_put.o: shmem_test_put.c
	$(CC) $(CFLAGS) -o $@ -c $^

shmem_test_get.o: shmem_test_get.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	$(RM) *.o shmem_test_put shmem_test_get
