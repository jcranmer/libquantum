/* shor.c: header for implementation of Shor's quantum algorithm for factorization
 */

#include "quantum_reg.h"

// Testing functions

/* Prints each qreg state and its corresponding modular exponentiation (stored in scratch).
 * Only prints 'num_outputs' entries (or all if NULL).
 */
void dump_mod_exp_results(quantum_reg* qreg, int* num_outputs);

// Classical functions (perform purely non-quantum computation)

/* Determines number of qubits required to store the given number */
int qubits_required(int num);
