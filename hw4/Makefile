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

lib: libhw2.a

hw_tester: % : hw.o libhw.a
	$(LD) -o hw_tester $^ $(LDFLAGS)

libhw.a: relaxation.o relax_jacobi.o relax_template.o misc.o
	$(AR) cru libhw.a $^

%.o: %.c jacobi.h header.h Makefile
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -rf *.o *.a hw_tester

cleano:
	rm -rf *.o
