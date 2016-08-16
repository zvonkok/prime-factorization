/*
 * Dateiname: trialdivision.c 
 * Versions/Build-nummer: 0.2 
 *
 * Erstellungsdatum: 1.05.2005
 * Datum der letzten Änderung: 9.05.2005
 *
 * Autor: Zvonko Krnjajic
 *
 * Dependencies: gmp
 *
 * Zweck des Programms: Uses Trialdivision 
 *                      to factor a large integer 
 *
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>


int main(void) {
	
	char * last;
	int i = 0;
	int len = 0;
	
	mpz_t num;
	mpz_init(num);
	mpz_set_ui(num,2);

	while (i < 100) {
		mpz_get_str(last, 10, num);
		len = strlen(last);
		printf("LAST  %s",last[len-1]);
		mpz_set_str(num, last, 10);
		mpz_add_ui(num, num, 1);
	}
	
	
	return 0;
}


