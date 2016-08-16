/*
 * File: pollard_p1.c 
 * Version: 0.2 
 *
 * Created: 1.05.2005
 * Last modified: 8.05.2005
 *
 * Author: Zvonko Krnjajic
 *
 * Dependencies: gmp
 *
 * Description: Uses Pollard p-1 method to factor a large integer 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gmp.h>


#define true 1

char * progname;

void print_usage_and_exit () {
	
	fprintf (stderr, "usage: %s nnn ...\n", progname);
	exit (-1);
	
}

int min(a, b) {
	if (a > b)
		return b;
	else
		return a;
	
}


extern void ppexpo(mpz_t k, mpz_t B0, mpz_t B1);
extern void mpz_min_ui(mpz_t rop, mpz_t op1, unsigned long int op2);


/* Big Prime Variation is the second step of Pollard p-1
 * Method. */
void bigprimevar(mpz_t d, mpz_t y, mpz_t N, mpz_t B) {

	int count;
 
	mpz_t y2, q, next_prime, diff, z;

	mpz_t ym1;

	mpz_init(y2);
	mpz_init(q);
	mpz_init(next_prime);
	mpz_init(diff);
	mpz_init(z);

	mpz_init(ym1);

	
	mpz_set_ui(q, 3);
	mpz_powm(y, y, q, N);
	
	mpz_sub_ui(z, y, 1);

	mpz_set_ui(d, 0);

	count = 1;

	while (mpz_cmp(q, B) < 0) {
		
		mpz_nextprime(next_prime, q);
		mpz_sub(diff, next_prime, q); 
		
		mpz_mul(y, y, diff);
		mpz_mod(y, y, N);

		mpz_sub_ui(ym1, y, 1);
		mpz_mul(z, z, ym1);
		mpz_mod(z, z, N);
		
		if (++count >= 1000) {
			gmp_printf(".");
			count = 0;
			mpz_gcd(d, z, N);
			if (mpz_cmp_ui(d, 1) > 0)
				break;
		}
		mpz_set(q, next_prime);
	} 
	
	return;
}



void p1_factbpv(mpz_t factor, mpz_t N, int bound) {
	
	int anz0 = 10;
	
	mpz_t base, d, B0, B0_add_anz0, B1, ex, N_sub_2, seed;
	mpz_init(base);
	mpz_init(d);
	mpz_init(B0);
	mpz_init(B1);
	mpz_init(ex);
	mpz_init(N_sub_2);
	mpz_init(B0_add_anz0);
	mpz_init_set_ui(seed, time(0));
	
	/* random state initialization */
	mpz_mul(seed, seed, seed);
	mpz_mul(seed, seed, seed);
	gmp_randstate_t state;
	gmp_randinit_default(state);
	gmp_randseed(state, seed);

	/* sieve the factor = 2  */
	if (mpz_divisible_ui_p(N, 2) != 0) {
		mpz_set_ui(factor, 2);
		return;
	} 

	/* base := 2 + random(n-2) */
	mpz_sub_ui(N_sub_2, N, 2);
	mpz_add_ui(N_sub_2, N_sub_2, 2);
	mpz_urandomm(base, state, N_sub_2);

	
	/* d := gcd(base, N) */
	mpz_gcd(d, base, N);

	/* if d > 1 then return d end; */
	if (mpz_cmp_ui(d, 1) > 0) {
		mpz_set(factor, d); return;
	}

	 
	/* for B0 := 0 to bound - 1 by anz0 do */
	mpz_set_ui(B0, 0);

	while (mpz_cmp_ui(B0, (bound - 1)) <= 0) {
		/* ex := ppexpo(B0, B1 = B0 + 256); */
		mpz_add_ui(B1, B0, (int)anz0);
		ppexpo(ex, B0, B1);

		/* base := base ** ex mod N */
		mpz_powm(base, base, ex, N);
		
		/* If it happens that N is a multiple of 
		 * base^ex mod N, restart first step with
		 * new values (base, exp ...) */
		if (mpz_cmp_ui(base, 0) == 0){
			mpz_set(factor, N); 
			return;
		}

		/* If it happens that base^ex is congruent
		 * 1 mod N, its a try to restart 1 step 
		 * with a new base */
		if (mpz_cmp_ui(base, 1) == 0) {
			mpz_urandomm(base, state, N_sub_2);
		}	
		
		/* d := gcd(base - 1, N)*/
		mpz_sub_ui(base, base, 1);
		mpz_gcd(d, base, N);
		
		if (mpz_cmp_ui(d,1) > 0) {
			mpz_set(factor, d); return;
		}
		
		/* B0 := B0 + anz0 */
		mpz_add_ui(B0, B0, anz0);
	}
	
	
	if (mpz_cmp_ui(d, 1) == 0) {
		mpz_t B;
		mpz_init_set_ui(B, bound);
		mpz_mul_ui(B, B, 100);
		bigprimevar(d, base, N, B);
		
	} 
	
	if ((mpz_cmp_ui(d, 1) == 0) || (mpz_cmp_ui(d, 0) == 0) ) {
		gmp_printf("d = 1, FEHLGESCHLAGEN bound erhöhen");
		exit(0);
		
		
	}
		
	mpz_set(factor, d);
}

void dofactoring(mpz_t N) {
	
	mpz_t factor;
	mpz_init_set_ui(factor, 0);
	
	if (mpz_probab_prime_p(N, 10) != 0) {
		gmp_printf(" %Zd ", N); return;
	}
	
	p1_factbpv(factor, N, 1280000);
	
	gmp_printf("%Zd", factor);
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
