CC = oshcc
LD = oshcc
CFLAGS = -g3 -std=c11 -D_GNU_SOURCE
LDFLAGS = -g -lm
#CFLAGS = -Wall -O3 -DNDEBUG
#LDFLAGS = -O3
AR = ar

.PHONY: all

all: hw_tester

fast: CFLAGS = -std=c11 -D_GNU_SOURCE -O3
fast: clean hw_tester

lib: libhw.a

hw_tester: % : hw.o libhw.a
	$(LD) -o hw_tester $^ $(LDFLAGS)

libhw.a: relaxation.o relax_jacobi.o relax_template.o relax_shmem.o misc.o cn_vec.o
	$(AR) cru libhw.a $^

%.o: %.c jacobi.h header.h Makefile
	$(CC) -c $< -o $@ $(CFLAGS)

# Handy/CNDS
cn_vec.o: lib/handy/cnds/cn_shvec.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm -rf *.o *.a hw_tester

cleano:
	rm -rf *.o
