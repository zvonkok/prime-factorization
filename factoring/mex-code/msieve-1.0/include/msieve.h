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

#ifndef _MSIEVE_H_
#define _MSIEVE_H_

#ifdef __cplusplus
extern "C" {
#endif

	/* Lightweight factoring API */

#include <util.h>
#include <mp.h>

/* version info */

#define MSIEVE_MAJOR_VERSION 1
#define MSIEVE_MINOR_VERSION 0

/* The final output from the factorization is a linked
   list of msieve_factor structures, one for each factor
   found. */

enum msieve_factor_type {
	MSIEVE_COMPOSITE,
	MSIEVE_PRIME,
	MSIEVE_PROBABLE_PRIME
};

typedef struct msieve_factor {
	enum msieve_factor_type factor_type;
	char *number;
	struct msieve_factor *next;
} msieve_factor;

/* These flags are used to 'communicate' status and
   configuration info back and forth to a factorization
   in progress */

enum msieve_flags {
	MSIEVE_DEFAULT_FLAGS = 0,		/* just a placeholder */
	MSIEVE_FLAG_USE_LOGFILE = 0x01,	    /* append log info to a logfile */
	MSIEVE_FLAG_LOG_TO_STDOUT = 0x02,   /* print log info to the screen */
	MSIEVE_FLAG_STOP_SIEVING = 0x04,    /* tell library to stop sieving
					       when it is safe to do so */
	MSIEVE_FLAG_FACTORIZATION_DONE = 0x08,  /* set by the library if a
						   factorization completed */
	MSIEVE_FLAG_SIEVING_IN_PROGRESS = 0x10  /* set by the library when 
						   any sieving operations are 
						   in progress */
};
	
/* One factorization is represented by a msieve_obj
   structure. This contains all the static information
   that gets passed from one stage of the factorization
   to another. If this was C++ it would be a simple object */

typedef struct {
	char *input;		  /* pointer to string version of the 
				     integer to be factored */
	msieve_factor *factors;   /* linked list of factors found (in
				     ascending order */
	uint32 flags;		  /* input/output flags */
	char *savefile_name;      /* name of the savefile that will be
				     used for this factorization */
	FILE *savefile;		  /* current state of savefile */
	char *logfile_name;       /* name of the logfile that will be
				     used for this factorization */
	FILE *logfile;	          /* current state of logfile */
	uint32 seed1, seed2;      /* current state of random number generator
				     (updated as random numbers are created) */
	char *savefile_buf;       /* circular buffer for savefile output */
	uint32 savefile_buf_off;   /* current offset into savefile buffer */
	time_t timestamp;          /* number of seconds factorization took */
	char mp_sprintf_buf[32 * MAX_MP_WORDS+1]; /* scratch space for 
						printing big integers */
} msieve_obj;

msieve_obj * msieve_obj_new(char *input_integer,
			    uint32 flags,
			    char *savefile_name,
			    char *logfile_name,
			    uint32 seed1,
			    uint32 seed2);

msieve_obj * msieve_obj_free(msieve_obj *obj);

void msieve_run(msieve_obj *obj);
				
#define MSIEVE_DEFAULT_LOGFILE "msieve.log"
#define MSIEVE_DEFAULT_SAVEFILE "msieve.dat"

#ifdef __cplusplus
}
#endif

#endif /* _MSIEVE_H_ */
