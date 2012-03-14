/* test.c: simple testing program for base libquantum functionality
*/

#include "complex.h"
#include "quantum_reg.h"
#include "quantum_gates.h"

int main(int argc, char** argv) {
	// Complex
	complex_t op1,op2;
	op1.real = 1;
	op1.imag = 1;
	op2.real = 2;
	op2.imag = -0.5;
	quda_complex_add(op1,op2);

	// Quantum Register
	quantum_reg qreg;
	if(quda_quantum_reg_init(&qreg,16) == -1) return -1;

	quda_quantum_reg_set(&qreg,42);
	if(quda_quantum_reg_enlarge(&qreg,NULL) == -1) return -1;

	quda_quantum_reg_trim(&qreg);
	quda_quantum_reg_delete(&qreg);

	return 0;
}
