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

#ifndef _MP_H_
#define _MP_H_

#include <util.h>

/* Basic multiple-precision arithmetic implementation. Precision
   is hardwired not to exceed ~125 digits. There is twice that 
   much storage available for each multiple-precision quantity. 
   Numbers are stored in two's-complement binary form, in little-
   endian word order.

   The array of bits for the number is always composed of 32-bit
   words. This is because I want things to be portable and there's 
   no support in C for 128-bit data types, so that 64x64 multiplies
   and 128/64 divides would need assembly language support */

#define MAX_MP_WORDS 2*13

typedef struct {
	uint32 nwords;		/* number of nonzero words in val[] */
	uint32 val[MAX_MP_WORDS];
} mp_t;

	/* initialize an mp_t */

#define mp_clear(a) memset(a, 0, sizeof(mp_t))
#define mp_copy(a, b) memcpy(b, a, sizeof(mp_t))

	/* return the number of bits needed to hold an mp_t.
   	   This is equivalent to floor(log2(a)) + 1. */

uint32 mp_bits(mp_t *a);

	/* Addition and subtraction; a + b = sum
	   or a - b = diff. 'b' may be an integer or 
	   another mp_t. sum or diff may overwrite 
	   the input operands */

void mp_add(mp_t *a, mp_t *b, mp_t *sum);
void mp_add_1(mp_t *a, uint32 b, mp_t *sum);
void mp_sub(mp_t *a, mp_t *b, mp_t *diff);
void mp_sub_1(mp_t *a, uint32 b, mp_t *diff);

	/* return -1, 0, or 1 if a is less than, equal to,
	   or greater than b, respectively */

int32 mp_cmp(const mp_t *a, const mp_t *b);

	/* quick test for zero or one mp_t */

#define mp_is_zero(a) ((a)->nwords == 0)
#define mp_is_one(a) ((a)->nwords == 1 && (a)->val[0] == 1)

	/* Shift 'a' right by 'shift' bit positions.
	   The result may overwrite 'a'. shift amount
	   must not exceed 32*MAX_MP_WORDS */

void mp_rshift(mp_t *a, uint32 shift, mp_t *res);

	/* Right-shift 'a' by an amount equal to the
	   number of trailing zeroes. Return the shift count */

uint32 mp_rjustify(mp_t *a, mp_t *res);

	/* multiply a by b. 'b' is either a 1-word
	   operand or an mp_t. In the latter case, 
	   the product must fit in MAX_MP_WORDS words
	   and may not overwrite the input operands. */

void mp_mul(mp_t *a, mp_t *b, mp_t *prod);
void mp_mul_1(mp_t *a, uint32 b, mp_t *x);

static INLINE uint32 mp_modmul_1(uint32 a, uint32 b, uint32 n) {
	uint32 ans;
	uint64 acc;
	acc = (uint64)a * (uint64)b;

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
	asm("divl %3  \n\t"
	     : "=d"(ans)
	     : "a"((uint32)(acc)), "0"((uint32)(acc >> 32)), "g"(n) : "cc");
#else
	ans = (uint32)(acc % n);
#endif
	return ans;
}

	/* calculate the generalized reciprocal of 'a'.
	   This quantity is needed for the division and
	   square root routines. 'a' may not be overwritten. */

void mp_recip(mp_t *a, mp_t *recip);

	/* General-purpose division routines. mp_divrem
	   divides num by denom, putting the quotient in
	   quot (if not NULL) and the remainder in rem
	   (if not NULL). recip is the generalized reciprocal
	   of denom; if this is NULL, the reciprocal is 
	   calculated internally. No aliasing is allowed,
	   and all quantities must fit in MAX_MP_WORDS/2 words.
	   num may, however, exceed the square of denom */

void mp_divrem(mp_t *num, mp_t *denom, mp_t *recip, mp_t *quot, mp_t *rem);
#define mp_div(n, d, r, q) mp_divrem(n, d, r, q, NULL);
#define mp_mod(n, d, r, rem) mp_divrem(n, d, r, NULL, rem);

	/* Division routine where the denominator is a
	   single word. The quotient is written to quot
	   (if not NULL) and the remainder is returned.
	   quot may overwrite the input */

uint32 mp_divrem_1(mp_t *num, uint32 denom, mp_t *quot);

/*-------------------------------------------------------
 mp_mod_1 is the a time-critical routine; inline it */

