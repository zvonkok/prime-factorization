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

static uint32 build_matrix(uint32 ncols, la_col_t *cols, 
		    	relation_list_t *full_relations, 
		    	cycle_list_t *cycle_list);

static void reduce_matrix(msieve_obj *obj,
			uint32 *nrows, 
			uint32 *ncols, 
			la_col_t *cols);

static uint64 * block_lanczos(msieve_obj *obj,
				uint32 nrows, 
				uint32 ncols, 
				la_col_t *cols);

#define BIT(x) ((uint64)(1) << (x))

static const uint64 bitmask[64] = {
	BIT( 0), BIT( 1), BIT( 2), BIT( 3), BIT( 4), BIT( 5), BIT( 6), BIT( 7),
	BIT( 8), BIT( 9), BIT(10), BIT(11), BIT(12), BIT(13), BIT(14), BIT(15),
	BIT(16), BIT(17), BIT(18), BIT(19), BIT(20), BIT(21), BIT(22), BIT(23),
	BIT(24), BIT(25), BIT(26), BIT(27), BIT(28), BIT(29), BIT(30), BIT(31),
	BIT(32), BIT(33), BIT(34), BIT(35), BIT(36), BIT(37), BIT(38), BIT(39),
	BIT(40), BIT(41), BIT(42), BIT(43), BIT(44), BIT(45), BIT(46), BIT(47),
	BIT(48), BIT(49), BIT(50), BIT(51), BIT(52), BIT(53), BIT(54), BIT(55),
	BIT(56), BIT(57), BIT(58), BIT(59), BIT(60), BIT(61), BIT(62), BIT(63),
};

/*------------------------------------------------------------------*/
void solve_linear_system(msieve_obj *obj, uint32 fb_size, 
		    la_col_t **vectors, uint32 *vsize,
		    uint64 **bitfield, relation_list_t *full_relations, 
		    cycle_list_t *cycle_list) {

	/* Generate linear dependencies among the relations
	   in full_relations and partial_relations */

	uint32 i;
	la_col_t *cols;
	uint32 nrows, ncols;
	uint32 total_weight;

	ncols = fb_size + NUM_EXTRA_RELATIONS;
	nrows = fb_size;

	if (full_relations->num_relations +
	    cycle_list->num_cycles < ncols) {
		logprintf(obj, "error: not enough relations to find "
				"linear dependencies\n");
		exit(-1);
	}

	cols = (la_col_t *)calloc((size_t)ncols, sizeof(la_col_t));

	/* convert the list of relations from the sieving 
	   stage into a matrix. */

	total_weight = build_matrix(ncols, cols, full_relations, cycle_list);

	logprintf(obj, "%u x %u system, weight %u (avg %4.2f/col)\n", 
				fb_size, ncols, total_weight, 
				(double)total_weight / ncols);

	/* reduce the matrix dimensions to ignore almost
	   empty rows */

	reduce_matrix(obj, &nrows, &ncols, cols);

	/* solve the linear system */

	do {
		*bitfield = block_lanczos(obj, nrows, ncols, cols);
	} while(*bitfield == NULL);

	for (i = 0; i < ncols; i++)
		free(cols[i].data);

	*vectors = cols;
	*vsize = ncols;
}

