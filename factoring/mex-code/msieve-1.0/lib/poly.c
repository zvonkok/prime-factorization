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

int sort_ascending(const void *x, const void *y);

/*--------------------------------------------------------------------*/
void poly_init(sieve_conf_t *conf, uint32 sieve_size) {

	uint32 i, j;
	uint32 start_bits;
	uint32 num_factors, rem;
	uint32 num_derived_poly;
	mp_t t0, t1;
	msieve_obj *obj = conf->obj;

	/* compute the base value that will be used to generate
	   future MPQS polynomials. This is sqrt(2*n)/sieve_size */

	mp_add(conf->n, conf->n, &t0);
	mp_isqrt(&t0, &t1);
	mp_add_1(&t1, 1, &t1);
	if (t1.nwords == 1)
		mp_divrem_1(&t1, 1000, &t1);
	else
		mp_divrem_1(&t1, sieve_size, &t1);

	mp_copy(&t1, &conf->target_a);
	conf->a_bits = mp_bits(&t1);

	/* Determine the allowable range for factors 
	   of the 'a' coefficient in MPQS polynomials. We
	   simplify the factor selection code later by
	   marking the offsets where factor base primes 
	   increase in size by one bit.
	   
	   Note that any factor selected for use in 'a'
	   values gets only one sieve root and not two. To
	   compensate, the sieve code duplicates the sieve
	   root and cuts its log value in half. However,
	   for primes above conf->small_fb_size the sieve code 
	   uses a hashtable that doesn't know this, and having
	   duplicated roots there would corrupt the trial
	   factoring of sieve values. Hence the pool of primes
	   that SIQS can use is limited to factor base offsets
	   less than small_fb_size. This is desirable anyway,
	   since these primes are small and we get to choose
	   more of them, reusing 'a' values for longer */

	i = 10;
	memset(conf->factor_bounds, 0, sizeof(conf->factor_bounds));
	memset(conf->factor_bits, 0, sizeof(conf->factor_bits));
	memset(conf->poly_factors, 0, sizeof(conf->poly_factors));
	memset(conf->poly_tmp_b, 0, sizeof(conf->poly_tmp_b));
	start_bits = conf->factor_base[i].logprime;
	conf->factor_bounds[start_bits] = i;

	while (i < conf->fb_size) {
		if (conf->factor_base[i].logprime > 13)
			break;

		if (conf->factor_base[i].logprime > start_bits) {
			start_bits = conf->factor_base[i].logprime;
			conf->factor_bounds[start_bits] = i;
		}
		i++;
	}
	conf->factor_bounds[start_bits+1] = i;
	 
	/* Determine the number of factors that each 'a' value
	   will have, and also the number of bits in each factor.
	   Factors can have 7-12 bits, favoring primes in the
	   11-12 bit range. */

	for (i = 11, num_factors = rem = 0; i >= 7; i--) {

		num_factors = conf->a_bits / i;
		rem = conf->a_bits % i;

		/* stop searching for factor sizes if the
		   factor base can handle the current set */

		if (conf->factor_bounds[i] == 0 || num_factors == 1)
			continue;

		if (rem == 0) {

			/* the number of bits in the combined
			   list is exactly the right size */

			if (conf->factor_bounds[i+1] > 0)
				break;
		}
		else if (rem <= num_factors) {

			/* the number of bits in the combined
			   list is not large enough, but is
			   close enough so that some primes can
			   be one bit larger to compensate. This
			   will work if the pool of available primes
			   has candidates in the i and i+1 bin */

			if (conf->factor_bounds[i+1] > 0 &&
			    conf->factor_bounds[i+2] > 0)
				break;
		}
		else if ((i - rem) <= num_factors) {

			/* the number of bits in the combined
			   list is too small by more than the number 
			   of primes. Add an extra prime to the 
			   list and make several primes one bit smaller.
			   This will work if the pool of available primes
			   has candidates in the i and i-1 bin */

			if (conf->factor_bounds[i+1] > 0 &&
			    conf->factor_bounds[i-1] > 0)
				break;
		}
	}
	if (i < 7 || num_factors < 2 || num_factors > MAX_POLY_FACTORS) {
		logprintf(obj, "fatal error: poly selection failed\n");
		exit(-1);
	}

	/* explicitly list out the number of bits each prime
	   in the list will have. Some primes may be one bit
	   too large or too small */

	for (j = 0; j < num_factors; j++)
		conf->factor_bits[j] = i;
	if (rem <= num_factors) {
		for (j = 0; j < rem; j++)
			conf->factor_bits[j]++;
	}
	else {
		conf->factor_bits[j] = i;
		num_factors++;
		for (j = 0; j < (i - rem); j++) 
			conf->factor_bits[j]--;
	}
	conf->num_poly_factors = num_factors;

	/* verify the collection adds up to what is expected */

	for (i = j = 0; j < num_factors; j++) {
		i += conf->factor_bits[j];
	}
	if (i != conf->a_bits) {
		logprintf(obj, "failure assigning bits of poly factors\n");
		exit(-1);
	}

	/* allocate scratch structures */

	num_derived_poly = 1 << (num_factors - 1);
	conf->root_correction = (uint32 *)malloc(num_derived_poly * 
						sizeof(uint32));
	conf->next_poly_action = (uint8 *)malloc(num_derived_poly * 
						sizeof(uint8));
	conf->curr_b = (mp_t *)malloc(num_derived_poly * sizeof(mp_t));

	conf->poly_b_small[0] = (uint32 *)malloc(conf->small_fb_size * 
						num_factors * sizeof(uint32));
	for (i = 1; i < num_factors; i++) {
		conf->poly_b_small[i] = conf->poly_b_small[i-1] + 
						conf->small_fb_size;
	}
	conf->poly_b_array = NULL;
	if (conf->fb_size > conf->small_fb_size) {
		conf->poly_b_array = (uint32 *)malloc(
					num_factors * sizeof(uint32) *
					(conf->fb_size - conf->small_fb_size));
	}
}

