/*
 * File: trialdivision_mex.c 
 * Version: 0.2 
 *
 * Created: 1.05.2005
 * Last update: 9.05.2005
 *
 * Autor: Zvonko Krnjajic
 *
 * Dependencies: gmp
 *
 * Description: Uses Trialdivision method to factor a large integer 
 *
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <mex.h>
#include <signal.h>
#include "gmp.h"

#define MAX_FACTORS 100

enum  { NONE = 0, BYTESTREAM, STRING } datatype;

/* Print usage and return to MATLAB */
void print_help() {
	mexPrintf("trialdivision uses a brute force method to factor N\n\n");
	mexPrintf("Usage 1: uint8(factor) = trialdivision(uint8(N))\n");
	mexPrintf("Usage 2: char(factor)  = trialdivision(char(N))\n\n");
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


/* A brute-force method of finding a divisor of an 
 * integer N by simply plugging in one or a set of 
 * integers (prime list, generated primes, ...) 
 * and seeing if they divide N. */
void trialdivision(mpz_t factor, mpz_t N) {
	
	mpz_t square;
	mpz_init(square);
	
	mpz_set_ui(factor,2);
	
	mpz_mul(square, factor, factor);
	/* This is done until factor^2 is smaller 
	 * then N */
	while(mpz_cmp(square, N) < 0) {
		
		if (mpz_divisible_p(N, factor) != 0) {
			return;
		}
		/* To get the next prime the function mpz_nextprime() 
		 * is used. This function uses a probabilistic algorithm
		 * to identify primes. Alternatively the Ertosthenes sieve
		 * can be used to generate primes. */
		mpz_nextprime(factor, factor); 
		mpz_mul(square, factor, factor);
		
		
		/* im still alife */
		/*vitalsign();*/
	}

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
			
			/* Create Matrix for the return arguments*/ 
			dims[0]=1;
			dims[1]=size;
			/* Allocate the heap for the result */
			uint8factor = mxCreateNumericArray(2,(const *)&dims,mxUINT8_CLASS,mxREAL);
			if( uint8factor == NULL )
				mexErrMsgTxt("C'ant create uint8 from rop.");		
		
			/* Get the result pointer */
			ptr_plhs = (char*)mxGetPr(uint8factor);
		
			/* Copy Data to the result pointer */
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
	
	trialdivision(factor, N);
	
	dofactoring(factor);
	mpz_divexact(N, N, factor);
	dofactoring(N);
}

void handle_signal(int sig) {

	mexPrintf("\nreceived signal %d; shutting down\n", sig);
	
/*	if (obj && (obj->flags & MSIEVE_FLAG_SIEVING_IN_PROGRESS))
		obj->flags |= MSIEVE_FLAG_STOP_SIEVING;
	else
		_exit(0);*/
}

/* The gataway function between MATLAb and C */
void mexFunction(int nlhs, mxArray *plhs[], 
	         int nrhs, const mxArray *prhs[]) { 

	mpz_t N;
	mpz_init(N);
	
	int status;
	
	
	
	if (signal(SIGINT, handle_signal) == SIG_ERR) {
	        mexErrMsgTxt("could not install handler on SIGINT\n");
	        
	}
	if (signal(SIGTERM, handle_signal) == SIG_ERR) {
	        mexErrMsgTxt("could not install handler on SIGTERM\n");
	        
	}     
	
	
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
			
			/* Create Matrix for the return arguments */ 
			dims[0]=1;
			dims[1]=size;
			/* Allocate the heap for the result */
			plhs[0] = mxCreateNumericArray(2,(const *)&dims,mxUINT8_CLASS,mxREAL);
			if( plhs[0] == NULL )
				mexErrMsgTxt("C'ant create uint8 from rop.");		
		
			/* Get the result pointer */
			ptr_plhs = (char*)mxGetPr(plhs[0]);
		
			/* Copy Data to the result pointer */
			for(i=0;i<size;i++)
				*(ptr_plhs+i) = *(str+i);
			
			mpz_clear(N);
			nlhs = 1;
			return;
		
		}

	}
	
	
	dofactoring(N);
	
	mexPrintf("count  %d", count);/*debug */
	
	/* remove trailing cells we just want the
	 * factors to show up as the result */
	mxSetN(factors, count);
	plhs[0] = factors;
}

