/* quantum_reg.c: quantum register source
*/

#include "quantum_reg.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h> // DEBUG

int quda_quantum_reg_init(quantum_reg* qreg, int qubits) {
	qreg->qubits = qubits;
	qreg->size = (int)(DEFAULT_QTS_RATIO*qubits);
	qreg->scratch = 0;
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

// TODO: Registers are currently hard-limited to 64 total real/scratch qubits
void quda_quantum_add_scratch(int n, quantum_reg* qreg) {
	qreg->scratch += n;
}

void quda_quantum_clear_scratch(quantum_reg* qreg) {
	uint64_t mask = (1 << qreg->qubits)-1;
	int i;
	for(i=0;i<qreg->num_states;i++) {
		qreg->states[i].state &= mask;
	}

	qreg->scratch = 0;
	quda_quantum_reg_coalesce(qreg);
}

inline void quda_quantum_collapse_scratch(quantum_reg* qreg) {
	quda_quantum_range_measure_and_collapse(qreg->qubits,qreg->qubits+qreg->scratch,qreg,NULL);
}

inline int quda_quantum_scratch_bit(int index, quantum_reg* qreg) {
	return qreg->qubits + index;
}

int quda_quantum_reg_measure(quantum_reg* qreg, uint64_t* retval,int scratch) {
	if(retval == NULL) return -2;
	float f = quda_rand_float();
	int i;
	for(i=0;i<qreg->num_states;i++) {
		if(!quda_complex_eq(qreg->states[i].amplitude,QUDA_COMPLEX_ZERO)) {
			f -= quda_complex_abs_square(qreg->states[i].amplitude);
			if(f < 0) {
				//printf("Chose state %d: state == %lu, amp == (%f,%f)\n",i,
				//		qreg->states[i].state & ((1 << qreg->qubits)-1),
				//		qreg->states[i].amplitude.real,qreg->states[i].amplitude.imag); // DEBUG
				if(!scratch && qreg->scratch > 0) {
					uint64_t mask = (1 << qreg->qubits)-1;
					*retval = qreg->states[i].state & mask;
				} else {
					*retval = qreg->states[i].state;
				}
				return 0;
			}
		}
	}

	return -1;
}

int quda_quantum_reg_measure_and_collapse(quantum_reg* qreg, uint64_t* retval) {
	if(retval == NULL) return -2;
	if(qreg->scratch > 0) {
		/* This can conceivably result in multiple instances of the same state.
		 * While this function is technically resilient to duplicate states,
		 * its fairness is somewhat questionable.
		 */
		quda_quantum_clear_scratch(qreg);
	}
	float f = quda_rand_float();
	int i;
	//printf("%d states\n",qreg->num_states); // DEBUG
	for(i=0;i<qreg->num_states;i++) {
		//printf("qreg->state[%d].state = %lu\n",i,qreg->states[i].state); // DEBUG
		//printf("qreg->state[%d].amplitude = (%f,%f)\n",i,qreg->states[i].amplitude.real,qreg->states[i].amplitude.imag); // DEBUG
		if(!quda_complex_eq(qreg->states[i].amplitude,QUDA_COMPLEX_ZERO)) {
			f -= quda_complex_abs_square(qreg->states[i].amplitude);
			if(f < 0) {
				uint64_t mask = (1 << qreg->qubits)-1;
				*retval = qreg->states[i].state & mask;
				//*retval = qreg->states[i].state;
				qreg->states[0].state = qreg->states[i].state;
				qreg->states[0].amplitude = QUDA_COMPLEX_ONE;
				qreg->num_states = 1;
				return 0;
			}
		}
	}

	return -1;
}

int quda_quantum_range_measure_and_collapse(int start, int end, quantum_reg* qreg, uint64_t* retval) {
	int i;
	uint64_t res = 0;
	for(i=0;i<end;i++) {
		res |= quda_quantum_bit_measure_and_collapse(i,qreg) << i;
	}

	if(retval) {
		*retval = res;
	}

	return 0;
}
/*	// old range_measure_and_collapse - appears to be invalid
	uint64_t res;
	// Measure entire register (classical operation)
	int err = quda_quantum_reg_measure(qreg,&res,1);
	if(err != 0) {
		return err;
	}
	//printf("res: %lu\n",res); // DEBUG

	// Collapse states to those possible
	uint64_t mask = ((1 << (end-start)) - 1) << start;
	//printf("mask: %lu\n",res); // DEBUG
	//uint64_t ores = res; // DEBUG
	res &= mask; // determine measured range's state from measured state
	//printf("res & mask: %lu\n",res); // DEBUG
	float p = 0;
	int i;
	for(i=0;i<qreg->num_states;i++) {
		// TODO: Actually prune in this loop instead of just invalidating
		// DEBUG BLOCK
		if(qreg->states[i].state == ores) {
			printf("Found matching state. state & res == res: %d\n",(qreg->states[i].state & res) == res);
		}
		// END DEBUG BLOCK
		if((qreg->states[i].state & res) == res) {
			// this is a valid state, accumulate probability to renormalize
			p += quda_complex_abs_square(qreg->states[i].amplitude);
		} else { // this is an invalid state -- nullify
			qreg->states[i].amplitude = QUDA_COMPLEX_ZERO;
		}
	}
	//printf("RM&C: #states BEFORE: %d\n",qreg->num_states); // DEBUG

	// TODO: Remove this call for optimization within the above loop
	quda_quantum_reg_prune(qreg);

	//printf("RM&C: #states AFTER: %d\n",qreg->num_states); // DEBUG
	// Renormalize
	float k = sqrt(1.0f/p);
	for(i=0;i<qreg->num_states;i++) {
		qreg->states[i].amplitude = quda_complex_rmul(qreg->states[i].amplitude,k);
	}

	if(retval) {
		*retval = res;
	}

	return 0;
}
*/

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
			if(p > f) return 1; // short-circuits iteration if already past threshold
		}
	}

	return 0;
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
				if(i == end) { // if no non-zero elements to copy
					break;	
				}
			}

			if(i < end) {
				// non-zero element found, copy it
				qreg->states[i] = qreg->states[end--];
				// NOTE: Next line can be avoided by using 'end' instead of 'i' to set 'num_states'
				//if(i == end) break; // allowing i to increment can cause 'num_states' errors
			} else {
				// algorithm is done, need to set length to i
				break;
			}
		}
	}

	printf("PRUNE: Reduced states from %d ",qreg->num_states); // DEBUG
	if(quda_complex_eq(qreg->states[i].amplitude,QUDA_COMPLEX_ZERO)) {
		qreg->num_states = end;
	} else {
		qreg->num_states = end+1;
	}
	printf("to %d\n",qreg->num_states); // DEBUG
	if(quda_complex_eq(qreg->states[qreg->num_states-1].amplitude,QUDA_COMPLEX_ZERO)) {
		printf("PRUNE: Violation. Zero last element.\n"); // TESTING
	}
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
	qreg->size += increase;

	return 0;
}

