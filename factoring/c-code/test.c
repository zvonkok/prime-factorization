#include <stdio.h>
#include <gmp.h>
#include <limits.h>


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
	
	gmp_printf("b %Zd\n", b);
	gmp_printf("s %Zd\n ", s);
	/* if s = 0 then return (0, 0); end; */
	if (mpz_cmp_ui(s, 0) == 0) {

		mpz_set_ui(xx[0], 0);
		mpz_set_ui(xx[1], 0);
		return;
	}
	
	/* x1 := x; z := 1; */
	mpz_set(x1, x); 
	gmp_printf("x1 %Zd\n ",x1 );
	mpz_set_ui(z, 1);
	gmp_printf("z %Zd\n ",z );
		
	/* P1 := (x * x * x + b*x*x + x) mod N; */
	
	mpz_pow_ui(xp3, x, 3);
	mpz_mul(xp2, x, x);
	mpz_mul(xp2, xp2, b);
	gmp_printf("xp3 %Zd\n", xp3);
	gmp_printf("xp2 %Zd\n", xp2);
	gmp_printf("xp2*b %Zd\n", xp2);
	
	mpz_add(xp3, xp3, xp2);
	mpz_add(xp3, xp3, x);
	
	gmp_printf("xp3 %Zd\n", xp3);
	mpz_mod(P1, xp3, N);
	
	gmp_printf("P1 %Zd\n ",P1 );
	
	
	/* P1inv := mod_inverse(P1, N); */
	if (mpz_invert(P1inv, P1, N) == 0) {
		
		gmp_printf("P1inv %Zd\n", P1inv);
		
		/* if P1inv = 0 then return (gcd(N, P1), 0); end; */	
		mpz_gcd(d, P1, N);
		gmp_printf("gcd(N, P1)  %Zd \n",d );
		mpz_set(xx[0], d);
		mpz_set_ui(xx[1], 0);
		return;
	}
	
	gmp_printf("P1inv %Zd\n", P1inv);
	
	
	k = mpz_sizeinbase(s,2);
	
	printf("k %ld\n", k );
	
	gmp_printf("x %Zd\n", x );
	
	/* for k := bit_length(s) - 2 to 0 by -1 do */
	for (k = (k-2); k >= 0; k--) {

		printf("k  %ld\n", k);
		/* zinv := mod_inverse(2 * z, N); */
		mpz_mul_ui(z2, z, 2);
		if (mpz_invert(zinv, z2, N) == 0) {
			mpz_set_ui(zinv, 0);
		}
		gmp_printf(" zinv %Zd\n ", zinv );

		/* if zinv = 0 then return (gcd(N, 2 * z), 0); end; */
		if (mpz_cmp_ui(zinv, 0) == 0) {
			
			mpz_gcd(d, N, z2);
			gmp_printf(" gcd(N, 2 * z) %Zd \n", d);
			mpz_set(xx[0], d);
			mpz_set_ui(xx[1], 0);
			return;
		}
		gmp_printf("b   %Zd  x   %Zd\n", b, x );
		/* Pprime := (3 * x * x + 2*b*x + 1) mod N */
		mpz_mul(xp2, x, x);
		mpz_mul_ui(xp2, xp2, 3);
		gmp_printf("3*x*x %Zd\n", xp2);
		
		gmp_printf("## x  %Zd  b  %Zd\n",x, b);
		mpz_mul(bx, b, x);
		gmp_printf("b*x %Zd\n", bx);
		mpz_mul_ui(bx, bx, 2);
		gmp_printf("2*b*x %Zd\n", bx);
		
		mpz_add(xp2, xp2, bx);
		mpz_add_ui(xp2, xp2, 1); 
		
		mpz_mod(Pprime, xp2, N);
		gmp_printf("Pprime  %Zd\n", Pprime );
		
		/* mu := (Pprime * P1inv * zvin) mod N;  */
		mpz_mul(Pprime, Pprime, P1inv);
		mpz_mul(Pprime, Pprime, zinv);
		mpz_mod(mu, Pprime, N);
		gmp_printf("mu %Zd \n",mu );

		/* xold := x; */
		mpz_set(xold, x);
		gmp_printf(" xold %Zd\n", xold  );

		/* x := (P1 * mu * mu - 2 * xold) mod N;  */
		mpz_mul(P1mu2, mu, mu);
		mpz_mul(P1mu2, P1mu2, P1);
		mpz_mul_ui(xold2, xold, 2);
		mpz_sub(P1mu2, P1mu2, xold2);
		mpz_mod(x, P1mu2, N);
		gmp_printf(" x  %Zd\n",x );

		/* z := (-z - mu * (x - xold)) mod N;  */
 		mpz_neg(z, z);
		mpz_sub(xold, x, xold);
		mpz_mul(xold, mu, xold);
		mpz_sub(z, z, xold);
		mpz_mod(z, z, N);
		gmp_printf(" z  %Zd \n", z);
		
		
		/* if bit_test(s, k) then */
		if (mpz_tstbit(s,k)) {
			printf("bit_test %d \n", mpz_tstbit(s,k) );
		
			gmp_printf("x  %Zd\n", x);
			gmp_printf("x1  %Zd\n", x1);
			/* mu := mod_inverse(x - x1, N); */
			mpz_sub(x_sub_x1, x, x1);
			gmp_printf("x-x1  %Zd\n", x_sub_x1);
			if (mpz_invert(mu, x_sub_x1, N) == 0) {
				
				mpz_set_ui(xx[0], 0);
				mpz_set_ui(xx[1], 0);
				return;
			}
			gmp_printf(" mu %Zd \n", mu );
			
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
			gmp_printf("mu %Zd \n", mu );
			
			/* x := (P1 * mu * mu - x - x1) mod N  */
			mpz_mul(P1mu2, mu, mu);
			//gmp_printf("mu*mu %Zd\n", P1mu2);
			mpz_mul(P1mu2, P1mu2, P1);
			
			mpz_sub(P1mu2, P1mu2, x);
			mpz_sub(P1mu2, P1mu2, x1);
						
			mpz_mod(x, P1mu2, N);
			gmp_printf(" x %Zd\n", x );
			
			/* z := (-1 - mu * (x - x1)) mod N */
			mpz_sub(x_sub_x1, x, x1);
			mpz_mul(mu, mu, x_sub_x1);
			
			mpz_neg(mu, mu);
			mpz_sub_ui(mu, mu, 1);
			/*mpz_ui_sub(x1, -1, x1);*/
			mpz_mod(z, mu, N);
			gmp_printf("z %Zd\n ", z );


		}
	}	
	
	gmp_printf("x zuletzt %Zd\n", x);
	mpz_set(xx[0], x);
	mpz_set_ui(xx[1], 1);
	return;
}




int main(void){


	
	 
	mpz_t x, s, b, n, xx[2];
		 
	mpz_init_set_ui(x, 23412111);
	mpz_init_set_ui(s, 2334);
	mpz_init_set_ui(b, 2);
	mpz_init_set_ui(n, 17);
	
	mpz_init_set_ui(xx[0],0);
	mpz_init_set_ui(xx[1],0);
	
	
	gmp_printf("ellmult x %Zd s %Zd b %Zd n %Zd\n", x, s, b, n);
	ellmult(xx, x, s, b, n);
	
	gmp_printf("xx[0] %Zd xx[1] %Zd\n", xx[0], xx[1]);
	
	return 0;
}




