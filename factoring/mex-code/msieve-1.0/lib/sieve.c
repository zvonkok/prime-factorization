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

static void collect_relations(sieve_conf_t *conf,
			      uint32 target_relations);

static uint32 process_poly_block(sieve_conf_t *conf, 
				  uint32 poly_start,
				  uint32 num_poly);
	

static uint32 check_sieve_val(sieve_conf_t *conf,
			      uint32 block, 
			      uint32 index, 
			      uint32 sign_of_index,
			      uint32 bits,
			      mp_t *a,
			      mp_t *b,
			      mp_t *c,
			      uint32 poly_index,
			      hashtable_t *hash_bucket);

static void fill_sieve_block(sieve_conf_t *conf,
			     hashtable_t *hash_bucket);

static uint32 do_sieving_internal(sieve_conf_t *conf,
				  uint32 max_relations);

/*--------------------------------------------------------------------*/
void do_sieving(msieve_obj *obj, mp_t *n, 
		mp_t **poly_a_list, poly_t **poly_list,
		fb_t *factor_base, sieve_param_t *params, 
		uint32 multiplier,
		uint32 sieve_block_size,
		relation_list_t **full_relations,
		relation_list_t **partial_relations,
		cycle_list_t **cycle_list) {

	sieve_conf_t conf;
	uint32 bound;
	uint32 i;
	uint32 bits;
	uint32 fb_size = params->fb_size;
	uint32 sieve_size = params->sieve_size;
	uint32 num_sieve_blocks;
	uint32 error_bits;
	uint32 max_relations, relations_found;

	/* fill in initial sieve parameters */

	conf.obj = obj;
	conf.n = n;
	conf.multiplier = multiplier;
	conf.factor_base = factor_base;
	conf.fb_size = params->fb_size;
	conf.poly_list = NULL;
	conf.full_list = NULL;
	conf.partial_list = NULL;
	conf.hashtable_plus = NULL;
	conf.hashtable_minus = NULL;

	/* round the size of one sieve block up to a power 
	   of two (if it's not a power of two already) */

	i = 0;
	while (((uint32)(1) << i) < sieve_block_size)
		i++;
	conf.log2_sieve_block_size = i;
	conf.sieve_block_size = sieve_block_size = 1 << i;
	conf.sieve_array = (uint8 *)aligned_malloc(sieve_block_size, 64);

	logprintf(obj, "using sieve block of %u\n", sieve_block_size);

	/* The sieve code is optimized for sieving intervals that are
	   extremely small. To reduce the overhead of using a large
	   factor base, cache blocking is used for the sieve interval
	   *and* the factor base; this requires that sieving takes place
	   for several polynomials simultaneously. */
	
	switch (sieve_block_size) {
	case 8192:
		conf.fb_block = 32;
		conf.poly_block = 8;
		break;
	case 16384:
		conf.fb_block = 64;
		conf.poly_block = 8;
		break;
	case 32768:
		conf.fb_block = 100;
		conf.poly_block = 16;
		break;
	default:
		conf.fb_block = 200;
		conf.poly_block = 16;
		break;
	}

	/* round the size of the sieve interval up to an
	   integral number of sieve blocks */

	num_sieve_blocks = ((sieve_size + sieve_block_size - 1) &
			~(uint32)(sieve_block_size - 1)) / sieve_block_size;

 	bound = conf.factor_base[conf.fb_size - 1].prime;
	logprintf(obj, "using a sieve bound of %u (%u primes)\n", 
				bound, conf.fb_size);

	/* The trial factoring code can only handle 32-bit
	   factors, so the single large prime bound must
	   fit in 32 bits. Note that the SQUFOF code can
	   only manage 31-bit factors, so that single large
	   prime relations can have a 32-bit factor but
	   double large prime relations are limited to 31-bit
	   factors */

	if ((uint32)(-1) / bound <= params->large_mult)
		bound = (uint32)(-1);
	else
		bound *= params->large_mult;

	logprintf(obj, "using large prime bound of %u (%d bits)\n", 
			bound, (int32)(log((double)bound)/log(2.0)));

	/* choose the cutoff prime value beyond which the 
	   hashtable is used. Note that because the sieving
	   code multiplies by precomputed reciprocals
	   instead of doing division, there is an implicit
	   limit: the product of the sieve size and the largest
	   of the small factor base primes must be less than
	   2^40. For big factorizations this means that (each
	   half of) the sieve interval is limited to 8-16 million. */

	if (sieve_block_size < 32768)
		conf.small_fb_size = MIN(fb_size, 3000);
	else
		conf.small_fb_size = MIN(fb_size, 4000);

	if (((uint64)(1) << 40) < (uint64)sieve_size * 
	    (uint64)(conf.factor_base[conf.small_fb_size - 1].prime)) {
		logprintf(obj, "error: factor base size or "
				"sieve size too large\n");
		exit(-1);
	}
	conf.packed_fb = (packed_fb_t *)malloc(conf.small_fb_size *
						sizeof(packed_fb_t));

	/* choose the FB offset below which remainder operations
	   will be used when trial factoring. This is either the
	   first factor base prime greater than 8 bits in size,
	   or 50, whichever is larger */

	i = MIN_FB_OFFSET + 1; 
	while (i < fb_size && conf.factor_base[i-1].prime < 256)
		i++;
	i = MAX(i, 50);
	conf.tiny_fb_size = MIN(i, fb_size);

	/* compute max_fb2, the square of the largest factor base prime */

	i = conf.factor_base[fb_size - 1].prime;
	mp_clear(&conf.max_fb2);
	conf.max_fb2.nwords = 1;
	conf.max_fb2.val[0] = i;
	mp_mul_1(&conf.max_fb2, i, &conf.max_fb2);

	mp_clear(&conf.large_prime_max2);

	/* fill in sieving cutoffs that leverage the small
	   prime variation. Do not skip small primes if the
	   factor base is too small */

	error_bits = (uint32)(log((double)bound) / log(2.0) + 0.5);
	bits = mp_bits(conf.n);

	if (fb_size < 800) {
		conf.cutoff2 = (uint32)(1.5 * error_bits);
		conf.cutoff1 = conf.cutoff2;
		conf.sieve_fb_start = MIN_FB_OFFSET + 1;
	}
	else {
		/* Turn on double large primes if n is
		   85 digits or more */

		if (bits >= 282) {
			mp_t *large_max2 = &conf.large_prime_max2;

			/* the double-large-prime cutoff is equal to
			   the single-large-prime cutoff raised to the 1.8
			   power. We don't square the single cutoff because
			   relations with two really big large primes will
			   almost never survive the filtering step */

			mp_clear(large_max2);
			large_max2->nwords = 1;
			large_max2->val[0] = bound;
			mp_mul_1(large_max2, 
				(uint32)pow((double)bound, 0.8), 
				large_max2);

			conf.cutoff2 = mp_bits(large_max2);
			logprintf(obj, "using double large prime bound of "
				"%s (%u-%u bits)\n",
				mp_sprintf(large_max2, 10, obj->mp_sprintf_buf),
				mp_bits(&conf.max_fb2), 
				conf.cutoff2);

			/* the trigger for trial factoring a sieve value
			   is in general larger than the double large prime 
			   cutoff. Empirically, adding about 2 bits per
			   5 digits of input above 90 digits yields an 
			   increase in partial relations found without 
			   noticeably affecting the sieving time */

			if (bits >= 298) {
				conf.cutoff2 += (uint32)(0.117647 * 
							(bits-298) + 1.5);
			}
			logprintf(obj, "using trial factoring cutoff "
					"of %u bits\n", conf.cutoff2);
		}
		else {
			conf.cutoff2 = error_bits;
		}
		conf.cutoff1 = conf.cutoff2 + 30;
		conf.sieve_fb_start = conf.tiny_fb_size;
	}

	/* initialize the relation lists */

	conf.full_list = (relation_list_t *)calloc((size_t)1, 
					sizeof(relation_list_t));

	conf.partial_list = (relation_list_t *)calloc((size_t)1, 
					sizeof(relation_list_t));

	/* set up the hashtable if it's going to be used; otherwise
	   the rest of the sieve code will silently ignore it.
	   
	   There is one hash bin for every sieve block in the sieve
	   interval; there are also twice as many such bins because
	   we collect positive and negative sieve offsets into separate
	   hashtables. Finally, we sieve over up to POLY_BLOCK_SIZE
	   polynomials at the same time, so the number of hash bins
	   is similarly multiplied.
	   
	   Needless to say, that's a *lot* of hash bins! The sieve
	   interval must be extremely small to keep the hashtable
	   size down; that's okay, very small sieve intervals are
	   actually more likely to contain smooth relations */

	conf.hashtable_plus = (hashtable_t *)calloc((size_t)(conf.poly_block *
							num_sieve_blocks), 
							sizeof(hashtable_t));
	conf.hashtable_minus = (hashtable_t *)calloc((size_t)(conf.poly_block *
							num_sieve_blocks), 
							sizeof(hashtable_t));
	if (fb_size > conf.small_fb_size) {
		for (i = 0; i < conf.poly_block * num_sieve_blocks; i++) {
			conf.hashtable_plus[i].num_alloc = 1000;
			conf.hashtable_plus[i].list = (hash_entry_t *)
					malloc(1000 * sizeof(hash_entry_t));

			conf.hashtable_minus[i].num_alloc = 1000;
			conf.hashtable_minus[i].list = (hash_entry_t *)
					malloc(1000 * sizeof(hash_entry_t));
		}
	}

	/* fill in miscellaneous parameters */

	conf.num_sieve_blocks = num_sieve_blocks;
	conf.large_prime_max = bound;

	/* initialize the polynomial generation code */

	poly_init(&conf, sieve_size);

	/* initialize the bookkeeping for tracking partial relations */

	conf.components = 0;
	conf.vertices = 0;
	conf.cycle_hashtable = (uint32 *)calloc((size_t)(1 << LOG2_CYCLE_HASH),
						sizeof(uint32));
	conf.cycle_table_size = 1;
	conf.cycle_table_alloc = 10000;
	conf.cycle_table = (cycle_t *)malloc(conf.cycle_table_alloc * 
						sizeof(cycle_t));

	/* stop when this many relations are found */

	max_relations = fb_size + 3 * NUM_EXTRA_RELATIONS / 2;

	/* do all the sieving, and save the lists of
	   relations that were generated */

	obj->flags |= MSIEVE_FLAG_SIEVING_IN_PROGRESS;

	relations_found = do_sieving_internal(&conf, max_relations);

	/* free all of the sieving structures first, to leave
	   more memory for the postprocessing step. Do *not* free
	   the polynomial subsystem yet. Sieving is only considered
	   finished when the savefile has received all pending output */

	flush_savefile(obj);
	fclose(obj->savefile);
	obj->savefile = NULL;
	obj->flags &= ~MSIEVE_FLAG_SIEVING_IN_PROGRESS;

	for (i = 0; i < conf.poly_block * num_sieve_blocks; i++) {
		free(conf.hashtable_plus[i].list);
		free(conf.hashtable_minus[i].list);
	}
	free(conf.hashtable_plus);
	free(conf.hashtable_minus);

	free(conf.packed_fb);
	aligned_free(conf.sieve_array);

	/* if enough relations are available, do the postprocessing
	   and save the results where the rest of the program can
	   find them */

	if (relations_found >= max_relations) {
	        if(obj->flags & (MSIEVE_FLAG_USE_LOGFILE |
	    		   MSIEVE_FLAG_LOG_TO_STDOUT)) {
			fprintf(stderr, "sieving complete, "
					"commencing postprocessing\n");
		}
		filter_relations(&conf);
		*full_relations = conf.full_list;
		*partial_relations = conf.partial_list;
		*cycle_list = conf.cycle_list;
		*poly_a_list = conf.poly_a_list;
		*poly_list = conf.poly_list;
	}

	poly_free(&conf);
	free(conf.cycle_table);
	free(conf.cycle_hashtable);
}