void quda_quantum_reg_coalesce(quantum_reg* qreg) {
	if(qreg->num_states < 2) return;
	qsort(qreg->states,qreg->num_states,sizeof(quantum_state_t),qstate_compare);
	//printf("COALESCE: old states: %d\n",qreg->num_states); // DEBUG

	int i,j;
	int renorm = 0;
	for(i=1,j=0;i<qreg->num_states;i++) {
		if(qreg->states[j].state == qreg->states[i].state) {
			/*// DEBUG BLOCK
			printf("coalescing states %d and %d.\n",j,i);
			printf("amplitude grows from (%f,%f) to ",qreg->states[j].amplitude.real,
					qreg->states[j].amplitude.imag);
			*/// END DEBUG BLOCK
			renorm |= quda_amplitude_coalesce(&qreg->states[j].amplitude,
					&qreg->states[i].amplitude);
			//printf("(%f,%f) (state[%d])\n",qreg->states[j].amplitude.real,qreg->states[j].amplitude.imag,j); // DEBUG
		} else {
			j = i;
		}
	}

	if(renorm) {
		quda_quantum_reg_renormalize(qreg);
	}

	// TODO: Optimize pruning into the above pass or make it optional/conditional
	quda_quantum_reg_prune(qreg);
	//printf("COALESCE: new states: %d\n",qreg->num_states); // DEBUG
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

void quda_quantum_reg_renormalize(quantum_reg* qreg) {
	int i;
	float p = 0.0f;
	for(i=0;i<qreg->num_states;i++) {
		p += quda_complex_abs_square(qreg->states[i].amplitude);
	}

	// Apply renormalization
	float k = sqrt(1.0f/p);
	for(i=0;i<qreg->num_states;i++) {
		qreg->states[i].amplitude = quda_complex_rmul(qreg->states[i].amplitude,k);
	}
}

// TODO: Optimize logic in this function
int quda_amplitude_coalesce(complex_t* dest, complex_t* toadd) {
	int renorm = 0;
	if(toadd->real != 0.0f) {
		if(dest->real == 0.0f) {
			dest->real = toadd->real;
		} else {
			if((dest->real > 0 && toadd->real < 0) || (dest->real < 0 && toadd->real > 0)) {
				renorm = 1;
			}
			dest->real = sqrt(dest->real * dest->real + toadd->real * toadd->real);
		}
		toadd->real = 0.0f;
	}

	if(toadd->imag != 0.0f) {
		if(dest->imag == 0.0f) {
			dest->imag = toadd->imag;
		} else {
			if((dest->imag > 0 && toadd->imag < 0) || (dest->imag < 0 && toadd->imag > 0)) {
				renorm = 1;
			}
			dest->imag = sqrt(dest->imag * dest->imag + toadd->imag * toadd->imag);
		}
		toadd->imag = 0.0f;
	}

	return renorm;
}

float quda_rand_float() {
	return rand()/(float)RAND_MAX;
}

int qstate_compare(const void* qstate1, const void* qstate2) {
	uint64_t s1 = ((quantum_state_t*)qstate1)->state;
	uint64_t s2 = ((quantum_state_t*)qstate2)->state;
	if(s1 == s2) {
		return 0;
	}
	return (s1 < s2) ? -1 : 1;
}
