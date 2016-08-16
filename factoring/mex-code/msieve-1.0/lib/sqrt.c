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

/*--------------------------------------------------------------------*/
int sort_mp (const void *x, const void *y) {
	mp_t **xx = (mp_t **)x;
	mp_t **yy = (mp_t **)y;
	return mp_cmp(xx[0], yy[0]);
}

/*--------------------------------------------------------------------*/
void find_factors(msieve_obj *obj, mp_t *n, 
		fb_t *factor_base, uint32 fb_size,
		la_col_t *vectors, uint32 vsize, 
		uint64 *null_vectors, uint32 multiplier,
		mp_t *poly_a_list, poly_t *poly_list) {

	/* Perform the square root phase of MPQS. null_vectors
	   contains 64 linear dependencies of the relations in
	   vectors[], which has vsize elements. The code constructs
	   two numbers X and Y (mod n) that are each the product
	   of a huge number of factors derived from vectors[]. It
	   then computes gcd(X+-Y, n) for each linear dependency.
	   gcd values that are not 1 or n are a nontrivial factor
	   of n (or a product of nontrivial factors). For n the
	   product of two prime factors this will happen 2/3 of the 
	   time per dependency, on average.
	   
	   More specifically, vectors[] is a list of relations,
	   each of the form

   	   (a[i] * x[i] + b[i])^2 = prod(factors[i][]) mod n

	   X is a product of the (a[i] * x[i] + b[i]) and Y is
	   the product of sqrt(prod(factors[i][])), both mod n. 
	   The code never needs to calculate explicit square roots, 
	   because it knows the complete factorization of Y. Whether 
	   relation i contributes to dependency j is determined by 
	   the value of the j_th bit of null_vectors[i].

	   Because this implementation uses the double large prime
	   variation of MPQS, a single relation can actually be composed
	   of the product of several entities of the above form. In that
	   case, all entities are multiplied into X and Y (or all
	   are skipped).

	   Note that the code doesn't stop with one nontrivial
	   factor; it prints them all. If you go to so much work
	   and the other dependencies are there for free, why not
	   use them? */

	mp_t r, x, y, tmp, sum;
	uint32 i, j, k, m, num_found; 
	uint64 mask;
	uint32 *fb_counts;
	uint32 large_primes[200], num_large_primes;
	uint32 num_relations, prime;
	relation_t *relation;
	mp_t *factor_found[2 * 64];

	num_found = 0;
	fb_counts = (uint32 *)malloc(fb_size * sizeof(uint32));
	mp_recip(n, &r);

	/* For each dependency */

	for (mask = 1; mask; mask <<= 1) {
		memset(fb_counts, 0, fb_size * sizeof(uint32));
		mp_clear(&x);
		mp_clear(&y);
		x.nwords = y.nwords = x.val[0] = y.val[0] = 1;

		/* For each sieve relation */

		for (i = 0; i < vsize; i++) {

			/* If the relation is not scheduled to
			   contribute to x and y, skip it */

			if (!(null_vectors[i] & mask))
				continue;
			
			/* compute the number of sieve_values */

			num_large_primes = 0;
			if (vectors[i].r0 != NULL)
				num_relations = 1;
			else
				num_relations = vectors[i].r1->num_relations;

			/* for all sieve values */

			for (j = 0; j < num_relations; j++) {
				mp_t *a, *b;
				poly_t *poly;
				uint32 sieve_offset;
				uint32 sign_of_index;

				if (vectors[i].r0 != NULL)
					relation = vectors[i].r0;
				else
					relation = vectors[i].r1->list[j];
				
				/* reconstruct a[i], b[i], x[i] and
				   the sign of x[i]. Drop the subscript
				   from here on. */

				poly = poly_list + relation->poly_idx;
				b = &poly->b;
				a = poly_a_list + poly->a_idx;
				sieve_offset = relation->sieve_offset & 
								0x7fffffff;
				sign_of_index = relation->sieve_offset >> 31;

				/* Form (a * sieve_offset + b). Note that 
				   sieve_offset can be negative; in that
				   case the minus sign is implicit. We don't
				   have to normalize mod n because there
				   are an even number of negative values
				   to multiply together */
	
				mp_mul_1(a, sieve_offset, &sum);
				if (sign_of_index == POSITIVE)
					mp_add(&sum, b, &sum);
				else
					mp_sub(&sum, b, &sum);
	
				/* multiply the sum into x */
	
				mp_mul(&x, &sum, &tmp);
				mp_mod(&tmp, n, &r, &x);
	
				/* do not multiply the factors associated 
				   with this relation into y; instead, just 
				   update the count for each factor base 
				   prime. Unlike ordinary MPQS, the list
				   of factors is for the complete factor-
				   ization of a*(a*x^2+b*x+c), so the 'a' 
				   in front need not be treated separately */
	
				for (k = 0; k < relation->num_factors; k++)
					fb_counts[relation->fb_offsets[k]]++;
					
				/* if the sieve value contains one or more
				   large primes, accumulate them in a 
				   dedicated table. Do not multiply them
				   into y until all of the sieve values
				   for this relation have been processed */

				for (k = 0; k < 2; k++) {
					prime = relation->large_prime[k];
					if (prime == 1)
						continue;

					for (m = 0; m < num_large_primes; m++) {
						if (prime == large_primes[2*m]){
							large_primes[2*m+1]++;
							break;
						}
					}
					if (m == num_large_primes) {
						large_primes[2*m] = prime;
						large_primes[2*m+1] = 1;
						num_large_primes++;
					}
				}
			}

			for (j = 0; j < num_large_primes; j++) {
				for (k = 0; k < large_primes[2*j+1]/2; k++) {
					mp_mul_1(&y, large_primes[2*j], &tmp);
					mp_mod(&tmp, n, &r, &y);
				}
			}
		}

		/* For each factor base prime p, compute 
			p ^ ((number of times p occurs in y) / 2) mod n
		   then multiply it into y. This is enormously
		   more efficient than multiplying by one p at a time */

		for (i = MIN_FB_OFFSET; i < fb_size; i++) {
			uint32 mask2 = 0x80000000;
			uint32 exponent = fb_counts[i] / 2;
			mp_t prod;
			uint32 prime = factor_base[i].prime;

			if (exponent == 0)
				continue;

			mp_clear(&tmp);
			tmp.nwords = 1;
			tmp.val[0] = factor_base[i].prime; 

			while (!(exponent & mask2))
				mask2 >>= 1;
			for (mask2 >>= 1; mask2; mask2 >>= 1) {
				mp_mul(&tmp, &tmp, &prod);
				mp_mod(&prod, n, &r, &tmp);
				if (exponent & mask2) {
					mp_mul_1(&tmp, prime, &prod);
					mp_mod(&prod, n, &r, &tmp);
				}
			}
			mp_mul(&tmp, &y, &prod);
			mp_mod(&prod, n, &r, &y);
		}

		/* compute gcd(x+-y, n). If it's not 1 or n, and we 
		   haven't seen the result before, save it */

		for (j = 0; j < 2; j++) {
			if (j == 0)
				mp_add(&x, &y, &tmp);
			else {
				if (mp_cmp(&x, &y) > 0)
					mp_sub(&x, &y, &tmp);
				else
					mp_sub(&y, &x, &tmp);
			}
			mp_gcd(&tmp, n, &tmp);
			if (mp_cmp(&tmp, n) != 0 && !mp_is_one(&tmp)) {
				for (i = 0; i < num_found; i++)
					if (mp_cmp(factor_found[i], &tmp) == 0)
						break;
				if (i == num_found) {
					factor_found[num_found] = 
						(mp_t *)malloc(sizeof(mp_t));
					mp_copy(&tmp, factor_found[num_found]);
					num_found++;
				}
			}
		}
	}

	/* walk through the list of factors and reduce each
	   to a single prime */

	for (i = 0; i < num_found; i++) {
		for (j = 0; j < num_found; j++) {
			int32 sign1, sign2;

			if (i == j)
				continue;

			/* do i and j have any factors in common? */

			mp_gcd(factor_found[i], factor_found[j], &tmp);
			if (mp_is_one(&tmp))
				continue;

			/* yes, divide the factor out of i or j but 
			   not both. Also don't divide if the factor 
			   equals i or equals j */

			sign1 = mp_cmp(factor_found[i], &tmp);
			sign2 = mp_cmp(factor_found[j], &tmp);

			if (sign2 != 0) {
				mp_div(factor_found[j], &tmp, NULL, &x);
				mp_copy(&x, factor_found[j]);
			}
			else if (sign1 != 0) {
				mp_div(factor_found[i], &tmp, NULL, &x);
				mp_copy(&x, factor_found[i]);
			}
		}
	}

	/* sort the remaining factors */

	if (num_found == 0 || (num_found == 1 && multiplier > 1))
		logprintf(obj, "No factors found :(\n");
	else
		qsort(factor_found, (size_t)num_found, 
			sizeof(relation_t *), sort_mp);

	/* Walk through the list again and print any factors 
	   that have not been printed already */

	for (i = 0; i < num_found; i++) {
		for (j = 0; j < i; j++) {
			if (mp_cmp(factor_found[i], factor_found[j]) == 0)
				break;
		}
		if (j == i && 
		    !(factor_found[j]->nwords == 1 && 
		      multiplier > 1 &&
		      multiplier % factor_found[j]->val[0] == 0)) {

		      	enum msieve_factor_type factor_type;
			
			factor_type = (mp_is_prime(factor_found[i],
						&obj->seed1, &obj->seed2)) ? 
						MSIEVE_PROBABLE_PRIME :
						MSIEVE_COMPOSITE;
			add_next_factor(obj, factor_found[i], factor_type);
		}
	}

	for (i = 0; i < num_found; i++)
		free(factor_found[i]);

	free(fb_counts);
}

