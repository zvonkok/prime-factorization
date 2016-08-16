/*
* File: pollard_rho_engine.c 
* Version: 0.2 
* 
* Created: 1.05.2005
* Last modified: 8.05.2005
*
* Author: Zvonko Krnjajic
*
* Dependencies: gmp
*
* Description: Iterates a formula until it falls into a cycle
*	       and compares xi to x2i for all i.
*
*/

#include "gmp.h"

/* The engine can fail for two reasons. 
 * First the factors of N are so big that the iteration dont
 * fall into a cycle. 
 * Second it happens that xi = yi => di = N. */
void pollard_rho_engine(mpz_t d, mpz_t x, mpz_t y, mpz_t N) {
	
	mpz_t diff;
	mpz_init(diff);
	
	mpz_set(y, x);
		
	/* Iteration formula: f(x) = (x^2 + 2) % N*/
	while (mpz_cmp_ui(d, 1) <= 0) {	
		/* x = f(x) % N */
		mpz_mul(x, x, x);
		mpz_add_ui(x, x, 2);
		mpz_mod(x, x, N);
		
		/* y = f(f(y)) % N */ 
		mpz_mul(y, y, y);
		mpz_add_ui(y, y, 2);                         
		mpz_mod(y, y, N);
		 
		mpz_mul(y, y, y);
		mpz_add_ui(y, y, 2);
		mpz_mod(y, y, N);
		
		/* d = gcd((x - y), n) */
		mpz_sub(diff, x, y);
		mpz_gcd(d, diff, N);
		
		if (mpz_cmp(d, N) == 0)
			break;
		
	}
}
