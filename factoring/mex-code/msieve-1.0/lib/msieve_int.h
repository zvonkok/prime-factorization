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

#ifndef _MSIEVE_INT_H_
#define _MSIEVE_INT_H_

/* An implementation of the Self-Initializing Multiple 
   Polynomial Quadratic Sieve algorithm for integer 
   factorization. */

#include <msieve.h>
#include <mp.h>

/*------------------- SIEVE RELATED DECLARATIONS ---------------------*/

/* All of the cache blocking that the sieve performs boils
   down to the following number. This should be a power of two
   and should probably be the size of your L1 cache. For
   certain platforms the code will attempt to determine this
   number automatically */

#define DEFAULT_SIEVE_BLOCK_SIZE 65536

/* There's a limit to how many factors can contribute to
   SIQS polynomials */

#define MAX_POLY_FACTORS 16

/* Structure representing one prime in the factor base */

typedef struct {
	uint32 prime : 24;	/* the factor base prime */
	uint8 logprime;		/* log2(prime)rounded to nearest */
	uint32 root1;	        /* the two square roots of an MPQS */
	uint32 root2;	        /* polynomial (-1 means don't use) */
	uint32 modsqrt;	        /* a square root of n mod prime */
	uint32 recip;
} fb_t;

/* A packed representation of the above structure, used
   during the sieving phase only. */

typedef struct {
	uint32 prime : 24;	/* the factor base prime */
	uint8 logprime;		/* log2(prime)rounded to nearest */
	uint32 next_loc1;	/* the two next sieve locations for */
	uint32 next_loc2;	/*	the above two roots */
} packed_fb_t;

/* Offset 0 in the array of factor base primes is reserved
   for the "prime" -1. This allows the sieve to be over both
   positive and negative values. When actually dividing by
   factor base primes, start with the fb_t at this position in
   the factor base array: */

#define MIN_FB_OFFSET 1

/* structure representing one MPQS polynomial. Every
   relation found by the sieve must be associated with
   (i.e. must refer to) one of these. For sieve offset
   x, the MPQS polynomial is a * x + b */

typedef struct poly_t {
	uint32 a_idx;		/* offset into a list of 'a' values */
	mp_t b;			/* the MPQS 'b' value */
} poly_t;

/* If a sieve value turns out to be smooth, the following is used
   to record information for use in later stages of the algorithm.
   One sieve value is packed into a relation_t, and a relation_list_t
   contains a collection of sieve values */

#define POSITIVE 0
#define NEGATIVE 1

typedef struct {
	uint32 sieve_offset;	/* value of x in a*x+b */
	uint32 poly_idx;	/* pointer to this relation's MPQS poly */
	uint32 num_factors;	/* size of list of factors */
	uint32 large_prime[2];	/* for partial relations, the leftover 
					factor(s). Either factor may equal 1 */
	uint32 *fb_offsets;	/* The array of offsets into the factor base
					which contain primes dividing this
					sieve value. Duplicate offsets are
					possible. Sorted in ascending order */
} relation_t;

typedef struct {
	uint32 num_relations;	/* number of relations in the list */
	relation_t **list;	/* list of relations */
} relation_list_t;

typedef struct {
	uint32 num_cycles;	/* number of cycles in the list */
	relation_list_t *list;	/* each cycle gets one relation list */
} cycle_list_t;

/* Once the sieving stage has collected enough relations,
   we'll need a few extra relations to make sure that the
   linear algebra stage finds linear dependencies */

#define NUM_EXTRA_RELATIONS 64

/* The sieving phase uses a hashtable to handle the large
   factor base primes. The hashtable is an array of type
   hashtable_t, each entry of which contains an array 
   of type hash_entry_t */

typedef struct {
	uint32 logprime : 8;       /* the logprime field from the factor base */
	uint32 prime_index : 24;   /* factor base array offset for entry */
	uint32 sieve_offset;       /* offset within one hash bin */
} hash_entry_t;

typedef struct {
	uint32 num_alloc;     /* total list size this hashtable position */
	uint32 num_used;      /* number occupied at this hashtable position */
	hash_entry_t *list;   /* list of entries at this hashtable position */
} hashtable_t;

/* Configuration of the sieving code requires passing the
   following structure */

typedef struct {
	uint32 bits;       /* size of integer this config info applies to */
	uint32 fb_size;    /* number of factor base primes */
	uint32 large_mult; /* the large prime multiplier */
	uint32 sieve_size; /* the size of the sieve (actual sieve is 2x this) */
} sieve_param_t;

