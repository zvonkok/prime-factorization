#include <gmp.h>
#include <stdio.h>
#include <limits.h>

/* Lucas-Lehmer Sequence */
void mpz_lls_mod(mpz_t a, mpz_t ex, mpz_t N) {

	int bit = 0;
	int bit0 = 0;
	
	
	mpz_t v, w;
	
	mpz_init(v);
	mpz_init(w);
	
	long int i = 0;
		
	i = mpz_sizeinbase(ex, 2);
	 
	++i; 
	
	printf("###################### %ld", i);
	
	
	printf("i  lla == %ld\n", i);
	
	bit = 0;
	
	while(--i >= 0) {
		bit0 = bit;
		bit = mpz_tstbit(ex, i);
		if (bit != bit0) {
			mpz_swap(v, w);
		}
		/* w := 2*v*w - a */
		mpz_mul(w, v, w);
		mpz_mul_ui(w, w, 2);
		mpz_sub(w, w, a);
		mpz_mod(w, w, N);
		
		/* v := 2*v*v - 1 */
		mpz_mul(v, v, v);
		mpz_mul_ui(v, v, 2);
		mpz_sub_ui(v, v, 1);
		mpz_mod(v, v, N);
	}
	if (bit == 0) {
		mpz_mod(v, v, N);
		mpz_set(a, v);
	} else {
		mpz_mod(w, w, N);
		mpz_set(a, w);
	}	
}




