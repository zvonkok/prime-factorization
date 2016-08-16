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
	
	mpz_t num, rop, op1, op2;
	mpz_init(num);
	mpz_init(rop);
	mpz_init(op1);
	mpz_init(op2);
	mpz_set_ui(num,2047);
	mpz_set_ui(rop,0);
	mpz_set_ui(op1,2048);
	mpz_set_ui(op2,66420900);
	mpz_invert(rop, op1, op2);

	gmp_printf("Inverse: %Zd", rop);	
	return 0;
}


