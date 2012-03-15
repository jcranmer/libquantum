/* complex.h: header for complex numbers and operations
*/

#ifndef __QUDA_COMPLEX_H
#define __QUDA_COMPLEX_H

#define __QUDA_USE_BRANCH_CUT
#define QUDA_PI 3.141592654f
#define QUDA_E  2.718281828f

typedef struct complex_t {
	float real;
	float imag;
} complex_t;

extern const complex_t QUDA_I;
extern const complex_t QUDA_COMPLEX_ZERO;
extern const complex_t QUDA_COMPLEX_ONE;

#ifdef __QUDA_USE_BRANCH_CUT
/* The standard branch cut is defined as (-PI,PI] */
extern int QUDA_BRANCH_CUT_LOWER;
extern int QUDA_BRANCH_CUT_UPPER;

/* Set branch cut for use in arg/log to (lower, lower+2*PI) */
void quda_complex_set_branch_cut(float lower);
#endif

/* Copy a complex number */
complex_t quda_complex_copy(complex_t c);

/* Test equality of two complex numbers (returns 1 if equal, 0 otherwise) */
int quda_complex_eq(complex_t op1, complex_t op2);

/* Complex absolute square
 * Equivalent to a complex number times its conjugate
 * For a quantum state amplitude, this value represents the probability
 * of the quantum state.
 */
float quda_complex_abs_square(complex_t c);

/* Complex modulus (absolute value) */
float quda_complex_abs(complex_t c);

/* Complex argument (vector angle) -- explicitly dependent on branch cut */
float quda_complex_arg(complex_t c);

/* Complex conjugate */
complex_t quda_complex_conj(complex_t c);

/* Complex negation */
complex_t quda_complex_neg(complex_t c);

/* Standard mathematical operations between complex numbers and reals */
complex_t quda_complex_radd(complex_t c, float f);
complex_t quda_complex_rsub(complex_t c, float f);
complex_t quda_complex_rmul(complex_t c, float f);
complex_t quda_complex_rdiv(complex_t c, float f);

/* Standard mathematical operations between complex numbers */
complex_t quda_complex_add(complex_t op1, complex_t op2);
complex_t quda_complex_sub(complex_t op1, complex_t op2);
complex_t quda_complex_mul(complex_t op1, complex_t op2);
complex_t quda_complex_div(complex_t op1, complex_t op2);

/* Multiplication by imaginary number i (QUDA_I) */
complex_t quda_complex_mul_i(complex_t c);

/* Multiplication by imaginary number -i (QUDA_I) */
complex_t quda_complex_mul_ni(complex_t c);

/* Complex reciprocal */
complex_t quda_complex_rcp(complex_t c);

/* Complex exponential (e^c) */
complex_t quda_complex_exp(complex_t c);

/* Complex to integer power (c^p) */
complex_t quda_complex_ipow(complex_t c, int p);

/* Complex power op1^(op2) */
complex_t quda_complex_pow(complex_t op1, complex_t op2);

/* Complex natural logarithm -- implicitly dependent on branch cut */
complex_t quda_complex_log(complex_t c);

/* Complex trigonometric functions */
complex_t quda_complex_sin(complex_t c);
complex_t quda_complex_cos(complex_t c);
complex_t quda_complex_tan(complex_t c);

/* Inverse complex trigonometric functions */
complex_t quda_complex_asin(complex_t c);
complex_t quda_complex_acos(complex_t c);
complex_t quda_complex_atan(complex_t c);

/* Complex hyperbolic trigonometric functions */
complex_t quda_complex_sinh(complex_t c);
complex_t quda_complex_cosh(complex_t c);
complex_t quda_complex_tanh(complex_t c);

/* Complex inverse hyperbolic trigonometric functions */
complex_t quda_complex_asinh(complex_t c);
complex_t quda_complex_acosh(complex_t c);
complex_t quda_complex_atanh(complex_t c);

#endif // __QUDA_COMPLEX_H