/* Factor a number up to 62 bits in size using the SQUFOF
   algorithm. Returns zero if the factorization failed for 
   whatever reason, otherwise returns one factor up to 31 bits.
   Note that the factor returned may be 1, indicating a
   trivial factorization you probably don't want */

uint32 squfof(mp_t *n);

/* The sieving code needs a special structure for determining
   the number of cycles in a collection of partial relations */

#define LOG2_CYCLE_HASH 22

typedef struct {
	uint32 next;
	uint32 prime;
	uint32 data;
	uint32 count;
} cycle_t;

/* To avoid huge parameter lists passed between sieving
   routines, all of the relevant data used in the sieving
   phase is packed into a single structure. Routines take
   the information they need out of this. */

typedef struct {
	msieve_obj *obj;     /* object controlling entire factorization */

	mp_t *n;             /* the number to factor (scaled by multiplier)*/
	uint32 multiplier;   /* small multiplier for n (may be composite) */
	uint8 *sieve_array;  /* scratch space used for one sieve block */
	uint32 sieve_block_size; /* the allocated size of the above array */
	uint32 poly_block;       /* number of FB primes in a block of work */
	uint32 fb_block;     /* number of polynomials simultaneously sieved */
	uint32 log2_sieve_block_size;  /* log2(sieve_block_size) */
	fb_t *factor_base;       /* the factor base to use */
	packed_fb_t *packed_fb;  /* scratch space for a packed version of it */
	uint32 fb_size;          /* number of factor base primes (includes -1)*/
	uint32 sieve_fb_start;  /* starting FB offset for sieving */
	uint32 tiny_fb_size;    /* FB offset below which remainder ops used */
	uint32 small_fb_size;   /* FB offsets between tiny_fb_size and 
	                           small_fb_size use reciprocals for trial fac-
				   toring, use packed_fb for sieving, and do
				   not use blocking. FB offsets above this
				   number use hashtables for sieving and for
				   trial factoring */
	hashtable_t *hashtable_plus;  /* hash bins for positive sieve values */
	hashtable_t *hashtable_minus; /* hash bins for negative sieve values */
	uint32 num_sieve_blocks; /* number of sieve blocks in sieving interval*/
	uint32 cutoff1;          /* if log2(sieve value) exceeds this number,
				    a little trial division is performed */
	uint32 cutoff2;          /* if log2(sieve value) exceeds this number,
				    full trial division is performed */
	relation_list_t *full_list;     /* list of full relations */
	relation_list_t *partial_list;  /* list of partial relations */
	cycle_list_t *cycle_list;   /* cycles derived from partial relations */

	/* bookkeeping information for creating polynomials */

	mp_t target_a;            /* optimal value of 'a' */
	uint32 a_bits;            /* required number of bits in the 'a' value 
	                                          of all MPQS polynomials */

	uint32 total_poly_a;    /* total number of polynomial 'a' values */
	mp_t *poly_a_list;        /* list of 'a' values for MPQS polys */
	poly_t *poly_list;   /* list of MPQS polynomials */

	mp_t curr_a;	  	  /* the current 'a' value */
	mp_t *curr_b;             /* list of all the 'b' values for that 'a' */
	uint32 *root_correction;  /* see poly.c */
	uint8 *next_poly_action;  /* see poly.c */

	uint32 num_poly_factors;  /* number of factors in poly 'a' value */
	uint32 factor_bounds[25]; /* bounds on FB offsets for primes that */
	                          /*        can be factors of MPQS polys */	
	uint32 poly_factors[MAX_POLY_FACTORS];  /* factorization of curr. 'a' */
	uint8 factor_bits[MAX_POLY_FACTORS]; /* size of each factor of 'a' */
	mp_t poly_tmp_b[MAX_POLY_FACTORS];  /* temporary quantities */
	uint32 *poly_b_array;      /* precomputed values for all factor base
	                              primes, used to compute new polynomials */
	uint32 *poly_b_small[MAX_POLY_FACTORS];

	/* bookkeeping information for double large primes */

	uint32 large_prime_max;  /* the cutoff value for keeping a partial
				    relation; actual value, not a multiplier */
	mp_t max_fb2;          /* the square of the largest factor base prime */
	mp_t large_prime_max2; /* the cutoff value for factoring partials */

	cycle_t *cycle_table;      /* list of all the vertices in the graph */
	uint32 cycle_table_size;   /* number of vertices filled in the table */
	uint32 cycle_table_alloc;  /* number of cycle_t structures allocated */
	uint32 *cycle_hashtable;   /* hashtable to index into cycle_table */
	uint32 components;         /* connected components (see relation.c) */
	uint32 vertices;           /* vertices in graph (see relation.c) */

} sieve_conf_t;

/* rmember a factor that was just found */

