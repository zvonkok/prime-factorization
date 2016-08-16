
MATH = -lm
LIBS = -I/opt/local/include -L/opt/local/lib -lgmp
CFLAGS = -g -Wall 
OBJECTS = probedivision fermat pollard_rho pollard_p1


factoring_algorithms:	ecm williams_pp1 pollard_rho trialdivision fermat pollard_p1 test
			

test:		test.c modular.c
		gcc -o test test.c ppexpo.c mpz_min_max.c mpz_lls_mod.c $(CFLAGS) $(LIBS)
		gcc -o modular modular.c $(CFLAGS) $(LIBS)
		
			
trialdivision:	trialdivision.c
		gcc -o trialdivision trialdivision.c $(CFLAGS) $(LIBS)


fermat:	fermat.c 
	gcc  -o fermat   fermat.c $(CFLAGS) $(LIBS)
	

pollard_rho:	pollard_rho.c pollard_rho_engine.c
		gcc -o pollard_rho pollard_rho.c \
		pollard_rho_engine.c $(CFLAGS) $(LIBS) 
		
pollard_p1:	pollard_p1.c ppexpo.c mpz_min_max.c
		gcc -o pollard_p1  pollard_p1.c ppexpo.c mpz_min_max.c $(CFLAGS) $(LIBS) $(MATH)
		
williams_pp1:	williams_pp1.c
		gcc -o williams_pp1 williams_pp1.c ppexpo.c \
		mpz_min_max.c mpz_lls_mod.c $(CFLAGS) $(LIBS) 

ecm:		ecm.c
		gcc -o ecm ecm.c mpz_min_max.c ppexpo.c $(CFLAGS) $(LIBS)
		
clean:		
		rm probedivision fermat pollard_rho pollard_p1 \
			williams_pp1 siqs_siever sample_siqs test ecm
