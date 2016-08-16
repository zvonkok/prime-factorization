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

#include "msieve_int.h"

sieve_param_t prebuilt_params[] = {
	{64, 100, 40, 1 * 65536},
	{128, 500, 40, 1 * 65536},
	{183, 2000, 40, 1 * 65536},
	{200, 3000, 50, 3 * 65536},
	{212, 5400, 100, 3 * 65536},
	{233, 11000, 80, 3 * 65536},
	{249, 30000, 100, 3 * 65536},
	{266, 50000, 100, 3 * 65536},

	{283, 55000, 80, 3 * 65536},
	{298, 60000, 80, 3 * 65536},
	{315, 65000, 150, 3 * 65536},
	{332, 70000, 300, 3 * 65536},
	{348, 75000, 300, 3 * 65536},
	{363, 85000, 300, 3 * 65536},
	{379, 95000, 300, 3 * 65536},
	{395,105000, 300, 3 * 65536},
	{415,120000, 300, 3 * 65536},
};

	/* For big factorizations, some people have reported errors
	   that indicate that the savefiles were corrupted. I have
	   sometimes noticed (in other programs) that high-speed 
	   formatted output to a text file will occaisionally get 
	   corrupted.
	
	   Hence, all writes to the savefile are manually buffered,
	   so that disk writes are always in large blocks that get
	   flushed immediately. */

#define SAVEFILE_BUF_SIZE 65536

typedef struct {
	uint32 *list;
	uint32 num_primes;
} prime_list_t;


static uint32 get_sieve_block_size(msieve_obj *obj);

static void get_sieve_params(uint32 bits, 
			     sieve_param_t *params);

static fb_t * build_factor_base(msieve_obj *obj, 
				mp_t *n, 
				prime_list_t *prime_list,
				uint32 *fb_size, 
				uint32 *multiplier);

static fb_t * build_one_factor_base(msieve_obj *obj, 
				    mp_t *n, 
				    prime_list_t *prime_list, 
				    uint32 *fb_size);

static void do_trial_factor(msieve_obj *obj, 
			    prime_list_t *prime_list, 
			    mp_t *n);

static void fill_prime_list(prime_list_t *prime_list, 
				uint32 fb_size);

/*--------------------------------------------------------------------*/
msieve_obj * msieve_obj_new(char *input_integer, uint32 flags,
			    char *savefile_name, char *logfile_name,
			    uint32 seed1, uint32 seed2) {

	msieve_obj *obj = (msieve_obj *)calloc((size_t)1, sizeof(msieve_obj));

	if (obj == NULL)
		return obj;
	
	obj->input = input_integer;
	obj->flags = flags;
	obj->seed1 = seed1;
	obj->seed2 = seed2;
	obj->savefile_name = MSIEVE_DEFAULT_SAVEFILE;
	if (savefile_name)
		obj->savefile_name = savefile_name;
	obj->logfile_name = MSIEVE_DEFAULT_LOGFILE;
	if (logfile_name)
		obj->logfile_name = logfile_name;
	
	obj->savefile_buf = (char *)malloc((size_t)SAVEFILE_BUF_SIZE);
	if (obj->savefile_buf == NULL) {
		free(obj);
		return NULL;
	}
	return obj;
}

/*--------------------------------------------------------------------*/
msieve_obj * msieve_obj_free(msieve_obj *obj) {

	msieve_factor *curr_factor;

	curr_factor = obj->factors;
	while (curr_factor != NULL) {
		msieve_factor *next_factor = curr_factor->next;
		free(curr_factor->number);
		free(curr_factor);
		curr_factor = next_factor;
	}

	free(obj->savefile_buf);
	free(obj);
	return NULL;
}

