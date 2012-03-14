/* quantum_gates.h: header file for quantum gate functions
*/

#ifndef __QUDA_QUANTUM_GATES_H
#define __QUDA_QUANTUM_GATES_H

#include "quantum_reg.h"

#define ONE_OVER_SQRT_2 0.707106781f

// One-bit quantum gates

/* Applies the quantum Hadamard gate to the target bit of a quantum register.
 * Puts the target bit into a superposition of two states.
 * |0> -> k(|0> + |1>); |1> -> k(|0>-|1>) where k=1/sqrt(2)
 * Returns -1 on failure if a register expansion was necessary but failed.
 * Returns 0 on success;
 */
int quda_quantum_hadamard_gate(int target, quantum_reg* qreg);

/* Applies the quantum Pauli X gate to the target bit of a quantum register.
 * Also known as the Sigma X gate or the Quantum Not gate.
 */
void quda_quantum_pauli_x_gate(int target, quantum_reg* qreg);

/* Applies the quantum Pauli Y gate to the target bit of a quantum register.
 * Also known as the Sigma Y gate.
 */
void quda_quantum_pauli_y_gate(int target, quantum_reg* qreg);

/* Applies the quantum Pauli Z gate to the target bit of a quantum register.
 * Also known as the Sigma Z gate.
 */
void quda_quantum_pauli_z_gate(int target, quantum_reg* qreg);

/* Applies the quantum Phase gate to the target bit of a quantum register.
 * This is the square root of the Pauli/Sigma Z gate.
 */
void quda_quantum_phase_gate(int target, quantum_reg* qreg);

/* Applies the quantum PI/8 gate to the target bit of a quantum register.
 * This is the square root of the Phase gate.
 */
void quda_quantum_pi_over_8_gate(int target, quantum_reg* qreg);

// Two-bit quantum gates

/* Applies the quantum Swap gate to exchange the states of the two target bits of a 
 * quantum register.
 */
void quda_quantum_swap_gate(int target1, int target2, quantum_reg* qreg);

/* Applies the quantum Controlled-Not gate to the target bit of a quantum register
 * if the control bit is set.
 * Also known as the Controlled-X gate.
 */
void quda_quantum_controlled_not_gate(int control, int target, quantum_reg* qreg);

/* Applies the Controlled-Y gate to the target bit of a quantum register if the
 * control bit is set.
 */
void quda_quantum_controlled_y_gate(int control,int target, quantum_reg* qreg);

/* Applies the Controlled-Z gate to the target bit of a quantum register if the
 * control bit is set.
 */
void quda_quantum_controlled_z_gate(int control, int target, quantum_reg* qreg);

// Three-bit quantum gates

/* Applies the quantum Toffoli gate (Controlled-Controlled-Not) to the
 * target bit of a quantum register if both control1 and control2 are set.
 */
void quda_quantum_toffoli_gate(int control1, int control2, int target, quantum_reg* qreg);

/* Applies the quantum Fredkin gate (Controlled-Swap) to exchange the two
 * target bits of a quantum register.
 */
void quda_quantum_fredkin_gate(int control, int target1, int target2, quantum_reg* qreg);

#endif // __QUDA_QUANTUM_GATES_H
