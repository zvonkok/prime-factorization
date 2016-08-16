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

#define HASH_MULT ((uint32)40499 * 65543)
#define HASH(a) (((a) * HASH_MULT) >> (32 - LOG2_CYCLE_HASH))

/*--------------------------------------------------------------------*/
void free_relation_list(relation_list_t *list) {

	uint32 i;

	for (i = 0; i < list->num_relations; i++) {
		free(list->list[i]->fb_offsets);
		free(list->list[i]);
	}
	free(list->list);
	free(list);
}

/*--------------------------------------------------------------------*/
void free_cycle_list(cycle_list_t *cycle_list) {

	uint32 i;

	for (i = 0; i < cycle_list->num_cycles; i++)
		free(cycle_list->list[i].list);
	free(cycle_list->list);
	free(cycle_list);
}

/*--------------------------------------------------------------------*/
static int compare_relations(const void *x, const void *y) {

	/* Callback used to sort a list of sieve relations.
	   Sorting is by size of large primes, then by number
	   of factors, then by factor values. Only the first
	   rule is needed for ordinary MPQS, but with self-
	   initialization we have to detect duplicate relations,
	   and this is easier if they are sorted as described */

	relation_t **xx = (relation_t **)x;
	relation_t **yy = (relation_t **)y;
	uint32 i;

	if (xx[0]->large_prime[1] > yy[0]->large_prime[1])
		return 1;
	if (xx[0]->large_prime[1] < yy[0]->large_prime[1])
		return -1;

	if (xx[0]->large_prime[0] > yy[0]->large_prime[0])
		return 1;
	if (xx[0]->large_prime[0] < yy[0]->large_prime[0])
		return -1;

	if (xx[0]->num_factors > yy[0]->num_factors)
		return 1;
	if (xx[0]->num_factors < yy[0]->num_factors)
		return -1;

	for (i = 0; i < xx[0]->num_factors; i++) {
		if (xx[0]->fb_offsets[i] > yy[0]->fb_offsets[i])
			return 1;
		if (xx[0]->fb_offsets[i] < yy[0]->fb_offsets[i])
			return -1;
	}
	return 0;
}

/*--------------------------------------------------------------------*/
static void purge_duplicate_relations(msieve_obj *obj,
				relation_list_t *rlist) {

	uint32 i, j;
	
	/* remove duplicates from rlist */

	if (rlist->num_relations < 2)
		return;

	qsort(rlist->list, (size_t)rlist->num_relations,
		sizeof(relation_t *), compare_relations);
	
	for (i = 1, j = 0; i < rlist->num_relations; i++) {
		if (compare_relations(rlist->list + j,
				      rlist->list + i) == 0) {
			free(rlist->list[i]->fb_offsets);
			free(rlist->list[i]);
		}
		else {
			rlist->list[++j] = rlist->list[i];
		}
	}

	j++;
	if (j != rlist->num_relations)
		logprintf(obj, "freed %d duplicate relations\n", 
					rlist->num_relations - j);
	rlist->num_relations = j;
}

/*--------------------------------------------------------------------*/
void save_relation(sieve_conf_t *conf, uint32 sieve_offset,
		uint32 *fb_offsets, uint32 num_factors, 
		relation_list_t *list, uint32 poly_index,
		uint32 large_prime1, uint32 large_prime2) {

	/* output a relation in ascii to a text file. The output
	   format is

	   R [-]<sieve_value> <poly_index> <list_of_fb_offsets> L <large_primes>

	   All numbers are in hex without leading zeros, to save space. 
	   One or both large primes may be 1
	   
	   NOTE: poly_index is local to the current 'a' value; hence if
	   and 'a' value allows 128 polynomials then 0 <= poly_index < 128.
	   The filtering stage will have to map that number to another 
	   number unique among all polynomials occurring in the 
	   factorization */

	uint32 i, j;
	char buf[512];

	i = sprintf(buf, "R ");

	if (sieve_offset & 0x80000000)
		i += sprintf(buf + i, "-%x ", sieve_offset & 0x7fffffff);
	else
		i += sprintf(buf + i, "%x ", sieve_offset & 0x7fffffff);
	
	i += sprintf(buf + i, "%x ", poly_index);

	for (j = 0; j < num_factors; j++)
		i += sprintf(buf + i, "%x ", fb_offsets[j]);

	if (large_prime1 < large_prime2)
		i += sprintf(buf + i, "L %x %x\n", large_prime1, large_prime2);
	else
		i += sprintf(buf + i, "L %x %x\n", large_prime2, large_prime1);

	print_to_savefile(conf->obj, buf);

	/* for partial relations, also update the bookeeping for
	   tracking the number of fundamental cycles */

	if (large_prime1 != large_prime2)
		add_to_cycles(conf, large_prime1, large_prime2);

	list->num_relations++;
}