/*--------------------------------------------------------------------*/
static uint32 do_sieving_internal(sieve_conf_t *conf, 
				uint32 max_relations) {

	uint32 num_relations = 0;
	uint32 update;
	relation_list_t *full_list = conf->full_list;
	relation_list_t *partial_list = conf->partial_list;
	msieve_obj *obj = conf->obj;

	/* open the savefile; if the file already
	   exists and the first line contains n in base 16,
	   then read in the large primes of relations that have
	   already been found */

	obj->savefile = fopen(obj->savefile_name, "a+");
	if (obj->savefile == NULL) {
		logprintf(obj, "cannot open savefile\n");
		exit(-1);
	}
	fseek(obj->savefile, (long)0, SEEK_END);
	if (ftell(obj->savefile) != 0) {
		char buf[256];
		mp_t t0;

		fseek(obj->savefile, (long)0, SEEK_SET);
		fgets(buf, (int)sizeof(buf), obj->savefile);
		mp_str2mp(buf, &t0, 16);
		if (mp_cmp(conf->n, &t0) != 0) {

			/* the savefile was for a different n. Truncate
			   the file and write the present n. I hope you
			   backed up savefiles you wanted! */

			fclose(obj->savefile);
			obj->savefile = fopen(obj->savefile_name, "w");
			print_to_savefile(obj, mp_sprintf(conf->n, 16,
						obj->mp_sprintf_buf));
			print_to_savefile(obj, "\n");
		}
		else {
			/* Read in the large primes for all the relations
			   in the savefile, and count the number of
			   cycles that can be formed. Note that no check
			   for duplicate or corrupted relations is made here;
			   the cycle finder will rebuild everything from 
			   scratch when sieving finishes, and does all the 
			   verification at that point */

			fgets(buf, (int)sizeof(buf), obj->savefile);

			while (!feof(obj->savefile)) {
				uint32 prime1, prime2;
				char *tmp = strchr(buf, 'L');

				if (buf[0] != 'R' || tmp == NULL) {
					fgets(buf, (int)sizeof(buf), 
							obj->savefile);
					continue;
				}

				read_large_primes(tmp, &prime1, &prime2);
				if (prime1 == prime2) {
					full_list->num_relations++;
				}
				else {
					add_to_cycles(conf, prime1, prime2);
					partial_list->num_relations++;
				}
				fgets(buf, (int)sizeof(buf), 
					obj->savefile);
			}
			fseek(obj->savefile, (long)0, SEEK_END);

			logprintf(obj, "restarting with %u full and %u "
					"partial relations\n",
					full_list->num_relations, 
					partial_list->num_relations);
	
			num_relations = full_list->num_relations + 
					partial_list->num_relations +
					conf->components - conf->vertices;
		}
	}
	else {
		/* start a new savefile; write n to it */

		print_to_savefile(obj, mp_sprintf(conf->n, 16,
						obj->mp_sprintf_buf));
		print_to_savefile(obj, "\n");
	}

	/* choose how many full relations to collect before
	   printing a progress update */

	if (max_relations >= 62000)
		update = 5;
	else if (max_relations >= 50000)
		update = 25;
	else if (max_relations >= 10000)
		update = 100;
	else
		update = 200;
	update = MIN(update, max_relations / 10);

	if (num_relations < max_relations &&
	    (obj->flags & (MSIEVE_FLAG_USE_LOGFILE |
	    		   MSIEVE_FLAG_LOG_TO_STDOUT))) {
		fprintf(stderr, "\nsieving in progress "
				"(press Ctrl-C to pause)\n");
	}

	/* sieve until at least that many relations have
	   been found, then update the number of fulls and
	   partials. This way we can declare sieving to be
	   finished the moment enough relations are available */

	while (!(obj->flags & MSIEVE_FLAG_STOP_SIEVING) && 
		num_relations < max_relations) {

		collect_relations(conf, update);

		num_relations = full_list->num_relations + 
				partial_list->num_relations +
				conf->components - conf->vertices;

	    	if (obj->flags & (MSIEVE_FLAG_USE_LOGFILE |
	    		   	  MSIEVE_FLAG_LOG_TO_STDOUT)) {
			fprintf(stderr, "%u relations (%u full + "
				"%u combined from %u partial), need %u\r",
					num_relations,
					full_list->num_relations,
					partial_list->num_relations +
					conf->components - conf->vertices,
					partial_list->num_relations,
					max_relations);
			fflush(stderr);
		}
	}

	if (obj->flags & (MSIEVE_FLAG_USE_LOGFILE |
	    		   MSIEVE_FLAG_LOG_TO_STDOUT))
		fprintf(stderr, "\n");

	logprintf(obj, "%u relations (%u full + %u combined from "
			"%u partial), need %u\n",
				num_relations, full_list->num_relations,
				partial_list->num_relations +
				conf->components - conf->vertices,
				partial_list->num_relations,
				max_relations);
	return num_relations;
}