/*------------------------------------------------------------------*/
void reduce_matrix(msieve_obj *obj, uint32 *nrows, 
			uint32 *ncols, la_col_t *cols) {

	/* Perform light filtering on the nrows x ncols
	   matrix specified by cols[]. The processing here is
	   limited to deleting columns that contain a singleton
	   row, then resizing the matrix to have a few more
	   columns than rows. Because deleting a column reduces
	   the counts in several different rows, the process
	   must iterate to convergence.
	   
	   Note that this step is not intended to make the Lanczos
	   iteration run any faster (though it will); it's just
	   that if we don't go to this trouble then there are 
	   factorizations for which the matrix step will fail 
	   outright  */

	uint32 r, c, i, j, k;
	uint32 passes;
	uint32 *counts;
	uint32 reduced_rows;
	uint32 reduced_cols;

	/* count the number of nonzero entries in each row */

	counts = (uint32 *)calloc((size_t)*nrows, sizeof(uint32));
	for (i = 0; i < *ncols; i++) {
		for (j = 0; j < cols[i].weight; j++)
			counts[cols[i].data[j]]++;
	}

	reduced_rows = *nrows;
	reduced_cols = *ncols;
	passes = 0;

	do {
		r = reduced_rows;

		/* remove any columns that contain the only entry
		   in one or more rows, then update the row counts
		   to reflect the missing column. Iterate until
		   no more columns can be deleted */

		do {
			c = reduced_cols;
			for (i = j = 0; i < reduced_cols; i++) {
				la_col_t *col = cols + i;
				for (k = 0; k < col->weight; k++) {
					if (counts[col->data[k]] < 2)
						break;
				}
	
				if (k < col->weight) {
					for (k = 0; k < col->weight; k++) {
						counts[col->data[k]]--;
					}
					free(col->data);
				}
				else {
					cols[j++] = cols[i];
				}
			}
			reduced_cols = j;
		} while (c != reduced_cols);
	
		/* count the number of rows that contain a
		   nonzero entry */

		for (i = reduced_rows = 0; i < *nrows; i++) {
			if (counts[i])
				reduced_rows++;
		}

		/* Because deleting a column reduces the weight
		   of many rows, the number of nonzero rows may
		   be much less than the number of columns. Delete
		   more columns until the matrix has the correct
		   aspect ratio. Columns at the end of cols[] are
		   the heaviest, so delete those (and update the
		   row counts again) */

		for (i = reduced_rows + NUM_EXTRA_RELATIONS;
		     i < reduced_cols; i++) {

			la_col_t *col = cols + i;
			for (j = 0; j < col->weight; j++) {
				counts[col->data[j]]--;
			}
			free(col->data);
		}

		/* if any columns were deleted in the previous step,
		   then the matrix is less dense and more columns
		   can be deleted; iterate until no further deletions
		   are possible */

		reduced_cols = reduced_rows + NUM_EXTRA_RELATIONS;
		passes++;

	} while (r != reduced_rows);

	logprintf(obj, "reduce to %u x %u in %u passes\n", 
			reduced_rows, reduced_cols, passes);

	free(counts);

	/* record the final matrix size. Note that we can't touch
	   nrows because all the column data (and the sieving relations
	   that produced it) would have to be updated */

	*ncols = reduced_cols;
}

/*------------------------------------------------------------------*/
static uint32 merge_relations(uint32 *merge_array,
		  uint32 *src1, uint32 n1,
		  uint32 *src2, uint32 n2) {

	/* Given two sorted lists of integers, merge
	   the lists into a single sorted list with
	   duplicate entries removed. If a particular
	   entry occurs an even number of times in the
	   two input lists, don't add it to the final list
	   at all. Returns the number of elements in the
	   resulting list */

	uint32 i1, i2, val1, val2, count1, count2;
	uint32 num_merge;

	i1 = i2 = 0;
	num_merge = 0;

	while (i1 < n1 && i2 < n2) {
		val1 = src1[i1];
		val2 = src2[i2];

		if (val1 < val2) {
			count1 = 0;
			do {
				i1++; count1++;
			} while (i1 < n1 && src1[i1] == val1);

			if (count1 & 1)
				merge_array[num_merge++] = val1;
		}
		else if (val1 > val2) {
			count2 = 0;
			do {
				i2++; count2++;
			} while (i2 < n2 && src2[i2] == val2);

			if (count2 & 1)
				merge_array[num_merge++] = val2;
		}
		else {
			count1 = count2 = 0;
			do {
				i1++; count1++;
			} while (i1 < n1 && src1[i1] == val1);
			do {
				i2++; count2++;
			} while (i2 < n2 && src2[i2] == val2);

			if ( (count1 + count2) & 1 )
				merge_array[num_merge++] = val1;
		}
	}

	if (i2 == n2) {
		src2 = src1;
		i2 = i1;
		n2 = n1;
	}

	while (i2 < n2) {
		count2 = 0; val2 = src2[i2];

		do {
			i2++; count2++;
		} while (i2 < n2 && src2[i2] == val2);

		if (count2 & 1)
			merge_array[num_merge++] = val2;
	}

	return num_merge;
}

