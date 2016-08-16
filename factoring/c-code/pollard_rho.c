/*
* File: pollard_rho.c 
* Version: 0.2 
*
* Created: 1.05.2005
* Last modified: 8.05.2005
*
* Author: Zvonko Krnjajic
*
* Dependencies: gmp, accumdiff.c
*
* Description: Uses Pollard-Rho method to factor a large integer 
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gmp.h>

#define TRUE 1
#define COUNT 65535  /* How many tries */

char * progname;

void print_usage_and_exit () {
	fprintf (stderr, "usage: %s nnn ...\n", progname);
	exit (-1);
}

/* further details pollard_rho_engine.c */
extern void pollard_rho_engine(mpz_t d, mpz_t x, mpz_t y, mpz_t N);


/* There are two aspects to the Pollard rho factorization method. 
 * The first is the idea of iterating a formula until it falls 
 * into a cycle. The second part of Pollard's method concerns 
 * detection of the fact that a sequence has become periodic. 
 * Pollard's suggestion was to use the idea attributed to Floyd 
 * of comparing xi to x2i for all i. */
void pollard_rho(mpz_t factor, mpz_t N) {

	int i;	
	gmp_randstate_t state;
	mpz_t x, y, d, seed;
	mpz_init(x);
	mpz_init(y);
	mpz_init(d);
	mpz_init_set_ui(seed, time(0));
	mpz_set_ui(factor, 0);
	
	/* random state initialization */
	mpz_and(seed, seed, N);
	gmp_randinit_default(state);
	gmp_randseed(state, seed);
	
	/* If pollard_rho_engine fails try 65535 more time */
	for (i = 0; i < COUNT; i++) {
		
		mpz_urandomm(x, state, N);
		
		/* If we are lucky we found a factor */
		mpz_gcd(d, x, N);
		if ((mpz_cmp_ui(d, 1) > 0) && (mpz_cmp(d, N) < 0)) {
			mpz_set(factor, d); return;
		}

		mpz_set_ui(d, 1);
		
		pollard_rho_engine(d, x, y, N);
						
		if ((mpz_cmp_ui(d, 1) > 0) && (mpz_cmp(d, N) < 0)) {
			mpz_set(factor, d); return;
		}
	}
	printf("Failed to complete factorization of");
	gmp_printf(" %Zd ");
}



/* Most factoring algorithms work in a similar way. First 
* of all a probabilistic prime test is done to identify a 
* prime number. If this number is not a prime the actual 
* factoring is done. After getting a real factor of N, 
* factor und N are recursively factored. */
void dofactoring(mpz_t N) {
	
	mpz_t factor;
	mpz_init_set_ui(factor, 0);
	
	if (mpz_cmp_ui(N, 1) == 0)
		return;
	
	/* mpz_probab_prime_p does some trial divisions, 
	* then some Miller-Rabin probabilistic primality 
	* tests to determine if N is a prime */
	if (mpz_probab_prime_p(N, 10) != 0) {
	
		gmp_printf("%Zd ", N); 
		
		return;
	}
	
	
	pollard_rho(factor, N);
	
	mpz_divexact(N, N, factor);
	
	dofactoring(factor);
	
	dofactoring(N);
}



int main(int argc, char* argv[])
{ 
	mpz_t N;
	mpz_init(N);
	
	progname = argv[0];
	
	if (argc < 2) {
		print_usage_and_exit ();
	}
	
	if (mpz_set_str(N, argv[1], 0) != 0) {
		print_usage_and_exit ();
	}
	
	if ((mpz_cmp_ui(N, 1) == 0) || (mpz_cmp_ui(N, 3) <= 0)) {
		gmp_printf("Not going to factorize %Zd \n", N); 
		return 0;
	}
long int a,b;
	a = time(0);
	printf("START %ld\n",a);
	dofactoring(N);
	puts("");
	b = time(0);
	printf("END %ld\n",b);
	printf("DIFF %ld\n",(b-a));
	
	return 0;
} 
