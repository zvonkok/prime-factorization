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
#include <mex.h>
#include <time.h>
#include "gmp.h"

#define TRUE 1
#define COUNT 65535  /* How many tries */


#define MAX_FACTORS 100

enum  { NONE = 0, BYTESTREAM, STRING } datatype;

/* Print usage and return to MATLAB */
void print_help() {
	mexPrintf("pollard_rho uses the Pollard-Rho method to factor N\n\n");
	mexPrintf("Usage 1: uint8(factor) = pollard_rho(uint8(N))\n");
	mexPrintf("Usage 2: char(factor)  = pollard_rho(char(N))\n\n");
}

/* do not exceed the array, so count the factors we get */
int count;
	
/* return value to MATLAB */
mxArray * factors;


int sign = 0;
/* vitalsign prints a dot to symbolize that
 * the method is still working */
void vitalsign() {
	if (++sign > 1000) {
		mexPrintf(".");
		sign = 0;
	}
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
	
	if (mpz_cmp_ui(N, 1) <= 0)
		return;
	
	/* mpz_probab_prime_p does some trial divisions, 
	 * then some Miller-Rabin probabilistic primality 
	 * tests to determine if N is a prime */
	if (mpz_probab_prime_p(N, 10) != 0) {
		
		/* if we found a factor return it to matlab
		 * as a string or a bytestream */
		if (datatype == STRING) {
			
			char * str = NULL; 
			str = mpz_get_str(NULL, 10, N);
			if (str == NULL) {
				mexErrMsgTxt("C'ant convert the rop to a string with base 10.");
			}
			if (count < MAX_FACTORS) {
				mxSetCell(factors, count, mxCreateString(str));
				++count;
				return;
			} else {
				mexPrintf(" 100 Factors ..."); return;
			}
			
		} else if (datatype == BYTESTREAM) {
			
			char * str = NULL;
			char * ptr_plhs = NULL;
			int size = 0;
			int dims[2];
			int i = 0;
			
			mxArray * uint8factor; 
			
			size = mpz_out_rawm(&str,N);
			if ( size <= 0 )
				mexErrMsgTxt("C'ant create bytestream from rop.");
			
			// Create Matrix for the return arguments 
			dims[0]=1;
			dims[1]=size;
			// Allocate the heap for the result
			uint8factor = mxCreateNumericArray(2,(const *)&dims,mxUINT8_CLASS,mxREAL);
			if( uint8factor == NULL )
				mexErrMsgTxt("C'ant create uint8 from rop.");		
		
			// Get the result pointer
			ptr_plhs = (char*)mxGetPr(uint8factor);
		
			// Copy Data to the result pointer
			for(i=0;i<size;i++)
				*(ptr_plhs+i) = *(str+i);
			
			if (count < MAX_FACTORS) {
				mxSetCell(factors, count, uint8factor);
				++count;
				return;
			} else {
				mexPrintf(" 100 Factors ..."); return;
			}
		}
		
	}
	
	pollard_rho(factor, N);
	
	dofactoring(factor);
	mpz_divexact(N, N, factor);
	dofactoring(N);
}



/* The gataway function between MATLAb and C */
void mexFunction(int nlhs, mxArray *plhs[], 
	         int nrhs, const mxArray *prhs[]) { 

	mpz_t N;
	mpz_init(N);
	
	int status;
	
	/* we have only one return value (cell array) */
	factors = mxCreateCellMatrix(1, MAX_FACTORS);
	nlhs = 1;
	count = 0;
	
	/* oh dear check for proper number of arguments 
	 * and parse all input values depending if its
	 * a bytestream or string */
	if (nrhs < 1) {
		print_help();
		mexErrMsgTxt("Wrong numger of arguments.");
	}
	
	if (mxIsUint8(prhs[0])){

		char * bytestream = NULL;
		int size;	

		datatype = BYTESTREAM;
		
		bytestream = (char *) mxGetPr(prhs[0]);
		size = mxGetN(prhs[0]);
		
		status = mpz_inp_rawm(N,bytestream,size);
		if (status <= 0 )
			mexErrMsgTxt("C'ant extract N from bytestream.");		
	}
	
	if (mxIsChar(prhs[0])){

		int status;

		datatype = STRING;
		
		status = mpz_init_set_str(N,mxArrayToString(prhs[0]),0);
		if ( status == -1 )
			mexErrMsgTxt("C'ant extract N from string.");

	}

	if (datatype == NONE) {	
		print_help();
		mexErrMsgTxt("Unrecognized datatype.");
	}

	
	/* If N < 3 just return N,  makes
	 * no sense to factor */
	if ((mpz_cmp_ui(N, 3) < 0)) {
		/* output as string otherwise as bytestream */
		if (datatype == STRING) {
			
			char * str = NULL; 	
			str = mpz_get_str(NULL, 10, N);
			if (str == NULL) {
				mexErrMsgTxt("C'ant convert the rop to a string with base 10.");
			}
			plhs[0] = mxCreateString(str);
			if (plhs[0] == NULL) {
				mexErrMsgTxt("C'ant create string from rop.");
			}
			
			mpz_clear(N);
			nlhs = 1;
			return;
			
		} else if (datatype == BYTESTREAM) {
			
			char * str = NULL;
			char * ptr_plhs = NULL;
			int size = 0;
			int dims[2];
			int i = 0;
			
			size = mpz_out_rawm(&str,N);
			if ( size <= 0 )
				mexErrMsgTxt("C'ant create bytestream from rop.");
			
			// Create Matrix for the return arguments 
			dims[0]=1;
			dims[1]=size;
			// Allocate the heap for the result
			plhs[0] = mxCreateNumericArray(2,(const *)&dims,mxUINT8_CLASS,mxREAL);
			if( plhs[0] == NULL )
				mexErrMsgTxt("C'ant create uint8 from rop.");		
		
			// Get the result pointer
			ptr_plhs = (char*)mxGetPr(plhs[0]);
		
			// Copy Data to the result pointer
			for(i=0;i<size;i++)
				*(ptr_plhs+i) = *(str+i);
			
			mpz_clear(N);
			nlhs = 1;
			return;
		
		}

	}
	
	
	dofactoring(N);
	
	mexPrintf("count  %d", count);//debug
	
	/* remove trailing cells we just want the
	 * factors to show up as the result */
	mxSetN(factors, count);
	plhs[0] = factors;
}