/*--------------------------------------------------------------------*/
void msieve_run(msieve_obj *obj) {

	char *n_string;
	uint32 i, j;
	mp_t n;
	uint32 bits, fb_size;
	prime_list_t prime_list;
	fb_t *factor_base;
	sieve_param_t params;
	relation_list_t *full_relations = NULL;
	relation_list_t *partial_relations = NULL;
	cycle_list_t *cycle_list = NULL;
	poly_t *poly_list = NULL;
	mp_t *poly_a_list = NULL;
	la_col_t *vectors;
	uint64 *bitfield;
	uint32 vsize;
	uint32 multiplier;

	mp_str2mp(obj->input, &n, 10);
	n_string = mp_sprintf(&n, 10, obj->mp_sprintf_buf);

	logprintf(obj, "\n");
	logprintf(obj, "\n");
	logprintf(obj, "Msieve v. %d.%02d\n", MSIEVE_MAJOR_VERSION, 
					MSIEVE_MINOR_VERSION);
	obj->timestamp = time(NULL);
	if (obj->flags & MSIEVE_FLAG_LOG_TO_STDOUT) {
		printf("%s", ctime(&obj->timestamp));
	}

	logprintf(obj, "random seeds: %08x %08x\n", obj->seed1, obj->seed2);
	logprintf(obj, "factoring %s (%d digits)\n", 
				n_string, strlen(n_string));

	if (mp_is_zero(&n)) {
		add_next_factor(obj, &n, MSIEVE_PRIME);
		obj->flags |= MSIEVE_FLAG_FACTORIZATION_DONE;
		return;
	}

	/* Calculate an initial estimate of the factor base size */

	bits = mp_bits(&n);
	get_sieve_params(bits, &params);
	if (params.fb_size < 100)
		params.fb_size = 100;

	/* Do trial division up to that bound. If any factors
	   are found they'll get divided out of n */

	fill_prime_list(&prime_list, params.fb_size);
	do_trial_factor(obj, &prime_list, &n);

	/* handle degenerate factorizations */

	if (mp_is_one(&n)) {
		free(prime_list.list);
		obj->flags |= MSIEVE_FLAG_FACTORIZATION_DONE;
		return;
	}
	if (mp_is_prime(&n, &obj->seed1, &obj->seed2)) {
		add_next_factor(obj, &n, MSIEVE_PROBABLE_PRIME);
		free(prime_list.list);
		obj->flags |= MSIEVE_FLAG_FACTORIZATION_DONE;
		return;
	}

	/* detect if n is a perfect power. Try extracting any root
	   whose value would exceed the trial factoring bound above */

	i = prime_list.num_primes;
	j = 1;
	while (((uint32)(1) << j) < prime_list.list[i - 1])
		j++;
	i = bits / j;

	do {
		mp_t n2;
		if (mp_iroot(&n, i, &n2) == 0) {
			enum msieve_factor_type factor_type;

			factor_type = (mp_is_prime(&n2, &obj->seed1,
					&obj->seed2)) ? MSIEVE_PROBABLE_PRIME :
							MSIEVE_COMPOSITE;

			for (j = 0; j < i; j++)
				add_next_factor(obj, &n2, factor_type);

			obj->flags |= MSIEVE_FLAG_FACTORIZATION_DONE;
			free(prime_list.list);
			return;
		}
	} while(--i > 1);

	/* If n is small enough, switch to another factoring method */

	bits = mp_bits(&n);
	if (bits <= 62) {
		mp_t n2;
		i = squfof(&n);
		if (i > 1) {
			enum msieve_factor_type factor_type1;
			enum msieve_factor_type factor_type2;

			mp_clear(&n2);
			n2.nwords = 1;
			n2.val[0] = i;
			mp_divrem_1(&n, i, &n);

			factor_type1 = (mp_is_prime(&n, &obj->seed1,
					&obj->seed2)) ? MSIEVE_PROBABLE_PRIME :
							MSIEVE_COMPOSITE;
			factor_type2 = (mp_is_prime(&n2, &obj->seed1,
					&obj->seed2)) ? MSIEVE_PROBABLE_PRIME :
							MSIEVE_COMPOSITE;

			if (mp_cmp(&n, &n2) < 0) {
				add_next_factor(obj, &n, factor_type1);
				add_next_factor(obj, &n2, factor_type2);
			}
			else {
				add_next_factor(obj, &n2, factor_type2);
				add_next_factor(obj, &n, factor_type1);
			}
			obj->flags |= MSIEVE_FLAG_FACTORIZATION_DONE;
			free(prime_list.list);
			return;
		}
	}

	/* Beyond this point we have to use the quadratic sieve. */

	/* Recalculate the factor base bound */

	get_sieve_params(bits, &params);
	if (params.fb_size < 100)
		params.fb_size = 100;

	/* build the factor base for real */

	factor_base = build_factor_base(obj, &n, &prime_list, 
					&params.fb_size, &multiplier);
	fb_size = params.fb_size;
	logprintf(obj, "using multiplier of %u\n", multiplier);
	free(prime_list.list);
	
	/* Proceed with the algorithm */

	do_sieving(obj, &n, &poly_a_list, &poly_list, factor_base, 
		   &params, multiplier, get_sieve_block_size(obj),
		   &full_relations, &partial_relations, &cycle_list);

	if (full_relations == NULL || partial_relations == NULL ||
	    cycle_list == NULL || poly_a_list == NULL || poly_list == NULL) {

		free(factor_base);

		obj->timestamp = i = (uint32)(time(NULL) - obj->timestamp);
		logprintf(obj, "elapsed time %02d:%02d:%02d\n", i / 3600,
					(i % 3600) / 60, i % 60);
		return;
	}

	solve_linear_system(obj, fb_size, &vectors, 
				&vsize, &bitfield, 
				full_relations, cycle_list);

	find_factors(obj, &n, factor_base, fb_size, 
			vectors, vsize, bitfield, multiplier,
			poly_a_list, poly_list);

	free(factor_base);
	free(vectors);
	free(bitfield);
	free_cycle_list(cycle_list);
	free_relation_list(full_relations);
	free_relation_list(partial_relations);
	free(poly_list);
	free(poly_a_list);
	obj->flags |= MSIEVE_FLAG_FACTORIZATION_DONE;

	obj->timestamp = i = (uint32)(time(NULL) - obj->timestamp);
	logprintf(obj, "elapsed time %02d:%02d:%02d\n", i / 3600,
				(i % 3600) / 60, i % 60);
}