/*--------------------------------------------------------------------*/
void read_large_primes(char *buf, uint32 *prime1, uint32 *prime2) {

	char *next_field;
	uint32 p1, p2;

	*prime1 = p1 = 1;
	*prime2 = p2 = 2;
	if (*buf != 'L')
		return;

	buf++;
	while (isspace(*buf))
		buf++;
	if (isxdigit(*buf)) {
		p1 = strtoul(buf, &next_field, 16);
		buf = next_field;
	}
	else {
		return;
	}

	while (isspace(*buf))
		buf++;
	if (isxdigit(*buf))
		p2 = strtoul(buf, &next_field, 16);
	
	if (p1 < p2) {
		*prime1 = p1;
		*prime2 = p2;
	}
	else {
		*prime1 = p2;
		*prime2 = p1;
	}
}

/*--------------------------------------------------------------------*/
static relation_t * read_relation(sieve_conf_t *conf, 
				char *relation_buf) {
	
	/* Convert an ascii representation of a sieve relation
	   into a relation_t. Also verify that the relation is
	   not somehow corrupted.

	   Note that the parsing here is not really bulletproof;
	   if you want bulletproof, build an XML spec and then a
	   parser for *that* */

	relation_t *r;
	uint32 i, j, k;
	uint32 num_factors;
	char *next_field;
	uint32 sieve_offset;
	uint32 sign_of_index;
	uint32 fb_offsets[64];
	mp_t t0, t1;
	uint32 num_poly_factors = conf->num_poly_factors;
	uint32 *poly_factors = conf->poly_factors;
	uint32 num_derived_poly = 1 << (num_poly_factors - 1);
	uint32 poly_index;

	/* read the sieve offset (which may be negative) */

	relation_buf += 2;
	sign_of_index = POSITIVE;
	if (*relation_buf == '-') {
		sign_of_index = NEGATIVE;
		relation_buf++;
	}

	if (!isxdigit(*relation_buf))
		return NULL;

	sieve_offset = strtoul(relation_buf, &next_field, 16);
	relation_buf = next_field;
	while (isspace(*relation_buf))
		relation_buf++;

	/* read the polynomial index, make sure that
	   0 <= poly_index < num_derived_poly */

	if (!isxdigit(*relation_buf))
		return NULL;

	poly_index = strtoul(relation_buf, &next_field, 16);
	relation_buf = next_field;
	if (poly_index >= num_derived_poly)
		return NULL;
	while (isspace(*relation_buf))
		relation_buf++;

	/* keep reading factors (really offsets into the factor
	   base) until an 'L' is encountered or a failure occurs */

	num_factors = 0;
	while (num_factors < 64 && *relation_buf != 'L') {
		if (isxdigit(*relation_buf)) {
			i = strtoul(relation_buf, &next_field, 16);

			/* factor base offsets must be sorted into
			   ascending order */

			if (num_factors > 0 && i < fb_offsets[num_factors-1])
				return NULL;
			else
				fb_offsets[num_factors++] = i;
			
			relation_buf = next_field;
		}
		else {
			break;
		}
		while (isspace(*relation_buf))
			relation_buf++;
	}

	if (*relation_buf != 'L')
		return NULL;

	/* allocate the relation_t and start filling it */

	r = (relation_t *)malloc(sizeof(relation_t));
	r->sieve_offset = sieve_offset | (sign_of_index << 31);
	r->poly_idx = poly_index;
	r->num_factors = num_factors + num_poly_factors;
	r->fb_offsets = (uint32 *)malloc(r->num_factors * sizeof(uint32));
	read_large_primes(relation_buf, r->large_prime, r->large_prime + 1);

	/* combine the factors of the sieve value with
	   the factors of the polynomial 'a' value; the 
	   linear algebra code has to know about both.
	   Because both lists are sorted, this is just
	   a merge operation */

	i = j = k = 0;
	while (i < num_factors && j < num_poly_factors) {
		if (fb_offsets[i] < poly_factors[j]) {
			r->fb_offsets[k++] = fb_offsets[i++];
		}
		else if (fb_offsets[i] > poly_factors[j]) {
			r->fb_offsets[k++] = poly_factors[j++];
		}
		else {
			r->fb_offsets[k] = fb_offsets[i++];
			r->fb_offsets[k+1] = poly_factors[j++];
			k += 2;
		}
	}
	while (i < num_factors)
		r->fb_offsets[k++] = fb_offsets[i++];
	while (j < num_poly_factors)
		r->fb_offsets[k++] = poly_factors[j++];
	
	/* The relation has now been read in; verify that it
	   really works. First compute (a * sieve_offset + b)^2 - n */

	mp_mul_1(&conf->curr_a, sieve_offset, &t0);
	if (sign_of_index == POSITIVE)
		mp_add(&t0, &(conf->curr_b[r->poly_idx]), &t0);
	else
		mp_sub(&t0, &(conf->curr_b[r->poly_idx]), &t0);

	mp_mul(&t0, &t0, &t1);
	if (mp_cmp(&t1, conf->n) > 0) {
		/* if the squared value is larger than n, the
		   first factor in the list must not be -1 */

		mp_sub(&t1, conf->n, &t1);
		if (r->fb_offsets[0] == 0)
			goto read_failed;
	}
	else {
		/* otherwise the first factor in the list 
		   must equal -1 */

		mp_sub(conf->n, &t1, &t1);
		if (r->fb_offsets[0] != 0)
			goto read_failed;
	}

	/* make sure that the list of factors we have
	   for this relation represent the complete
	   factorization */

	for (i = 0; i < r->num_factors; i++) {
		uint32 prime;
		uint32 fb_offset = r->fb_offsets[i];

		if (i > 0 && fb_offset == 0)
			break;

		if (fb_offset == 0 || fb_offset >= conf->fb_size)
			continue;

		prime = conf->factor_base[fb_offset].prime;
		if (mp_divrem_1(&t1, prime, &t1) != 0)
			break;
	}
	if (r->large_prime[0] > 1)
		if (mp_divrem_1(&t1, r->large_prime[0], &t1) != 0)
			goto read_failed;
	if (r->large_prime[1] > 1)
		if (mp_divrem_1(&t1, r->large_prime[1], &t1) != 0)
			goto read_failed;

	if (mp_is_one(&t1))
		return r;

read_failed:
	free(r->fb_offsets);
	free(r);
	return NULL;
}

