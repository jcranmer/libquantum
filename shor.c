/* shor.c: implementation of Shor's quantum algorithm for factorization
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "quantum_stdlib.h"
#include "shor.h"

/* TODO: Change input parsing and/or accepted parameters
 * Currently mirrors libquantum's formatting exactly to allow correctness testing.
 * This technically makes our code licensed under the GPL v2 unless removed.
 */
int main(int argc, char** argv) {
	if(argc == 1) {
		printf("Usage: sim [number]\n\n");
		return 3;
	}

	int N = atoi(argv[1]);

	if(N < 15) {
		printf("Invalid number\n\n");
		return 3;
	}

	// Find a number x relatively prime to n
	//srand(13); // TESTING - guarantees |4> as output reg state
	srand(time(NULL));
	int x;
restart:
	do {
		x = rand() % N;
	} while(quda_gcd_div(N,x) > 1 || x < 2);

	//x = 8; // DEBUG
	//x = 7; // TESTING
	//x = 13; // TESTING2
	printf("Random seed: %i\n", x);

	int L = qubits_required(N);
	//int width = qubits_required(N*N);
	//int width = 2*L+2;
	//int width = 11; // TESTING
	//int width = 4; // TESTING2
	int width = L; // Minimum accuracy generic test

	printf("N = %i, %i qubits required\n", N, width+L);

	quantum_reg qr1;
	quda_quantum_reg_init(&qr1,width);
	quda_quantum_reg_set(&qr1,0);

	int err;
	/* DEBUG */
	err = quda_check_normalization(&qr1);
	err |= quda_weak_check_amplitudes(&qr1);
	if(err != 0) {
		printf("ERROR: ");
	}
	printf("After quda_quantum_reg_set()\n");
	/* END DEBUG */

	quda_quantum_hadamard_all(&qr1);

	/* DEBUG */
	err = quda_check_normalization(&qr1);
	err |= quda_weak_check_amplitudes(&qr1);
	if(err != 0) {
		printf("ERROR: ");
	}
	printf("After quda_quantum_hadamard_all()\n");
	/* END DEBUG */

	//quda_quantum_add_scratch(3*L+2,&qr1); // Full scratch not needed for classical exp_mod_n
	quda_quantum_add_scratch(L,&qr1); // effectively creates 'output' subregister for exp_mod_n() (+TESTING)
	quda_classical_exp_mod_n(x,N,&qr1);

	/*// TESTING - Verifies exactly for x = 7 (width = 11)
	int outputs = 7;
	dump_mod_exp_results(&qr1,&outputs);
	exit(0);
	*/// END TESTING

	// TESTING 2 - Verfies exactly for x = 13, width = 4
	//dump_mod_exp_results(&qr1,NULL); // TESTING 2 (x=13,width=4)
	//exit(0);
	// END TESTING2

	/* DEBUG */
	err = quda_check_normalization(&qr1);
	err |= quda_weak_check_amplitudes(&qr1);
	if(err != 0) {
		printf("ERROR: ");
	}
	printf("After quda_classical_exp_mod_n()\n");
	printf("Going into quantum_collapse_scratch()\n");
	/* END DEBUG */

	/* libquantum measures all of its scratch bits here for some reason.
	 * Presumedly, this reduces memory overhead by finding the single most likely value of
	 * x^(hadamarded-input) % n. Thus their qreg would be left with only inputs that correspond
	 * to the most likely output. It may also be a correctness issue since they use the least
	 * significant register bits to store scratch (with no differentiation from regular bits).
	 * Comment the next line for the opposite effect (no coalescing).
	 */
	// TESTING - this function is now correct
	quda_quantum_collapse_scratch(&qr1); // Explain implicit measurement; stupid not to call this

	/* DEBUG */
	err = quda_check_normalization(&qr1);
	err |= quda_weak_check_amplitudes(&qr1);
	if(err != 0) {
		printf("ERROR: ");
	}
	printf("After quantum_collapse_scratch()\n");
	printf("Going into quantum_fourier_transform()\n");
	//qsort(qr1.states,qr1.num_states,sizeof(quantum_state_t),qstate_compare); // TESTING
	// TESTING - dump verified as correct for (x,width) in {(7,11),(13,4)}
	//quda_quantum_reg_dump(&qr1,"BEFORE_FOURIER");
	/* END DEBUG */

	quda_quantum_fourier_transform(&qr1);

	/* DEBUG */
	err = quda_check_normalization(&qr1);
	err |= quda_weak_check_amplitudes(&qr1);
	if(err != 0) {
		printf("ERROR: ");
	}
	printf("After quda_quantum_fourier_transform()\n");
	printf("Going into reg_measure_and_collapse\n");
	//quda_quantum_reg_dump(&qr1,"AFTER_FOURIER");  // TESTING - results appear incorrect
	/* END DEBUG */

	uint64_t result;
	int res = quda_quantum_reg_measure_and_collapse(&qr1,&result);
	if(res == -1) {
		printf("Invalid result (normalization error).\n");
		return -1;
	} else if(result == 0) {
		// NOTE: This is actually a valid result for 15 with (x=7,width=11) ~.25 prob
		// Creates fraction 0/1, expands to 0/2, 2 is a valid period
		printf("Measured zero.\n");
		goto restart; // CTS TESTING
		return 0;
	}

	// FIXME - Fractional expansion may be incorrect for our implementation
	uint64_t denom = 1 << width;
	quda_classical_continued_fraction_expansion(&result,&denom);

	printf("fractional approximation is %lu/%lu.\n", result, denom);

	if((denom % 2 == 1) && (2*denom < (1<<width))) {
		printf("Odd denominator, trying to expand by 2.\n");
		denom *= 2;
    }

	if(denom % 2 == 1) {
		printf("Odd period, try again.\n");
		goto restart; // CTS TESTING
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
		goto restart; // CTS TESTING
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
