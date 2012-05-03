/* complex.c: operations on complex numbers
*/

#include <math.h>
#include "complex.h"

#ifndef QUDA_GATE
#define QUDA_GATE
#endif

QUDA_GATE const complex_t QUDA_I = { .real = 0.0f, .imag = 1.0f };
QUDA_GATE const complex_t QUDA_COMPLEX_ZERO = { .real = 0.0f, .imag = 0.0f };
QUDA_GATE const complex_t QUDA_COMPLEX_ONE = { .real = 1.0f, .imag = 0.0f };

#undef __QUDA_USE_BRANCH_CUT
#ifdef __QUDA_USE_BRANCH_CUT
int QUDA_BRANCH_CUT_LOWER = -1.0*QUDA_PI;
int QUDA_BRANCH_CUT_UPPER = QUDA_PI;
#endif

#ifdef __QUDA_USE_BRANCH_CUT
QUDA_GATE void quda_complex_set_branch_cut(float lower) {
	QUDA_BRANCH_CUT_LOWER = lower;
	QUDA_BRANCH_CUT_UPPER = lower+2*QUDA_PI;
}
#endif

QUDA_GATE complex_t quda_complex_copy(complex_t c) {
	complex_t res;
	res.real = c.real;
	res.imag = c.imag;
	return res;
}

// TODO: Implement float-tolerant approximate equivalence check
QUDA_GATE int quda_complex_eq(complex_t op1, complex_t op2) {
	if(op1.real == op2.real && op1.imag == op2.imag) {
		return 1;
	}
	return 0;
}

QUDA_GATE float quda_complex_abs_square(complex_t c) {
	return c.real*c.real + c.imag*c.imag;
}

QUDA_GATE float quda_complex_abs(complex_t c) {
	float res = c.real*c.real + c.imag*c.imag;
	return sqrt(res);
}

QUDA_GATE float quda_complex_arg(complex_t c) {
	if(c.real == 0.0f) {
		if(c.imag > 0) {
			return QUDA_PI/2.0f;
		} else {
			return QUDA_PI/-2.0f;
		}
	}
	float res = atan(c.imag/c.real);
	#ifdef __CUDA_USE_BRANCH_CUT
	// TODO: Implement cleaner branch cut enforcement
	while(res <= QUDA_BRANCH_CUT_LOWER) res+=2*QUDA_PI;
	while(res > QUDA_BRANCH_CUT_UPPER) res-=2*QUDA_PI;
	#endif
  return res;
}

QUDA_GATE complex_t quda_complex_conj(complex_t c) {
	complex_t res;
	res.real = c.real;
	res.imag = -c.imag;
	return res;
}

QUDA_GATE complex_t quda_complex_neg(complex_t c) {
	complex_t res;
	res.real = -c.real;
	res.imag = -c.imag;
	return res;
}

QUDA_GATE complex_t quda_complex_radd(complex_t c, float f) {
	complex_t res;
	res.real = c.real+f;
	res.imag = c.imag;
	return res;
}

QUDA_GATE complex_t quda_complex_rsub(complex_t c, float f) {
	complex_t res;
	res.real = c.real - f;
	res.imag = c.imag;
	return res;
}

QUDA_GATE complex_t quda_complex_rmul(complex_t c, float f) {
	complex_t res;
	res.real = c.real*f;
	res.imag = c.imag*f;
	return res;
}

QUDA_GATE complex_t quda_complex_rdiv(complex_t c, float f) {
	complex_t res;
	res.real = c.real/f;
	res.imag = c.imag/f;
	return res;
}

QUDA_GATE complex_t quda_complex_add(complex_t op1, complex_t op2) {
	complex_t res;
	res.real = op1.real + op2.real;
	res.imag = op1.imag + op2.imag;
	return res;
}

QUDA_GATE complex_t quda_complex_sub(complex_t op1, complex_t op2) {
	complex_t res;
	res.real = op1.real - op2.real;
	res.imag = op1.imag - op2.imag;
	return res;
}

QUDA_GATE complex_t quda_complex_mul(complex_t op1, complex_t op2) {
	complex_t res;
	res.real = op1.real*op2.real - op1.imag*op2.imag;
	res.imag = op1.imag*op2.real + op1.real*op2.imag;
	return res;
}

QUDA_GATE complex_t quda_complex_div(complex_t op1, complex_t op2) {
	float denom = op2.real*op2.real + op2.imag*op2.imag;
	complex_t res;
	res.real = (op1.real*op2.real + op1.imag*op2.imag)/denom;
	res.imag = (op1.imag*op2.real - op1.real*op2.imag)/denom;
	return res;
}

QUDA_GATE complex_t quda_complex_mul_i(complex_t c) {
	complex_t res;
	res.real = -c.imag;
	res.imag = c.real;
	return res;
}

QUDA_GATE complex_t quda_complex_mul_ni(complex_t c) {
	complex_t res;
	res.real = c.imag;
	res.imag = -c.real;
	return res;
}

QUDA_GATE complex_t quda_complex_rcp(complex_t c) {
	float denom = c.real*c.real + c.imag*c.imag;
	complex_t res;
	res.real = c.real/denom;
	res.imag = -c.imag/denom;
	return res;
}

