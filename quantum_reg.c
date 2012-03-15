/* quantum_reg.c: quantum register source
*/

#include "quantum_reg.h"
#include <stdlib.h>
#include <math.h>

int quda_quantum_reg_init(quantum_reg* qreg, int qubits) {
	qreg->qubits = qubits;
	qreg->size = (int)(DEFAULT_QTS_RATIO*qubits);
	qreg->num_states = 0;
	qreg->states = (quantum_state_t*)malloc(qreg->size*sizeof(quantum_state_t));
	if(qreg->states == NULL) {
		return -1;
	}

	return 0;
}

void quda_quantum_reg_set(quantum_reg* qreg, uint64_t state) {
	qreg->num_states = 1;
	qreg->states[0].state = state;
	qreg->states[0].amplitude = QUDA_COMPLEX_ONE;
}

void quda_quantum_reg_delete(quantum_reg* qreg) {
	free(qreg->states);
}

void quda_quantum_bit_set(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		qreg->states[i].state = qreg->states[i].state | mask;
	}

	quda_quantum_reg_coalesce(qreg);
}

void quda_quantum_bit_reset(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = ~(1 << target);
	for(i=0;i<qreg->num_states;i++) {
		qreg->states[i].state = qreg->states[i].state & mask;
	}

	quda_quantum_reg_coalesce(qreg);
}

/* Performs a measurement on the quantum register */
int quda_quantum_reg_measure(quantum_reg* qreg, uint64_t* retval) {
	if(retval == NULL) return -2;
	float f = quda_rand_float();
	int i;
	for(i=0;i<qreg->num_states;i++) {
		if(!quda_complex_eq(qreg->states[i].amplitude,QUDA_COMPLEX_ZERO)) {
			f -= quda_complex_abs_square(qreg->states[i].amplitude);
			if(f < 0) {
				*retval = qreg->states[i].state;
				return 0;
			}
		}
	}

	return -1;
}

/* Performs a real-world quantum measurement.
 * The register collapses to the physical state measured with probability 1.
 */
int quda_quantum_reg_measure_and_collapse(quantum_reg* qreg, uint64_t* retval) {
	if(retval == NULL) return -2;
	float f = quda_rand_float();
	int i;
	for(i=0;i<qreg->num_states;i++) {
		if(!quda_complex_eq(qreg->states[i].amplitude,QUDA_COMPLEX_ZERO)) {
			f -= quda_complex_abs_square(qreg->states[i].amplitude);
			if(f < 0) {
				*retval = qreg->states[i].state;
				qreg->states[0].state = qreg->states[i].state;
				qreg->states[0].amplitude = QUDA_COMPLEX_ONE;
				qreg->num_states = 1;
				return 0;
			}
		}
	}

	return -1;
}

/* Measure 1 bit of a quantum register */
int quda_quantum_bit_measure(int target, quantum_reg* qreg) {
	float p = 0;
	float f = quda_rand_float();
	uint64_t mask = 1 << target;
	int i;
	// Accumulate probability that the bit is in state |1>
	for(i = 0;i<qreg->num_states;i++) {
		if(qreg->states[i].state & mask) {
			p += quda_complex_abs_square(qreg->states[i].amplitude);
			// TODO: Determine overhead of this comparison
			if(p > quda_rand_float()) return 1;
		}
	}

	return 0;
	//return quda_rand_float() < p ? 1 : 0;
}