/*--------------------------------------------------------------------*/
void add_next_factor(msieve_obj *obj, mp_t *n, 
			enum msieve_factor_type factor_type) {

	msieve_factor *new_factor = (msieve_factor *)malloc(
					sizeof(msieve_factor));
	char *type_string;
	char *tmp;
	size_t len;

	if (factor_type == MSIEVE_PRIME)
		type_string = "p";
	else if (factor_type == MSIEVE_COMPOSITE)
		type_string = "c";
	else
		type_string = "prp";

	/* Copy n. We could use strdup(), but that causes
	   warnings for gcc on AMD64 */

	tmp = mp_sprintf(n, 10, obj->mp_sprintf_buf);
	len = strlen(tmp) + 1;
	new_factor->number = (char *)malloc((size_t)len);
	memcpy(new_factor->number, tmp, len);

	new_factor->factor_type = factor_type;
	new_factor->next = NULL;

	if (obj->factors != NULL) {
		msieve_factor *curr_factor = obj->factors;
		while (curr_factor->next != NULL)
			curr_factor = curr_factor->next;
		curr_factor->next = new_factor;
	}
	else {
		obj->factors = new_factor;
	}

	logprintf(obj, "%s%d factor: %s\n", type_string, 
				(int32)(len - 1),
				new_factor->number);
}

/*--------------------------------------------------------------------*/
void print_to_savefile(msieve_obj *obj, char *buf) {

	if (obj->savefile_buf_off + strlen(buf) + 1 >= SAVEFILE_BUF_SIZE) {
		fprintf(obj->savefile, "%s", obj->savefile_buf);
		fflush(obj->savefile);
		obj->savefile_buf_off = 0;
	}

	obj->savefile_buf_off += sprintf(obj->savefile_buf + 
				obj->savefile_buf_off, "%s", buf);
}

void flush_savefile(msieve_obj *obj) {

	fprintf(obj->savefile, "%s", obj->savefile_buf);
	fflush(obj->savefile);
	obj->savefile_buf_off = 0;
}

