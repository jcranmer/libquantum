/* quantum_stdlib.c: common useful operations for quantum registers
*/

#include <stdio.h>
#include <math.h>
#include "quantum_stdlib.h"
#include "quantum_gates.h"
#include "complex.h"

//#define QUDA_STDLIB_DEBUG
#define QUDA_CFE_STEP 0.000005
#define QUDA_FLOAT_ERR 1e-7

// Testing functions
int quda_check_normalization(quantum_reg* qreg) {
	int i;
	float p = 0.0;
	for(i=0;i<qreg->num_states;i++) {
		p += quda_complex_abs_square(qreg->states[i].amplitude);
	}

	
	if(qreg->num_states > 0 && (p < 1.0 - QUDA_FLOAT_ERR || p > 1.0)) {
		#ifdef QUDA_STDLIB_DEBUG
		printf("Normalization error. P = %f\n",p);
		#endif
		return -1;
	} else {
		printf("NORM OK, P = %f\n",p); // DEBUG
	}
	return 0;
}

int quda_weak_check_amplitudes(quantum_reg* qreg) {
	int i;
	int err = 0;
	for(i=0;i<qreg->num_states;i++) {
		if(quda_complex_abs_square(qreg->states[i].amplitude) > 1.0) {
			#ifdef QUDA_STDLIB_DEBUG
			printf("Amplitude error. state[%d] --> (%f,%f)\n",i,qreg->states[i].amplitude.real,
					qreg->states[i].amplitude.imag);
			#endif
			err = 1;
		}
	}

	return err;
}

void quda_quantum_reg_dump(quantum_reg* qreg, char* tag) {
	int i;
	uint64_t mask = (1 << qreg->qubits)-1;
	uint64_t smask = ((1 << qreg->scratch)-1) << qreg->qubits;
	printf("QREG_DUMP: %d states\n",qreg->num_states);
	for(i=0;i<qreg->num_states;i++) {
		if(tag) printf("%s: ",tag);
		printf("qreg->states[%d].state = %lu (bits,scratch)=(%lu,%lu)\n",i,qreg->states[i].state,
				qreg->states[i].state & mask,(qreg->states[i].state & smask) >> qreg->qubits);
		if(tag) printf("%s: ",tag);
		printf("qreg->states[%d].amplitude = (%f,%f)\n",i,qreg->states[i].amplitude.real,
				qreg->states[i].amplitude.imag);
	}
}

// Utility functions
int quda_quantum_hadamard_range(int start,int end,quantum_reg* qreg) {
	int i,res;
	for(i=start;i<end;i++) {
		res = quda_quantum_hadamard_gate(i,qreg);
		if(res) {
			return -1;
		}
	}
	return 0;
}

int quda_quantum_hadamard_after(int start,quantum_reg* qreg) {
	return quda_quantum_hadamard_range(start,qreg->qubits,qreg);
}

int quda_quantum_hadamard_all(quantum_reg* qreg) {
	return quda_quantum_hadamard_range(0,qreg->qubits,qreg);
}

// Original implementation
void quda_quantum_fourier_transform(quantum_reg* qreg) {
	int q = qreg->qubits-1;
	int i,j;
  printf("Number of states: %d\n", qreg->num_states);
	for(i=q;i>=0;i--) {
		for(j=q;j>i;j--) {
			#ifdef QUDA_STDLIB_DEBUG
			printf("Performing c-R_%d (PI/%lu) on (%d,%d)\n",j-i+1,(uint64_t)1 << (j-i),j,i); // DEBUG
			#endif
			quda_quantum_controlled_rotate_k_gate(j,i,qreg,j-i+1);
		}
		#ifdef QUDA_STDLIB_DEBUG
		printf("Performing hadamard(bit %d)\n",i); // DEBUG
		#endif
		quda_quantum_hadamard_gate(i,qreg);
  printf("Number of states after hadamard %d: %d\n", i, qreg->num_states);
	}

	// TODO: Consider using SWAP gate here instead
	for(i=0;i<qreg->qubits/2;i++) {
		quda_quantum_controlled_not_gate(i,q-i,qreg);
		quda_quantum_controlled_not_gate(q-i,i,qreg);
		quda_quantum_controlled_not_gate(i,q-i,qreg);
	}
}

// Classical functions

void quda_classical_exp_mod_n(int x, int n, quantum_reg* qreg) {
	int i;
	for(i=0;i<qreg->num_states;i++) {
		uint64_t value = quda_mod_pow_simple(x,qreg->states[i].state,n);
		value <<= qreg->qubits; // move to 'output' register (scratch space)
		qreg->states[i].state |= value;
	}

	
}
void quda_classical_continued_fraction_expansion(uint64_t* num, uint64_t* denom) {
	uint64_t orig_denom = *denom;
	float f = *num/(float)orig_denom;

	#ifdef QUDA_STDLIB_DEBUG
	printf("Measured %lu (%f)\n",*num,f);
	#endif

	float g = f;
	uint64_t num1 = 1;
	uint64_t num2 = 0;
	uint64_t denom1 = 0;
	uint64_t denom2 = 1;
	int i;

	do {
		i = (int)(g+QUDA_CFE_STEP);
		g -= i-QUDA_CFE_STEP;
		g = 1.0/g;

		if(i * denom1 + denom2 > orig_denom) {
			break;
		}

		*num = i * num1 + num2;
		*denom = i * denom1 | denom2;
		num2 = num1;
		denom2 = denom1;
		num1 = *num;
		denom1 = *denom;
		
	} while(fabsf((*num/(float)*denom) - f) > 1.0 / (2 * orig_denom));
}

// Helper functions

int quda_gcd_div(int x, int y) {
	int temp;
	while(y != 0) {
		temp = y;
		y = x % y;
		x = temp;
	}

	return x;
}

int quda_gcd_sub(int x, int y) {
	if(x == 0) {
		return y;
	}

	while(y != 0) {
		if(x > y) {
			x -= y;
		} else {
			y -= x;
		}
	}

	return x;
}

uint64_t quda_mod_pow_simple(int b, uint64_t e, int n) {
	uint64_t res = 1;
	uint64_t i;
	for(i=0;i<e;i++) {
		res = (res*b) % n;
	}

	return res;
}

uint64_t quda_mod_pow_bin(int b, uint64_t e, int n) {
	uint64_t res = 1;
	while(e > 0) {
		if((e & 1) == 1) {
			res = (res*b) % n;
		}
		e >>= 1;
		b = (b*b) % n;
	}

	return res;
}