/*--------------------------------------------------------------------*/
static void read_poly_a(sieve_conf_t *conf, 
			char *poly_buf) {

	uint32 i, j;
	char *next_field;
	uint32 num_factors = conf->num_poly_factors;
	uint32 *poly_factors = conf->poly_factors;
	mp_t *a = &conf->curr_a;

	/* The ascii format for a polynomial 'a' value is
	
	   A <list_of_fb_offsets>

	   Read in the list and multiply together the primes
	   corresponding to these factor base offsets 

	   If any factors could not be read or do not appear in
	   sorted order, set them to a wrong default value. All
	   relations corresponding to the bad 'a' value will be
	   flagged as corrupt later */
	   
	poly_buf += 2;
	for (i = 0; i < num_factors; i++) {
		if (isxdigit(*poly_buf)) {
			j = strtoul(poly_buf, &next_field, 16);
			poly_buf = next_field;

			if (i > 0 && j <= poly_factors[i-1])
				poly_factors[i] = conf->fb_size - 1;
			else
				poly_factors[i] = j;
		}
		else {
			poly_factors[i] = conf->fb_size - 1;
		}
		while (isspace(*poly_buf))
			poly_buf++;
	}

	mp_clear(a);
	a->nwords = a->val[0] = 1;
	for (i = 0; i < conf->num_poly_factors; i++) {
		uint32 j = poly_factors[i];

		if (j < conf->fb_size) {
			uint32 prime = conf->factor_base[j].prime;
			mp_mul_1(a, prime, a);
		}
	}
}

/*--------------------------------------------------------------------*/
static cycle_t *get_table_entry(cycle_t *table, uint32 *hashtable,
				uint32 prime, uint32 new_entry_offset) {

	/* return a pointer to a unique cycle_t specific
	   to 'prime'. The value of 'prime' is hashed and
	   the result used to index 'table'. If prime does
	   not appear in table, specify 'new_entry_offset'
	   as the table entry for prime.

	   Values of 'prime' which hash to the same offset
	   in hashtable are connected by a linked list of
	   offsets in 'table'. */

	uint32 offset, first_offset;
	cycle_t *entry = NULL;

	first_offset = HASH(prime);
	offset = hashtable[first_offset];

	/* follow the list of entries corresponding to
	   primes that hash to 'offset', stopping if either
	   the list runs out or we find an entry that's
	   dedicated to 'prime' already in the table */

	while (offset != 0) {
		entry = table + offset;
		if (entry->prime == prime)
			break;
		offset = entry->next;
	}

	/* if an entry was not found, initialize a new one */

	if (offset == 0) {
		entry = table + new_entry_offset;
		entry->next = hashtable[first_offset];
		entry->prime = prime;
		entry->data = 0;
		entry->count = 0;
		hashtable[first_offset] = new_entry_offset;
	}

	return entry;
}

