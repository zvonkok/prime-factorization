/*
 * File: fermat.c 
 * Version: 0.2 
 *
 * Created: 1.05.2005
 * Last modified: 8.05.2005
 *
 * Author: Zvonko Krnjajic
 *
 * Dependencies: gmp
 *
 * Description: Uses Fermat method to factor a large integer 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <time.h>

char * progname;

void print_usage_and_exit () {
	fprintf (stderr, "usage: %s nnn ...\n", progname);
	exit (-1);
}


/* Given a number n, Fermat's factorization methods 
 * looks for integers x and y such that N = x^2 - y^2. 
 * Then N = (x-y)(y+x) and N is factored. */ 
void fermat(mpz_t factor, mpz_t N) {
	
	mpz_t u, v, delta, root;
	
	mpz_init(u);
	mpz_init(v);
	mpz_init(delta);
	mpz_init(root);
	
	
	mpz_set_ui(factor, 2);
	if (mpz_divisible_p(N, factor) != 0) {
		return;
	}
	
	/* As the first trial for x, we try sqrt(N) */
	if (mpz_perfect_square_p(N) != 0) {
		mpz_sqrt(factor, N);
		return;
	} 
	
	/* Then check if delta_x1 = x1^2 - n is a square number. If not
	 * try x2 = x1 + 1 and check if delta_x2 = delta_x1 + 2*x1 + 1 is a 
	 * square number. Subsequent differences are obtained simply by 
	 * adding two. u = 2*x1 + 1
	 * delta_x3 = u + 2; 
	 * delta_x4 = u + 2 + 2; ...*/
	mpz_sqrt(root, N);
	mpz_add_ui(root, root, 1);
	
	
	/* u = 2 * root + 1 */
	mpz_mul_ui(u, root, 2);
	mpz_add_ui(u, u, 1);
	/* v = 1 */
	mpz_set_ui(v, 1);
	
	/* delta = root^2 - m */
	mpz_mul(root, root, root);
	mpz_sub(delta, root, N);
	
	
	/* To avoid the multiple call to mpz_perfect_square_p the delta 
	 * value is watched. If delta > 0 u is incremented by 2 (the next 
	 * delta is calculated). Else if delta < 0 v is decremented by 2 (the 
	 * previous delta is calculated).
	 * As a result we get a factor if delta = 0 */  
	while (mpz_cmp_ui(delta, 0) !=0 ) {
		while (mpz_cmp_ui(delta, 0) < 0 ) {
			mpz_add(delta, delta, u);
			mpz_add_ui(u, u, 2);
		}
		while (mpz_cmp_ui(delta, 0) > 0 ) {
			mpz_sub(delta, delta, v);
			mpz_add_ui(v, v, 2);
		}
	} 
	mpz_add(u, u, v);
	mpz_sub_ui(u, u, 2);
	mpz_tdiv_q_ui(u, u, 2); 
	mpz_set(factor, u);
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
		gmp_printf(" %Zd ", N); return;
	}
	
	fermat(factor, N);
	
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
