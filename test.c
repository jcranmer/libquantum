/* test.c: simple testing program for base libquantum functionality
*/

#include <math.h>
#include <stdio.h>
#include "complex.h"
#include "quantum_reg.h"
#include "quantum_gates.h"

#define CHECK_COMPLEX_RESULT(val, compreal, compimag, explain) \
  do { \
  if (abs(val.real - compreal) > 1e-3 || abs(val.imag - compimag) > 1e-3) \
    printf("FAIL TEST " #explain ": got " \
      "%.3f + %.3fi, expected %.3f + %.sfi\n", \
      val.real, val.imag, compreal, compimag); \
  else \
    printf("PASS TEST " #explain "\n"); \
  } while(0)


// Not ready yet.
//void test_1qubit_matrix(quantum_reg *reg, complex_t matrix[4][4], ??? func) {
//  // Initialize reg to represent |0> ans
//}

int main(int argc, char** argv) {
	// Complex
	complex_t op1,op2;
	op1.real = 1;
	op1.imag = 1;
	op2.real = 2;
	op2.imag = -0.5;
	complex_t res = quda_complex_add(op1,op2);
  CHECK_COMPLEX_RESULT(res, 3, 0.5, "Simple complex addition");

	// Quantum Register
	quantum_reg qreg;
	if(quda_quantum_reg_init(&qreg,16) == -1) return -1;

	quda_quantum_reg_set(&qreg,42);
	if(quda_quantum_reg_enlarge(&qreg,NULL) == -1) return -1;

	quda_quantum_reg_trim(&qreg);
	quda_quantum_reg_delete(&qreg);

  return 0;
}