/*--------------------------------------------------------------------*/
static uint32 add_to_hashtable(cycle_t *table, uint32 *hashtable, 
			uint32 prime1, uint32 prime2, 
			uint32 default_table_entry, 
			uint32 *components, uint32 *vertices) {

	/* update the list of cycles to reflect the presence
	   of a partial relation with large primes 'prime1'
	   and 'prime2'.

	   There are three quantities to track, the number of
	   edges, components and vertices. The number of cycles 
	   in the graph is then e + c - v. There is one edge for
	   each partial relation, and one vertex for each prime
	   that appears in the graph (these are easy to count).

	   A connected component is a group of primes that are
	   all reachable from each other via edges in the graph.
	   All of the vertices that are in a cycle must belong
	   to the same connected component. Think of a component
	   as a tree of vertices; one prime is chosen arbitrarily
	   as the 'root' of that tree.
	   
	   The number of new primes added to the graph (0, 1, or 2)
	   is returned */

	uint32 root[2];
	uint32 root1, root2;
	uint32 i;
	uint32 num_new_entries = 0;

	/* for each prime */

	for (i = 0; i < 2; i++) {
		uint32 prime = ((i == 0) ? prime1 : prime2);
		uint32 offset; 
		cycle_t *entry;

		/* retrieve the cycle_t corresponding to that
		   prime from the graph (or allocate a new one) */

		entry = get_table_entry(table, hashtable,
					prime, default_table_entry);
		entry->count++;

		if (entry->data == 0) {

			/* if this prime has not occurred in the graph
			   before, increment the number of vertices and
			   the number of components, then make the table
			   entry point to itself. */

			num_new_entries++;
			default_table_entry++;

			offset = entry - table;
			entry->data = offset;
			(*components)++;
			(*vertices)++;
		}
		else {
			/* the prime is already in the table, which
			   means we can follow a linked list of pointers
			   to other primes until we reach a cycle_t
			   that points to itself. This last cycle_t is
			   the 'root' of the connected component that
			   contains 'prime'. Save its value */

			cycle_t *first_entry, *next_entry;

			first_entry = entry;
			next_entry = table + entry->data;
			while (entry != next_entry) {
				entry = next_entry;
				next_entry = table + next_entry->data;
			}
				
			/* Also perform path compression: now that we
			   know the value of the root for this prime,
			   make all of the pointers in the primes we
			   visited along the way point back to this root.
			   This will speed up future root lookups */

			offset = entry->data;
			entry = first_entry;
			next_entry = table + entry->data;
			while (entry != next_entry) {
				entry->data = offset;
				entry = next_entry;
				next_entry = table + next_entry->data;
			}
		}

		root[i] = offset;
	}
				
	/* If the roots for prime1 and prime2 are different,
	   then they lie within separate connected components.
	   We're about to connect this edge to one of these
	   components, and the presence of the other prime
	   means that these two components are about to be
	   merged together. Hence the total number of components
	   in the graph goes down by one. */

	root1 = root[0];
	root2 = root[1];
	if (root1 != root2)
		(*components)--;
	
	/* This partial relation represents an edge in the
	   graph; we have to attach this edge to one or the
	   other of the connected components. Attach it to
	   the component whose representative prime is smallest;
	   since small primes are more common, this will give
	   the smaller root more edges, and will potentially
	   increase the number of cycles the graph contains */

	if (table[root1].prime < table[root2].prime)
		table[root2].data = root1;
	else
		table[root1].data = root2;
	
	return num_new_entries;
}

/*--------------------------------------------------------------------*/
void add_to_cycles(sieve_conf_t *conf, uint32 prime1, uint32 prime2) {

	/* Top level routine for updating the graph of partial
	   relations */

	uint32 table_size = conf->cycle_table_size;
	uint32 table_alloc = conf->cycle_table_alloc;
	cycle_t *table = conf->cycle_table;
	uint32 *hashtable = conf->cycle_hashtable;

	/* make sure there's room for new primes */

	if (table_size + 2 >= table_alloc) {
		table_alloc = conf->cycle_table_alloc = 2 * table_alloc;
		conf->cycle_table = (cycle_t *)realloc(conf->cycle_table,
						table_alloc * sizeof(cycle_t));
		table = conf->cycle_table;
	}

	conf->cycle_table_size += add_to_hashtable(table, hashtable, 
						prime1, prime2, 
						table_size, 
						&conf->components, 
						&conf->vertices);
}