/*--------------------------------------------------------------------*/
void logprintf(msieve_obj *obj, char *fmt, ...) {

	va_list ap;

	/* do *not* initialize 'ap' and use it twice; this
	   causes crashes on AMD64 */

	if (obj->flags & MSIEVE_FLAG_USE_LOGFILE) {
		time_t t = time(NULL);
		char buf[64];
		FILE *logfile = fopen(obj->logfile_name, "a");

		if (logfile == NULL) {
			fprintf(stderr, "cannot open logfile\n");
			exit(-1);
		}

		va_start(ap, fmt);
		buf[0] = 0;
		strcpy(buf, ctime(&t));
		*(strchr(buf, '\n')) = 0;
		fprintf(logfile, "%s  ", buf);
		vfprintf(logfile, fmt, ap);
		fclose(logfile);
		va_end(ap);
	}
	if (obj->flags & MSIEVE_FLAG_LOG_TO_STDOUT) {
		va_start(ap, fmt);
		vfprintf(stdout, fmt, ap);
		va_end(ap);
	}
}

/*--------------------------------------------------------------------*/
static void fill_prime_list(prime_list_t *prime_list, uint32 fb_size) {

	uint32 i, j, k;
	uint32 prime, rem;
	uint32 *list;
	uint32 num_primes;

	num_primes = prime_list->num_primes = MAX(1500, 2 * fb_size + 100);
	list = prime_list->list = (uint32 *)malloc(prime_list->num_primes * 
						sizeof(uint32));
	list[0] = 2;
	list[1] = 3;
	for (i = 2, j = 5; i < num_primes; j += 2) {
		for (k = 1, rem = 0; k < i; k++) {
			prime = list[k];
			rem = j % prime;
			if (prime * prime > j || rem == 0)
				break;
		}
		if (rem != 0)
			list[i++] = j;
	}
}

/*--------------------------------------------------------------------*/
static void do_trial_factor(msieve_obj *obj, 
			    prime_list_t *prime_list, 
			    mp_t *n) {

	uint32 i, j;
	uint32 prime;
	uint32 num_primes = prime_list->num_primes;
	uint32 *list = prime_list->list;
	mp_t tmp;

	mp_clear(&tmp);
	tmp.nwords = 1;

	for (i = 0; i < num_primes; i++) {
		prime = list[i];

		j = mp_mod_1(n, prime);
		while (j == 0) {
			tmp.val[0] = prime;
			add_next_factor(obj, &tmp, MSIEVE_PRIME);
			mp_divrem_1(n, prime, n);
			j = mp_mod_1(n, prime);
		}
	}
}

/*--------------------------------------------------------------------*/
static fb_t * build_one_factor_base(msieve_obj *obj, mp_t *n, 
				    prime_list_t *prime_list, 
				    uint32 *fb_size) {
	uint32 prime;
	uint32 modn;
	uint32 i, j;
	uint32 start_fb_size = *fb_size;
	fb_t *fb_array, *fbptr;
	uint32 num_primes = prime_list->num_primes;
	uint32 *list = prime_list->list;

	/* Allocate space for the factor base */

	fb_array = (fb_t *)malloc(start_fb_size * sizeof(fb_t));

	fbptr = fb_array + MIN_FB_OFFSET;
	fbptr->prime = 2;
	fbptr++;

	for (i = 1, j = MIN_FB_OFFSET+1; i < num_primes; i++) {

		/* find a prime p less than the factor base bound
		   for which (x^2 mod p) = (n mod p) has a solution
		   for some value of x */

		if (j == start_fb_size)
			break;

		prime = list[i];

		modn = mp_mod_1(n, prime);
		if (modn == 0) {
			/* the number to be factored is divisible by p */

			fbptr->prime = prime;
			fbptr->logprime = (uint8)(log((double)prime) / 
						log(2.0) + .5);
			fbptr++;
			j++;
			continue;
		}

		if (mp_legendre_1(modn, prime) != 1)
			continue;

		/* solve (x^2 mod p) = (n mod p) for x. */

		fbptr->prime = prime;
		fbptr->modsqrt = mp_modsqrt_1(modn, prime, 
					&obj->seed1, &obj->seed2);
		fbptr->logprime = (uint8)(log((double)prime) / log(2.0) + .5);
		fbptr++;
		j++;
	}

	*fb_size = j;
	return fb_array;
}

