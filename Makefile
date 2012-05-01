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

test: libquantum.a test.c complex.h quantum_reg.h quantum_gates.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o test test.c libquantum.a

shor: libquantum.a shor.c shor.h quantum_stdlib.h quantum_reg.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o shor shor.c libquantum.a

check: test
	@echo ./test
	@./test | grep '^FAIL'; \
	if [ $$? = 0 ]; then exit 1; else exit 0; fi

clean:
	rm test libquantum.a *.o