/*--------------------------------------------------------------------*/
static void collect_relations(sieve_conf_t *conf, 
			      uint32 target_relations) {
	
	uint32 i;
	uint32 relations_found = 0;
	uint32 num_poly = 1 << (conf->num_poly_factors - 1);
	uint32 poly_block = MIN(num_poly, conf->poly_block);

	/* top-level sieving loop: keep building polynomials
	   and sieving with them until at least target_relations
	   relations have been found */

	while (relations_found < target_relations) {
		
		/* build the next batch of polynomials. For
		   big factorizations there may be thousands
		   of them */

		build_base_poly(conf);

		/* Do the sieving for all polynomials, handling
		   batches of 'poly_block' polynomials at a time. */

		for (i = 0; i < num_poly; i += poly_block) {
			relations_found += process_poly_block(conf, i, 
							      poly_block);
		}

		/* see if anybody wants sieving to stop. Note that
		   we only make this check after sieving for the
		   entire current batch of polynomials has finished */

		if (conf->obj->flags & MSIEVE_FLAG_STOP_SIEVING)
			return;
	}
}

/*--------------------------------------------------------------------*/
static void add_to_hashtable(hashtable_t *entry,
			     uint32 sieve_offset, 
			     uint32 mask, 
			     uint32 prime_index, 
			     uint32 logprime) {

	/* add a 'sieve update' to the hashtable bin pointed
	   to by 'entry'. Hashing is based on the top few
	   bits of sieve_offset, and the range of sieve values
	   represented by one hash bin is given by 'mask'.

	   Note that after the first polynomial it's unlikely
	   that any hash bin will need to grow in size. */

	uint32 i = entry->num_used;
	hash_entry_t new_entry;

	if (!(i % 8))
		PREFETCH(entry->list + i + 8);

	if (i == entry->num_alloc) {
		entry->num_alloc = 2 * i;
		entry->list = (hash_entry_t *)realloc(entry->list,
						2 * i * sizeof(hash_entry_t));
	}
	new_entry.logprime = logprime;
	new_entry.prime_index = prime_index;
	new_entry.sieve_offset = sieve_offset & mask;
	entry->list[i] = new_entry;
	entry->num_used++;
}
	