/*--------------------------------------------------------------------*/
static uint32 purge_singletons(msieve_obj *obj, relation_t *list, 
				uint32 num_relations,
				cycle_t *table, uint32 *hashtable) {
	
	/* given a list of relations and the graph from the
	   sieving stage, remove any relation that contains
	   a prime that only occurs once in the graph. Because
	   removing a relation removes a second prime as well,
	   this process must be iterated until no more relations
	   are removed */

	uint32 num_left;
	uint32 i, j, k;
	uint32 passes = 0;

	logprintf(obj, "begin with %u relations\n", num_relations);

	do {
		num_left = num_relations;

		/* for each partial relation */

		for (i = j = 0; i < num_relations; i++) {
			relation_t *r = list + i;
			uint32 prime;
			cycle_t *entry;

			/* for each prime in that relation */

			for (k = 0; k < 2; k++) {
				prime = r->large_prime[k];
				entry = get_table_entry(table, hashtable,
							prime, 0);

				/* if the relation is due to be removed,
				   decrement the count of its other
				   primes in the graph. The following is
				   specialized for two primes */

				if (entry->count < 2) {
					prime = r->large_prime[k ^ 1];
					entry = get_table_entry(table, 
								hashtable, 
								prime, 0);
					entry->count--;
					break;
				}
			}

			if (k == 2)
				list[j++] = list[i];
		}
		num_relations = j;
		passes++;

	} while (num_left != num_relations);
				
	logprintf(obj, "reduce to %u relations in %u passes\n", 
				num_left, passes);
	return num_left;
}

/*--------------------------------------------------------------------*/
static void enumerate_cycle(msieve_obj *obj, 
			    cycle_list_t *cycle_list, 
			    cycle_t *table,
			    cycle_t *entry1, cycle_t *entry2,
			    relation_list_t *rlist, 
			    relation_t *final_relation) {

	/* given two entries out of the hashtable, corresponding
	   to two distinct primes, generate the list of relations
	   that participate in the cycle that these two primes
	   have just created. final_relation is the relation_t
	   to which the two primes belong, and the completed cycle
	   is packed into a relation_list_t and added to cycle_list */

	uint32 traceback1[100];
	uint32 traceback2[100];
	uint32 num1, num2;
	uint32 i, j;
	relation_list_t *r;

	/* Follow each cycle_t back up the graph until
	   the root component for this cycle is reached.
	   For each prime encountered along the way, save
	   the offset of the relation containing that prime */

	num1 = 0;
	while (entry1 != table + entry1->data) {
		if (num1 >= 100) {
			logprintf(obj, "warning: cycle too long, "
					"skipping it\n");
			return;
		}
		traceback1[num1++] = entry1->count;
		entry1 = table + entry1->data;
	}

	num2 = 0;
	while (entry2 != table + entry2->data) {
		if (num2 >= 100) {
			logprintf(obj, "warning: cycle too long, "
					"skipping it\n");
			return;
		}
		traceback2[num2++] = entry2->count;
		entry2 = table + entry2->data;
	}

	/* Now walk backwards through the lists, until
	   either one list runs out or a relation is
	   encountered that does not appear in both lists */

	while (num1 > 0 && num2 > 0) {
		if (traceback1[num1 - 1] != traceback2[num2 - 1])
			break;
		num1--; 
		num2--;
	}

	/* Now that we know how many relations are in the
	   cycle, allocate a relation_list_t to hold them */

	r = cycle_list->list + cycle_list->num_cycles;
	r->num_relations = num1 + num2 + 1;
	r->list = (relation_t **)malloc(r->num_relations * 
					sizeof(relation_t *));
	
	/* Combine the two lists of relations */
	for (i = 0; i < num1; i++)
		r->list[i] = rlist->list[traceback1[i]];

	for (j = 0; j < num2; j++, i++)
		r->list[i] = rlist->list[traceback2[j]];

	/* Add the relation that created the cycle in the
	   first place */

	r->list[i] = final_relation;
	cycle_list->num_cycles++;
}

/*--------------------------------------------------------------------*/
int sort_cycles(const void *x, const void *y) {
	relation_list_t *xx = (relation_list_t *)x;
	relation_list_t *yy = (relation_list_t *)y;

	/* Callback for sorting a list of cycles by the
	   total number of factors in all relations occurring
	   in the cycle */

	uint32 i, num1, num2;

	for (i = num1 = 0; i < xx->num_relations; i++)
		num1 += xx->list[i]->num_factors;
	for (i = num2 = 0; i < yy->num_relations; i++)
		num2 += yy->list[i]->num_factors;
	
	return num1 - num2;
}