static INLINE uint32 mp_mod_1(mp_t *num, uint32 denom) {
	int32 i;
	uint32 rem = 0;
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
	for (i = num->nwords - 1; i >= 0; i--) {
		asm("divl %3"
			: "=d"(rem)
			: "0"(rem), "a"(num->val[i]), "r"(denom) : "cc" );
	}
#else
	uint64 acc = 0;
	for (i = num->nwords - 1; i >= 0; i--) {
		acc = (uint64)rem << 32 | (uint64)num->val[i];
		rem = (uint32)(acc % denom);
	}
#endif
	return rem;
}
/*-----------------------------------------------------*/

	/* Calculate floor(i_th root of 'a'). The input must fit in
	   MAX_MP_WORDS/2 words. Note that i should be 'small'. The
	   return value is zero if res is an exact i_th root of 'a' */

uint32 mp_iroot(mp_t *a, uint32 i, mp_t *res);
#define mp_isqrt(a, res) mp_iroot(a, 2, res)

	/* Calculate greatest common divisor of x and y.
	   Any quantities may alias */

void mp_gcd(mp_t *x, mp_t *y, mp_t *out);
uint32 mp_gcd_1(uint32 x, uint32 y);

	/* Print routines: print the input mp_t to a file
	   (if f is not NULL) and also return a pointer to
	   a string representation of the input (requires
	   sufficient scratch space to be passed in). The 
	   input is printed in radix 'base' (2 to 36). The
	   maximum required size for scratch space is
	   32*MAX_MP_WORDS+1 bytes (i.e. enough to print
	   out 'a' in base 2) */

char * mp_print(mp_t *a, uint32 base, FILE *f, char *scratch);
#define mp_printf(a, base, scratch) mp_print(a, base, stdout, scratch)
#define mp_fprintf(a, base, f, scratch) mp_print(a, base, f, scratch)
#define mp_sprintf(a, base, scratch) mp_print(a, base, NULL, scratch)

	/* A multiple-precision version of strtoul(). The
	   string 'str' is converted from an ascii representation
	   of radix 'base' (2 to 36) into an mp_t. If base is 0,
	   the base is assumed to be 16 if the number is preceded
	   by "0x", 8 if preceded by "0", and 10 otherwise. Con-
	   version stops at the first character that cannot belong
	   to radix 'base', or otherwise at the terminating NULL.
	   The input is case insensitive. */

void mp_str2mp(char *str, mp_t *a, uint32 base);

	/* modular exponentiation: raise 'a' to the power 'b' 
	   mod 'n' and return the result. a and b may exceed n.
	   In the multiple precision case, the result may not
	   alias any of the inputs */

void mp_expo(mp_t *a, mp_t *b, mp_t *n, mp_t *res);

static INLINE uint32 mp_expo_1(uint32 a, uint32 b, uint32 n) {

	uint32 res = 1;
	while (b) {
		if (b & 1)
			res = mp_modmul_1(res, a, n);
		a = mp_modmul_1(a, a, n);
		b = b >> 1;
	}
	return res;
}

	/* Return the Legendre symbol for 'a'. This is 1 if
	   x * x = a (mod p) is solvable for some x, -1 if not, 
	   and 0 if a and p have factors in common. p must be 
	   an odd prime, and a may exceed p */

int32 mp_legendre_1(uint32 a, uint32 p);
int32 mp_legendre(mp_t *a, mp_t *p);

	/* For odd prime p, solve 'x * x = a (mod p)' for x and
	   return the result. Assumes legendre(a,p) = 1 (this is
	   not verified). mp_modsqrt2 solves x * x = a mod(p^2)
	   for x; it returns the smallest of the two positive roots
	   
	   These and the next few routines use random numbers, but
	   since they are intended to be 'stateless' the state of the
	   random number generator is passed in as 'seed1' and 'seed2'.
	   This state is updated as random numbers are produced */

uint32 mp_modsqrt_1(uint32 a, uint32 p, uint32 *seed1, uint32 *seed2);
void mp_modsqrt(mp_t *a, mp_t *p, mp_t *res, uint32 *seed1, uint32 *seed2);
void mp_modsqrt2(mp_t *a, mp_t *p, mp_t *res, uint32 *seed1, uint32 *seed2);

	/* Generate a random number between 0 and 2^bits - 1 */

void mp_rand(uint32 bits, mp_t *res, uint32 *seed1, uint32 *seed2);

	/* mp_is_prime returns 1 if the input is prime and 0 
	   otherwise. mp_random_prime generates a random number 
	   between 0 and 2^bits - 1 which is probably prime. 
	   mp_next_prime computes the next number greater than p
	   which is prime, and returns (res - p). The probability 
	   of these routines accidentally declaring a composite 
	   to be prime is < 4 ^ -NUM_WITNESSES, and probably is
	   drastically smaller than that */

#define NUM_WITNESSES 20
int32 mp_is_prime(mp_t *p, uint32 *seed1, uint32 *seed2);
void mp_random_prime(uint32 bits, mp_t *res, uint32 *seed1, uint32 *seed2);
uint32 mp_next_prime(mp_t *p, mp_t *res, uint32 *seed1, uint32 *seed2);

#endif
