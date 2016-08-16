#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gmp.h>

char * progname;

void print_usage_and_exit () {

        fprintf (stderr, "usage: %s nnn ...\n", progname);
	exit (-1);

}


#define RAND_NUM_LIMIT 64000

extern void ppexpo(mpz_t ex, mpz_t B0, mpz_t B1);
extern void mpz_min_ui(mpz_t rop, mpz_t op1, unsigned long int op2);


void ellmult(mpz_t xx[], mpz_t x, mpz_t s, mpz_t b, mpz_t N) {

	mpz_t x1, xold, z, z2,  zinv, P1, P1inv, Pprime, mu, d;

	mpz_t xp3, xp2, bx,  P1mu2, xold2, x_sub_x1;
	
	long int k = 0;
	
	
	mpz_init(x1);
	mpz_init(xold);
	mpz_init(z);
	mpz_init(z2);
	mpz_init(zinv);
	mpz_init(P1);
	mpz_init(P1inv);
	mpz_init(Pprime);
	mpz_init(mu);
	mpz_init(d);
	mpz_init(bx);
	
	mpz_init(xp2);
	mpz_init(xp3); 
	mpz_init(P1mu2);
	mpz_init(xold2);
	mpz_init(x_sub_x1);
	
	/* if s = 0 then return (0, 0); end; */
	if (mpz_cmp_ui(s, 0) == 0) {

		mpz_set_ui(xx[0], 0);
		mpz_set_ui(xx[1], 0);
		return;
	}
	
	/* x1 := x; z := 1; */
	mpz_set(x1, x); 
	mpz_set_ui(z, 1);
		
	/* P1 := (x * x * x + b*x*x + x) mod N; */
	
	mpz_pow_ui(xp3, x, 3);
	mpz_mul(xp2, x, x);
	mpz_mul(xp2, xp2, b);
	mpz_add(xp3, xp3, xp2);
	mpz_add(xp3, xp3, x);
	
	mpz_mod(P1, xp3, N);
	
	
	/* P1inv := mod_inverse(P1, N); */
	if (mpz_invert(P1inv, P1, N) == 0) {
		
		
		/* if P1inv = 0 then return (gcd(N, P1), 0); end; */	
		mpz_gcd(d, P1, N);
		mpz_set(xx[0], d);
		mpz_set_ui(xx[1], 0);
		return;
	}
	
	
	
	k = mpz_sizeinbase(s,2);
	
	/* for k := bit_length(s) - 2 to 0 by -1 do */
	for (k = (k-2); k >= 0; k--) {

		/* zinv := mod_inverse(2 * z, N); */
		mpz_mul_ui(z2, z, 2);
		if (mpz_invert(zinv, z2, N) == 0) {
			mpz_set_ui(zinv, 0);
		}

		/* if zinv = 0 then return (gcd(N, 2 * z), 0); end; */
		if (mpz_cmp_ui(zinv, 0) == 0) {
			
			mpz_gcd(d, N, z2);
			mpz_set(xx[0], d);
			mpz_set_ui(xx[1], 0);
			return;
		}
		/* Pprime := (3 * x * x + 2*b*x + 1) mod N */
		mpz_mul(xp2, x, x);
		mpz_mul_ui(xp2, xp2, 3);
		
		mpz_mul(bx, b, x);
		mpz_mul_ui(bx, bx, 2);
		
		mpz_add(xp2, xp2, bx);
		mpz_add_ui(xp2, xp2, 1); 
		
		mpz_mod(Pprime, xp2, N);
		
		/* mu := (Pprime * P1inv * zvin) mod N;  */
		mpz_mul(Pprime, Pprime, P1inv);
		mpz_mul(Pprime, Pprime, zinv);
		mpz_mod(mu, Pprime, N);

		/* xold := x; */
		mpz_set(xold, x);

		/* x := (P1 * mu * mu - 2 * xold) mod N;  */
		mpz_mul(P1mu2, mu, mu);
		mpz_mul(P1mu2, P1mu2, P1);
		mpz_mul_ui(xold2, xold, 2);
		mpz_sub(P1mu2, P1mu2, xold2);
		mpz_mod(x, P1mu2, N);

		/* z := (-z - mu * (x - xold)) mod N;  */
 		mpz_neg(z, z);
		mpz_sub(xold, x, xold);
		mpz_mul(xold, mu, xold);
		mpz_sub(z, z, xold);
		mpz_mod(z, z, N);
		
		
		/* if bit_test(s, k) then */
		if (mpz_tstbit(s,k)) {
		
			/* mu := mod_inverse(x - x1, N); */
			mpz_sub(x_sub_x1, x, x1);
			if (mpz_invert(mu, x_sub_x1, N) == 0) {
				
				mpz_set_ui(xx[0], 0);
				mpz_set_ui(xx[1], 0);
				return;
			}
			
			/* if mu = 0 then return (gcd(N, x - x1), 0); end; */
			if (mpz_cmp_ui(mu, 0) == 0) {
				
				mpz_gcd(d, N, x_sub_x1);
				mpz_set(xx[0], d);
				mpz_set_ui(xx[1], 0);
				return;
			}
			/* mu := mu * (z - 1) mod N; */
			mpz_sub_ui(z, z, 1);
			mpz_mul(z, mu, z);
			mpz_mod(mu, z, N);
			
			/* x := (P1 * mu * mu - x - x1) mod N  */
			mpz_mul(P1mu2, mu, mu);
			mpz_mul(P1mu2, P1mu2, P1);
			
			mpz_sub(P1mu2, P1mu2, x);
			mpz_sub(P1mu2, P1mu2, x1);
						
			mpz_mod(x, P1mu2, N);
			
			/* z := (-1 - mu * (x - x1)) mod N */
			mpz_sub(x_sub_x1, x, x1);
			mpz_mul(mu, mu, x_sub_x1);
			
			mpz_neg(mu, mu);
			mpz_sub_ui(mu, mu, 1);
			/*mpz_ui_sub(x1, -1, x1);*/
			mpz_mod(z, mu, N);
		

		}
	}	
	
	mpz_set(xx[0], x);
	mpz_set_ui(xx[1], 1);
	return;
}