/*------------------------------------------------------------------*/
static uint32 build_matrix(uint32 ncols, la_col_t *cols, 
			   relation_list_t *full_relations, 
			   cycle_list_t *cycle_list) {

	/* Convert lists of relations from the sieving stage
	   into a sparse matrix. The matrix is stored by
	   columns, pointed to by 'cols'. The total number 
	   of nonzero entries in the matrix is returned */

	uint32 i, j, k;
	uint32 total_weight = 0;
	relation_t **list;
	la_col_t *col;

	/* handle all the full relations */

	list = full_relations->list;
	for (i = 0; i < ncols && i < full_relations->num_relations; i++) {
		col = &cols[i];
		col->r0 = list[i];
		col->r1 = NULL;
		col->data = (uint32 *)malloc(col->r0->num_factors * 
						sizeof(uint32));
		col->weight = merge_relations(col->data,
			     list[i]->fb_offsets, list[i]->num_factors,
			     NULL, 0);
		total_weight += col->weight;
	}

	/* turn the list of cycles into a list of matrix columns.
	   Cycles are assumed to be sorted in order of increasing
	   number of relations, so that any list members that are
	   not used would have created the heaviest matrix columns
	   anyway */

	for (j = 0; i < ncols; i++, j++) {
		relation_list_t *rlist = cycle_list->list + j;
		uint32 *buf1, *buf2;
		uint32 num1, num2;

		col = &cols[i];
		col->r0 = NULL;
		col->r1 = rlist;

		/* make a conservative estimate for the column
		   weight: the sum of all the factors in all relations
		   of cycle j */

		for (num1 = 0, k = 0; k < rlist->num_relations; k++)
			num1 += rlist->list[k]->num_factors;

		buf1 = (uint32 *)malloc(num1 * sizeof(uint32));
		buf2 = (uint32 *)malloc(num1 * sizeof(uint32));

		/* merge the first pair of relations */

		list = rlist->list;
		num2 = merge_relations(buf2,
			     list[0]->fb_offsets, list[0]->num_factors,
			     list[1]->fb_offsets, list[1]->num_factors);
		
		/* merge each succeeding relation into the accumulated
		   matrix column */

		for (k = 2; k < rlist->num_relations; k++) {
			num1 = num2;
			memcpy(buf1, buf2, num1 * sizeof(uint32));
			num2 = merge_relations(buf2, buf1, num1,
						list[k]->fb_offsets, 
						list[k]->num_factors);
		}

		free(buf1);
		col->weight = num2;
		col->data = (uint32 *)realloc(buf2, num2 * sizeof(uint32));
		total_weight += num2;
	}

	return total_weight;
}

/*-------------------------------------------------------------------*/
void mul_64x64_64x64(uint64 *a, uint64 *b, uint64 *c ) {

	/* c[][] = x[][] * y[][], where all operands are 64 x 64
	   (i.e. contain 64 words of 64 bits each). The result
	   may overwrite a or b. */

	uint64 ai, bj, accum;
	uint64 tmp[64];
	uint32 i, j;

	for (i = 0; i < 64; i++) {
		j = 0;
		accum = 0;
		ai = a[i];

		while (ai) {
			bj = b[j];
			if( ai & 1 )
				accum ^= bj;
			ai >>= 1;
			j++;
		}

		tmp[i] = accum;
	}
	memcpy(c, tmp, sizeof(tmp));
}

/*-----------------------------------------------------------------------*/
void precompute_Nx64_64x64(uint64 *x, uint64 *c) {

	/* Let x[][] be a 64 x 64 matrix in GF(2), represented
	   as 64 words of 64 bits each. Let c[][] be an 8 x 256
	   matrix of 64-bit words. This code fills c[][] with
	   a bunch of "partial matrix multiplies". For 0<=i<256,
	   the j_th row of c[][] contains the matrix product

	   	( i << (8*j) ) * x[][]

	   where the quantity in parentheses is considered a 
	   1 x 64 vector of elements in GF(2). The resulting
	   table can dramatically speed up matrix multiplies
	   by x[][]. */

	uint64 accum, xk;
	uint32 i, j, k, index;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 256; i++) {
			k = 0;
			index = i;
			accum = 0;
			while (index) {
				xk = x[k];
				if (index & 1)
					accum ^= xk;
				index >>= 1;
				k++;
			}
			c[i] = accum;
		}

		x += 8;
		c += 256;
	}
}

