/* quantum_reg.c: quantum register source
*/

#include "quantum_reg.h"
#include <stdlib.h>

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

int qstate_compare(const void* qstate1, const void* qstate2) {
	uint64_t diff = ((quantum_state_t*)qstate1)->state - ((quantum_state_t*)qstate2)->state;
	if(diff == 0) return 0;
	return diff > 0 ? 1 : -1;
}
