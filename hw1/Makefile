CC = gcc
LD = gcc
CFLAGS = -Wall -O3 -DNDEBUG
LDFLAGS = -O3
AR = ar

.PHONY: all
all: hw1_tester
lib: libhw1.a

hw1_tester: % : hw1.o libhw1.a
	$(LD) -o hw1_tester $^ $(LDFLAGS)

libhw1.a: jacobi.o misc.o
	$(AR) cru libhw1.a $^

%.o: %.c jacobi.h header.h Makefile
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -rf *.o *.a
