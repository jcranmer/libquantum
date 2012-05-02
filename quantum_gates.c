/* quantum_gates.c: source file for quantum gate functions
*/

#include <math.h>
#include "quantum_gates.h"
#include "complex.h"
#include <stdio.h> // DEBUG

// One-bit quantum gates
int quda_quantum_hadamard_gate(int target, quantum_reg* qreg) {
	// If needed, enlarge qreg to make room for state splits resulting from this gate
	int diff = 2*qreg->num_states - qreg->size;
	if(diff > 0) {
		if(quda_quantum_reg_enlarge(qreg,&diff) == -1) return -1;
	}

	uint64_t mask = 1 << target;
	int i;
	for(i=0;i<qreg->num_states;i++) {
		// Flipped state must be created
		qreg->states[qreg->num_states+i].state = qreg->states[i].state ^ mask;
		// For this state, must just modify amplitude
		qreg->states[i].amplitude = quda_complex_rmul(qreg->states[i].amplitude,
				ONE_OVER_SQRT_2);
		// Copy amplitude to created state
		qreg->states[qreg->num_states+i].amplitude = qreg->states[i].amplitude;

		if(qreg->states[i].state & mask) {
			qreg->states[i].amplitude = quda_complex_neg(qreg->states[i].amplitude);
		}
		// DEBUG BLOCK
		if(quda_complex_abs_square(qreg->states[i].amplitude) > 1.0f) {
			printf("PROBLEM (hadamard): state[%d].amplitude = (%f,%f)\n",i,
					qreg->states[i].amplitude.real,qreg->states[i].amplitude.imag);
		}
		// END DEBUG BLOCK
	}

	printf("HADAMARD: increasing # states from %d ",qreg->num_states); // DEBUG
	qreg->num_states = 2*qreg->num_states;
	printf("to %d\n",qreg->num_states); // DEBUG

	/*// DEBUG BLOCK
	printf("BEFORE reg_coalesce()\n");
	printf("new num_states: %d\n",qreg->num_states);
	printf("state[0].amp: %f,%f\n",qreg->states[0].amplitude.real,qreg->states[0].amplitude.imag);
	int err = 0;
	float p = 0.0f;
	for(i=0;i<qreg->num_states;i++) {
		float temp = quda_complex_abs_square(qreg->states[i].amplitude);
		p += temp;
		if(temp > 1.0) {
			printf("Amplitude error. state[%d]: %lu --> (%f,%f)\n",i,qreg->states[i].state,
					qreg->states[i].amplitude.real, qreg->states[i].amplitude.imag);
			err = 1;
		}
	}
	printf("sum of all probabilties: %f\n",p);
	*/// END DEBUG BLOCK

	// TODO: Ideally, make this call optional or conditional
	quda_quantum_reg_coalesce(qreg);

	/*// DEBUG BLOCK
	printf("AFTER reg_coalesce()\n");
	printf("num_states reduced to: %d\n",qreg->num_states);
	err = 0;
	p = 0.0f;
	for(i=0;i<qreg->num_states;i++) {
		float temp = quda_complex_abs_square(qreg->states[i].amplitude);
		p += temp;
		if(temp > 1.0) {
			printf("Amplitude error. state[%d]: %lu --> (%f,%f)\n",i,qreg->states[i].state,
					qreg->states[i].amplitude.real, qreg->states[i].amplitude.imag);
			err = 1;
		}
	}
	printf("sum of all probabilties: %f\n",p);
	if(err == 0) printf("No problem within Hadamard gate.\n");
	*/// END DEBUG BLOCK

	return 0;
}

void quda_quantum_pauli_x_gate(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		qreg->states[i].state = qreg->states[i].state ^ mask;
	}
}

void quda_quantum_pauli_y_gate(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		qreg->states[i].state = qreg->states[i].state ^ mask;
		qreg->states[i].amplitude = quda_complex_mul_i(qreg->states[i].amplitude);

		// TODO: Look at overhead of conditional mul_ni vs negation
		if(qreg->states[i].state & mask) {
			qreg->states[i].amplitude = quda_complex_neg(qreg->states[i].amplitude);
		}
	}
}

void quda_quantum_pauli_z_gate(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		if(qreg->states[i].state & mask) {
			qreg->states[i].amplitude = quda_complex_neg(qreg->states[i].amplitude);
		}
	}
}

void quda_quantum_phase_gate(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		if(qreg->states[i].state & mask) {
			qreg->states[i].amplitude = quda_complex_mul_i(qreg->states[i].amplitude);
		}
	}
}

void quda_quantum_pi_over_8_gate(int target, quantum_reg* qreg) {
	complex_t c = { .real = ONE_OVER_SQRT_2, .imag = ONE_OVER_SQRT_2 };
	int i;
	uint64_t mask = 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		if(qreg->states[i].state & mask) {
			qreg->states[i].amplitude = quda_complex_mul(qreg->states[i].amplitude,c);
		}
	}
}

