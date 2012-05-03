CC=gcc -std=c99
CFLAGS=-g -Wall -Werror -pedantic
LDFLAGS=-lm

all: libquantum.a

libquantum.a: complex.o quantum_reg.o quantum_gates.o quantum_stdlib.o
	ar rcs libquantum.a complex.o quantum_reg.o quantum_gates.o quantum_stdlib.o

complex.o: complex.c complex.h 
	$(CC) $(CFLAGS) -c complex.c

quantum_reg.o: quantum_reg.c quantum_reg.h
	$(CC) $(CFLAGS) -c quantum_reg.c

quantum_gates.o: quantum_gates.c quantum_gates.h complex.h
	$(CC) $(CFLAGS) -c quantum_gates.c

quantum_stdlib.o: quantum_stdlib.c quantum_stdlib.h quantum_reg.h quantum_gates.h complex.h
	$(CC) $(CFLAGS) -c quantum_stdlib.c

%.o: %.cu
	nvcc -gencode=arch=compute_13,code=\"sm_13,compute_13\" \
		-gencode=arch=compute_20,code=\"sm_20,compute_20\" -o $@ -m64 \
		-c $< -DUNIX -O2 -I/usr/local/cuda/include

test: libquantum.a test.c complex.h quantum_reg.h quantum_gates.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o test test.c libquantum.a

shor: libquantum.a shor.c shor.h quantum_stdlib.h quantum_reg.h cuda_stdlib.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o shor shor.c libquantum.a cuda_stdlib.o -lcudart

check: test
	@echo ./test
	@./test | grep '^FAIL'; \
	if [ $$? = 0 ]; then exit 1; else exit 0; fi

clean:
	rm -f test libquantum.a *.o
