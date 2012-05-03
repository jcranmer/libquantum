/* test.c: simple testing program for base libquantum functionality
*/

#include <math.h>
#include <stdio.h>
#include "complex.h"
#include "quantum_reg.h"
#include "quantum_gates.h"

#define CHECK_COMPLEX_RESULT(val, compreal, compimag, explain) \
  do { \
  if (fabs((val).real - compreal) > 1e-3 || fabs((val).imag - compimag) > 1e-3) \
    printf("FAIL TEST " explain ": got " \
      "%.3f + %.3fi, expected %.3f + %.3fi\n", \
      (double)(val).real, (double)(val).imag, (double)compreal, (double)compimag); \
  else \
    printf("PASS TEST " explain "\n"); \
  } while(0)

#define VERIFY_REGISTER(qureg, nbits, check, func) \
  do { \
    unsigned verified = (1U << (1U << nbits)) - 1; \
    float total_probability = 0; \
    for (int s = 0; s < qureg.num_states; s++) { \
      complex_t *amplitude = &qureg.states[s].amplitude; \
      int state = qureg.states[s].state; \
      if ((verified & (1U << state)) == 0) { \
        printf("FAIL: state %d seen multiple times\n", state); \
      } \
      verified &= ~(1U << state); \
      complex_t *entry = &check[state]; \
      printf("Checking projection onto state |%d>\n", state); \
      CHECK_COMPLEX_RESULT(*amplitude, entry->real, entry->imag, \
        "Verifying gate " #func); \
      printf("Value: %.3f + %.3fi\n", amplitude->real, amplitude->imag); \
      total_probability += quda_complex_abs_square(*amplitude); \
    } \
    if (fabs(total_probability - 1) > 1e-3) { \
      printf("FAIL TEST verifying gate " #func ": Saw only %d (unseen: %x)\n", \
        qureg.num_states, verified); \
      printf("FAIL TEST verifying gate " #func ": total probability: %.3f\n", \
        (double)total_probability); \
    } \
  } while (0)

#define INVOKE1 0
#define INVOKE2 1, 0
#define INVOKE3 2, 1, 0
#define TEST_GATE(nbits, func, ...) \
do { \
  static complex_t matrix[1 << nbits][1 << nbits] = __VA_ARGS__; \
  quantum_reg qureg; \
  quda_quantum_reg_init(&qureg, 1); \
  /* How does it map basis elements? */ \
  for (int i = 0; i < (1 << nbits); i++) { \
    quda_quantum_reg_set(&qureg, i); \
    func(INVOKE##nbits, &qureg); \
    printf("Testing " #func " with basis |%d>\n", i); \
    VERIFY_REGISTER(qureg, nbits, matrix[i], func); \
  } \
  /* How does it map the uniform state? */ \
  complex_t uniform[1 << nbits]; \
  int newsize = (1 << nbits) - qureg.size; \
  quda_quantum_reg_enlarge(&qureg, newsize); \
  qureg.num_states = 1 << nbits; \
  float sum = 0.0f; \
  for (int i = 0; i < (1 << nbits); i++) { \
    float div = sqrt(1 << nbits); \
    uniform[i] = QUDA_COMPLEX_ZERO; \
    for (int j = 0; j < (1 << nbits); j++) { \
      uniform[i] = quda_complex_add(uniform[i], matrix[j][i]); \
    } \
    sum += quda_complex_abs_square(uniform[i]); \
    qureg.states[i].state = i; \
    qureg.states[i].amplitude = quda_complex_rdiv(QUDA_COMPLEX_ONE, div); \
  } \
  sum = sqrt(sum); \
  for (int i = 0; i < (1 << nbits); i++) \
    uniform[i] = quda_complex_rdiv(uniform[i], sum); \
  func(INVOKE##nbits, &qureg); \
  printf("Testing " #func " with uniform distribution\n"); \
  VERIFY_REGISTER(qureg, nbits, uniform, func); \
  quda_quantum_reg_delete(&qureg); \
} while (0)

int main(int argc, char** argv) {
	// Complex
	complex_t op1,op2;
	op1.real = 1;
	op1.imag = 1;
	op2.real = 2;
	op2.imag = -0.5;
	complex_t res = quda_complex_add(op1,op2);
  CHECK_COMPLEX_RESULT(res, 3, 0.5, "Simple complex addition");

  // Courtesy of jsmath.cpp
#define M_SQRT1_2 0.70710678118654752440f
  // 1-qubit gates
  TEST_GATE(1, quda_quantum_pauli_x_gate, {{{0,0},{1,0}},{{1,0},{0,0}}});
  TEST_GATE(1, quda_quantum_pauli_y_gate, {{{0,0},{0,-1}},{{0,1},{0,0}}});
  TEST_GATE(1, quda_quantum_pauli_z_gate, {{{1,0},{0,0}},{{0,0},{-1,0}}});
  TEST_GATE(1, quda_quantum_hadamard_gate,
    {{{M_SQRT1_2,0},{M_SQRT1_2,0}},{{M_SQRT1_2,0},{-M_SQRT1_2,0}}});
  TEST_GATE(1, quda_quantum_phase_gate, {{{1,0},{0,0}},{{0,0},{0,1}}});
  TEST_GATE(1, quda_quantum_pi_over_8_gate,
      {{{1,0},{0,0}},{{0,0},{M_SQRT1_2,M_SQRT1_2}}});

  // 2-qubit gates
  TEST_GATE(2, quda_quantum_swap_gate,
    {{{1,0},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{1,0},{0,0}},
     {{0,0},{1,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{1,0}}});
  TEST_GATE(2, quda_quantum_controlled_not_gate,
    {{{1,0},{0,0},{0,0},{0,0}},
     {{0,0},{1,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{1,0}},
     {{0,0},{0,0},{1,0},{0,0}}});
  TEST_GATE(2, quda_quantum_controlled_y_gate,
    {{{1,0},{0,0},{0,0},{0,0}},
     {{0,0},{1,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{0,-1}},
     {{0,0},{0,0},{0,1},{0,0}}});
  TEST_GATE(2, quda_quantum_controlled_z_gate,
    {{{1,0},{0,0},{0,0},{0,0}},
     {{0,0},{1,0},{0,0},{0,0}},
     {{0,0},{0,0},{1,0},{0,0}},
     {{0,0},{0,0},{0,0},{-1,0}}});

  // 3-qubit gates
  TEST_GATE(3, quda_quantum_toffoli_gate,
    {{{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{0,0},{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{1,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{1,0},{0,0},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{0,0},{1,0},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{0,0},{0,0},{1,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,0}},
     {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,0},{0,0}}});
  TEST_GATE(3, quda_quantum_fredkin_gate,
    {{{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{0,0},{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{1,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{1,0},{0,0},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{0,0},{1,0},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,0},{0,0}},
     {{0,0},{0,0},{0,0},{0,0},{0,0},{1,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,0}}});


	// Quantum Register
	quantum_reg qreg;
	if(quda_quantum_reg_init(&qreg,16) == -1) return -1;

	quda_quantum_reg_set(&qreg,42);
	if(quda_quantum_reg_enlarge(&qreg, -1) == -1) return -1;

	quda_quantum_reg_trim(&qreg);
	quda_quantum_reg_delete(&qreg);

  return 0;
}