void add_next_factor(msieve_obj *obj, mp_t *n, 
			enum msieve_factor_type factor_type);

/* savefile-related functions */

/* pull out the large primes from a relation read from
   the savefile */

void read_large_primes(char *buf, uint32 *prime1, uint32 *prime2);

/* given the primes from a sieve relation, add
   that relation to the graph used for tracking
   cycles */

void add_to_cycles(sieve_conf_t *conf, 
			uint32 large_prime1, 
			uint32 large_prime2);

/* encapsulate all of the information conerning a sieve
   relation and dump it to the savefile */

void save_relation(sieve_conf_t *conf,
		  uint32 sieve_offset, 
		  uint32 *fb_offsets, 
		  uint32 num_factors, 
		  relation_list_t *list, 
		  uint32 poly_index,
		  uint32 large_prime1,
		  uint32 large_prime2);

/* the savefile is manually buffered */

void print_to_savefile(msieve_obj *obj, char *buf);
void flush_savefile(msieve_obj *obj);

/* emit logging information */

void logprintf(msieve_obj *obj, char *fmt, ...);

/* perform postprocessing on a list of relations */

void filter_relations(sieve_conf_t *conf);

/* Initialize/free polynomial scratch data. Note that
   poly_free can only be called after the relation
   filtering phase */

void poly_init(sieve_conf_t *conf, uint32 sieve_size);
void poly_free(sieve_conf_t *conf);

/* compute a random polynomial 'a' value, and also
   compute all of the 'b' values, all of the precomputed
   quantities for the 'b' values, and all of the initial
   roots in the factor base */

void build_base_poly(sieve_conf_t *conf);

/* As above, except it's assumed that conf->poly_factors
   and conf->curr_a are already filled in. Factor base roots
   are not affected */

void build_derived_poly(sieve_conf_t *conf);

/* The main function to perform sieving.
	obj is the object controlling this factorization
	n is the number to be factored
	poly_list is a linked list of all the polynomials created
	factor_base contains the factor base (duh)
	params is used to initialize and configure the sieve code
	multiplier is a small integer by which n is scaled
	sieve_block_size gives the size of one cache block
	full_relations contains all the smooth sieve values
		that the sieving stage has found
	partial_relations contains all the sieve values that are
		one factor away from being smooth. The list is
		sorted in order of increasing leftover prime */

void do_sieving(msieve_obj *obj, mp_t *n, mp_t **poly_a_list, 
		poly_t **poly_list, fb_t *factor_base, 
		sieve_param_t *params, 
		uint32 multiplier,
		uint32 sieve_block_size,
		relation_list_t **full_relations,
		relation_list_t **partial_relations,
		cycle_list_t **cycle_list);

void free_relation_list(relation_list_t *list);
void free_cycle_list(cycle_list_t *list);

/*--------------LINEAR ALGEBRA RELATED DECLARATIONS ---------------------*/

/* Structures used during the linear algebra stage */

/* A column of the matrix represents one sieve relation */

typedef struct col_t {
	uint32 weight;		/* Number of nonzero entries in this column */
	uint32 *data;		/* The list of occupied rows in this column */
	relation_t *r0;		/* Sieve relation corresponding to this 
					column. For partials, r0 = NULL */
	relation_list_t *r1;	/* A list of partial relations. For full
					relations, r1 = NULL */
} la_col_t;

/* Find linear dependencies.
	obj is the object controlling this factorization
	fb_size is the size of the factor base
	vectors is the list of relations that the linear algebra
		code constructs
	vsize the size of the list of vectors. The linear algebra
		code chooses this number; the only guarantee is that
		it is at least fb_size + NUM_EXTRA_RELATIONS
	bitfield is an array of vsize numbers. Bit i of word j tells
		whether vectors[j] is used in nullspace vector i. 
		Essentially, bitfield[] is a collection of 32 nullspace
		vectors packed together. Changing this to some other number
		like 64 is pretty easy.
	The last two arguments are the relation lists returned from
		the sieving stage */

void solve_linear_system(msieve_obj *obj, uint32 fb_size, 
		    la_col_t **vectors, uint32 *vsize,
		    uint64 **bitfield, 
		    relation_list_t *full_relations, 
		    cycle_list_t *cycle_list);

/*-------------- MPQS SQUARE ROOT RELATED DECLARATIONS ---------------------*/

void find_factors(msieve_obj *obj, mp_t *n, fb_t *factor_base, 
		uint32 fb_size, la_col_t *vectors, uint32 vsize, 
		uint64 *null_vectors, uint32 multiplier,
		mp_t *a_list, poly_t *poly_list);

#endif /* _MSIEVE_INT_H_ */
