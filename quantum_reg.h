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
	int num_states;
	quantum_state_t* states;
} quantum_reg;

/* Initializes a quantum register with the specified number of qubits.
 * Returns 0 on success or -1 if allocation fails.
 */
int quda_quantum_reg_init(quantum_reg* qreg, int qubits);

/* Sets the register to a single physical state with probability 1. */
void quda_quantum_reg_set(quantum_reg* qreg, uint64_t state);

/* Frees the given register for deletion. 
 * If the passed qreg was dynamically allocated, it must still be freed separately.
 */
void quda_quantum_reg_delete(quantum_reg* qreg);

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

int qstate_compare(const void* qstate1, const void* qstate2);

#endif // __QUDA_QUANTUM_REG_H