QUDA_GATE complex_t quda_complex_exp(complex_t c) {
	float expo = exp(c.real);
	complex_t res;
	res.real = expo*cos(c.imag);
	res.imag = expo*sin(c.imag);
	return res;
}

// TODO: compare performance of quda_complex_ipow functions
QUDA_GATE complex_t quda_complex_ipow(complex_t c,int p) {
	complex_t res;
	if(quda_complex_eq(c,QUDA_COMPLEX_ZERO)) {
		res = QUDA_COMPLEX_ZERO;
	} else {
		float r2 = pow(quda_complex_abs(c),p);
		float theta = p*quda_complex_arg(c);
		res.real = r2*cos(theta);
		res.imag = r2*sin(theta);
	}
	return res;
}

/*
QUDA_GATE complex_t quda_complex_ipow(complex_t c, int p) {
	if(quda_complex_eq(c,QUDA_COMPLEX_ZERO)) {
		return quda_complex_copy(QUDA_COMPLEX_ZERO);
	}
	return quda_complex_exp(quda_complex_rmul(quda_complex_log(c),p));
}
*/

QUDA_GATE complex_t quda_complex_pow(complex_t op1, complex_t op2) {
	if(quda_complex_eq(op1,QUDA_COMPLEX_ZERO)) {
		return quda_complex_copy(QUDA_COMPLEX_ZERO);
	}
	return quda_complex_exp(quda_complex_mul(quda_complex_log(op1),op2));
}

QUDA_GATE complex_t quda_complex_log(complex_t c) {
	complex_t res;
	res.real = log(quda_complex_abs(c));
	res.imag = quda_complex_arg(c);
  return res;
}

// TODO: Unwrap some of the complex trig calls (&| hyperbolics) to optimize (especially tan)
QUDA_GATE complex_t quda_complex_sin(complex_t c) {
	complex_t num = quda_complex_sub(quda_complex_exp(quda_complex_mul_i(c)),
			quda_complex_exp(quda_complex_mul_ni(c)));
	complex_t res;
	res.real = 0.5f*num.imag;
	res.imag = -0.5f*num.real;
	return res;
}

QUDA_GATE complex_t quda_complex_cos(complex_t c) {
	complex_t res = quda_complex_add(quda_complex_exp(quda_complex_mul_i(c)),
			quda_complex_exp(quda_complex_mul_ni(c)));
	res.real *= 0.5f;
	res.imag *= 0.5f;
	return res;
}

QUDA_GATE complex_t quda_complex_tan(complex_t c) {
	return quda_complex_div(quda_complex_sin(c),quda_complex_cos(c));
}

// TODO: Clean up inverse trig (&| hyperbolic) call stacks if possible
QUDA_GATE complex_t quda_complex_asin(complex_t c) {
	return quda_complex_mul_ni(quda_complex_log(quda_complex_add(quda_complex_mul_i(c),
			quda_complex_exp(quda_complex_rmul(quda_complex_log(quda_complex_sub(QUDA_COMPLEX_ONE,
			quda_complex_mul(c,c))),0.5f)))));
}

QUDA_GATE complex_t quda_complex_acos(complex_t c) {
	return quda_complex_mul_ni(quda_complex_log(quda_complex_add(c,quda_complex_exp(
			quda_complex_rmul(quda_complex_log(quda_complex_rsub(quda_complex_mul(c,c),1.0f)),
			0.5f)))));
}

QUDA_GATE complex_t quda_complex_atan(complex_t c) {
	return quda_complex_rmul(quda_complex_mul_i(quda_complex_log(quda_complex_div(
			quda_complex_add(QUDA_I,c),quda_complex_sub(QUDA_I,c)))),0.5f);
}

QUDA_GATE complex_t quda_complex_sinh(complex_t c) {
	complex_t res = quda_complex_sub(quda_complex_exp(c),quda_complex_exp(quda_complex_neg(c)));
	res.real *= 0.5f;
	res.imag *= 0.5f;
	return res;
}

QUDA_GATE complex_t quda_complex_cosh(complex_t c) {
	complex_t res = quda_complex_add(quda_complex_exp(c),quda_complex_exp(quda_complex_neg(c)));
	res.real *= 0.5f;
	res.imag *= 0.5f;
	return res;
}

QUDA_GATE complex_t quda_complex_tanh(complex_t c) {
	return quda_complex_div(quda_complex_sinh(c),quda_complex_cosh(c));
}

QUDA_GATE complex_t quda_complex_asinh(complex_t c) {
	return quda_complex_log(quda_complex_add(c,quda_complex_exp(quda_complex_rmul(quda_complex_log(
			quda_complex_radd(quda_complex_mul(c,c),1.0f)),0.5f))));
}

QUDA_GATE complex_t quda_complex_acosh(complex_t c) {
	return quda_complex_log(quda_complex_add(c,quda_complex_exp(quda_complex_rmul(quda_complex_log(
			quda_complex_rsub(quda_complex_mul(c,c),1.0f)),0.5f))));
}

QUDA_GATE complex_t quda_complex_atanh(complex_t c) {
	return quda_complex_rmul(quda_complex_log(quda_complex_div(quda_complex_radd(c,1.0f),
			quda_complex_sub(QUDA_COMPLEX_ONE,c))),0.5f);
}