/*--------------------------------------------------------------------*/
#define PACKED_SIEVE_MASK ((uint64)0x80808080 << 32 | 0x80808080)

static uint32 process_poly_block(sieve_conf_t *conf, 
				  uint32 poly_start,
				  uint32 num_poly) {
	
	/* The core of the sieving code. This routine performs
	   the sieving and relation collection for 'num_poly'
	   polynomials at a time.

	   Because this is the self-initializing quadratic sieve,
	   groups of polynomials are related. There are 'total_poly'
	   polynomials that share the same 'a' value; we number them
	   from 0 to total_poly-1. poly_start refers to the index
	   of the initial polynomial of the group of num_poly. 

	   Ordinarily, you initialize one polynomial, sieve with it,
	   initialize the next polynomial, sieve with it, etc. However,
	   we want the sieve interval to be small, and when it's very
	   small but the factor base is large we will spend a lot
	   of time initializing polynomials and only a little time 
	   sieving. Self-initialization improves that a lot, but when
	   the factor base exceeds the cache size, even for previous
	   versions of this program, polynomial initialization would
	   take 50% of the total sieving stage runtime when the sieve
	   interval is small. We could make the interval larger, but
	   that means smooth relations occur less often; sieve time
	   does not scale well but poly initialization time does.

	   This routine takes a novel approach: for small factor base
	   primes it sieves one polynomial at a time. However, for
	   all the other factor base primes (90+% of the factor base,
	   in a big factorization) it does not sieve at all, but fills
	   a hashtable with the locations of which prime updates which
	   sieve offset. There is a separate hashtable for each polynomial,
	   and a separate hashtable for positive and negative sieve
	   values. Because these hashtables do not interact with each other,
	   we can do all the sieving for a few factor base primes in one
	   polynomial, then switch to the next polynomial and do the 
	   sieving for the same primes. For these few primes all of the 
	   sieving for one poly happens at once, and so the roots of these 
	   primes can then be updated to become roots of the next polynomial.

	   The upshot is that we deal with a block of primes at a time,
	   and for that block do all the sieving for num_poly polynomials;
	   the roots for each prime get updated whenever sieving for a
	   polynomial has finished. This in effect performs a tiling of
	   the sieve interval, the factor base, and the auxiliary information
	   needed to switch polynomials. When the tile size of these three
	   quantities is chosen small enough, everything fits in cache
	   for the duration of the sieving.

	   Note that putting all the updates into a hashtable uses up
	   much more space than sieving with it; however, when filling up
	   a hashtable linearly only one cache line per hash bin is active
	   at any given time, and if the number of bins is small enough
	   the processor can fit all the updates into store buffers as
	   they happen.  Constrast that with filling up a conventional 
	   sieve interval, where all the cache lines in the interval
	   are active.

	   All of the hashtables are filled in first; then we proceed one
	   polynomial at a time. For each polynomial we sieve with the
	   small factor base primes, add in the sieve values from the 
	   appropriate set of hash bins (the bin size matches the sieve
	   block size for this implementation), then do trial factoring
	   on presumed smooth sieve values. There's one more perk to doing
	   things this way: since a hash bin can be made to contain the
	   actual primes as well as the log values of those primes, 
	   trial factoring can be sped up an order of magnitude by reusing
	   the hash bins to detect primes that divide sieve values.

	   In short: black magic, awful mess, really fast. */

	uint32 i, j, k, m;
	uint32 relations_found = 0;
	uint32 log2_sieve_block_size = conf->log2_sieve_block_size;
	uint32 sieve_block_size = conf->sieve_block_size;
	uint32 num_sieve_blocks = conf->num_sieve_blocks;
	uint32 sieve_size = num_sieve_blocks * sieve_block_size;
	uint32 fb_size = conf->fb_size;
	fb_t *factor_base = conf->factor_base;
	uint32 num_factors = conf->num_poly_factors;
	uint32 total_poly = 1 << (num_factors - 1);
	hashtable_t *hashtable_plus;
	hashtable_t *hashtable_minus;
	uint32 poly_index;
	uint32 *poly_b_array;
	uint32 mask = sieve_block_size - 1;

	/* all hash bins start off empty */

	for (i = 0; i < num_poly * num_sieve_blocks; i++) {
		conf->hashtable_plus[i].num_used = 0;
		conf->hashtable_minus[i].num_used = 0;
	}

	poly_b_array = conf->poly_b_array;
	i = conf->small_fb_size;

	/* for each block of factor base primes */

	while (i < fb_size) {
		uint32 fb_block = MIN(conf->fb_block, fb_size - i);
		fb_t *fb_start = factor_base + i;

		hashtable_plus = conf->hashtable_plus;
		hashtable_minus = conf->hashtable_minus;
		poly_index = poly_start;

		/* for polynomial j */

		for (j = 0; j < num_poly; j++) {

			uint32 correction, next_action;
			uint32 *poly_b_start;

			for (k = 0; k < fb_block; k++) {
				fb_t *fbptr = fb_start + k;
				uint32 prime = fbptr->prime;
				uint32 root1, root2;
				uint8 logprime = fbptr->logprime;

				if (k % 4 == 0)
					PREFETCH(fbptr + 4);

				/* grab the two roots of each prime in
				   the block, and do all the sieving over
				   positive and negative values. For each
				   sieve value, use the upper bits of the 
				   value to determine the hash bin of 
				   polynomial j to update */

				root1 = fbptr->root1;
				while (root1 < sieve_size) {
					add_to_hashtable(hashtable_plus + 
					       	(root1>>log2_sieve_block_size),
						root1, mask, i + k, logprime);
					root1 += prime;
				}
	
				root1 = prime - fbptr->root1;
				while (root1 < sieve_size) {
					add_to_hashtable(hashtable_minus + 
					       	(root1>>log2_sieve_block_size),
						root1, mask, i + k, logprime);
					root1 += prime;
				}

				root2 = fbptr->root2;
				while (root2 < sieve_size) {
					add_to_hashtable(hashtable_plus + 
					       	(root2>>log2_sieve_block_size),
						root2, mask, i + k, logprime);
					root2 += prime;
				}
	
				root2 = prime - fbptr->root2;
				while (root2 < sieve_size) {
					add_to_hashtable(hashtable_minus + 
					       	(root2>>log2_sieve_block_size),
						root2, mask, i + k, logprime);
					root2 += prime;
				}
			}

			/* this block has finished sieving for polynomial
			   j; now select the new roots for polynomial j+1.
			   Note that there is no polynomial with index
			   'total_poly', so no update is needed after the
			   last polynomial has updated its hashtable */

			if (++poly_index == total_poly)
				continue;

			k = poly_index;
			next_action = conf->next_poly_action[k];
			correction = conf->root_correction[k] -
					conf->root_correction[k-1];
			poly_b_start = poly_b_array;
			
			/* update the two roots for each prime in the block */

			for (k = 0; k < fb_block; 
					k++, poly_b_start += num_factors) {
	
				fb_t *fbptr = fb_start + k;
				uint32 prime = fbptr->prime;
				uint32 root1 = fbptr->root1;
				uint32 root2 = fbptr->root2;
	
				m = poly_b_start[next_action & 0x7f];
				if (next_action & 0x80) {
					root1 += m;
					root2 += m;
				}
				else {
					root1 += prime - m;
					root2 += prime - m;
				}
	
				if (root1 >= prime)
					root1 -= prime;
				if (root2 >= prime)
					root2 -= prime;
	
				root1 += correction;
				root2 += correction;
				if ((int32)correction >= 0) {
					if (root1 >= prime)
						root1 -= prime;
					if (root2 >= prime)
						root2 -= prime;
				}
				else {
					if ((int32)root1 < 0)
						root1 += prime;
					if ((int32)root2 < 0)
						root2 += prime;
				}
	
				/* store the roots for the next iteration.
				   Do not sort them in ascending order; the
				   loop above does not require it, and the
				   unpredictable branch required to do so
				   takes a large fraction of the total poly
				   initialization time! */

				fbptr->root1 = root1;
				fbptr->root2 = root2;
			}

			/* polynomial j is finished; point to the
			   hashtables for polynomial j+1 */

			hashtable_plus += num_sieve_blocks;
			hashtable_minus += num_sieve_blocks;
		}

		/* current block has updated all hashtables;
		   go on to the next block */

		i += fb_block;
		poly_b_array += fb_block * num_factors;
	}

	/* All of the hashtables have been filled; now proceed
	   one polynomial at a time and sieve with the small
	   factor base primes */

	hashtable_plus = conf->hashtable_plus;
	hashtable_minus = conf->hashtable_minus;
	poly_index = poly_start;

	/* for each polynomial */

	for (i = 0; i < num_poly; i++) {

		mp_t *a, *b;
		mp_t c, tmp;
		uint32 cutoff1;
		uint32 block;
		uint8 *sieve_array = conf->sieve_array;
		uint64 *packed_sieve = (uint64 *)conf->sieve_array;
		packed_fb_t *packed_fb = conf->packed_fb;
		uint32 correction, next_action;

		/* make temporary copies of all of the roots; sieving
		   cannot take place all at once like the large FB primes
		   got to do, so we must update the temporary roots as
		   we go from sieve block to sieve block */

		for (j = MIN_FB_OFFSET + 1; j < conf->small_fb_size; j++) {
			fb_t *fbptr = factor_base + j;
			packed_fb_t *pfbptr = packed_fb + j;
			pfbptr->prime = fbptr->prime;
			pfbptr->logprime = fbptr->logprime;
			pfbptr->next_loc1 = fbptr->root1;
			pfbptr->next_loc2 = fbptr->root2;
		}

		/* Form 'c', equal to (b * b - n) / a (exact
		   division). Having this quantity available
		   makes trial factoring a little faster.

		   Also double the 'b' value, since nobody
		   else uses it and the trial factoring code
		   prefers 2*b */

		a = &conf->curr_a;
		b = conf->curr_b + poly_start + i;
		mp_mul(b, b, &tmp);
		mp_sub(conf->n, &tmp, &tmp);
		mp_div(&tmp, a, NULL, &c);

		mp_add(b, b, b);

		/* choose the first sieving cutoff */

		cutoff1 = mp_bits(&c);
		if (cutoff1 >= conf->cutoff1)
			cutoff1 -= conf->cutoff1;
		else
			cutoff1 = 0;

		/* for each positive sieve block */

		for (block = 0; block < num_sieve_blocks; block++) {
	
			uint32 block_start = block << log2_sieve_block_size;
	
			/* initialize the sieve array */

			memset(sieve_array, (int8)(cutoff1 - 1), 
				(size_t)sieve_block_size);
	
			/* do the sieving and add in the values from the
			   hash bin corresponding to this sieve block */

			fill_sieve_block(conf, hashtable_plus + block);
	
			for (j = 0; j < sieve_block_size / 8; j++) {

				/* test 8 sieve values at a time for large
				   logarithms. The sieve code actually
				   decrements by the logarithms of primes, so
				   if a value is smooth its top bit will be
				   set. We can easily test for this condition */

				if ((packed_sieve[j] & PACKED_SIEVE_MASK)
								== (uint64)(0))
					continue;
	
				/* one or more of the 8 values is probably
				   smooth. Test one at a time and attempt
				   to factor them */

				for (k = 0; k < 8; k++) {
					uint32 bits = sieve_array[8 * j + k];
					if (bits > cutoff1)
						relations_found += 
							check_sieve_val(conf, 
								block_start, 
								8 * j + k, 
								POSITIVE,
								cutoff1 + 
								    257 - bits,
								a, b, &c,
								poly_index,
								hashtable_plus
								    + block);
				}
			}
		}

		/* reinitialize the roots, and now negate them */

		for (j = MIN_FB_OFFSET + 1; j < conf->small_fb_size; j++) {
			fb_t *fbptr = factor_base + j;
			packed_fb_t *pfbptr = packed_fb + j;
			uint32 prime = fbptr->prime;

			if (fbptr->root1 == (uint32)(-1)) {
				pfbptr->next_loc1 = (uint32)(-1);
				pfbptr->next_loc2 = (uint32)(-1);
			}
			else {
				pfbptr->next_loc1 = prime - fbptr->root2;
				pfbptr->next_loc2 = prime - fbptr->root1;
			}
		}

		/* repeat the sieving procedure for negative
		   sieve values */

		for (block = 0; block < num_sieve_blocks; block++) {
	
			uint32 block_start = block << log2_sieve_block_size;
	
			/* all values in the sieve array start off with this
			   cutoff score (minus 1), and the sieving code 
			   decrements values that are divisible by factor 
			   base primes. */
	
			memset(sieve_array, (int8)(cutoff1 - 1), 
				(size_t)sieve_block_size);

			fill_sieve_block(conf, hashtable_minus + block);
	
			for (j = 0; j < sieve_block_size / 8; j++) {
				if ((packed_sieve[j] & PACKED_SIEVE_MASK)
								== (uint64)(0))
					continue;
	
				for (k = 0; k < 8; k++) {
					uint32 bits = sieve_array[8 * j + k];
					if (bits > cutoff1)
						relations_found += 
							check_sieve_val(conf, 
								block_start, 
								8 * j + k, 
								NEGATIVE,
								cutoff1 + 
								    257 - bits,
								a, b, &c,
								poly_index,
								hashtable_minus
							        + block);
				}
			}
		}
	
		/* update the roots for each of the small factor
		   base primes to correspond to the next polynomial */

		if (++poly_index == total_poly)
			break;

		k = poly_index;
		next_action = conf->next_poly_action[k];
		correction = conf->root_correction[k] -
				conf->root_correction[k-1];
		poly_b_array = conf->poly_b_small[next_action & 0x7f];

		for (j = MIN_FB_OFFSET + 1; j < conf->small_fb_size; j++) {

			fb_t *fbptr = factor_base + j;
			uint32 prime = fbptr->prime;
			uint32 root1 = fbptr->root1;
			uint32 root2 = fbptr->root2;

			if (root1 == (uint32)(-1))
				continue;

			m = poly_b_array[j];
			if (next_action & 0x80) {
				root1 += m;
				root2 += m;
			}
			else {
				root1 += prime - m;
				root2 += prime - m;
			}

			if (root1 >= prime)
				root1 -= prime;
			if (root2 >= prime)
				root2 -= prime;

			root1 += correction;
			root2 += correction;
			if ((int32)correction >= 0) {
				if (root1 >= prime)
					root1 -= prime;
				if (root2 >= prime)
					root2 -= prime;
			}
			else {
				if ((int32)root1 < 0)
					root1 += prime;
				if ((int32)root2 < 0)
					root2 += prime;
			}

			if (root2 > root1) {
				fbptr->root1 = root1;
				fbptr->root2 = root2;
			}
			else {
				fbptr->root1 = root2;
				fbptr->root2 = root1;
			}
		}
		hashtable_plus += num_sieve_blocks;
		hashtable_minus += num_sieve_blocks;
	}

	return relations_found;
}