/*--------------------------------------------------------------------*/
#define SMALL_PRIME_BOUND 5000

static fb_t * build_factor_base(msieve_obj *obj, mp_t *n, 
				prime_list_t *prime_list,
				uint32 *fb_size, uint32 *multiplier) {

	/* Fills in all the factor base information */

	uint32 i, j;
	fb_t *fb_array, *best_fb_array;
	uint32 next_fb_size, best_fb_size;
	double score, best_score;
	mp_t n2;
	uint32 best_mult; 
	const uint32 mult_list[] = {1, 3, 5, 7, 11, 13,
				15, 17, 19, 21, 23,  
				29, 31, 33, 35, 37, 39};

	best_score = 0;
	score = 0;
	best_fb_array = NULL;
	best_fb_size = 0;
	best_mult = 0;

	/* for each small prime multiplier */

	for (i = 0; i < sizeof(mult_list) / sizeof(uint32); i++) {

		uint32 curr_mult = mult_list[i];

		mp_mul_1(n, curr_mult, &n2);

		/* only consider multipliers for which 
		   (multiplier) * n = 1 mod 4; a multiplier
		   of 1 is always okay */

		if (curr_mult > 1) {
			if ((n2.val[0] & 3) != 1)
				continue;
		}

		/* build the entire factor base corresponding
		   to (multiplier)*n */

		next_fb_size = *fb_size;
		fb_array = build_one_factor_base(obj, &n2, prime_list,
						&next_fb_size);

		/* compute the 'score' for this factor base.
		   This is intended as a measure of how rich
		   the factor base is in small primes; the more
		   small primes are present, the higher the score */

		if ((n->val[0] & 7) == 1)
			score = 2 * log(2.0);
		else
			score = 0;
		
		for (j = MIN_FB_OFFSET + 1; j < next_fb_size; j++) {
			fb_t *fbptr = fb_array + j;
			uint32 prime = fbptr->prime;

			if (prime > SMALL_PRIME_BOUND)
				break;

			if (curr_mult % prime == 0)
				score += log((double)prime) / (prime - 1);
			else
				score += 2.0 * log((double)prime) / (prime - 1);
		}

		/* see if this factor base is better than any
		   encountered thus far. The computation below
		   is an estimate of the 'effective size' of 
		   the scaled n */

		score -= 0.5 * log((double)curr_mult);

		if (score > best_score) {
			if (best_fb_array != NULL)
				free(best_fb_array);
			best_score = score;
			best_fb_array = fb_array;
			best_fb_size = next_fb_size;
			best_mult = curr_mult;
		}
		else {
			free(fb_array);
		}
	}

	/* Now that the factor base has been chosen, precompute
	   the integer reciprocal of all factor base primes 
	   between 8 and 17 bits in size (inclusive) */
	
	for (i = MIN_FB_OFFSET + 1; i < best_fb_size; i++) {
		uint64 num;
		uint32 prime, bits, recip;

		prime = best_fb_array[i].prime;
		for (j = prime - 1, bits = 0; j; j >>= 1)
			bits++;

		if (bits < 8)
			continue;
		if (bits > 17)
			break;

		num = (uint64)(1) << 40;
		recip = (uint32)(num / (uint64)prime);
		best_fb_array[i].recip = recip;
	}

	/* from here on, the rest of the code will factor
	   (multiplier * n) instead of n */

	mp_mul_1(n, best_mult, n);
	*fb_size = best_fb_size;
	*multiplier = best_mult;
	return best_fb_array;
}