/*--------------------------------------------------------------------*/
void filter_relations(sieve_conf_t *conf) {

	/* Perform all of the postprocessing on the list
	   of relations from the sieving phase. There are
	   two main jobs, reading in all the relations that
	   will be used and then determining the list of 
	   cycles in which partial relations appear. Care
	   should be taken to avoid wasting huge amounts of
	   memory */

	msieve_obj *obj = conf->obj;
	uint32 *hashtable = conf->cycle_hashtable;
	cycle_t *table = conf->cycle_table;
	uint32 num_poly_factors = conf->num_poly_factors;
	uint32 num_derived_poly = 1 << (num_poly_factors - 1);
	uint32 *final_poly_index = conf->root_correction;
	uint32 num_relations, num_cycles, num_poly;
	cycle_list_t *cycle_list;
	relation_t *initial_list;
	relation_list_t *full_list = conf->full_list;
	relation_list_t *partial_list = conf->partial_list;

	uint32 i, j, k, passes, start;
	uint32 curr_a_idx, curr_poly_idx, curr_partial, curr_initial; 
	uint32 total_poly_a;
	uint32 poly_saved;
	uint32 cycle_bins[8] = {0};
	uint32 max_cycle_length;
	char buf[256];

 	/* Rather than reading all the relations in and 
	   then removing singletons, read only the large 
	   primes of each relation into an initial list,
	   remove the singletons, and then only read in
	   the relations that survive. This avoids reading
	   in useless relations (and usually the polynomials 
	   they would need).
	   
	   Also count the total number of full and partial
	   relations; don't trust that these numbers start
	   off correct */

	obj->savefile = fopen(obj->savefile_name, "r");
	if (obj->savefile == NULL) {
		logprintf(obj, "error: filtering cannot open savefile\n");
		exit(-1);
	}

	initial_list = (relation_t *)malloc(1000 * sizeof(relation_t));
	curr_partial = 1000;
	i = j = 0;
	total_poly_a = 0;
	fgets(buf, (int)sizeof(buf), obj->savefile);

	while (!feof(obj->savefile)) {
		char *start;

		switch (buf[0]) {
		case 'A':
			total_poly_a++;
			break;

		case 'R':
			start = strchr(buf, 'L');
			if (start != NULL) {
				uint32 prime1, prime2;
				read_large_primes(start, &prime1, &prime2);
				if (j == curr_partial) {
					curr_partial = 3 * curr_partial / 2;
					initial_list = (relation_t *)realloc(
							initial_list,
							curr_partial *
							sizeof(relation_t));
				}
				if (prime1 == prime2) {
					i++;
				}
				else {
					initial_list[j].poly_idx = j;
					initial_list[j].large_prime[0] = prime1;
					initial_list[j].large_prime[1] = prime2;
					j++;
				}
			}
			break;
		}

		fgets(buf, (int)sizeof(buf), obj->savefile);
	}
	full_list->num_relations = i;
	partial_list->num_relations = j;
	num_relations = purge_singletons(obj, initial_list, 
					j, table, hashtable);
	initial_list = (relation_t *)realloc(initial_list, num_relations * 
							sizeof(relation_t));

	/* Create the relation lists, now that we know
	   how many relations each list will have. Also
	   initialize the lists of polynomial 'a' and 
	   'b' values */

	partial_list->num_relations = num_relations;
	full_list->list = (relation_t **)malloc(full_list->num_relations *
						sizeof(relation_t *));
	partial_list->list = (relation_t **)malloc(partial_list->num_relations *
						sizeof(relation_t *));
	num_poly = 1000;
	conf->poly_list = (poly_t *)malloc(num_poly * sizeof(poly_t));
	conf->poly_a_list = (mp_t *)malloc(total_poly_a * sizeof(mp_t));
	
	/* initialize the running counts of relations and
	   polynomials */

	i = j = k = 0;
	curr_initial = 0;
	curr_partial = (uint32)(-1);
	curr_poly_idx = (uint32)(-1);
	curr_a_idx = (uint32)(-1);
	poly_saved = 0;
	logprintf(obj, "attempting to read %u full and %u partial relations\n",
			full_list->num_relations, partial_list->num_relations);

	/* Read in the relations and the polynomials they use
	   at the same time. */

	fseek(obj->savefile, (long)0, SEEK_SET);

	while (i < full_list->num_relations ||
		j < partial_list->num_relations) {
		
		char *tmp;
		uint32 prime1, prime2;
		relation_t *r;

		/* read in the next entity */

		if (feof(obj->savefile))
			break;
		fgets(buf, (int)sizeof(buf), obj->savefile);

		switch (buf[0]) {
		case 'A':
			/* Read in a new 'a' value */

			curr_a_idx++;
			read_poly_a(conf, buf);
			mp_copy(&conf->curr_a, conf->poly_a_list+curr_a_idx);

			/* build all of the 'b' values associated with it */

			build_derived_poly(conf);

			/* all 'b' values start off unused */

			memset(final_poly_index, -1, num_derived_poly *
							sizeof(uint32));
			break;

		case 'R':
			/* read in a new relation. First find the 
			   large primes; these will determine
	     		   if a relation is full or partial */

			tmp = strchr(buf, 'L');
			if (tmp == NULL)
				break;

			read_large_primes(tmp, &prime1, &prime2);
			if (prime1 != prime2) {

				/* for partial relations, check if this 
				   relation was purged by the singleton 
				   filtering. If it survived then its 
				   ordinal ID will be in the next entry 
				   of initial_list. 
			   
				   First move up the index of initial_list 
				   until the relation index to check is >= 
				   the one we have (it may have gotten behind 
				   because relations were corrupted) */

				curr_partial++;
				while (curr_initial < num_relations &&
					initial_list[curr_initial].poly_idx <
							curr_partial) {
					curr_initial++;
				}

				/* check if this relation is a singleton; 
				   skip it if so */

				if (initial_list[curr_initial].poly_idx != 
							curr_partial)
					break;
				curr_initial++;
			}

			/* convert the ASCII text of the relation to a
			   relation_t, verifying correctness in the process */

			r = read_relation(conf, buf);
			if (r == NULL) {
				logprintf(obj, "failed to read relation %d\n", 
						i + curr_partial);
				break;
			}

			/* if necessary, save the b value corresponding 
			   to this relation */

			if (final_poly_index[r->poly_idx] == (uint32)(-1)) {
				if (k == num_poly) {
					num_poly *= 2;
					conf->poly_list = (poly_t *) realloc(
							conf->poly_list,
							num_poly *
							sizeof(poly_t));
				}
				conf->poly_list[k].a_idx = curr_a_idx;
				mp_copy(&(conf->curr_b[r->poly_idx]), 
					&(conf->poly_list[k].b));
				final_poly_index[r->poly_idx] = k;
				r->poly_idx = k++;
			}
			else {
				r->poly_idx = final_poly_index[r->poly_idx];
			}

			/* add the relation to the full or partial
			   relation list */

			if (r->large_prime[0] != r->large_prime[1])
				partial_list->list[j++] = r;
			else
				full_list->list[i++] = r;

			break;  /* done with this relation */
		}
	}

	/* update the structures with the counts of relations
	   and polynomials actually recovered */

	logprintf(obj, "recovered %u full and %u partial relations\n", i, j);
	logprintf(obj, "recovered %u polynomials\n", k);
	full_list->num_relations = i;
	partial_list->num_relations = num_relations = j;
	free(initial_list);
	fclose(obj->savefile);
	obj->savefile = NULL;
	conf->poly_list = (poly_t *)realloc(conf->poly_list,
					   k * sizeof(poly_t));

	/* begin the cycle generation process by purging
	   duplicate full and partial relations. For the
	   sake of consistency, always rebuild the graph
	   afterwards */

	purge_duplicate_relations(obj, full_list);
	purge_duplicate_relations(obj, partial_list);

	memset(hashtable, 0, sizeof(uint32) << LOG2_CYCLE_HASH);
	num_relations = partial_list->num_relations;
	conf->vertices = 0;
	conf->components = 0;
	conf->cycle_table_size = 1;

	for (i = 0; i < num_relations; i++) {
		relation_t *r = partial_list->list[i];
		add_to_cycles(conf, r->large_prime[0], r->large_prime[1]);
	}
	num_cycles = num_relations + conf->components - conf->vertices;

	/* The idea behind the cycle-finding code is this: the 
	   graph is composed of a bunch of connected components, 
	   and each component contains one or more cycles. To 
	   find the cycles, you build the 'spanning tree' for 
	   each component.

	   Think of the spanning tree as a binary tree; there are
	   no cycles in it because leaves are only connected to a
	   common root and not to each other. Any time you connect 
	   together two leaves of the tree, though, a cycle is formed.
	   So, for a spanning tree like this:

	         1
	         o
		/ \
	    2  o   o  3
	      / \   \
	     o   o   o
	     4   5   6

	   if you connect leaves 4 and 5 you get a cycle (4-2-5). If
	   you connect leaves 4 and 6 you get another cycle (4-2-1-3-6)
	   that will reuse two of the nodes in the first cycle. It's
	   this reuse that makes double large primes so powerful.

	   For our purposes, every edge in the tree above represents
	   a partial relation. Every edge that would create a cycle
	   comes from another partial relation. So to find all the cycles,
	   you begin with the roots of all of the connected components,
	   and then iterate through the list of partial relations until 
	   all have been 'processed'. A partial relation is considered 
	   processed when one or both of its primes is in the tree. If 
	   one prime is present then the relation gets added to the tree; 
	   if both primes are present then the relation creates one cycle 
	   but is *not* added to the tree. 
	   
	   It's really great to see such simple ideas do something so
	   complicated as finding cycles (and doing it very quickly) */

	/* First traverse the entire graph and remove any vertices
	   that are not the roots of connected components (i.e.
	   remove any primes whose cycle_t entry does not point
	   to itself */

	for (i = 0; i < (1 << LOG2_CYCLE_HASH); i++) {
		uint32 offset = hashtable[i];

		while (offset != 0) {
			cycle_t *entry = table + offset;
			if (offset != entry->data)
				entry->data = 0;
			offset = entry->next;
		}
	}

	/* we know how many cycles to expect */

	logprintf(obj, "attempting to build %u cycles\n", num_cycles);
	cycle_list = (cycle_list_t *)malloc(sizeof(cycle_list_t));
	cycle_list->num_cycles = 0;
	cycle_list->list = (relation_list_t *)malloc(num_cycles * 
					sizeof(relation_list_t));

	/* keep going until either all cycles are found, all
	   relations are processed, or cycles stop arriving. 
	   Normally these conditions all occur at the same time */

	for (start = passes = 0; start < num_relations && 
			cycle_list->num_cycles < num_cycles; passes++) {

		/* The list of partials up to index 'start' is con-
		   sidered processed. For all relations past that... */

		uint32 start_cycles = cycle_list->num_cycles;

		for (i = start; i < num_relations &&
				cycle_list->num_cycles < num_cycles; i++) {

			cycle_t *entry1, *entry2;
			relation_t *r = partial_list->list[i];
			
			/* retrieve the cycle_t entries associated
			   with the large primes in relation r. */

			entry1 = get_table_entry(table, hashtable, 
						r->large_prime[0], 0);
			entry2 = get_table_entry(table, hashtable, 
						r->large_prime[1], 0);

			/* if both vertices do not point to other
			   vertices, then neither prime has been added
			   to the graph yet, and r must remain unprocessed */

			if (entry1->data == 0 && entry2->data == 0)
				continue;

			/* if one or the other prime is part of the
			   graph, add r to the graph. The vertex not in
			   the graph points to the vertex that is, and
			   this entry also points to the relation that
			   is associated with r.

			   If both primes are in the graph, recover the
			   cycle this generates */

			if (entry1->data == 0) {
				entry1->data = entry2 - table;
				entry1->count = start;
			}
			else if (entry2->data == 0) {
				entry2->data = entry1 - table;
				entry2->count = start;
			}
			else {
				enumerate_cycle(obj, cycle_list, table, entry1,
						entry2, partial_list, r);
			}

			/* whatever happened above, the relation is
			   processed now; move it to position 'start'
			   of the relation list and increment 'start'.
			   The relation is now frozen at that position */

			partial_list->list[i] = partial_list->list[start];
			partial_list->list[start++] = r;
		}

		/* If this pass did not find any new cycles, then
		   we've reached steady state and are finished */

		if (cycle_list->num_cycles == start_cycles)
			break;
	}

	logprintf(obj, "found %u cycles in %u passes\n", 
				cycle_list->num_cycles, passes);
	
	/* sort the list of cycles so that the cycles with
	   the smallest number of factors will come first. 
	   If the linear algebra code skips any cycles it
	   will therefore skip the ones with the most factors */

	qsort(cycle_list->list, (size_t)(cycle_list->num_cycles), 
		sizeof(relation_list_t), sort_cycles);

	conf->cycle_list = cycle_list;

	/* print out a histogram of cycle lengths for infor-
	   mational purposes */

	for (i = max_cycle_length = 0; i < cycle_list->num_cycles; i++) {
		num_relations = cycle_list->list[i].num_relations;

		if (num_relations > max_cycle_length)
			max_cycle_length = num_relations;

		if (num_relations >= 9)
			cycle_bins[7]++;
		else
			cycle_bins[num_relations-2]++;
	}
	logprintf(obj, "distribution of cycle lengths:\n");
	for (i = 0; i < 7; i++) {
		if (cycle_bins[i]) {
			logprintf(obj, "   length %d : %d\n", 
					i + 2, cycle_bins[i]);
		}
	}
	if (cycle_bins[i])
		logprintf(obj, "   length %u+: %u\n", i + 2, cycle_bins[i]);
	logprintf(obj, "largest cycle: %u relations\n", max_cycle_length);
}