/*-------------------------------------------------------------------*/
void mul_Nx64_64x64_acc(uint64 *v, uint64 *x, uint64 *c, 
			uint64 *y, uint32 n) {

	/* let v[][] be a n x 64 matrix with elements in GF(2), 
	   represented as an array of n 64-bit words. Let c[][]
	   be an 8 x 256 scratch matrix of 64-bit words.
	   This code multiplies v[][] by the 64x64 matrix 
	   x[][], then XORs the n x 64 result into y[][] */

	uint32 i;
	uint64 word;

	precompute_Nx64_64x64(x, c);

	for (i = 0; i < n; i++) {
		word = v[i];
		y[i] ^=  c[ 0*256 + ((word>> 0) & 0xff) ]
		       ^ c[ 1*256 + ((word>> 8) & 0xff) ]
		       ^ c[ 2*256 + ((word>>16) & 0xff) ]
		       ^ c[ 3*256 + ((word>>24) & 0xff) ]
		       ^ c[ 4*256 + ((word>>32) & 0xff) ]
		       ^ c[ 5*256 + ((word>>40) & 0xff) ]
		       ^ c[ 6*256 + ((word>>48) & 0xff) ]
		       ^ c[ 7*256 + ((word>>56)       ) ];
	}
}

/*-------------------------------------------------------------------*/
void mul_64xN_Nx64(uint64 *x, uint64 *y,
		   uint64 *c, uint64 *xy, uint32 n) {

	/* Let x and y be n x 64 matrices. This routine computes
	   the 64 x 64 matrix xy[][] given by transpose(x) * y.
	   c[][] is a 256 x 8 scratch matrix of 64-bit words. */

	uint32 i;

	memset(c, 0, 256 * 8 * sizeof(uint64));
	memset(xy, 0, 64 * sizeof(uint64));

	for (i = 0; i < n; i++) {
		uint64 xi = x[i];
		uint64 yi = y[i];
		c[ 0*256 + ( xi        & 0xff) ] ^= yi;
		c[ 1*256 + ((xi >>  8) & 0xff) ] ^= yi;
		c[ 2*256 + ((xi >> 16) & 0xff) ] ^= yi;
		c[ 3*256 + ((xi >> 24) & 0xff) ] ^= yi;
		c[ 4*256 + ((xi >> 32) & 0xff) ] ^= yi;
		c[ 5*256 + ((xi >> 40) & 0xff) ] ^= yi;
		c[ 6*256 + ((xi >> 48) & 0xff) ] ^= yi;
		c[ 7*256 + ((xi >> 56)       ) ] ^= yi;
	}


	for(i = 0; i < 8; i++) {

		uint32 j;
		uint64 a0, a1, a2, a3, a4, a5, a6, a7;

		a0 = a1 = a2 = a3 = 0;
		a4 = a5 = a6 = a7 = 0;

		for (j = 0; j < 256; j++) {
			if ((j >> i) & 1) {
				a0 ^= c[0*256 + j];
				a1 ^= c[1*256 + j];
				a2 ^= c[2*256 + j];
				a3 ^= c[3*256 + j];
				a4 ^= c[4*256 + j];
				a5 ^= c[5*256 + j];
				a6 ^= c[6*256 + j];
				a7 ^= c[7*256 + j];
			}
		}

		xy[ 0] = a0; xy[ 8] = a1; xy[16] = a2; xy[24] = a3;
		xy[32] = a4; xy[40] = a5; xy[48] = a6; xy[56] = a7;
		xy++;
	}
}

