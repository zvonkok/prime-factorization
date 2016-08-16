#include <gmp.h>

/* Lucas-Lehmer Sequence */
void mpz_lls_mod(mpz_t a, mpz_t index, mpz_t N) {
	
	
	gmp_printf("LLC a=%Zd index=%Zd N=%Zd", a, index, N);
	
	mpz_t i;
	
	mpz_t n2;
	mpz_t n1;
	mpz_t n0;
	
	mpz_t a_mul_n1;
	
	
	
	mpz_init(n2);
	mpz_init(n1);
	mpz_init(a_mul_n1);
	
	mpz_set(n1, a);
	mpz_init_set_ui(n0, 1);
	
	mpz_sub_ui(index, index, 1);
	mpz_init_set_ui(i, 0);
	
	if (mpz_cmp_ui(index, 1) <= 0) {
		mpz_mod(a, a, N);
		return;
	}
	
	while (mpz_cmp(i, index) < 0) {
		/* n2 := 2 * a * n1 - n0 */
		mpz_mul(a_mul_n1, a, n1);
		mpz_mul_ui(a_mul_n1, a_mul_n1, 2);
		mpz_sub(n2, a_mul_n1, n0);
		
		mpz_set(n0, n1);
		mpz_set(n1, n2);
		
		mpz_add_ui(i, i, 1);
	}
	
	mpz_mod(a, n2, N);
}