/*--------------------------------------------------------------------*/
void poly_free(sieve_conf_t *conf) {

	free(conf->root_correction);
	free(conf->next_poly_action);
	free(conf->poly_b_array);
	free(conf->poly_b_small[0]);
	free(conf->curr_b);
}

/*--------------------------------------------------------------------*/
void build_derived_poly(sieve_conf_t *conf) {

	/* Given a fonfig struct with conf->curr_a and
	   conf->poly_factors already filled in, calculate
	   all of the 'b' values corresponding to this 'a'
	   and also the two peices of scratch information
	   associated with each 'b'. */

	uint32 i;
	mp_t tmp;
	mp_t *a = &conf->curr_a;
	mp_t *b = conf->curr_b;
	mp_t *poly_tmp_b = conf->poly_tmp_b;
	uint32 num_factors = conf->num_poly_factors;
	uint32 *poly_factors = conf->poly_factors;
	fb_t *factor_base = conf->factor_base;
	uint32 correction;
	uint32 num_derived_poly;

	/* compute the intermediate values that will be
	   used to form the 'b' coefficient of this and
	   the next 1<<(num_factors-1) polynomials */

	for (i = 0; i < num_factors; i++) {
		mp_t adivp;
		uint32 rem;
		fb_t *fbptr = factor_base + poly_factors[i];
		uint32 prime = fbptr->prime;
		uint32 modsqrt = fbptr->modsqrt;

		mp_divrem_1(a, prime, &adivp);
		rem = mp_mod_1(&adivp, prime);
		rem = mp_expo_1(rem, prime - 2, prime);
		rem = mp_modmul_1(rem, modsqrt, prime);

		if (rem > prime / 2)
			rem = prime - rem;

		mp_mul_1(&adivp, rem, poly_tmp_b + i);
	}

	/* The first 'b' coefficient is the sum of all the
	   temporary b values. Also double each temporary value. */
	
	mp_clear(b);
	for (i = 0; i < num_factors; i++) {
		mp_add(b, poly_tmp_b + i, b);
		mp_add(poly_tmp_b + i, poly_tmp_b + i, poly_tmp_b + i);
	}

	/* There's a lot about self-initializing MPQS that is not
	   mentioned in Contini's paper. First, ordinary MPQS reduces
	   every 'b' value mod a, but we can't do that here. That's
	   because the precomputations below need to work with un-
	   reduced numbers, since (b mod a) mod p != b mod p in
	   general. But this also means that b can exceed a, and can
	   be negative. My homebrew multiple precision library
	   cannot deal with negative numbers, so the rest of the code
	   wants b to always be positive.

	   We work around the problem by cheating. The whole idea here
	   is to find a and b such that we can compute the values x for
	   which (a*x+b)^2-n is divisible by small primes. If b is between
	   a and 2*a, and we know the corresponding set of x values
	   (which the precomputing below will find), then we can implicitly
	   work with (a*(x+1) + (b-a))^2 - n. This forms a new b value
	   that is smaller than a, and we can find the set of x values
	   by just adding 1 to each x value that we had for the original b. 
	   Negative values of b are treated similarly.

	   So we need to store a correction, equal to the integer quotient 
	   (current b)/a, that is added to all the x values computed by 
	   later polynomials. Every time a new 'b' value is computed, we 
	   update the correction and then reduce b mod a. In this way,
	   the rest of the code gets the 'b' values it wants, and we still
	   initialize polynomials fast. 
	   
	   For this first polynomial, we actually compute the x values the 
	   hard way, so no correction is needed. Also, the first b value
	   is always positive, so actual division is not needed to find 
	   the first correction */

	correction = 0;
	while (mp_cmp(b, a) >= 0) {
		mp_sub(b, a, b);
		correction++;
	}
	conf->root_correction[0] = correction;

	/* Now derive all of the 'b' values that will use
	   the same 'a' value */

	num_derived_poly = 1 << (num_factors - 1);

	for (i = 1; i < num_derived_poly; i++) {

		/* Determine which temporary value will be used to 
		   compute the next b, and whether it will be added or 
		   subtracted. See Contini's SIQS paper for details 
		   on why this works. Also modify the correction to 
		   account for the new b value.
		   
		   When generating the roots for later polynomials,
		   we have to remember which temporary value was used;
		   its identity, along with a bit to choose addition
		   or subtraction, goes into conf->next_poly_action[] */

		uint32 bitpos = 0;

		b = conf->curr_b + i;

		while ((i & (1 << bitpos)) == 0)
			bitpos++;
	
		conf->next_poly_action[i] = bitpos;
		if (i & (1 << (bitpos+1)) ) {
			mp_add(b-1, poly_tmp_b + bitpos, b);
	
			if (mp_cmp(b, a) >= 0) {
				mp_sub(b, a, b);
				correction++;
			}
		}
		else {
			mp_copy(b-1, &tmp);
			if (mp_cmp(poly_tmp_b + bitpos, &tmp) >= 0) {
				mp_add(&tmp, a, &tmp);
				correction--;
			}
			mp_sub(&tmp, poly_tmp_b + bitpos, b);
			conf->next_poly_action[i] |= 0x80;
		}
		conf->root_correction[i] = correction;
	}
}