/*-------------------------------------------------------------------*/
static uint32 find_nonsingular_sub(msieve_obj *obj,
				uint64 *t, uint32 *s, 
				uint32 *last_s, uint32 last_dim, 
				uint64 *w) {

	/* given a 64x64 matrix t[][] (i.e. sixty-four
	   64-bit words) and a list of 'last_dim' column 
	   indices enumerated in last_s[]: 
	   
	     - find a submatrix of t that is invertible 
	     - invert it and copy to w[][]
	     - enumerate in s[] the columns represented in w[][] */

	uint32 i, j;
	uint32 dim;
	uint32 cols[64];
	uint64 M[64][2];
	uint64 mask, *row_i, *row_j;
	uint64 m0, m1;

	/* M = [t | I] for I the 64x64 identity matrix */

	for (i = 0; i < 64; i++) {
		M[i][0] = t[i]; 
		M[i][1] = bitmask[i];
	}

	/* put the column indices from last_s[] into the
	   back of cols[], and copy to the beginning of cols[]
	   any column indices not in last_s[] */

	mask = 0;
	for (i = 0; i < last_dim; i++) {
		cols[63 - i] = last_s[i];
		mask |= bitmask[last_s[i]];
	}
	for (i = j = 0; i < 64; i++) {
		if (!(mask & bitmask[i]))
			cols[j++] = i;
	}

	/* compute the inverse of t[][] */

	for (i = dim = 0; i < 64; i++) {
	
		/* find the next pivot row and put in row i */

		mask = bitmask[cols[i]];
		row_i = M[cols[i]];

		for (j = i; j < 64; j++) {
			row_j = M[cols[j]];
			if (row_j[0] & mask) {
				m0 = row_j[0];
				m1 = row_j[1];
				row_j[0] = row_i[0];
				row_j[1] = row_i[1];
				row_i[0] = m0; 
				row_i[1] = m1;
				break;
			}
		}
				
		/* if a pivot row was found, eliminate the pivot
		   column from all other rows */

		if (j < 64) {
			for (j = 0; j < 64; j++) {
				row_j = M[cols[j]];
				if ((row_i != row_j) && (row_j[0] & mask)) {
					row_j[0] ^= row_i[0];
					row_j[1] ^= row_i[1];
				}
			}

			/* add the pivot column to the list of 
			   accepted columns */

			s[dim++] = cols[i];
			continue;
		}

		/* otherwise, use the right-hand half of M[]
		   to compensate for the absence of a pivot column */

		for (j = i; j < 64; j++) {
			row_j = M[cols[j]];
			if (row_j[1] & mask) {
				m0 = row_j[0];
				m1 = row_j[1];
				row_j[0] = row_i[0];
				row_j[1] = row_i[1];
				row_i[0] = m0; 
				row_i[1] = m1;
				break;
			}
		}
				
		if (j == 64) {
			logprintf(obj, "lanczos error: submatrix "
					"is not invertible\n");
			return 0;
		}
			
		/* eliminate the pivot column from the other rows
		   of the inverse */

		for (j = 0; j < 64; j++) {
			row_j = M[cols[j]];
			if ((row_i != row_j) && (row_j[1] & mask)) {
				row_j[0] ^= row_i[0];
				row_j[1] ^= row_i[1];
			}
		}

		/* wipe out the pivot row */

		row_i[0] = row_i[1] = 0;
	}

	/* the right-hand half of M[] is the desired inverse */
	
	for (i = 0; i < 64; i++) 
		w[i] = M[i][1];

	/* The block Lanczos recurrence depends on all columns
	   of t[][] appearing in s[] and/or last_s[]. 
	   Verify that condition here */

	mask = 0;
	for (i = 0; i < dim; i++)
		mask |= bitmask[s[i]];
	for (i = 0; i < last_dim; i++)
		mask |= bitmask[last_s[i]];

	if (mask != (uint64)(-1)) {
		logprintf(obj, "lanczos error: not all columns used\n");
		return 0;
	}

	return dim;
}

/*-------------------------------------------------------------------*/
void mul_MxN_Nx64(uint32 vsize, uint32 ncols, la_col_t *A,
		uint64 *x, uint64 *b) {

	/* Multiply the vector x[] by the matrix A (stored
	   columnwise) and put the result in b[]. vsize
	   refers to the number of uint64's allocated for
	   x[] and b[]; vsize is probably different from ncols */

	uint32 i, j;

	memset(b, 0, vsize * sizeof(uint64));
	
	for (i = 0; i < ncols; i++) {
		la_col_t *col = A + i;
		uint32 *row_entries = col->data;
		uint64 tmp = x[i];

		for (j = 0; j < col->weight; j++) {
			b[row_entries[j]] ^= tmp;
		}
	}
}

/*-------------------------------------------------------------------*/
void mul_trans_MxN_Nx64(uint32 ncols, la_col_t *A,
			uint64 *x, uint64 *b) {

	/* Multiply the vector x[] by the transpose of the
	   matrix A and put the result in b[]. Since A is stored
	   by columns, this is just a matrix-vector product */

	uint32 i, j;

	for (i = 0; i < ncols; i++) {
		la_col_t *col = A + i;
		uint32 *row_entries = col->data;
		uint64 accum = 0;

		for (j = 0; j < col->weight; j++) {
			accum ^= x[row_entries[j]];
		}
		b[i] = accum;
	}
}

/*-----------------------------------------------------------------------*/
static void transpose_vector(uint32 ncols, uint64 *v, uint64 **trans) {

	/* Hideously inefficent routine to transpose a
	   vector v[] of 64-bit words into a 2-D array
	   trans[][] of 64-bit words */

	uint32 i, j;
	uint32 col;
	uint64 mask, word;

	for (i = 0; i < ncols; i++) {
		col = i / 64;
		mask = bitmask[i % 64];
		word = v[i];
		j = 0;
		while (word) {
			if (word & 1)
				trans[j][col] |= mask;
			word = word >> 1;
			j++;
		}
	}
}

