# -*- Makefile -*-

CC = gcc
CFLAGS ?= -g3 -Wall -std=c11 -D_GNU_SOURCE -pipe -fPIC -fopenmp -O3

LD = $(CC)
LDFLAGS = -g
LDLIBS = -lm -fopenmp
LOADLIBES = $(LDLIBS)

AR = ar
ARFLAGS = cru

all: hw3_tester

lib: libhw3.a

hw3_tester: % : hw.o libhw3.a
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

libhw3.a: relaxation.o relax_jacobi.o relax_template.o relax_openmp.o misc.o
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c jacobi.h header.h GNUmakefile
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	$(RM) *.o *.a hw3_tester

.PHONY: all clean
