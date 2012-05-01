/* quantum_stdlib.h: header for common useful operations for quantum registers
*/

#include "quantum_reg.h"

// Testing functions
int quda_check_normalization(quantum_reg* qreg);

int quda_weak_check_amplitudes(quantum_reg* qreg);

void quda_quantum_reg_dump(quantum_reg* qreg, char* tag);

// Utility functions

/* Applies the Hadamard gate to a range of bits [start,end) in a quantum register.
 * Returns -1 if any of the gate applications fail, 0 otherwise.
 */
int quda_quantum_hadamard_range(int start,int end,quantum_reg* qreg);

/* Applies the Hadamard gate to all bits after the given 'start' bit in a quantum register.
 * Does not affect scratch bits (hadamard_range() may be used directly for this purpose).
 * Returns -1 if any of the gate applications fail, 0 otherwise.
 */
int quda_quantum_hadamard_after(int start,quantum_reg* qreg);

/* Applies the Hadamard gate to all bits in a given quantum register.
 * Does not affect scratch bits (hadamard_range() may be used directly for this purpose).
 * Returns -1 if any of the gate applications fail, 0 otherwise.
 */
int quda_quantum_hadamard_all(quantum_reg* qreg);

/* Applies a Quantum Fourier Transform to the non-scratch qubits of a given register. */
void quda_quantum_fourier_transform(quantum_reg* qreg);

// Classical functions

/* Performs exponentiation mod n but does not explicitly use quantum gates.
 * Instead uses classical multiplication and data copying (copying impossible in quantum).
 * Requires n, the number to mod by, and x, a number relatively prime to n.
 * This helps to reduce a known (usu. Toffoli-gate) bottleneck since modular exponentiation
 * is the bottleneck of Shor's algorithm.
 */
void quda_classical_exp_mod_n(int x, int n, quantum_reg* qr);

/* Performs the continued fraction expansion to approximate the given result (*num)
 * with respect to the original denominator (*denom = 1 << reg_width, usually).
 * Outputs results in 'num' and 'denom'.
 * NOTE: This function does not collapse the quantum register. It does, however, modify the 'num'
 * value passed into it, so a copy of num should be created before calling this function if
 * 'num' needs to be saved.
 */
void quda_classical_continued_fraction_expansion(uint64_t* num, uint64_t* denom);

// Helper functions for other stdlib functions (generally simple subroutines)

/* Calculates gcd of x and y via the division-based Euclidean algorithm */
int quda_gcd_div(int x, int y);

/* Calculates gcd of x and y via the subtraction-based Euclidean algorithm */
int quda_gcd_sub(int x, int y);

/* Calculates (b^e) % n in a memory-efficient manner */
uint64_t quda_mod_pow_simple(int b, uint64_t e, int n);

/* Calculates (b^e) % n in a faster and more memory-efficient manner but introduces
 * an additional conditional per iteration.
 */
uint64_t quda_mod_pow_bin(int b, uint64_t e, int n);