void quda_quantum_rotate_k_gate(int target, quantum_reg* qreg, int k) {
	float temp = QUDA_PI / (1 << (k-1));
	complex_t c = { .real = cos(temp), .imag = sin(temp) };
	uint64_t mask = 1 << target;
	int i;
	for(i=0;i<qreg->num_states;i++) {
		if(qreg->states[i].state & mask) {
			qreg->states[i].amplitude = quda_complex_mul(qreg->states[i].amplitude,c);
		}
	}
}

// Two-bit quantum gates
void quda_quantum_swap_gate(int target1, int target2, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target1;
	mask |= 1 << target2;
	for(i=0;i<qreg->num_states;i++) {
		if((qreg->states[i].state & mask) != 0 && (~qreg->states[i].state & mask) != 0) {
			qreg->states[i].state = qreg->states[i].state ^ mask;
		}
	}
}

void quda_quantum_controlled_not_gate(int control, int target, quantum_reg* qreg) {
	int i;
	uint64_t cmask = 1 << control;
	uint64_t tmask = 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		if(qreg->states[i].state & cmask) {
			qreg->states[i].state = qreg->states[i].state ^ tmask;
			/*// DEBUG BLOCK
			if(quda_complex_abs_square(qreg->states[i].amplitude) > 1.0f) {
				printf("PROBLEM (c-not): state[%d].amplitude = (%f,%f)\n",i,
						qreg->states[i].amplitude.real,qreg->states[i].amplitude.imag);
			}
			*/// END DEBUG BLOCK
		}
	}
}

void quda_quantum_controlled_y_gate(int control,int target, quantum_reg* qreg) {
	int i;
	uint64_t cmask = 1 << control;
	uint64_t tmask = 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		// TODO: Look for ways to avoid nested conditionals
		if(qreg->states[i].state & cmask) {
			qreg->states[i].state = qreg->states[i].state ^ tmask;
			qreg->states[i].amplitude = quda_complex_mul_i(qreg->states[i].amplitude);

			// TODO: Look at overhead of conditional mul_ni vs negation
			if(qreg->states[i].state & tmask) {
				qreg->states[i].amplitude = quda_complex_neg(qreg->states[i].amplitude);
			}
		}
	}
}

void quda_quantum_controlled_z_gate(int control, int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << control;
	mask |= 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		if((qreg->states[i].state & mask) == mask) {
			qreg->states[i].amplitude = quda_complex_neg(qreg->states[i].amplitude);
		}
	}
}

void quda_quantum_controlled_phase_gate(int control, int target, quantum_reg* qreg) {	
	uint64_t mask = 1 << control;
	mask |= 1 << target;
	int i;
	for(i=0;i<qreg->num_states;i++) {
		if((qreg->states[i].state & mask) == mask) {
			//complex_t oldval = qreg->states[i].amplitude; // DEBUG
			qreg->states[i].amplitude = quda_complex_mul_i(qreg->states[i].amplitude);
			/*// DEBUG BLOCK
			if(quda_complex_abs_square(qreg->states[i].amplitude) > 1.0f) {
				printf("PROBLEM (c-phase): state[%d].amplitude = (%f,%f)\n",i,
						qreg->states[i].amplitude.real,qreg->states[i].amplitude.imag);
				printf("Old Value: (%f,%f)\n",oldval.real,oldval.imag);
			}
			*/// END DEBUG BLOCK
		}
	}
}

void quda_quantum_controlled_rotate_k_gate(int control, int target, quantum_reg* qreg, int k) {
	if(control-target != k-1) printf("C-r-k Violation!\n"); // DEBUG
	float temp = QUDA_PI / (1 << (k-1));
	complex_t c = { .real = cos(temp), .imag = sin(temp) };
	uint64_t mask = 1 << control;
	mask |= 1 << target;
	int i;
	for(i=0;i<qreg->num_states;i++) {
		if((qreg->states[i].state & mask) == mask) {
			qreg->states[i].amplitude = quda_complex_mul(qreg->states[i].amplitude,c);
		}
	}
}

// Three-bit quantum gates
void quda_quantum_toffoli_gate(int control1, int control2, int target, quantum_reg* qreg) {
	int i;
	uint64_t cmask = 1 << control1;
	cmask |= 1 << control2;
	uint64_t tmask = 1 << target;
	for(i=0;i<qreg->num_states;i++) {
		if((qreg->states[i].state & cmask) == cmask) {
			qreg->states[i].state = qreg->states[i].state ^ tmask;
		}
	}
}

void quda_quantum_fredkin_gate(int control, int target1, int target2, quantum_reg* qreg) {
	int i;
	uint64_t cmask = 1 << control;
	uint64_t tmask = 1 << target1;
	tmask |= 1 << target2;
	for(i=0;i<qreg->num_states;i++) {
		if((qreg->states[i].state & cmask) == cmask
        && (qreg->states[i].state & tmask) != 0
				&& (~qreg->states[i].state & tmask) != 0) {
			qreg->states[i].state = qreg->states[i].state ^ tmask;
		}
	}
}