void ec_fact0(mpz_t factor, mpz_t N, mpz_t a, int bound) {

	const int anz0 = 128;

	mpz_t x, B0, B0_add_anz0, B1, s, d, N_sub_1, seed;
	mpz_t xx[2];

	mpz_init(xx[0]);
	mpz_init(xx[1]);

	mpz_init(x);
	mpz_init(B0);
	mpz_init(B0_add_anz0);
	mpz_init(B1);
	mpz_init(s);
	mpz_init(d);
	mpz_init(N_sub_1);

	/* random state initialization */
	mpz_init_set_ui(seed, time(0));
	
	mpz_mul(seed, seed, seed);
	mpz_mul(seed, seed, seed);
	gmp_randstate_t state;
	gmp_randinit_default(state);
	gmp_randseed(state, seed);

	/* base := 1 + random(N-1) */
	mpz_sub_ui(N_sub_1, N, 1);
	mpz_urandomm(x, state, N_sub_1);
	mpz_add_ui(x, x, 1);

	/* for B0 := 0 to bound - 1 by anz0 do */
	mpz_set_ui(B0, 0);

	while (mpz_cmp_ui(B0, (bound - 1)) <= 0) {
		/* B1 := min(B0 + anz0, bound); */
		mpz_add_ui(B0_add_anz0, B0_add_anz0, anz0);
		mpz_min_ui(B1, B0_add_anz0, bound);

		/* s := ppexpo(B0, B1); */
		ppexpo(s, B0, B1);
		
		/* xx := mod_pemult(x, s, a, N); */	
		ellmult(xx, x, s, a, N);
		
		/* if xx[1] = 0 then */
		if (mpz_cmp_ui(xx[1], 0) == 0) {

			/* d := xx[0] */
			//mpz_set(d, xx[0]);

			/* if d > 1 and d < N then */
			if (mpz_cmp_ui(xx[0], 1) > 0) {
				;
			}
			/* return d */
			mpz_set(factor, xx[0]);
			return;
		} else {

			/* x := xx[0] */
			mpz_set(x, xx[0]);
		}
		mpz_add_ui(B0, B0, anz0); /* inc by 128 */
	}
}



void ec_factorize(mpz_t factor, mpz_t N, int bound, int anz) {

	int i;
	
	long int seed;

	mpz_t k, a, aa, d, t;//, seed;

	mpz_init(k);
	mpz_init(a);
	mpz_init(d);
	mpz_init(aa);
	//mpz_init(seed);

	
	/* we need only random number values < 64000
	 * we can use the stdclib rand() and srand()
	 * functions ... */
	//mpz_init_set_ui(t, 64000);
	seed = time(0);
	srand(seed);
	
	

	/* random state initialization */ 
	/*mpz_init_set_ui(seed, time(0));
	
	mpz_mul(seed, seed, seed);
	mpz_mul(seed, seed, seed);
	gmp_randstate_t state;
	gmp_randinit_default(state);
	gmp_randseed(state, seed);*/
	
	
	/* sieve the factor = 2  */
	if (mpz_divisible_ui_p(N, 2) != 0) {
		mpz_set_ui(factor, 2);
		return;
	}

	for (i=0; i < anz; i++) {
		
		/* a := random(64000) */
		//mpz_urandomm(a, state, N);
		//mpz_mod(a, a, t);	
		//lrand48();
		mpz_set_ui(a, rand() % RAND_NUM_LIMIT);
		
		/* d := gcd(a * a - 4, N);  */
		mpz_mul(aa, a, a);
		mpz_sub_ui(aa, aa, 4);
		mpz_gcd(d, aa, N);
		
	
		/* if d = 1 then */
		if (mpz_cmp_ui(d, 1) == 0) {

			/* d := ec_fact0(N, a, bound)  */
			
			ec_fact0(d, N, a, bound);
		}
		if ((mpz_cmp_ui(d, 1) > 0) && (mpz_cmp(d, N) < 0)) {
			mpz_set(factor, d);
			return;
		}
	}
	
	gmp_printf("FEHLGESCHLAGEN bound erhöhen !! \n");
	exit(0);
}


void dofactoring(mpz_t N) {  
	
	mpz_t factor;
	mpz_init(factor);
	mpz_set_ui(factor, 0);
	
	if (mpz_probab_prime_p(N, 10) != 0) {
		gmp_printf(" %Zd ", N); 
		return; 
	}
	
	ec_factorize(factor, N, 2000, 1000);
	
	gmp_printf(" %Zd ", factor);
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
	
	if ((mpz_cmp_ui(N, 1) == 0)) {
		puts("1"); return 0;
	}
	if ((mpz_cmp_ui(N, 3) <= 0)) { 
		puts("3"); return 0;
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