/*--------------------------------------------------------------------*/
static uint32 check_sieve_val(sieve_conf_t *conf, uint32 block_start, 
				uint32 index, uint32 sign_of_index,
				uint32 bits, mp_t *a, mp_t *b, mp_t *c,
				uint32 poly_index, hashtable_t *hash_bucket) {

	/* check a single sieve value for smoothness. This
	   routine is called quite rarely but is very compu-
	   tationally intensive. Returns 1 if the input sieve
	   value is completely factored, 0 otherwise. 
	   
	   There are a lot of things in this routine that are
	   not explained very well in references. The quadratic 
	   sieve uses a polynomial p(x) = (sqrt(n) + x), and trial 
	   division is performed on p(x)^2 - n. MPQS instead uses
	   a polynomial p(x) = a * x + b, where a and b are several
	   digits smaller than sqrt(n). 
	   
	   Thus, for MPQS p(x)^2 - n is usually much larger than
	   sqrt(n). However, the way a and b were computed, p(x)^2 - n
	   is divisible by a. Rather than compute p(x)^2 - n and divide
	   manually by a, you can instead compute a * x^2 + 2 * b * x + c,
	   where c is precomputed to be (b*b-n)/a. This quadratic 
	   polynomial happens to be (p(x)^2-n)/a, and its value is a
	   little less than sqrt(n) so you can trial divide like in QS.
	   c and 2*b were both precomputed by calling code, although
	   c is not used after the sieving phase and need not be saved */

	uint32 i, j;
	uint32 num_factors = 0;
	uint32 sieve_offset;
	uint32 fb_offsets[32 * MAX_MP_WORDS / 4];
	mp_t prod, res;
	fb_t *factor_base = conf->factor_base;
	uint32 tiny_fb_size = conf->tiny_fb_size;
	uint32 small_fb_size = conf->small_fb_size;
	hash_entry_t *list;
	uint32 lowbits;
	mp_t base, exponent, ans;
	uint32 cutoff2;

	/* Compute the polynomial index. Unlike QS, this
	   is guaranteed to be less than 32 bits */

	sieve_offset = block_start | index;

	/* if the index is positive, compute a*x^2 + 2*b*x + c, or
	   using Horner's rule, (a*x+2*b)*x + c.
	   if the index is negative, compute a*x^2 - 2*b*x + c, or
	   using Horner's rule, (a*x-2*b)*x + c. */

	mp_mul_1(a, sieve_offset, &prod);
	if (sign_of_index == POSITIVE) {
		mp_add(&prod, b, &prod);
	}
	else {
		/* An extremely subtle special case: if sieve_offset
		   is -1, then the polynomial value is a-2b+c; if this
		   was ordinary MPQS we could guarantee that 2b < a,
		   but with self-initialization this may not be the case. */

		if (sieve_offset == 1 && mp_cmp(a, b) < 0)
			return 0;

		mp_sub(&prod, b, &prod);
	}

	mp_mul_1(&prod, sieve_offset, &res);

	/* handle the '+c' part. Note that because c is negative,
	   the final polynomial value can have the OPPOSITE SIGN
	   of sieve_offset. The linear algebra code cares about 
	   the sign of 'res', but the MPQS square root phase cares
	   about the sign of sieve_offset. Thus, you have to track
	   both these values for every relation. 
	   
	   The literature considers this fact too low-level to mention */

	if (mp_cmp(&res, c) >= 0) {	/* poly value is positive */
		mp_sub(&res, c, &res);
	}
	else {				/* poly value is negative */
		mp_sub(c, &res, &res);
		fb_offsets[num_factors++] = 0;
	}

	/* now that the sieve value has been computed, calculate
	   the exact number of bits that would be needed to indicate
	   trial factoring is worthwhile. If sieve_offset is near
	   a real root of the polynomial then res can be several digits
	   smaller than calling code expects, and we don't want to
	   throw it away by accident (especially since the odds are
	   better that it will be smooth) */

	cutoff2 = mp_bits(&res);
	if (cutoff2 >= conf->cutoff2)
		cutoff2 -= conf->cutoff2;
	else
		cutoff2 = 0;

	/* Do trial division; factor out all primes in
	   the factor base from 'res'. First pull out
	   factors of two */

	lowbits = mp_rjustify(&res, &res);
	bits += lowbits;
	for (i = 0; i < lowbits; i++)
		fb_offsets[num_factors++] = MIN_FB_OFFSET;

	/* Now deal with all the rest of the factor base
	   primes. Rather than perform multiple-precision
	   divides, this code uses the fact that if 'prime'
	   divides 'res', then 'sieve_offset' lies on one 
	   of two arithmetic progressions. In other words,
	   sieve_offset = root1 + i*prime or root2 + j*prime
	   for some i or j, if prime divides res. Because
	   sieve_offset is always less than 32 bits, and
	   root1 and root2 are available, a single remainder
	   operation is enough to determine divisibility.
	   
	   The first step is a small amount of trial division
	   before comparison of log2(sieve_value) to cutoff2 */

	for (i = MIN_FB_OFFSET + 1; i < tiny_fb_size; i++) {
		fb_t *fbptr = factor_base + i;
		uint32 prime = fbptr->prime;
		uint32 root1 = fbptr->root1;
		uint32 root2 = fbptr->root2;
		uint32 logprime = fbptr->logprime;

		/* if the roots have not been computed, do
		   a multiple precision mod. The only value for
		   which this is true (in this loop) should be
		   the small multiplier */

		if (root1 == (uint32)(-1)) {
			if (mp_mod_1(&res, prime) == 0) {
				do {
					bits += logprime;
					fb_offsets[num_factors++] = i;
					mp_divrem_1(&res, prime, &res);
					j = mp_mod_1(&res, prime);
				} while (j == 0);
			}
			continue;
		}

		/* negate the roots if the sieve value is
		   assumed negative. Note that if one of the
		   roots is zero its negative is 'prime',
		   which the integer remainder below would not
		   compute. */

		if (sign_of_index == NEGATIVE) {
			root2 = prime - root2;
			if (root1)
				root1 = prime - root1;
		}

		j = sieve_offset % prime;
		if (j == root1 || j == root2) {
			do {
				bits += logprime;
				fb_offsets[num_factors++] = i;
				mp_divrem_1(&res, prime, &res);
				j = mp_mod_1(&res, prime);
			} while (j == 0);
		}
	}

	if (bits <= cutoff2)
		return 0;

	/* Now perform trial division for the rest of the
	   "small" factor base primes */

	for (; i < small_fb_size; i++) {
		fb_t *fbptr = factor_base + i;
		uint32 prime = fbptr->prime;
		uint32 root1 = fbptr->root1;
		uint32 root2 = fbptr->root2;
		uint32 recip = fbptr->recip;

		if (i % 4 == 0)
			PREFETCH(fbptr + 4);

		/* if the roots have not been computed, do
		   a multiple precision mod. The only values for
		   which this is true (in this loop) are the
		   factors of the polynomial 'a' value */

		if (root1 == (uint32)(-1)) {
			if (mp_mod_1(&res, prime) == 0) {
				do {
					fb_offsets[num_factors++] = i;
					mp_divrem_1(&res, prime, &res);
					j = mp_mod_1(&res, prime);
				} while (j == 0);
			}
			continue;
		}

		/* negate the roots if the sieve value is
		   assumed negative. Note that if one of the
		   roots is zero its negative is 'prime',
		   which the integer remainder below would not
		   compute. */

		if (sign_of_index == NEGATIVE) {
			root2 = prime - root2;
			if (root1)
				root1 = prime - root1;
		}

		j = (uint32)(((uint64)sieve_offset * (uint64)recip) >> 40);
		j = sieve_offset - j * prime;
		if (j >= prime)
			j -= prime;

		if (j == root1 || j == root2) {
			do {
				fb_offsets[num_factors++] = i;
				mp_divrem_1(&res, prime, &res);
				j = mp_mod_1(&res, prime);
			} while (j == 0);
		}
	}

	list = hash_bucket->list;

	/* This hashtable entry contains all the primes that
	   divide sieve offsets in this range, along with the
	   sieve offsets those primes divide. Hence trial
	   division for the entire factor base above small_fb_size
	   requires only iterating through this hashtable entry
	   and comparing sieve offsets

	   Not only does this not require any divisions, but
	   the number of entries in list[] is much smaller
	   than the full factor base (5-10x smaller) */

	for (i = 0; i < hash_bucket->num_used; i++) {
		if (i % 8 == 0)
			PREFETCH(list + i + 16);
	
		if (list[i].sieve_offset == index) {
			uint32 prime_index = list[i].prime_index;
			uint32 prime = factor_base[prime_index].prime;

			do {
				fb_offsets[num_factors++] = prime_index;
				mp_divrem_1(&res, prime, &res);
				j = mp_mod_1(&res, prime);
			} while (j == 0);
		}
	}

	/* encode the sign of sieve_offset into its top bit */

	sieve_offset |= sign_of_index << 31;
	
	/* If 'res' has been completely factored, save it
	   in the full list */

	if (res.nwords == 1 && res.val[0] == 1) {
		save_relation(conf, sieve_offset, fb_offsets, 
				num_factors, conf->full_list, poly_index,
				1, 1);
		return 1;
	}

	/* if 'res' is smaller than the bound for partial 
	   relations, save in the partial list */

	if (res.nwords == 1 && res.val[0] < conf->large_prime_max) {
		save_relation(conf, sieve_offset, fb_offsets, 
				num_factors, conf->partial_list, poly_index,
				1, res.val[0]);
		return 0;
	}

	/* if 'res' is smaller than the square of the 
	   largest factor base prime, then 'res' is
	   itself prime and is useless for our purposes. 
	   Note that single large prime relations will
	   always fail at this point */
	
	if (mp_cmp(&res, &conf->max_fb2) < 0)
		return 0;
	
	/* 'res' is not too small; see if it's too big */

	if (mp_cmp(&res, &conf->large_prime_max2) > 0)
		return 0;
	
	/* perform a base-2 pseudoprime test to make sure
	   'res' is composite */
	
	mp_sub_1(&res, 1, &exponent);
	mp_clear(&base);
	base.nwords = 1; base.val[0] = 2;
	mp_expo(&base, &exponent, &res, &ans);
	if (mp_is_one(&ans))
		return 0;
	
	/* *finally* attempt to factor 'res'; if successful,
	   and both factors are smaller than the single 
	   large prime bound, save 'res' as a partial-partial 
	   relation */
	
	i = squfof(&res);
	if (i > 1) {
		mp_divrem_1(&res, i, &res);
		if (i < conf->large_prime_max && res.nwords == 1 &&
				res.val[0] < conf->large_prime_max) {
		    
		        if (i == res.val[0]) {

				/* if 'res' is a perfect square, then this
				   is actually a full relation! */

				save_relation(conf, sieve_offset, fb_offsets, 
						num_factors, 
						conf->full_list, poly_index,
						i, res.val[0]);
				return 1;
			}
			else {
				save_relation(conf, sieve_offset, fb_offsets, 
						num_factors, 
						conf->partial_list, poly_index,
						i, res.val[0]);
			}
		}
	}
	return 0;
}