/*-----------------------------------------------------------------------*/
static void combine_cols(msieve_obj *obj, uint32 ncols, 
			uint64 *x, uint64 *v, 
			uint64 *ax, uint64 *av) {

	/* Once the block Lanczos iteration has finished, 
	   x[] and v[] will contain mostly nullspace vectors
	   between them, as well as possibly some columns
	   that are linear combinations of nullspace vectors.
	   Given vectors ax[] and av[] that are the result of
	   multiplying x[] and v[] by the matrix, this routine 
	   will use Gauss elimination on the columns of [ax | av] 
	   to find all of the linearly dependent columns. The
	   column operations needed to accomplish this are mir-
	   rored in [x | v] and the columns that are independent
	   are skipped. Finally, the dependent columns are copied
	   back into x[] and represent the nullspace vector output
	   of the block Lanczos code */

	uint32 i, j, k, bitpos, col, col_words;
	uint64 mask;
	uint64 *matrix[128], *amatrix[128], *tmp;

	col_words = (ncols + 63) / 64;

	for (i = 0; i < 128; i++) {
		matrix[i] = (uint64 *)calloc((size_t)col_words, 
					     sizeof(uint64));
		amatrix[i] = (uint64 *)calloc((size_t)col_words, 
					      sizeof(uint64));
	}

	/* operations on columns can more conveniently become 
	   operations on rows if all the vectors are first
	   transposed */

	transpose_vector(ncols, x, matrix);
	transpose_vector(ncols, v, matrix + 64);
	transpose_vector(ncols, ax, amatrix);
	transpose_vector(ncols, av, amatrix + 64);

	/* Keep eliminating rows until the unprocessed part
	   of amatrix[][] is all zero. The rows where this
	   happens correspond to linearly dependent vectors
	   in the nullspace */

	for (i = bitpos = 0; i < 128 && bitpos < ncols; bitpos++) {

		/* find the next pivot row */

		mask = bitmask[bitpos % 64];
		col = bitpos / 64;
		for (j = i; j < 128; j++) {
			if (amatrix[j][col] & mask) {
				tmp = matrix[i];
				matrix[i] = matrix[j];
				matrix[j] = tmp;
				tmp = amatrix[i];
				amatrix[i] = amatrix[j];
				amatrix[j] = tmp;
				break;
			}
		}
		if (j == 128)
			continue;

		/* a pivot was found; eliminate it from the
		   remaining rows */

		for (j++; j < 128; j++) {
			if (amatrix[j][col] & mask) {

				/* Note that the entire row, *not*
				   just the nonzero part of it, must
				   be eliminated; this is because the
				   corresponding (dense) row of matrix[][]
				   must have the same operation applied */

				for (k = 0; k < col_words; k++) {
					amatrix[j][k] ^= amatrix[i][k];
					matrix[j][k] ^= matrix[i][k];
				}
			}
		}
		i++;
	}

	/* transpose rows i to 64 back into x[] */

	for (j = 0; j < ncols; j++) {
		uint64 word = 0;

		col = j / 64;
		mask = bitmask[j % 64];

		for (k = i; k < 64; k++) {
			if (matrix[k][col] & mask)
				word |= bitmask[k];
		}
		x[j] = word;
	}

	for (i = 0; i < 128; i++) {
		free(matrix[i]);
		free(amatrix[i]);
	}

	/* determine exactly how many of the dependencies
	   are trivial. This may technically exceed i, 
	   though that's very unlikely */

	for (i = 0, mask = 0; i < ncols; i++)
		mask |= x[i];

	for (i = j = 0; i < 64; i++) {
		if (mask & bitmask[i])
			j++;
	}
	if (j == 0) {
		logprintf(obj, "lanczos error: only trivial "
				"dependencies found\n");
		exit(-1);
	}
	logprintf(obj, "recovered %d nontrivial dependencies\n", j);
}

