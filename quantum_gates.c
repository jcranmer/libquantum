/* quantum_gates.c: source file for quantum gate functions
*/

#include <math.h>
#include "quantum_gates.h"
#include "complex.h"
//#include <stdio.h> // DEBUG

#ifndef QUDA_GATE
#define QUDA_GATE
#endif

#ifndef FOR_EACH_STATE
#define FOR_EACH_STATE(qreg, i) for (i = 0; i < qreg->num_states; i++)
#define STATE(qreg, i) qreg->states[i].state
#define AMPLITUDE(qreg, i) qreg->states[i].amplitude
#endif

// One-bit quantum gates
#ifndef CUSTOM_HADAMARD
QUDA_GATE int quda_quantum_hadamard_gate(int target, quantum_reg* qreg) {
	// If needed, enlarge qreg to make room for state splits resulting from this gate
  int states = qreg->num_states;
	int diff = 2*states - qreg->size;
	if(diff > 0) {
		if(quda_quantum_reg_enlarge(qreg,diff) == -1) return -1;
	}

	uint64_t mask = 1 << target;
	int i;
	FOR_EACH_STATE(qreg, i) {
		// Flipped state must be created
		STATE(qreg, qreg->num_states+i) = STATE(qreg, i) ^ mask;
		// For this state, must just modify amplitude
		AMPLITUDE(qreg, i) = quda_complex_rmul(AMPLITUDE(qreg, i),
				ONE_OVER_SQRT_2);
		// Copy amplitude to created state
		AMPLITUDE(qreg, qreg->num_states+i) = AMPLITUDE(qreg, i);

		if(STATE(qreg, i) & mask) {
			AMPLITUDE(qreg, i) = quda_complex_neg(AMPLITUDE(qreg, i));
		}
	}

	qreg->num_states = 2*states;

	// TODO: Ideally, make this call optional or conditional
	quda_quantum_reg_coalesce(qreg);

	return 0;
}
#endif

QUDA_GATE void quda_quantum_pauli_x_gate(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target;
	FOR_EACH_STATE(qreg, i) {
		STATE(qreg, i) = STATE(qreg, i) ^ mask;
	}
}

QUDA_GATE void quda_quantum_pauli_y_gate(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target;
	FOR_EACH_STATE(qreg, i) {
		STATE(qreg, i) = STATE(qreg, i) ^ mask;
		AMPLITUDE(qreg, i) = quda_complex_mul_i(AMPLITUDE(qreg, i));

		// TODO: Look at overhead of conditional mul_ni vs negation
		if(STATE(qreg, i) & mask) {
			AMPLITUDE(qreg, i) = quda_complex_neg(AMPLITUDE(qreg, i));
		}
	}
}

QUDA_GATE void quda_quantum_pauli_z_gate(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target;
	FOR_EACH_STATE(qreg, i) {
		if(STATE(qreg, i) & mask) {
			AMPLITUDE(qreg, i) = quda_complex_neg(AMPLITUDE(qreg, i));
		}
	}
}

QUDA_GATE void quda_quantum_phase_gate(int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target;
	FOR_EACH_STATE(qreg, i) {
		if(STATE(qreg, i) & mask) {
			AMPLITUDE(qreg, i) = quda_complex_mul_i(AMPLITUDE(qreg, i));
		}
	}
}

QUDA_GATE void quda_quantum_pi_over_8_gate(int target, quantum_reg* qreg) {
	complex_t c = { .real = ONE_OVER_SQRT_2, .imag = ONE_OVER_SQRT_2 };
	int i;
	uint64_t mask = 1 << target;
	FOR_EACH_STATE(qreg, i) {
		if(STATE(qreg, i) & mask) {
			AMPLITUDE(qreg, i) = quda_complex_mul(AMPLITUDE(qreg, i),c);
		}
	}
}

QUDA_GATE void quda_quantum_rotate_k_gate(int target, quantum_reg* qreg, int k) {
	float temp = QUDA_PI / (1 << (k-1));
	complex_t c = { .real = cos(temp), .imag = sin(temp) };
	uint64_t mask = 1 << target;
	int i;
	FOR_EACH_STATE(qreg, i) {
		if(STATE(qreg, i) & mask) {
			AMPLITUDE(qreg, i) = quda_complex_mul(AMPLITUDE(qreg, i),c);
		}
	}
}

