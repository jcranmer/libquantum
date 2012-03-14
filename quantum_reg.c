/* quantum_reg.c: quantum register source
*/

#include "quantum_reg.h"

int main() { // testing only
	quantum_reg qreg;
	int res;
	res = quda_quantum_reg_init(&qreg,16);
	if(res == -1) exit(res);

	quda_quantum_reg_set(&qreg,42);
	quda_quantum_reg_enlarge(&qreg,NULL);
	quda_quantum_reg_trim(&qreg);
	quda_quantum_reg_delete(&qreg);

	return 0;
}

int quda_quantum_reg_init(quantum_reg* qreg, int qubits) {
	qreg->qubits = qubits;
	qreg->size = (int)(DEFAULT_QTS_RATIO*qubits);
	qreg->num_states = 0;
	qreg->states = (uint64_t*)malloc(qreg->size*sizeof(uint64_t));
	if(qreg->states == NULL) {
		return -1;
	}

	qreg->amplitudes = (complex_t*)malloc(qreg->size*sizeof(complex_t));
	if(qreg->amplitudes == NULL) {
		free(qreg->states);
		return -1;
	}

	return 0;
}

void quda_quantum_reg_set(quantum_reg* qreg, uint64_t state) {
	qreg->num_states = 1;
	qreg->states[0] = state;
	qreg->amplitudes[0] = QUDA_COMPLEX_ONE;
}

void quda_quantum_reg_delete(quantum_reg* qreg) {
	free(qreg->states);
	free(qreg->amplitudes);
}

int quda_quantum_reg_enlarge(quantum_reg* qreg,int* amount) {
	int increase;
	if(amount == NULL) {
		increase = qreg->size;
	} else {
		increase = *amount;
	}

	uint64_t* temp_states = malloc((qreg->size+increase)*sizeof(uint64_t));
	if(temp_states == NULL) {
		return -1;
	}

	complex_t* temp_amplitudes = malloc((qreg->size+increase)*sizeof(complex_t));
	if(temp_amplitudes == NULL) {
		free(temp_states);
		return -1;
	}

	int i,j;
	for(i=0,j=0;i<qreg->num_states;i++) {
		if(!quda_complex_eq(qreg->amplitudes[i],QUDA_COMPLEX_ZERO)) {
			temp_amplitudes[j] = qreg->amplitudes[i];
			temp_states[j++] = qreg->states[i];
		}
	}

	free(qreg->states);
	qreg->states = temp_states;
	free(qreg->amplitudes);
	qreg->amplitudes = temp_amplitudes;

	return 0;
}

void quda_quantum_reg_prune(quantum_reg* qreg) {
	int i,end;
	for(i=0,end=qreg->num_states-1;i < end;i++) {
		if(quda_complex_eq(qreg->amplitudes[i],QUDA_COMPLEX_ZERO)) {
			while(quda_complex_eq(qreg->amplitudes[end],QUDA_COMPLEX_ZERO)) {
				end--;
				if(i == end) break;
			}

			if(i < end) {
				qreg->amplitudes[i] = qreg->amplitudes[end];
				qreg->states[i] = qreg->states[end];
			} else {
				break;
			}
		}
	}
	qreg->num_states = end+1;
}

int quda_quantum_reg_trim(quantum_reg* qreg) {
	int old_states = qreg->num_states;
	quda_quantum_reg_prune(qreg);
	if(qreg->num_states < old_states) {
		uint64_t* temp_states = malloc(qreg->num_states*sizeof(uint64_t));
		if(temp_states == NULL) {
			return -1;
		}

		complex_t* temp_amplitudes = malloc(qreg->num_states*sizeof(complex_t));
		if(temp_amplitudes == NULL) {
			free(temp_states);
			return -1;
		}

		int i;
		for(i=0;i < qreg->num_states;i++) {
			temp_states[i] = qreg->states[i];
			temp_amplitudes[i] = qreg->amplitudes[i];
		}

		free(qreg->states);
		qreg->states = temp_states;
		free(qreg->amplitudes);
		qreg->amplitudes = temp_amplitudes;
	}

	return 0;
}
