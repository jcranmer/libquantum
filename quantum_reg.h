/* quantum_reg.h: header for quantum register structure
*/

#ifndef __QUDA_QUANTUM_REG_H
#define __QUDA_QUANTUM_REG_H

#include <stdlib.h>
#include <stdint.h>
#include "complex.h"

#define DEFAULT_QTS_RATIO 1.0 // default qubits-to-states ratio

typedef struct quantum_state_t {
	uint64_t state;
	complex_t amplitude;
} quantum_state_t;

// TODO: This is a fairly naive implementation using an arraylist. Some redesign is necessary.
// TODO: Never allow size to exceed 2^n and enforce low->high ordering of states in this case.
typedef struct quantum_reg {
	int qubits;
	int size;
	int scratch;
	int num_states;
	quantum_state_t* states;
} quantum_reg;

/* Initializes a quantum register with the specified number of qubits.
 * Returns 0 on success or -1 if allocation fails.
 */
int quda_quantum_reg_init(quantum_reg* qreg, int qubits);

/* Frees the given register for deletion. 
 * If the passed qreg was dynamically allocated, it must still be freed separately.
 */
void quda_quantum_reg_delete(quantum_reg* qreg);

/* Sets the register to a single physical state with probability 1. */
void quda_quantum_reg_set(quantum_reg* qreg, uint64_t state);

/* Sets a single bit of a quantum register to 1 with probability 1.
 * If the system is in a superposition, it is collapsed into the subset
 * of possible states allowed by this value.
 */
void quda_quantum_bit_set(int target, quantum_reg* qreg);

/* Sets a single bit of a quantum register to 0 with probability 1.
 * If the system is in a superposition, it is collapsed into the subset
 * of possible states allowed by this value.
 */
void quda_quantum_bit_reset(int target, quantum_reg* qreg);

/* Adds n scratch bits (always initialized to zero) to the quantum register.
 * They may be used in gates like any other bit, however they are always indexed AFTER the
 * register's actual bits. In a 16-qubit register, the first scratch bit (bit 0) will have
 * index 16 (referenced from the LSB). Consider using quda_quantum_scratch_bit() to help.
 */
void quda_quantum_add_scratch(int n, quantum_reg* qreg);

/* Clears all scratch bits from the quantum register and collapses/coalesces any states
 * dependent on them.
 */
void quda_quantum_clear_scratch(quantum_reg* qreg);

/* Collapses any superpositions of scratch bits into their most likely configuration. */
void quda_quantum_collapse_scratch(quantum_reg* qreg);

/* Returns the index of the desired scratch bit (for use in gates etc).
 * Simple helper function only included to allow major changes to register design.
 */
int quda_quantum_scratch_bit(int index, quantum_reg* qreg);

/* Performs a measurement on the quantum register and stores the state
 * in 'retval' if non-NULL.
 * Masks any scratch-space off from 'retval' UNLESS 'scratch' is set (non-zero).
 * Returns 0 on success, -2 on retval NULL, -1 on normalization error.
 */
// TODO: Attempt correction for minor normalization errors (ie floating point precision errors)
int quda_quantum_reg_measure(quantum_reg* qreg, uint64_t* retval, int scratch);

/* Performs a real-world quantum measurement and stores the state in 
 * 'retval' if non-NULL.
 * If there is scratch space, calls quda_quantum_clear_scratch() so that 'retval' only returns
 * the register's value (proper state).
 * On success, returns 0 and the register collapses to the physical state
 * measured with probability 1.
 * On failure, does not collapse. Returns -2 on retval NULL, -1 on
 * normalization error.
 */
// TODO: Attempt correction for minor normalization errors (ie floating point precision errors)
int quda_quantum_reg_measure_and_collapse(quantum_reg* qreg, uint64_t* retval);

/* Performs a real-world quantum measurement of a range [start,end) of bits and stores the state
 * in 'retval' if non-NULL. Scratch space is treated as regular bits within the given range.
 */
int quda_quantum_range_measure_and_collapse(int start, int end, quantum_reg* reg,uint64_t* retval);

/* Measure 1 bit of a quantum register */
int quda_quantum_bit_measure(int target, quantum_reg* qreg);

/* Perform a real-world quantum measurement of 1 quantum register bit.
 * The system collapses into the subset of possible states allowed by
 * the value of the measured bit.
 * Simultaneously prunes zero-amplitude states.
 * Does not coalesce identical states.
 */
int quda_quantum_bit_measure_and_collapse(int target, quantum_reg* qreg);

/* Removes zero-amplitude states from the register. */
void quda_quantum_reg_prune(quantum_reg* qreg);

/* Attempts to lengthen the quantum register's arraylists by the value at 'amount'.
 * If 'amount' is NULL, attempts to double size.
 * Returns 0 on success or -1 if allocation fails.
 * Simultaneously prunes zero-amplitude states from the arraylist on copy.
 */
int quda_quantum_reg_enlarge(quantum_reg* qreg,int* amount);

/* Attempts to merge any identical states present in the register.
 * Simultaneously prunes zero-amplitude states from the register.
 */
void quda_quantum_reg_coalesce(quantum_reg* qreg);

/* Resizes the register to free up any unused memory but preserves all current states.
 * Returns 0 on success, -1 on error (in which case no memory is freed).
 * First attempts to coalesce, which will also prune.
 * Should be used sparingly to avoid higher computational and allocation overheads.
 */
int quda_quantum_reg_trim(quantum_reg* qreg);

/* Renormalizes state amplitudes such that all probabilities sum to 1.0.
 */
void quda_quantum_reg_renormalize(quantum_reg* qreg);

/* Coaleses two amplitudes into the first, preserving the proper probability of both states.
 * Writes QUDA_COMPLEX_ZERO into the 'toadd' amplitude and the resulting amplitude into 'dest'.
 * Returns 1 if other amplitudes will need to be renormalized due to this operation, 0 otherwise.
 */
int quda_amplitude_coalesce(complex_t* dest, complex_t* toadd);

/* Generates a float in the range [0,1) */
// TODO: Look at performance implications of using 'double' here
float quda_rand_float();

/* Comparator for sorting the quantum_state array */
int qstate_compare(const void* qstate1, const void* qstate2);

#endif // __QUDA_QUANTUM_REG_H