/*--------------------------------------------------------------------*/
void build_base_poly(sieve_conf_t *conf) {

	/* Build the next MPQS polynomial and prepare the
	   factor base for using it */

	msieve_obj *obj = conf->obj;
	char buf[256];
	uint32 i, j, k;
	mp_t *a = &conf->curr_a;
	mp_t *b = conf->curr_b;
	mp_t tmp;
	fb_t *factor_base = conf->factor_base;
	uint32 *poly_factors = conf->poly_factors;
	uint32 num_factors = conf->num_poly_factors;
	uint8 *factor_bits = conf->factor_bits;
	uint32 *factor_bounds = conf->factor_bounds;
	mp_t *poly_tmp_b;
	uint32 curr_poly_factor;
	uint32 *poly_b_array;
	uint32 **poly_b_small;

	/* compute the factors for this polynomial;
	   filter out duplicate factors. Note that we
	   store factor base offsets and not the primes 
	   themselves   

	   Also compute 'a', the product of these factors */

	mp_clear(a);
	a->nwords = a->val[0] = 1;

	i = 0;
	while (i < num_factors - 1) {
		uint8 bits = factor_bits[i];
		uint32 range = factor_bounds[bits+1] - factor_bounds[bits];

		poly_factors[i] = factor_bounds[bits] + 
				get_rand(&obj->seed1, &obj->seed2) % range;

		for (j = 0; j < i; j++) {
			if (poly_factors[j] == poly_factors[i])
				break;
		}
		if (j == i) {		/* factor is accepted */
			uint32 prime = factor_base[poly_factors[i]].prime;
			mp_mul_1(a, prime, a);
			i++;
		}
	}

	/* the last factor is chosen to bring 'a' as close as
	   possible to the value of conf->target_a. The 'yield'
	   of the polynomial (in terms of number of relations to
	   expect) depends critically on 'a' being neither too
	   large nor too small. For 11-12-bit factors we'll get
	   to within ~0.1% of the optimal value */

	mp_div(&conf->target_a, a, NULL, &tmp);
	i = tmp.val[0];

	for (j = MIN_FB_OFFSET+1; j < conf->fb_size - 2; j++) {
		uint32 left = conf->factor_base[j].prime;
		uint32 right = conf->factor_base[j+1].prime;

		if (i >= left && i <= right)
			break;
	}

	for (; j < conf->fb_size - 2; j++) {
		uint32 left = conf->factor_base[j].prime;
		uint32 right = conf->factor_base[j+1].prime;
		uint32 prime, fb_offset;

		/* pick the factor base prime closest to the
		   ideal value (stored in 'i') */

		if (abs((int32)(i - right)) < abs((int32)(i - left))) {
			fb_offset = j + 1;
			prime = right;
		}
		else {
			fb_offset = j;
			prime = left;
		}

		/* keep it if it's not a duplicate */

		for (k = 0; k < num_factors - 1; k++) {
			if (poly_factors[k] == fb_offset)
				break;
		}
		if (k == num_factors - 1) {
			poly_factors[k] = fb_offset;
			mp_mul_1(a, prime, a);
			break;
		}
	}

	qsort(poly_factors, (size_t)num_factors, 
		sizeof(uint32), sort_ascending);

	/* Build all of the 'b' values corresponding to this 'a' */

	build_derived_poly(conf);

	/* Initialize the factor base */

	curr_poly_factor = 0;
	poly_b_array = conf->poly_b_array;
	poly_b_small = conf->poly_b_small;
	poly_tmp_b = conf->poly_tmp_b;

	for (i = MIN_FB_OFFSET + 1; i < conf->fb_size; i++) {
		fb_t *fbptr = factor_base + i;
		uint32 root1, root2;
		uint32 prime = fbptr->prime;

		if (i < 50 && conf->multiplier % prime == 0) {
			
			/* prime is part of the multiplier. Don't
			   sieve with it */

			root1 = root2 = (uint32)(-1);
		}
		else if (curr_poly_factor < num_factors &&
			 i == poly_factors[curr_poly_factor]) {

			/* the factor base prime divides 'a'. Skip it
			   (we can compute the necessary roots here
			   but it's harder for later polynomials) */

			root1 = root2 = (uint32)(-1);
			curr_poly_factor++;
		}
		else {
			/* compute the two polynomial-specific
			   square roots for each factor base prime */

			uint32 a_modp = mp_mod_1(a, prime);
			uint32 b_modp = prime - mp_mod_1(b, prime);

			root1 = b_modp + fbptr->modsqrt;
			root2 = b_modp + prime - fbptr->modsqrt;

			a_modp = mp_expo_1(a_modp, prime-2, prime);
			root1 = mp_modmul_1(root1, a_modp, prime);
			root2 = mp_modmul_1(root2, a_modp, prime);

			/* also perform the precomputations for the
			   set of future polynomials that will reuse this
			   'a' value. */

			if (i >= conf->small_fb_size) {
				for (j = 0; j < num_factors; j++) {
					b_modp = mp_mod_1(poly_tmp_b + j, 
							prime);
					poly_b_array[j] = mp_modmul_1(a_modp, 
							    b_modp, prime);
				}
				poly_b_array += num_factors;
			}
			else {
				for (j = 0; j < num_factors; j++) {
					b_modp = mp_mod_1(poly_tmp_b + j, 
							prime);
					poly_b_small[j][i] = mp_modmul_1(
							a_modp, b_modp, prime);
				}
			}
		}

		/* The sieving code uses shortcuts that
		   depend on root2 >= root1 at all times */

		if (root1 < root2) {
			fbptr->root1 = root1;
			fbptr->root2 = root2;
		}
		else {
			fbptr->root1 = root2;
			fbptr->root2 = root1;
		}

	}

	/* dump the 'a' value to disk */

	i = sprintf(buf, "A");
	for (j = 0; j < conf->num_poly_factors; j++)
		i += sprintf(buf + i, " %x", conf->poly_factors[j]);
	i += sprintf(buf + i, "\n");
	print_to_savefile(obj, buf);
}

/*--------------------------------------------------------------------*/
int sort_ascending(const void *x, const void *y) {

	uint32 *xx = (uint32 *)x;
	uint32 *yy = (uint32 *)y;
	return xx[0] - yy[0];
}