/*--------------------------------------------------------------------*/
static void fill_sieve_block(sieve_conf_t *conf,
			     hashtable_t *hash_bucket) {

	/* Routine for conventional sieving. This is a sophisticated
	   way of computing the following:
	   
	   	for (each prime in factor base) {
			for (each of the 2 roots for that prime) {
				start_offset = root;
				while (start_offset < sieve_block_size) {
					sieve_block[start_offset] -=
							log2(prime);
					start_offset += prime;
				}
				root = start_offset - sieve_block_size;
			}
		}
	   
	   Each successive call to fill_sieve_block() reuses the root
	   values that were computed in the previous call. Unlike the QS
	   version, since we know the size of the complete sieving interval
	   fill_sieve_block() can be called for all the positive sieve
	   values and then all the negative values.  */

	uint32 i;
	uint8 *sieve_array = conf->sieve_array;
	uint32 sieve_block_size = conf->sieve_block_size;
	uint32 sieve_fb_start = conf->sieve_fb_start;
	uint32 small_fb_size = conf->small_fb_size;
	packed_fb_t *packed_fb = conf->packed_fb;
	hash_entry_t *list;

	/* First update the sieve block with values corresponding
	   to the small primes in the factor base. Assuming the
	   entire block will fit in cache, each factor base prime
	   will update many locations within the block, and this
	   phase will run very fast */

	for (i = sieve_fb_start; i < small_fb_size; i++) {
		packed_fb_t *pfbptr = packed_fb + i;
		uint32 prime = pfbptr->prime;
		uint32 root1 = pfbptr->next_loc1;
		uint32 root2 = pfbptr->next_loc2;
		uint8 logprime = pfbptr->logprime;

		/* We assume that the starting positions for each 
		   root (i.e. 'next_loc1' and 'next_loc2') are 
		   stored in ascending order; thus, if next_loc2 
		   is within the sieve block then next_loc1 
		   automatically is too. This allows both root 
		   updates to be collapsed into a single loop, 
		   effectively unrolling by two. */

		if (root1 == (uint32)(-1))
			continue;

		while (root2 < sieve_block_size) {
			sieve_array[root1] -= logprime;
			sieve_array[root2] -= logprime;
			root1 += prime;
			root2 += prime;
		}

		/* once next_loc2 exceeds the sieve block size,
		   next_loc1 may still have one update left. In
		   that case, perform the update and switch 
		   next_loc1 and next_loc2, since next_loc1 is now
		   the larger root */

		if (root1 < sieve_block_size) {
			sieve_array[root1] -= logprime;
			root1 += prime;
			pfbptr->next_loc1 = root2 - sieve_block_size;
			pfbptr->next_loc2 = root1 - sieve_block_size;
		}
		else {
			pfbptr->next_loc1 = root1 - sieve_block_size;
			pfbptr->next_loc2 = root2 - sieve_block_size;
		}
	}

	/* Now update the sieve block with the rest of the
	   factor base. All of the offsets to update have
	   previously been collected into a hash table.
	   While the updates are to random offsets in the
	   sieve block, the whole block was cached by the
	   previous loop, and memory access to the hashtable
	   entry is predictable and can be prefetched */

	list = hash_bucket->list;

	for (i = 0; i < hash_bucket->num_used; i++) {
		uint8 logprime = list[i].logprime;
		uint32 sieve_offset = list[i].sieve_offset;

		if (i % 8 == 0)
			PREFETCH(list + i + 16);

		sieve_array[sieve_offset] -= logprime;
	}
}
