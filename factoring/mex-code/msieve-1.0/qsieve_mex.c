/*--------------------------------------------------------------------
This source distribution is placed in the public domain by its author,
Jason Papadopoulos. You may use it for any purpose, free of charge,
without having to notify anyone. I disclaim any responsibility for any
errors.

Optionally, please be nice and tell me if you find this source to be
useful. Again optionally, if you add to the functionality present here
please consider making those additions public too, so that others may 
benefit from your work.	
       				   --jasonp@boo.net 6/19/05
--------------------------------------------------------------------*/

#include <msieve.h>

msieve_obj *g_curr_factorization = NULL;

/*--------------------------------------------------------------------*/
void handle_signal(int sig) {

	msieve_obj *obj = g_curr_factorization;

	printf("\nreceived signal %d; shutting down\n", sig);
	
	if (obj && (obj->flags & MSIEVE_FLAG_SIEVING_IN_PROGRESS))
		obj->flags |= MSIEVE_FLAG_STOP_SIEVING;
	else
		_exit(0);
}

/*--------------------------------------------------------------------*/
void get_random_seeds(uint32 *seed1, uint32 *seed2) {

	uint32 tmp_seed1, tmp_seed2;

	/* In a multithreaded program, every msieve object
	   should have two unique, non-correlated seeds
	   chosen for it */

#if defined(__linux__)

	/* Yay! Cryptographic-quality nondeterministic randomness! */

	FILE *rand_device = fopen("/dev/urandom", "r");

	if (rand_device != NULL) {
		fread(&tmp_seed1, sizeof(uint32), (size_t)1, rand_device);
		fread(&tmp_seed2, sizeof(uint32), (size_t)1, rand_device);
		fclose(rand_device);
	}
	else

#endif
	{
		/* <Shrug> For everyone else, sample the current time,
		   the high-res timer (hopefully not correlated to the
		   current time), and the process ID. Multithreaded
		   applications should fold in the thread ID too */

		uint64 high_res_time = read_clock();
		tmp_seed1 = ((uint32)(high_res_time >> 32) ^
			     (uint32)time(NULL)) * 
			    (uint32)getpid();
		tmp_seed2 = (uint32)high_res_time;
	}

	/* The final seeds are the result of a multiplicative
	   hash of the initial seeds */

	(*seed1) = tmp_seed1 * ((uint32)40499 * 65543);
	(*seed2) = tmp_seed2 * ((uint32)40499 * 65543);
}

/*--------------------------------------------------------------------*/
void print_usage(char *progname) {

	printf("\nMsieve v. %d.%02d\n", MSIEVE_MAJOR_VERSION, 
					MSIEVE_MINOR_VERSION);

	printf("\nusage: %s [options] [one_number]\n", progname);
	printf("\noptions:\n"
	         "   -s <name> save intermediate results to <name>\n"
		 "             instead of the default %s\n"
	         "   -l <name> append log information to <name>\n"
		 "             instead of the default %s\n"
	         "   -i <name> read one or more integers to factor from\n"
		 "             <name> (default worktodo.ini) instead of\n"
		 "             from the command line\n"
		 "   -m        manual mode: enter numbers via standard input\n"
	         "   -q        quiet: do not generate any log information,\n"
		 "             only print any factors found\n"
	         "   -d <min>  deadline: if still sieving after <min>\n"
		 "             minutes, shut down gracefully (default off)\n"
	         "   -v        verbose: write log information to screen\n"
		 "             as well as to logfile\n\n",
		 MSIEVE_DEFAULT_SAVEFILE, MSIEVE_DEFAULT_LOGFILE);
}

