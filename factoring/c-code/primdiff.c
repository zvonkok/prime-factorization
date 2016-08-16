#include <stdio.h>
#include <gmp.h>

#define DIFF_FILE  "primdiff.dat"



void primdiff(mpz_t N) {
	
	int i;
	
	mpz_t count, prime, prime0, diff, maxdiff;
	
	mpz_init(count);
	mpz_init(prime);
	mpz_init(prime0);
	mpz_init(diff);
	mpz_init(maxdiff);
	
	size_t bytes; 
	
	FILE * fp;
	
	fp = fopen("primdiff.dat", "wb");
	
	if (fp == NULL) {
		printf("Fehler beim oeffnen von DIFF_FILE\n");
	}
	
	mpz_set_ui(prime0, 3); 
	mpz_set_ui(prime, 5); 
	mpz_set_ui(count, 0);
	mpz_set_ui(maxdiff, 0);
	
	while (mpz_cmp(prime0, N) < 0 ) {
		mpz_add_ui(count, count, 1);
		mpz_sub(diff, prime, prime0);
		if (mpz_cmp(diff, maxdiff) > 0) {
			mpz_set(maxdiff, diff);
			gmp_printf("%Zd : %Zd\n",diff, prime);
		}
		mpz_div_ui(diff, diff, 2);
		i = mpz_get_ui(diff);
		
		fprintf(fp, "%d ", i);
		
		
		mpz_set(prime0, prime);
		mpz_nextprime(prime, prime0);
	}
	
	fclose(fp);
	
#if 0
	fp = fopen(DIFF_FILE, "r");
	if (fp == NULL) {
		printf("Fehler beim oeffnen von DIFF_FILE\n");
	}
	
	while (bytes != EOF) {
		bytes = fscanf(fp, "%d ", &i);
		
		if (bytes == EOF) {
			return 0;
		} else {
			printf("%d", i);
		}
	}
#endif 
	
}



int main(void) {
	mpz_t N;
	mpz_init_set_ui(N, 1000000);
	gmp_printf("N %Zd\n", N);
	primdiff(N);
	return 0;
}