int quda_quantum_bit_measure_and_collapse(int target, quantum_reg* qreg) {
	// Measure bit conventionally
	// TODO: Can allow probability measurement from conventional to complete and remove it below
	int retval = quda_quantum_bit_measure(target,qreg);

	// Collapse states to those possible
	uint64_t mask = 1 << target;
	float p = 0;
	int i;
	for(i=0;i<qreg->num_states;i++) {
		// TODO: Ideally, remove nested conditions
		// TODO: Actually prune in this loop instead of just invalidating
		if(qreg->states[i].state & mask) {
			if(retval) { // this is a valid state, accumulate probability to renormalize
				p += quda_complex_abs_square(qreg->states[i].amplitude);
			} else { // this is an invalid state -- nullify
				qreg->states[i].amplitude = QUDA_COMPLEX_ZERO;
			}
		} else {
			if(!retval) { // valid state, accumulate probability
				p += quda_complex_abs_square(qreg->states[i].amplitude);
			} else { // invalid state -- nullify
				qreg->states[i].amplitude = QUDA_COMPLEX_ZERO;
			}
		}
	}

	// TODO: Remove this call for optimization within the above loop
	quda_quantum_reg_prune(qreg);

	// Renormalize
	float k = sqrt(1.0f/p);
	for(i=0;i<qreg->num_states;i++) {
		qreg->states[i].amplitude = quda_complex_rmul(qreg->states[i].amplitude,k);
	}

	return retval;
}

void quda_quantum_reg_prune(quantum_reg* qreg) {
	int i,end;
	for(i=0,end=qreg->num_states-1;i < end;i++) {
		if(quda_complex_eq(qreg->states[i].amplitude,QUDA_COMPLEX_ZERO)) {
			while(quda_complex_eq(qreg->states[end].amplitude,QUDA_COMPLEX_ZERO)) {
				end--;
				if(i == end) break;
			}

			if(i < end) {
				qreg->states[i] = qreg->states[end];
			} else {
				break;
			}
		}
	}
	qreg->num_states = end+1;
}

int quda_quantum_reg_enlarge(quantum_reg* qreg,int* amount) {
	int increase;
	if(amount == NULL) {
		increase = qreg->size;
	} else {
		increase = *amount;
	}

	quantum_state_t* temp_states = malloc((qreg->size+increase)*sizeof(quantum_state_t));
	if(temp_states == NULL) {
		return -1;
	}

	int i,j;
	for(i=0,j=0;i<qreg->num_states;i++) {
		if(!quda_complex_eq(qreg->states[i].amplitude,QUDA_COMPLEX_ZERO)) {
			temp_states[j++] = qreg->states[i];
		}
	}

	qreg->num_states = j;
	free(qreg->states);
	qreg->states = temp_states;

	return 0;
}

void quda_quantum_reg_coalesce(quantum_reg* qreg) {
	if(qreg->num_states < 2) return;
	qsort(qreg->states,qreg->num_states,sizeof(quantum_state_t),qstate_compare);

	int i,j;
	for(i=1,j=0;i<qreg->num_states;i++) {
		if(qreg->states[j].state == qreg->states[i].state) {
			qreg->states[j].amplitude = quda_complex_add(qreg->states[j].amplitude,
					qreg->states[i].amplitude);
			qreg->states[i].amplitude = QUDA_COMPLEX_ZERO;
		} else {
			j = i;
		}
	}

	// TODO: Optimize pruning into the above pass or make it optional/conditional
	quda_quantum_reg_prune(qreg);
}

int quda_quantum_reg_trim(quantum_reg* qreg) {
	int old_states = qreg->num_states;
	quda_quantum_reg_prune(qreg);
	if(qreg->num_states < old_states) {
		quantum_state_t* temp_states = malloc(qreg->num_states*sizeof(quantum_state_t));
		if(temp_states == NULL) {
			return -1;
		}

		int i;
		for(i=0;i < qreg->num_states;i++) {
			temp_states[i] = qreg->states[i];
		}

		free(qreg->states);
		qreg->states = temp_states;
	}

	return 0;
}

float quda_rand_float() {
	return rand()/(float)RAND_MAX;
}

int qstate_compare(const void* qstate1, const void* qstate2) {
	uint64_t diff = ((quantum_state_t*)qstate1)->state - ((quantum_state_t*)qstate2)->state;
	if(diff == 0) return 0;
	return diff > 0 ? 1 : -1;
}