/*--------------------------------------------------------------------*/
void factor_integer(char *buf, uint32 flags,
		    char *savefile_name,
		    char *logfile_name,
		    uint32 *seed1, uint32 *seed2) {
	
	char *int_start, *int_end;
	msieve_obj *obj;

	int_start = buf;
	while (*int_start && !isdigit(*int_start))
		int_start++;
	
	if (*int_start == 0)
		return;
	
	int_end = int_start + 1;
	while (*int_end && isdigit(*int_end))
		int_end++;

	if (int_end - int_start > 125) {
		printf("input integers must be 125 digits or less\n");
		return;
	}

	g_curr_factorization = msieve_obj_new(int_start, flags,
						savefile_name, logfile_name,
						*seed1, *seed2);
	if (g_curr_factorization == NULL) {
		printf("factoring initialization failed\n");
		return;
	}

	msieve_run(g_curr_factorization);

	if (!(g_curr_factorization->flags & MSIEVE_FLAG_FACTORIZATION_DONE)) {
		printf("\ncurrent factorization was interrupted\n");
		exit(0);
	}

	/* If no logging is specified, at least print out the
	   factors that were found */

	if (!(g_curr_factorization->flags & (MSIEVE_FLAG_USE_LOGFILE |
					MSIEVE_FLAG_LOG_TO_STDOUT))) {
		msieve_factor *factor = g_curr_factorization->factors;

		printf("\n");
		while (factor != NULL) {
			char *factor_type;

			if (factor->factor_type == MSIEVE_PRIME)
				factor_type = "p";
			else if (factor->factor_type == MSIEVE_COMPOSITE)
				factor_type = "c";
			else
				factor_type = "prp";

			printf("%s%d: %s\n", factor_type, 
					(int32)strlen(factor->number), 
					factor->number);
			factor = factor->next;
		}
		printf("\n");
	}

	/* save the current value of the random seeds, so that
	   the next factorization will pick up the pseudorandom
	   sequence where this factorization left off */

	*seed1 = g_curr_factorization->seed1;
	*seed2 = g_curr_factorization->seed2;

	/* avoid a race condition in the signal handler */

	obj = g_curr_factorization;
	g_curr_factorization = NULL;
	if (obj)
		msieve_obj_free(obj);
}

#ifdef WIN32
DWORD WINAPI countdown_thread(LPVOID pminutes) {
	DWORD minutes = *(DWORD *)pminutes;

	if (minutes > 0x7fffffff / 60000)
		minutes = 0;            /* infinite */

	Sleep(minutes * 60000);
	raise(SIGINT);
	return 0;
}

#else
void *countdown_thread(void *pminutes) {
	uint32 minutes = *(uint32 *)pminutes;

	if (minutes > 0xffffffff / 60)
		minutes = 0xffffffff / 60;   /* infinite */

	sleep(minutes * 60);
	raise(SIGINT);
	return NULL;
}
#endif

/*--------------------------------------------------------------------*/
int main(int argc, char **argv) {

	char buf[200];
	uint32 seed1, seed2;

	uint32 flags;


	int i;
	int32 deadline = 0;

	if (signal(SIGINT, handle_signal) == SIG_ERR) {
	        printf("could not install handler on SIGINT\n");
	        return -1;
	}
	if (signal(SIGTERM, handle_signal) == SIG_ERR) {
	        printf("could not install handler on SIGTERM\n");
	        return -1;
	}     


	i = 1;
	buf[0] = 0;
	get_random_seeds(&seed1, &seed2);

/*	if (deadline) {
#ifdef WIN32
		DWORD thread_id;
		CreateThread(NULL, 0, countdown_thread, 
				&deadline, 0, &thread_id);
#else
		pthread_t thread_id;
		pthread_create(&thread_id, NULL, 
				countdown_thread, &deadline);
#endif
	}*/
	if (isdigit(argv[i][0]))
				strncpy(buf, argv[i], sizeof(buf));
			
	flags &= ~(MSIEVE_FLAG_USE_LOGFILE | MSIEVE_FLAG_LOG_TO_STDOUT);

	if (isdigit(buf[0])) {
		factor_integer(buf, flags, savefile_name, 
				logfile_name, &seed1, &seed2);
	}

	return 0;
}
