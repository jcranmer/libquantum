/* shor.c: implementation of Shor's quantum algorithm for factorization
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "quantum_stdlib.h"
#include "cuda_stdlib.h"
#include "shor.h"

int use_cuda = 0;

/* TODO: Change input parsing and/or accepted parameters
 * Currently mirrors libquantum's formatting exactly to allow correctness testing.
 * This technically makes our code licensed under the GPL v2 unless removed.
 */
int main(int argc, char** argv) {
	if(argc == 1) {
		printf("Usage: sim [number] [rand] [use_cuda]\n\n");
		return 3;
	}

  if (argc > 3) {
    use_cuda = atoi(argv[2]);
  }

	int N = atoi(argv[1]);

	if(N < 15) {
		printf("Invalid number\n\n");
		return 3;
	}

	// Find a number x relatively prime to n
	srand(time(NULL));
	int x;
	do {
		x = rand() % N;
	} while(quda_gcd_div(N,x) > 1 || x < 2);

  if (argc > 2) {
    x = atoi(argv[2]);
  }
	printf("Random seed: %i\n", x);
  srand(x);

	int L = qubits_required(N);
	//int width = qubits_required(N*N); // commonly seen case
	//int width = 2*L+2; // commonly seen case
	int width = L; // basic case, but ~25% chance of measuring 0 for 15

	printf("N = %i, %i qubits required\n", N, width+L);

	quantum_reg qr1;
	quda_quantum_reg_init(&qr1,width);
	quda_quantum_reg_set(&qr1,0);

	quda_quantum_hadamard_all(&qr1);

	//quda_quantum_add_scratch(3*L+2,&qr1); // Extra scratch probably unnecessary
	quda_quantum_add_scratch(L,&qr1); // effectively creates 'output' subregister for exp_mod_n()
	quda_classical_exp_mod_n(x,N,&qr1);

	/* By the principle of implicit measurement, since we are effectively done with the 'output'
	 * subregister, it may be measured at any time. This measurement will collapse the register's
	 * scratch bits into a single possible state (and any generators that created it). We should
	 * ALWAYS do this in our simulator, since it reduces mem usage by at least half (if not more).
	 */
	quda_quantum_collapse_scratch(&qr1);

  if (use_cuda)
    quda_cu_quantum_fourier_transform(&qr1);
  else
    quda_quantum_fourier_transform(&qr1);

	uint64_t result;
	int res = quda_quantum_reg_measure_and_collapse(&qr1,&result);
	if(res == -1) {
		printf("Invalid result (normalization error).\n");
		return -1;
	} else if(result == 0) {
		// NOTE: This can (kind of) be a valid result for 15 with (x=7,width=11) ~.25 prob
		// Creates fraction 0/1, expands to 0/2, 2 is a valid period
		// Obviously doesn't hold for other numbers and thus may create erroneous results.
		printf("Measured zero.\n");
		return 0;
	}

	uint64_t denom = 1 << width;
	quda_classical_continued_fraction_expansion(&result,&denom);

	printf("fractional approximation is %lu/%lu.\n", result, denom);

	if((denom % 2 == 1) && (2*denom < (1<<width))) {
		printf("Odd denominator, trying to expand by 2.\n");
		denom *= 2;
    }

	if(denom % 2 == 1) {
		printf("Odd period, try again.\n");
		return 0;
	}

	printf("Possible period is %lu.\n", denom);

	int factor = pow(x,denom/2);
	int factor1 = quda_gcd_div(N,factor + 1);
	int factor2 = quda_gcd_div(N,factor - 1);
	if(factor1 > factor2) {
		factor = factor1;
	} else {
		factor = factor2;
	}

	if(factor < N && factor > 1) {
		printf("%d = %d * %d\n",N,factor,N/factor);
	} else {
		printf("Could not determine factors.\n");
	}

	quda_quantum_reg_delete(&qr1);

	return 0;
}

// Testing functions
void dump_mod_exp_results(quantum_reg* qreg, int* num_outputs) {
	int outputs;
	if(num_outputs) {
		outputs = *num_outputs;
	} else {
		outputs = qreg->num_states;
	}

	uint64_t mask = (1 << qreg->qubits)-1;
	uint64_t smask = ((1 << qreg->scratch)-1) << qreg->qubits;

	uint64_t val,sval;
	int i;
	for(i=0;i<outputs;i++) {
		val = qreg->states[i].state & mask;
		sval = (qreg->states[i].state & smask) >> qreg->qubits;
		printf("state[%d]: r_input = %lu, r_output =  %lu\n",i,val,sval);
	}
}

// Classical functions

int qubits_required(int num) {
	int i;
	num >>= 1;
	for(i=1;num > 0;i++) {
		num >>= 1;
	}
	return i;
}
