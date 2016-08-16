

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gmp.h>
#include <mpfr.h>


#define true 1

char * progname;

void print_usage_and_exit () {
	 
  fprintf (stderr, "usage: %s nnn ...\n", progname);
  exit (-1);
   
}

extern void ppexpo(mpz_t ex, mpz_t B0, mpz_t B1);
extern void mpz_min_ui(mpz_t rop, mpz_t op1, unsigned long int op2);
extern void mpz_lls_mod(mpz_t a, mpz_t index, mpz_t N);


void pp1_factorize(mpz_t factor, mpz_t N, int bound) {
	
	gmp_printf("\npp1_factorize: %Zd %Zd\n", factor, N);
	
	const int anz0 = 128;
	
	mpz_t a, a1, aa, d, B0, B0_add_anz0, B1, ex, N_sub_3, seed;
	mpz_init(a);
	mpz_init(a1);
	mpz_init(aa);
	mpz_init(d);
	mpz_init(B0);
	mpz_init(B1);
	mpz_init(ex);
	mpz_init(N_sub_3);
	mpz_init(B0_add_anz0);
		
	
	/* random state initialization */
	mpz_init_set_ui(seed, time(0));
	
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
	
	/* a := 2 + random(N - 3) */
	mpz_sub_ui(N_sub_3, N, 3);
	mpz_urandomm(a, state, N_sub_3);
	mpz_add_ui(a, a, 2);
	
	gmp_printf("a = %Zd\n", a); 
	
	if (mpz_cmp_ui(a, 2) < 0) { 
		mpz_set_ui(a, 2);
	}
	
		
	/* if (d := gcd(a * a - 1, N)) > 1 then return d end;*/
	mpz_mul(aa, a, a);
	mpz_sub_ui(aa, aa, 1);  
	mpz_gcd(d, aa, N);
	
	gmp_printf("gcd(a^2-1, N) d=%Zd \n",d); 
	
	if (mpz_cmp_ui(d,1) > 0) {
		mpz_set(factor, d); return;
	}
	
	
	/* for B0 := 0 to bound - 1 by anz0 do */
	mpz_set_ui(B0, 0);
	
	
	while (mpz_cmp_ui(B0, (bound - 1)) <= 0) {
		gmp_printf("WOOOO\n");
		
		/* B1 := min(B0 + anz0, bound); */
		mpz_add_ui(B1, B0, (int)anz0);
		gmp_printf("B0 %Zd B1 %Zd\n", B0, B1);
		
		
		
		/* ex := ppexpo(B0, B1); */ 
		ppexpo(ex, B0, B1);
		gmp_printf("ex %Zd B0 %Zd B1 %Zd\n", ex, B0, B1);
		
		
		/* a := mod_coshmult(a, ex, N) */
		
		gmp_printf("\na := mod_coshmult(a, ex, N); %Zd %Zd %Zd\n",a, ex, N);
		
		/* mod_coshmult(a, ex, N) */
		mpz_lls_mod(a, ex, N);
			
		
		gmp_printf("\na = %Zd \n", a);
		
		/* if a = 1 then return 0 end */
		if (mpz_cmp_ui(a, 1) == 0) {
			//mpz_set_ui(factor, 0); return;
			mpz_urandomm(a, state, N_sub_3);
		}
		
		 
		/*d := gcd(a - 1, N); */
		mpz_sub_ui(a1, a, 1);
		mpz_gcd(d, a1, N);
		
		gmp_printf("\nd = %Zd\n", d);
		
		/* if d > 1 then */
		if (mpz_cmp_ui(d, 1) > 0) {
			mpz_set(factor, d); return;
		}
		
		printf("HALLO");
		/* B0 := B0 + anz0 */
		mpz_add_ui(B0, B0, anz0);
		
		gmp_printf("B0, B0, anz0 %Zd", B0);
	}
	if ((mpz_cmp_ui(d, 1) == 0) || (mpz_cmp_ui(d, 0) == 0) ) {
		gmp_printf("d = 1, FEHLGESCHLAGEN bound erhöhen");
		exit(0);
		
		
	}
} 

void dofactoring(mpz_t N) {  
	
	mpz_t factor;
	mpz_init(factor);
	mpz_set_ui(factor, 0);
	
	if (mpz_probab_prime_p(N, 10) != 0) {
		gmp_printf(" %Zd ", N); 
		return; 
	}
	
	pp1_factorize(factor, N, 64000);
	
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
