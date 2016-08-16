/*
* File: ppexpo.c 
* Version: 0.2 
* 
* Created: 1.05.2005
* Last modified: 8.05.2005
*
* Author: Zvonko Krnjajic
*
* Dependencies: gmp
*
* Description: ppexpo(k, B0, B1) calculates the product of
*	       all primes between the two bounds B0 and B1
*
*/


#include <gmp.h>


extern void mpz_max_ui(mpz_t rop, mpz_t op1, unsigned long int op2);


/* ppexpo(k, B0, B1) calculates the product of 
 * all primes between the two bounds B0 and B1
 * k is return value. Furthermore the product
 * of all whole numbers is calculated between
 * B0 and B1 = sqrt(B0). All primes are gathered
 * with mpz_next_prime() function. */
 
void ppexpo(	/* output exponent */		mpz_t k, 
		/* input lower bound */		mpz_t B0, 
		/* input higher bound */		mpz_t B1) {
	
	mpz_t x, m0, m1, i;

	mpz_t isqrt_B0;
	mpz_t isqrt_B1;
	
	mpz_init(isqrt_B0);
	mpz_init(isqrt_B1);
	mpz_init(m0);
	mpz_init(m1);
	mpz_init(i);
	
	/* x := 1 */
	mpz_init_set_ui(x, 1);
	
	/* m0 := max(2, isqrt(B0)+1) */
	mpz_sqrt(isqrt_B0, B0);
	mpz_add_ui(isqrt_B0, isqrt_B0, 1);
	mpz_max_ui(isqrt_B0, isqrt_B0, 2);
	mpz_set(m0, isqrt_B0);
	
	/* m1 := isqrt(B1) */
	mpz_sqrt(isqrt_B1, B1);
	mpz_set(m1, isqrt_B1);
	
	
	/* calculate all whole numbers between bounds */
	/* for i := m0 to m1 do */
	mpz_set(i, m0);
	
	while (mpz_cmp(i, m1) <= 0) {
		/* x := x * i */
		mpz_mul(x, x, i);
		mpz_add_ui(i, i, 1);
	}
	
	/* if odd(B0) then inc(B0) end */
	if (mpz_odd_p(B0) != 0) {
		mpz_add_ui(B0, B0, 1);
	}

		
	/* calculate all primes between bounds */
	/* for i := B0 + 1 to B1 by 2 do*/
	mpz_set(i, B0);
	mpz_add_ui(i, i, 1); 
	//mpz_add_ui(B0, B0, 1);
	
	
	while (mpz_cmp(i, B1) <= 0) {
	
		if (mpz_probab_prime_p(i, 5) == 2) {
			mpz_mul(x, x, i);
		}
	
		mpz_add_ui(i, i, 2);
	}
	
	/* return x*/
	mpz_set(k, x);

}

/*
void bla() {
	while(1) {
		mpz_nextprime(i, i);
		if (mpz_cmp(i, B1) >= 0) {
			return;
		}
		gmp_printf("i = %Zd\n", i);
		mpz_mul(x, x, i);
		gmp_printf("x = %Zd\n", x);
	}
}
// */

