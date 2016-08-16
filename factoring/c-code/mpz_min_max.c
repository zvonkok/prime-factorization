#include <gmp.h>


void mpz_max(mpz_t rop, mpz_t op1, mpz_t op2) {
	
	if (mpz_cmp(op1, op2) > 0) {
		mpz_set(rop, op1);
	} else if (mpz_cmp(op1, op2) < 0){
		mpz_set(rop, op2);
	}
}

void mpz_min(mpz_t rop, mpz_t op1, mpz_t op2) {
	
	if (mpz_cmp(op1, op2) < 0) {
		mpz_set(rop, op1);
	} else if (mpz_cmp(op1, op2) > 0){
		mpz_set(rop, op2);
	}
}

void mpz_min_ui(mpz_t rop, mpz_t op1, unsigned long int op2) {
	
	if (mpz_cmp_ui(op1, op2) < 0) {
		mpz_set(rop, op1);
	} else if (mpz_cmp_ui(op1, op2) > 0){
		mpz_set_ui(rop, op2);
	}
}


void mpz_max_ui(mpz_t rop, mpz_t op1, unsigned long int op2) {
	
	if (mpz_cmp_ui(op1, op2) > 0) {
		mpz_set(rop, op1);
	} else if (mpz_cmp_ui(op1, op2) < 0){
		mpz_set_ui(rop, op2);
	}
}
