
MATH = -lm
LIBS = -lgmp
CFLAGS = -g -Wall -pg
OBJECTS = trialdivision 



mex-files:	${OBJECTS}

trialdivision:	trialdivision_mex.c
		mex -output trialdivision trialdivision_mex.c -lgmp

fermat:		fermat_mex.c 
		mex  -output fermat  fermat_mex.c 
	

pollard_rho:	pollard_rho.c pollard_rho_engine.c
		mex -output pollard_rho pollard_rho_mex.c pollard_rho_engine.c 
		
pollard_p1:	pollard_p1_mex.c ppexpo.c mpz_min_max.c
		mex -output pollard_p1  pollard_p1_mex.c ppexpo.c mpz_min_max.c 
		
williams_pp1:	williams_pp1_mex.c
		mex -output williams_pp1 williams_pp1_mex.c ppexpo.c mpz_min_max.c mpz_lls_mod.c 

ecm:		ecm.c
		mex -output ecm ecm.c 
		
clean:		
		rm *.dll