// Two-bit quantum gates
QUDA_GATE void quda_quantum_swap_gate(int target1, int target2, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << target1;
	mask |= 1 << target2;
	FOR_EACH_STATE(qreg, i) {
		if((STATE(qreg, i) & mask) != 0 && (~STATE(qreg, i) & mask) != 0) {
			STATE(qreg, i) = STATE(qreg, i) ^ mask;
		}
	}
}

QUDA_GATE void quda_quantum_controlled_not_gate(int control, int target, quantum_reg* qreg) {
	int i;
	uint64_t cmask = 1 << control;
	uint64_t tmask = 1 << target;
	FOR_EACH_STATE(qreg, i) {
		if(STATE(qreg, i) & cmask) {
			STATE(qreg, i) = STATE(qreg, i) ^ tmask;
		}
	}
}

QUDA_GATE void quda_quantum_controlled_y_gate(int control,int target, quantum_reg* qreg) {
	int i;
	uint64_t cmask = 1 << control;
	uint64_t tmask = 1 << target;
	FOR_EACH_STATE(qreg, i) {
		// TODO: Look for ways to avoid nested conditionals
		if(STATE(qreg, i) & cmask) {
			STATE(qreg, i) = STATE(qreg, i) ^ tmask;
			AMPLITUDE(qreg, i) = quda_complex_mul_i(AMPLITUDE(qreg, i));

			// TODO: Look at overhead of conditional mul_ni vs negation
			if(STATE(qreg, i) & tmask) {
				AMPLITUDE(qreg, i) = quda_complex_neg(AMPLITUDE(qreg, i));
			}
		}
	}
}

QUDA_GATE void quda_quantum_controlled_z_gate(int control, int target, quantum_reg* qreg) {
	int i;
	uint64_t mask = 1 << control;
	mask |= 1 << target;
	FOR_EACH_STATE(qreg, i) {
		if((STATE(qreg, i) & mask) == mask) {
			AMPLITUDE(qreg, i) = quda_complex_neg(AMPLITUDE(qreg, i));
		}
	}
}

QUDA_GATE void quda_quantum_controlled_phase_gate(int control, int target, quantum_reg* qreg) {	
	uint64_t mask = 1 << control;
	mask |= 1 << target;
	int i;
	FOR_EACH_STATE(qreg, i) {
		if((STATE(qreg, i) & mask) == mask) {
			AMPLITUDE(qreg, i) = quda_complex_mul_i(AMPLITUDE(qreg, i));
		}
	}
}

QUDA_GATE void quda_quantum_controlled_rotate_k_gate(int control, int target, quantum_reg* qreg, int k) {
	float temp = QUDA_PI / (1 << (k-1));
	complex_t c = { .real = cos(temp), .imag = sin(temp) };
	uint64_t mask = 1 << control;
	mask |= 1 << target;
	int i;
	FOR_EACH_STATE(qreg, i) {
		if((STATE(qreg, i) & mask) == mask) {
			AMPLITUDE(qreg, i) = quda_complex_mul(AMPLITUDE(qreg, i),c);
		}
	}
}

// Three-bit quantum gates
QUDA_GATE void quda_quantum_toffoli_gate(int control1, int control2, int target, quantum_reg* qreg) {
	int i;
	uint64_t cmask = 1 << control1;
	cmask |= 1 << control2;
	uint64_t tmask = 1 << target;
	FOR_EACH_STATE(qreg, i) {
		if((STATE(qreg, i) & cmask) == cmask) {
			STATE(qreg, i) = STATE(qreg, i) ^ tmask;
		}
	}
}

QUDA_GATE void quda_quantum_fredkin_gate(int control, int target1, int target2, quantum_reg* qreg) {
	int i;
	uint64_t cmask = 1 << control;
	uint64_t tmask = 1 << target1;
	tmask |= 1 << target2;
	FOR_EACH_STATE(qreg, i) {
		if((STATE(qreg, i) & cmask) == cmask
        && (STATE(qreg, i) & tmask) != 0
				&& (~STATE(qreg, i) & tmask) != 0) {
			STATE(qreg, i) = STATE(qreg, i) ^ tmask;
		}
	}
}