/*--------------------------------------------------------------------*/
static void get_sieve_params(uint32 bits, sieve_param_t *params) {

	sieve_param_t *low, *high;
	uint32 max_size;
	uint32 i, j, dist;

	/* For small inputs, use the first set of precomputed
	   parameters */

	if (bits < prebuilt_params[0].bits) {
		*params = prebuilt_params[0];
		return;
	}

	/* bracket the input size between two table entries */

	max_size = sizeof(prebuilt_params) / sizeof(sieve_param_t);
	if (bits >= prebuilt_params[max_size - 1].bits) {
		*params = prebuilt_params[max_size - 1];
		return;
	}

	/* if the input is too large, just use the last table entry.
	   This means that the choice of parameters is increasingly
	   inappropriate as the input becomes larger, but there's no
	   guidance on what to do in this case anyway. Note that with
	   double large primes the runtime taken seems to be very
	   insensitive to factor base sizes and such */

	for (i = 0; i < max_size - 1; i++) {
		if (bits < prebuilt_params[i+1].bits)
			break;
	}

	/* Otherwise the parameters to use are a weighted average 
	   of the two table entries the input falls between */

	low = &prebuilt_params[i];
	high = &prebuilt_params[i+1];
	dist = high->bits - low->bits;
	i = bits - low->bits;
	j = high->bits - bits;

	params->bits = bits;
	params->fb_size = (uint32)(
			 ((double)low->fb_size * j +
			  (double)high->fb_size * i) / dist + 0.5);
	params->large_mult = (uint32)(
			 ((double)low->large_mult * j +
			  (double)high->large_mult * i) / dist + 0.5);
	params->sieve_size = (uint32)(
			 ((double)low->sieve_size * j +
			  (double)high->sieve_size * i) / dist + 0.5);
}

/*--------------------------------------------------------------------*/
static uint32 get_sieve_block_size(msieve_obj *obj) {

	/* attempt to automatically detect the size of
	   the L1 cache; this is the most logical choice
	   for the sieve block size. The following should
	   guess right for most PCs and Macs when using gcc.

	   Otherwise, you have the source so just fill in
	   the correct number. */

	uint32 cache_size = 0;

#if defined(__GNUC__)
	#if defined(__i386__) || defined(__x86_64__)
	#define CPUID(code, a, b, c, d) asm volatile("cpuid" 		    \
					:"=a"(a), "=b"(b), "=c"(c), "=d"(d) \
					:"0"(code))

		/* reading the CPU-specific features of x86
		   processors is a simple 57-step process.
		   The following should be able to retrieve
		   the L1 cache size of any Intel or AMD
		   processor built after ~1995 */

		uint32 a, b, c, d;
		CPUID(0, a, b, c, d);
		if ((b & 0xff) == 'G') {		/* "GenuineIntel" */
			uint8 features[15];
			uint32 i;
			CPUID(2, a, b, c, d);

			features[0] = (a >> 8);
			features[1] = (a >> 16);
			features[2] = (a >> 24);
			features[3] = b;
			features[4] = (b >> 8);
			features[5] = (b >> 16);
			features[6] = (b >> 24);
			features[7] = c;
			features[8] = (c >> 8);
			features[9] = (c >> 16);
			features[10] = (c >> 24);
			features[11] = d;
			features[12] = (d >> 8);
			features[13] = (d >> 16);
			features[14] = (d >> 24);

			for (i = 0; i < 15; i++) {
				switch(features[i]) {
					case 0x0a:
					case 0x66:
						cache_size = 8192; break;
					case 0x0c:
					case 0x60:
					case 0x67:
						cache_size = 16384; break;
					case 0x2c:
					case 0x68:
						cache_size = 32768; break;
				}
			}
	
			/* It turns out to be better to use a cache
			   size that's too large if the true cache
			   size would cause too much overhead (i.e.
			   is too small) */

			if (cache_size < 32768)
				cache_size = DEFAULT_SIEVE_BLOCK_SIZE;

		}
		else if ((b & 0xff) == 'A') {		/* "AuthenticAMD" */
			CPUID(0x80000005, a, b, c, d);
			cache_size = 1024 * (c >> 24);
		}


	#elif defined(__ppc__)
		cache_size = 32768;
	#else
		cache_size = DEFAULT_SIEVE_BLOCK_SIZE;
	#endif
#else
	cache_size = DEFAULT_SIEVE_BLOCK_SIZE;
#endif

	if (cache_size != 8192 && cache_size != 16384 &&
	    cache_size != 32768 && cache_size != 65536 &&
	    cache_size != DEFAULT_SIEVE_BLOCK_SIZE) {
	    	logprintf(obj, "cache detection failed; using "
			  	"default cache size\n");
		cache_size = DEFAULT_SIEVE_BLOCK_SIZE;
	}

	return cache_size;
}