/*-----------------------------------------------------------------------*/
static uint64 * block_lanczos(msieve_obj *obj,
			uint32 nrows, uint32 ncols, la_col_t *B) {
	
	/* Solve Bx = 0 for some nonzero x; the computed
	   solution, containing up to 64 of these nullspace
	   vectors, is returned */

	uint64 *vnext, *v[3], *x, *v0;
	uint64 *winv[3];
	uint64 *vt_a_v[2], *vt_a2_v[2];
	uint64 *scratch;
	uint64 *d, *e, *f, *f2;
	uint64 *tmp;
	uint32 s[2][64];
	uint32 i, iter;
	uint32 n = ncols;
	uint32 dim0, dim1;
	uint64 mask0, mask1;
	uint32 vsize;

	/* allocate all of the size-n variables. Note that because
	   B has been preprocessed to ignore singleton rows, the
	   number of rows may really be less than nrows and may
	   be greater than ncols. vsize is the maximum of these
	   two numbers  */

	vsize = MAX(nrows, ncols);
	v[0] = (uint64 *)malloc(vsize * sizeof(uint64));
	v[1] = (uint64 *)malloc(vsize * sizeof(uint64));
	v[2] = (uint64 *)malloc(vsize * sizeof(uint64));
	vnext = (uint64 *)malloc(vsize * sizeof(uint64));
	x = (uint64 *)malloc(vsize * sizeof(uint64));
	v0 = (uint64 *)malloc(vsize * sizeof(uint64));
	scratch = (uint64 *)malloc(MAX(vsize, 256 * 8) * sizeof(uint64));

	/* allocate all the 64x64 variables */

	winv[0] = (uint64 *)malloc(64 * sizeof(uint64));
	winv[1] = (uint64 *)malloc(64 * sizeof(uint64));
	winv[2] = (uint64 *)malloc(64 * sizeof(uint64));
	vt_a_v[0] = (uint64 *)malloc(64 * sizeof(uint64));
	vt_a_v[1] = (uint64 *)malloc(64 * sizeof(uint64));
	vt_a2_v[0] = (uint64 *)malloc(64 * sizeof(uint64));
	vt_a2_v[1] = (uint64 *)malloc(64 * sizeof(uint64));
	d = (uint64 *)malloc(64 * sizeof(uint64));
	e = (uint64 *)malloc(64 * sizeof(uint64));
	f = (uint64 *)malloc(64 * sizeof(uint64));
	f2 = (uint64 *)malloc(64 * sizeof(uint64));

	/* The iterations computes v[0], vt_a_v[0],
	   vt_a2_v[0], s[0] and winv[0]. Subscripts larger
	   than zero represent past versions of these
	   quantities, which start off empty (except for
	   the past version of s[], which contains all
	   the column indices */
	   
	memset(v[1], 0, vsize * sizeof(uint64));
	memset(v[2], 0, vsize * sizeof(uint64));
	for (i = 0; i < 64; i++) {
		s[1][i] = i;
		vt_a_v[1][i] = 0;
		vt_a2_v[1][i] = 0;
		winv[1][i] = 0;
		winv[2][i] = 0;
	}
	dim0 = 0;
	dim1 = 64;
	mask1 = (uint64)(-1);
	iter = 0;

	/* The computed solution 'x' starts off random,
	   and v[0] starts off as B*x. This initial copy
	   of v[0] must be saved off separately */

	for (i = 0; i < n; i++)
		v[0][i] = (uint64)(get_rand(&obj->seed1, &obj->seed2)) << 32 |
		          (uint64)(get_rand(&obj->seed1, &obj->seed2));

	memcpy(x, v[0], vsize * sizeof(uint64));
	mul_MxN_Nx64(vsize, ncols, B, v[0], scratch);
	mul_trans_MxN_Nx64(ncols, B, scratch, v[0]);
	memcpy(v0, v[0], vsize * sizeof(uint64));

	/* perform the iteration */

	while (1) {
		iter++;

		/* multiply the current v[0] by a symmetrized
		   version of B, or B'B (apostrophe means 
		   transpose). Use "A" to refer to B'B  */

		mul_MxN_Nx64(vsize, ncols, B, v[0], scratch);
		mul_trans_MxN_Nx64(ncols, B, scratch, vnext);

		/* compute v0'*A*v0 and (A*v0)'(A*v0) */

		mul_64xN_Nx64(v[0], vnext, scratch, vt_a_v[0], n);
		mul_64xN_Nx64(vnext, vnext, scratch, vt_a2_v[0], n);

		/* if the former is orthogonal to itself, then
		   the iteration has finished */

		for (i = 0; i < 64; i++) {
			if (vt_a_v[0][i] != 0)
				break;
		}
		if (i == 64) {
			break;
		}

		/* Find the size-'dim0' nonsingular submatrix
		   of v0'*A*v0, invert it, and list the column
		   indices present in the submatrix */

		dim0 = find_nonsingular_sub(obj, vt_a_v[0], s[0], 
					    s[1], dim1, winv[0]);
		if (dim0 == 0)
			break;

		/* mask0 contains one set bit for every column
		   that participates in the inverted submatrix
		   computed above */

		mask0 = 0;
		for (i = 0; i < dim0; i++)
			mask0 |= bitmask[s[0][i]];

		/* compute d */

		for (i = 0; i < 64; i++)
			d[i] = (vt_a2_v[0][i] & mask0) ^ vt_a_v[0][i];

		mul_64x64_64x64(winv[0], d, d);

		for (i = 0; i < 64; i++)
			d[i] = d[i] ^ bitmask[i];

		/* compute e */

		mul_64x64_64x64(winv[1], vt_a_v[0], e);

		for (i = 0; i < 64; i++)
			e[i] = e[i] & mask0;

		/* compute f */

		mul_64x64_64x64(vt_a_v[1], winv[1], f);

		for (i = 0; i < 64; i++)
			f[i] = f[i] ^ bitmask[i];

		mul_64x64_64x64(winv[2], f, f);

		for (i = 0; i < 64; i++)
			f2[i] = ((vt_a2_v[1][i] & mask1) ^ 
				   vt_a_v[1][i]) & mask0;

		mul_64x64_64x64(f, f2, f);

		/* compute the next v */

		for (i = 0; i < n; i++)
			vnext[i] = vnext[i] & mask0;

		mul_Nx64_64x64_acc(v[0], d, scratch, vnext, n);
		mul_Nx64_64x64_acc(v[1], e, scratch, vnext, n);
		mul_Nx64_64x64_acc(v[2], f, scratch, vnext, n);
		
		/* update the computed solution 'x' */

		mul_64xN_Nx64(v[0], v0, scratch, d, n);
		mul_64x64_64x64(winv[0], d, d);
		mul_Nx64_64x64_acc(v[0], d, scratch, x, n);

		/* rotate all the variables */

		tmp = v[2]; 
		v[2] = v[1]; 
		v[1] = v[0]; 
		v[0] = vnext; 
		vnext = tmp;
		
		tmp = winv[2]; 
		winv[2] = winv[1]; 
		winv[1] = winv[0]; 
		winv[0] = tmp;
		
		tmp = vt_a_v[1]; vt_a_v[1] = vt_a_v[0]; vt_a_v[0] = tmp;
		
		tmp = vt_a2_v[1]; vt_a2_v[1] = vt_a2_v[0]; vt_a2_v[0] = tmp;

		memcpy(s[1], s[0], 64 * sizeof(uint32));
		mask1 = mask0;
		dim1 = dim0;
	}

	logprintf(obj, "lanczos halted after %d iterations\n", iter);

	/* free unneeded storage */

	free(vnext);
	free(scratch);
	free(v0);
	free(vt_a_v[0]);
	free(vt_a_v[1]);
	free(vt_a2_v[0]);
	free(vt_a2_v[1]);
	free(winv[0]);
	free(winv[1]);
	free(winv[2]);
	free(d);
	free(e);
	free(f);
	free(f2);

	/* if a recoverable failure occurred, start everything
	   over again */

	if (dim0 == 0) {
		logprintf(obj, "linear algebra failed; retrying...\n");
		free(x);
		free(v[0]);
		free(v[1]);
		free(v[2]);
		return NULL;
	}

	/* convert the output of the iteration to an actual
	   collection of nullspace vectors */

	mul_MxN_Nx64(vsize, ncols, B, x, v[1]);
	mul_MxN_Nx64(vsize, ncols, B, v[0], v[2]);

	combine_cols(obj, ncols, x, v[0], v[1], v[2]);

	/* verify that these really are linear dependencies of B */

	mul_MxN_Nx64(vsize, ncols, B, x, v[0]);

	for (i = 0; i < ncols; i++) {
		if (v[0][i] != 0)
			break;
	}
	if (i < ncols) {
		logprintf(obj, "lanczos error: dependencies don't work\n");
		exit(-1);
	}
	
	free(v[0]);
	free(v[1]);
	free(v[2]);
	return x;
}
