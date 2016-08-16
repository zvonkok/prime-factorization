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

#include <mp.h>

/*---------------------------------------------------------------*/
static uint32 num_nonzero_words(uint32 *x, uint32 max_words) {

	uint32 i;

	for (i = max_words; i && !x[i-1]; i--)
		;
	
	return i;
}

/*---------------------------------------------------------------*/
uint32 mp_bits(mp_t *a) {

	uint32 i, bits, mask;

	i = a->nwords;
	bits = 32 * i;
	if (bits == 0)
		return bits;

	mask = 0x80000000;
	while ( !(a->val[i-1] & mask) ) {
		bits--;
		mask >>= 1;
	}
	return bits;
}

/*---------------------------------------------------------------*/
void mp_add(mp_t *a, mp_t *b, mp_t *sum) {

	int32 i;
	uint32 carry = 0;
	uint32 acc;

	for (i = 0; i < MAX_MP_WORDS; i++) {
		acc = a->val[i] + carry;
		carry = (acc < a->val[i]);
		sum->val[i] = acc + b->val[i];
		carry += (sum->val[i] < acc);
	}
	sum->nwords = num_nonzero_words(sum->val, MAX_MP_WORDS);
}

/*---------------------------------------------------------------*/
void mp_add_1(mp_t *a, uint32 b, mp_t *sum) {

	int32 i;
	uint32 carry = b;
	uint32 acc;

	for (i = 0; carry && i < MAX_MP_WORDS; i++) {
		acc = a->val[i] + carry;
		carry = (acc < a->val[i]);
		sum->val[i] = acc;
	}
	if (a != sum) {
		for (; i < MAX_MP_WORDS; i++)
			sum->val[i] = a->val[i];
		sum->nwords = num_nonzero_words(sum->val, MAX_MP_WORDS);
	}
}

/*---------------------------------------------------------------*/
void mp_sub(mp_t *a, mp_t *b, mp_t *diff) {

	int32 i;
	uint32 borrow = 0;
	uint32 acc;

	for (i = 0; i < MAX_MP_WORDS; i++) {
		acc = a->val[i] - borrow;
		borrow = (acc > a->val[i]);
		diff->val[i] = acc - b->val[i];
		borrow += (diff->val[i] > acc);
	}
	diff->nwords = num_nonzero_words(diff->val, MAX_MP_WORDS);
}

/*---------------------------------------------------------------*/
void mp_sub_1(mp_t *a, uint32 b, mp_t *diff) {

	int32 i;
	uint32 borrow = b;
	uint32 acc;

	for (i = 0; borrow && i < MAX_MP_WORDS; i++) {
		acc = a->val[i] - borrow;
		borrow = (acc > a->val[i]);
		diff->val[i] = acc;
	}
	if (a != diff) {
		for (; i < MAX_MP_WORDS; i++)
			diff->val[i] = a->val[i];

		diff->nwords = num_nonzero_words(diff->val, MAX_MP_WORDS);
	}
}

/*---------------------------------------------------------------*/
int32 mp_cmp(const mp_t *a, const mp_t *b) {

	int32 i;

	if (a->nwords > b->nwords)
		return 1;
	if (a->nwords < b->nwords)
		return -1;

	for (i = a->nwords - 1; i >= 0; i--) {
		if (a->val[i] > b->val[i])
			return 1;
		if (a->val[i] < b->val[i])
			return -1;
	}

	return 0;
}

/*---------------------------------------------------------------*/
static void mp_addmul_1(mp_t *a, uint32 b, uint32 *x) {

	uint32 i;
	uint64 acc = 0;
	uint32 carry = 0;

	for (i = 0; i < a->nwords; i++) {
		acc = (uint64)a->val[i] * (uint64)b + 
		      (uint64)carry +
		      (uint64)x[i];
		x[i] = (uint32)acc;
		carry = (uint32)(acc >> 32);
	}
	x[i] = carry;
}

/*---------------------------------------------------------------*/
void mp_mul_1(mp_t *a, uint32 b, mp_t *x) {

	uint32 i;
	uint64 acc = 0;
	uint32 carry = 0;

	if (b == 0) {
		mp_clear(x);
		return;
	}

	for (i = 0; i < a->nwords; i++) {
		acc = (uint64)a->val[i] * (uint64)b + (uint64)carry;
		x->val[i] = (uint32)acc;
		carry = (uint32)(acc >> 32);
	}
	x->nwords = a->nwords;
	if (carry) {
		x->val[i++] = carry;
		x->nwords++;
	}
	for (; i < MAX_MP_WORDS; i++)
		x->val[i] = 0;
}

/*---------------------------------------------------------------*/
void mp_mul(mp_t *a, mp_t *b, mp_t *prod) {

	uint32 i;

	for (i = 0; i < MAX_MP_WORDS; i++) 
		prod->val[i] = 0;

	for (i = 0; i < a->nwords; i++) 
		mp_addmul_1(b, a->val[i], prod->val + i);
	
	prod->nwords = num_nonzero_words(prod->val, a->nwords + b->nwords);
}

/*---------------------------------------------------------------*/
void mp_rshift(mp_t *a, uint32 shift, mp_t *res) {

	int32 i;
	int32 words = a->nwords;
	int32 start_word = shift / 32;
	uint32 word_shift = shift & 31;
	uint32 comp_word_shift = 32 - word_shift;

	if (start_word > words) {
		mp_clear(res);
		return;
	}

	if (word_shift == 0) {
		for (i = 0; i < (words-start_word); i++)
			res->val[i] = a->val[start_word+i];
	}
	else {
		for (i = 0; i < (words-start_word-1); i++)
			res->val[i] = a->val[start_word+i] >> word_shift |
				a->val[start_word+i+1] << comp_word_shift;
		res->val[i] = a->val[start_word+i] >> word_shift;
		i++;
	}

	for (; i < MAX_MP_WORDS; i++)
		res->val[i] = 0;

	res->nwords = num_nonzero_words(res->val, (uint32)(words - start_word));
}

/*---------------------------------------------------------------*/
uint32 mp_rjustify(mp_t *a, mp_t *res) {

	uint32 i, bits;

	if (mp_is_zero(a))
		return 0;
	
	bits = mp_bits(a);
	for (i = 0; i < bits - 1; i++) {
		if (a->val[i / 32] & (1 << (i & 31)))
			break;
	}

	mp_rshift(a, i, res);
	return i;
}

/*---------------------------------------------------------------*/
void mp_recip(mp_t *a, mp_t *recip) {

	uint32 bits_in_a;
	int32 sign;
	mp_t a1, r, r2, prod;

	mp_sub_1(a, 1, &a1);
	bits_in_a = mp_bits(&a1);

	mp_clear(&r);
	r.val[bits_in_a / 32] = 1 << (bits_in_a & 31);
	r.nwords = bits_in_a / 32 + 1;
	mp_copy(&r, recip);

	do {
		mp_add(&r, &r, &r2);
		mp_mul(&r, &r, &prod);
		mp_rshift(&prod, bits_in_a, &r);
		mp_mul(&r, a, &prod);
		mp_rshift(&prod, bits_in_a, &r);
		mp_sub(&r2, &r, &r);
		sign = mp_cmp(&r, recip);
		mp_copy(&r, recip);
	} while (sign > 0);

	mp_mul(recip, a, &prod);
	mp_clear(&r2);
	r2.val[2 * bits_in_a / 32] = 1 << ((2 * bits_in_a) & 31);
	r2.nwords = 2 * bits_in_a / 32 + 1;
	mp_sub(&r2, &prod, &r2);

	while(r2.val[MAX_MP_WORDS-1] & 0x80000000) {
		mp_sub_1(recip, 1, recip);
		mp_add(&r2, a, &r2);
	}
}	

/*---------------------------------------------------------------*/
void mp_divrem(mp_t *num, mp_t *denom, mp_t *recip, 
		mp_t *quot, mp_t *rem) {

	uint32 i, bits;
	mp_t prod, quot2;
	mp_t tmp_quot, tmp_rem, tmp_recip;

	if (quot == NULL)
		quot = &tmp_quot;
	if (rem == NULL)
		rem = &tmp_rem;
	
	if (denom->nwords == 1) {
		mp_clear(rem);
		rem->val[0] = mp_divrem_1(num, denom->val[0], quot);
		if (rem->val[0] > 0)
			rem->nwords = 1;
		return;
	}

	if (recip == NULL) {
		mp_recip(denom, &tmp_recip);
		recip = &tmp_recip;
	}

	bits = mp_bits(recip) - 1;
	i = 0;

	while(1) {
		mp_rshift(num, bits - 1, &quot2);
		mp_mul(&quot2, recip, &prod);
		mp_rshift(&prod, bits + 1, &quot2);
		mp_mul(&quot2, denom, &prod);
		mp_sub(num, &prod, rem);

		if (i == 0)
			mp_copy(&quot2, quot);
		else
			mp_add(&quot2, quot, quot);

		if (mp_bits(rem) <= bits + 2)
			break;

		i++;
		num = rem;
	}

	while(mp_cmp(rem, denom) >= 0) {
		mp_add_1(quot, 1, quot);
		mp_sub(rem, denom, rem);
	}
}

/*---------------------------------------------------------------*/
uint32 mp_divrem_1(mp_t *num, uint32 denom, mp_t *quot) {

	int32 i;
	uint32 rem = 0;
	mp_t tmp_quot;
#if !defined(__GNUC__) || (!defined(__i386__) && !defined(__x86_64__))
	uint64 acc = 0;
#endif

	if (quot == NULL)
		quot = &tmp_quot;

	for (i = (int32)num->nwords; i < MAX_MP_WORDS; i++)
		quot->val[i] = 0;
	
	for (i = num->nwords - 1; i >= 0; i--) {

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

		uint32 quot1;
		asm("divl %4"
			: "=a"(quot1),"=d"(rem)
			: "1"(rem), "0"(num->val[i]), "r"(denom) );

		quot->val[i] = quot1;
#else
		acc = (uint64)rem << 32 | (uint64)num->val[i];
		quot->val[i] = (uint32)(acc / denom);
		rem = (uint32)(acc % denom);
#endif
	}

	i = num->nwords;
	quot->nwords = i;
	if (i && quot->val[i-1] == 0)
		quot->nwords--;

	return rem;
}
	
/*---------------------------------------------------------------*/
uint32 mp_iroot(mp_t *a, uint32 root, mp_t *res) {

	mp_t res2;
	uint32 bits;

	bits = mp_bits(a);
	if (bits % root)
		bits = bits / root + 1;
	else
		bits = bits / root;

	mp_clear(res);
	res->val[bits / 32] = 1 << (bits & 31);
	res->nwords = bits / 32 + 1;
	mp_copy(res, &res2);

	while (1) {
		mp_t q, pow;
		uint32 i;

		mp_copy(&res2, &pow);
		for (i = 2; i < root; i++) {
			mp_mul(&res2, &pow, &q);
			mp_copy(&q, &pow);
		}

		mp_div(a, &pow, NULL, &q);
		mp_mul_1(&res2, root - 1, &res2);
		mp_add(&res2, &q, &res2);
		mp_divrem_1(&res2, root, &res2);

		if (mp_cmp(&res2, res) >= 0) {
			mp_mul(&pow, res, &q);
			return mp_cmp(&q, a);
		}
		mp_copy(&res2, res);
	}
}

/*---------------------------------------------------------------*/
uint32 mp_gcd_1(uint32 x, uint32 y) {

	uint32 tmp;

	if (y < x) {
		tmp = x; x = y; y = tmp;
	}

	while (y > 0) {
		x = x % y;
		tmp = x; x = y; y = tmp;
	}
	return x;
}

/*---------------------------------------------------------------*/
void mp_gcd(mp_t *x, mp_t *y, mp_t *out) {

	mp_t x0, y0;
	mp_t *xptr, *yptr;
	mp_t *tmp;
	mp_t rem;
	int32 sign;

	mp_copy(x, &x0);
	mp_copy(y, &y0);
	if (mp_cmp(x, y) > 0) {
		xptr = &x0;
		yptr = &y0;
	}
	else {
		xptr = &y0;
		yptr = &x0;
	}

	sign = mp_cmp(xptr, yptr);
	mp_clear(out);
	out->nwords = 1;

	while (!mp_is_zero(yptr)) {
		if (xptr->nwords == 1) {
			out->val[0] = mp_gcd_1(xptr->val[0], 
					mp_mod_1(yptr, xptr->val[0]));
			return;
		}
		else if (yptr->nwords == 1) {
			out->val[0] = mp_gcd_1(yptr->val[0],
					mp_mod_1(xptr, yptr->val[0]));
			return;
		}

		if (sign > 0) {
			mp_mod(xptr, yptr, NULL, &rem);
			tmp = xptr;
			xptr = yptr;
			yptr = tmp;
		}
		else {
			mp_mod(yptr, xptr, NULL, &rem);
		}
		mp_copy(&rem, yptr);
		sign = mp_cmp(xptr, yptr);
	}

	mp_copy(xptr, out);
}

/*---------------------------------------------------------------*/
char * mp_print(mp_t *a, uint32 base, FILE *f, char *scratch) {

	mp_t tmp;
	char *bufptr;
	uint32 next_letter;

	mp_copy(a, &tmp);
	bufptr = scratch + 32 * MAX_MP_WORDS;
	*bufptr = 0;

	do {
		next_letter = mp_divrem_1(&tmp, base, &tmp);
		if (next_letter < 10)
			*(--bufptr) = '0' + next_letter;
		else
			*(--bufptr) = 'a' + (next_letter - 10);
	} while ( !mp_is_zero(&tmp) );
	
	if (f)
		fprintf(f, "%s", bufptr);

	return bufptr;
}

/*---------------------------------------------------------------*/
void mp_str2mp(char *str, mp_t *a, uint32 base) {

	char *str_start, *str_end;
	int32 digit;
	mp_t mult;

	mp_clear(a);

	if (base > 36) {
		return;
	}
	else if (base == 0) {
		if (str[0] == '0' && tolower(str[1]) == 'x') {
			base = 16; 
			str += 2;
		}
		else if (str[0] == '0') {
			base = 8; 
			str++;
		}
		else
			base = 10;
	}

	str_start = str_end = str;
	while (*str_end) {
		digit = tolower(*str_end);
		if (isdigit(digit))
			digit -= '0';
		else if (isalpha(digit))
			digit = digit - 'a' + 10;
		else
			break;

		if (digit >= (int32)base)
			break;
		str_end++;
	}

	mp_clear(&mult);
	mult.nwords = 1;
	mult.val[0] = 1;
	str_end--;

	while( str_end >= str_start && *str_end) {
		digit = tolower(*str_end);
		if (isdigit(digit))
			digit -= '0';
		else if (isalpha(digit))
			digit = digit - 'a' + 10;

		mp_addmul_1(&mult, (uint32)digit, a->val);
		mp_mul_1(&mult, base, &mult);
		str_end--;
	}
	a->nwords = num_nonzero_words(a->val, mult.nwords + 1);
}

/*---------------------------------------------------------------*/
int32 mp_legendre_1(uint32 a, uint32 p) {

	uint32 x, y, tmp;
	int32 out = 1;

	x = a;
	y = p;
	while (x) {
		while ((x & 1) == 0) {
			x = x / 2;
			if ( (y & 7) == 3 || (y & 7) == 5 )
				out = -out;
		}

		tmp = x;
		x = y;
		y = tmp;

		if ( (x & 3) == 3 && (y & 3) == 3 )
			out = -out;

		x = x % y;
	}
	if (y == 1)
		return out;
	return 0;
}

/*---------------------------------------------------------------*/
int32 mp_legendre(mp_t *a, mp_t *p) {

	mp_t *x, *y, *tmp;
	mp_t tmp_a, tmp_p;
	mp_t rem;
	int32 out = 1;

	mp_copy(a, &tmp_a);
	mp_copy(p, &tmp_p);
	x = &tmp_a;
	y = &tmp_p;

	while (!mp_is_zero(x)) {
		if (x->nwords == 1 && y->nwords == 1)
			return out * mp_legendre_1(x->val[0], y->val[0]);

		while ((x->val[0] & 1) == 0) {
			mp_rshift(x, 1, x);
			if ( (y->val[0] & 7) == 3 || 
			     (y->val[0] & 7) == 5 )
				out = -out;
		}

		tmp = x;
		x = y;
		y = tmp;

		if ( (x->val[0] & 3) == 3 && (y->val[0] & 3) == 3 )
			out = -out;

		mp_mod(x, y, NULL, &rem);
		mp_copy(&rem, x);
	}
	if (mp_is_one(y))
		return out;
	return 0;
}

/*---------------------------------------------------------------*/
void mp_expo(mp_t *a, mp_t *b, mp_t *n, mp_t *res) {

	mp_t r, prod;
	uint32 i, mask;

	mp_recip(n, &r);
	mp_clear(res);
	res->nwords = res->val[0] = 1;
	if (mp_is_zero(b))
		return;

	mask = (uint32)(0x80000000);
	i = b->nwords;
	while (mask) {
		if (b->val[i-1] & mask)
			break;
		mask >>= 1;
	}
	
	while (i) {
		mp_mul(res, res, &prod);
		mp_mod(&prod, n, &r, res);

		if (b->val[i-1] & mask) {
			mp_mul(res, a, &prod);
			mp_mod(&prod, n, &r, res);
		}

		mask >>= 1;
		if (mask == 0) {
			i--;
			mask = (uint32)(0x80000000);
		}
	}
}
		
/*---------------------------------------------------------------*/
void mp_rand(uint32 bits, mp_t *res, uint32 *seed1, uint32 *seed2) {

	uint32 i;
	uint32 words = (bits + 31) / 32;

	for (i = 0; i < words; i++)
		res->val[i] = get_rand(seed1, seed2);
	for (; i < MAX_MP_WORDS; i++)
		res->val[i] = 0;

	if (bits & 31)
		res->val[words-1] >>= 32 - (bits & 31);
	res->nwords = num_nonzero_words(res->val, words);
}

/*---------------------------------------------------------------*/
uint32 mp_modsqrt_1(uint32 a, uint32 p, 
			uint32 *seed1, uint32 *seed2) {

	uint32 a0 = a;

	if ( (p & 7) == 3 || (p & 7) == 7 ) {
		return mp_expo_1(a0, (p+1)/4, p);
	}
	else if ( (p & 7) == 5 ) {
		uint32 x, y;
		
		if (a0 >= p)
			a0 = a0 % p;
		x = mp_expo_1(a0, (p+3)/8, p);

		if (mp_modmul_1(x, x, p) == a0)
			return x;

		y = mp_expo_1(2, (p-1)/4, p);

		return mp_modmul_1(x, y, p);
	}
	else {
		uint32 d0, d1, a1, s, t, m;
		uint32 i;

		d0 = get_rand(seed1, seed2) % p;
		while (mp_legendre_1(d0, p) != -1)
			d0 = get_rand(seed1, seed2) % p;

		t = p - 1;
		s = 0;
		while (!(t & 1)) {
			s++;
			t = t / 2;
		}

		a1 = mp_expo_1(a0, t, p);
		d1 = mp_expo_1(d0, t, p);

		for (i = 0, m = 0; i < s; i++) {
			uint32 ad;

			ad = mp_expo_1(d1, m, p);
			ad = mp_modmul_1(ad, a1, p);
			ad = mp_expo_1(ad, (uint32)(1) << (s-1-i), p);
			if (ad == (p - 1))
				m += (1 << i);
		}

		a1 = mp_expo_1(a0, (t+1)/2, p);
		d1 = mp_expo_1(d1, m/2, p);
		return mp_modmul_1(a1, d1, p);
	}
}

/*---------------------------------------------------------------*/
void mp_modsqrt(mp_t *a, mp_t *p, mp_t *res,
		uint32 *seed1, uint32 *seed2) {

	mp_t *a0 = a;
	mp_t tmp, recip;

	if ( (p->val[0] & 7) == 3 || (p->val[0] & 7) == 7 ) {
		mp_add_1(p, 1, &tmp);
		mp_rshift(&tmp, 2, &tmp);
		mp_expo(a0, &tmp, p, res);
	}
	else if ( (p->val[0] & 7) == 5 ) {
		mp_t x, base;
		
		mp_recip(p, &recip);

		mp_add_1(p, 3, &tmp);
		mp_rshift(&tmp, 3, &tmp);
		mp_expo(a0, &tmp, p, res);

		mp_mul(res, res, &tmp);
		mp_mod(&tmp, p, &recip, &x);
		mp_mod(a0, p, &recip, &tmp);
		if (!mp_cmp(&x, &tmp))
			return;

		mp_sub_1(p, 1, &tmp);
		mp_rshift(&tmp, 2, &tmp);
		mp_clear(&base);
		base.nwords = 1;
		base.val[0] = 2;
		mp_expo(&base, &tmp, p, &x);
		mp_mul(&x, res, &tmp);
		mp_mod(&tmp, p, &recip, res);
	}
	else {
		mp_t d0, d1, a1, t, m;
		int32 i, j, s;
		uint32 bits = mp_bits(p);

		mp_rand(bits, &d0, seed1, seed2);
		while (mp_legendre(&d0, p) != -1)
			mp_rand(bits, &d0, seed1, seed2);

		mp_sub_1(p, 1, &t);
		s = mp_rjustify(&t, &t);

		mp_expo(a0, &t, p, &a1);
		mp_expo(&d0, &t, p, &d1);
		mp_clear(&m);
		mp_recip(p, &recip);

		for (i = 0; i < s; i++) {
			mp_t ad;

			mp_expo(&d1, &m, p, &ad);
			mp_mul(&ad, &a1, &tmp);
			mp_mod(&tmp, p, &recip, &ad);
			for (j = 0; j < (s-1-i); j++) {
				mp_mul(&ad, &ad, &tmp);
				mp_mod(&tmp, p, &recip, &ad);
			}
			mp_add_1(&ad, 1, &ad);
			if (!mp_cmp(&ad, p)) {
				m.nwords = i / 32 + 1;
				m.val[i / 32] |= 1 << (i & 31);
			}
		}

		mp_add_1(&t, 1, &t);
		mp_rshift(&t, 1, &t);
		mp_expo(a0, &t, p, &a1);
		mp_rshift(&m, 1, &m);
		mp_expo(&d1, &m, p, &tmp);
		mp_mul(&a1, &tmp, &d1);
		mp_mod(&d1, p, &recip, res);
	}
}
		
/*---------------------------------------------------------------*/
void mp_modsqrt2(mp_t *a, mp_t *p, mp_t *res,
		 uint32 *seed1, uint32 *seed2) {

	mp_t recip;
	mp_t p0, p1, r0, r1, r2;

	mp_recip(p, &recip);
	mp_mod(a, p, &recip, &r0);
	mp_modsqrt(a, p, &r1, seed1, seed2);

	mp_mul(&r1, &r1, &r2);
	mp_sub(a, &r2, &r0);
	mp_div(&r0, p, &recip, &r2);
	mp_mod(&r2, p, &recip, &r0);

	mp_add(&r1, &r1, &r2);
	mp_sub_1(p, 2, &p0);
	mp_expo(&r2, &p0, p, &p1);
	mp_mul(&p1, &r0, &r2);
	mp_mod(&r2, p, &recip, &p1);

	mp_mul(p, p, &r0);
	mp_mul(p, &p1, &r2);
	mp_add(&r2, &r1, &r2);
	mp_mod(&r2, &r0, NULL, res);

	mp_rshift(&r0, 1, &r1);
	if (mp_cmp(res, &r1) > 0)
		mp_sub(&r0, res, res);
}

/*---------------------------------------------------------------*/
int32 mp_is_prime(mp_t *p, uint32 *seed1, uint32 *seed2) {

	const uint32 factors[] = {3,5,7,11,13,17,19,23,29,31,37,41,43,
				  47,53,59,61,67,71,73,79,83,89,97,101,
				  103,107,109,113,127,131,137,139,149,
				  151,157,163,167,173,179,181,191,193,
				  197,199,211,223,227,229,233,239,241,251};

	uint32 i, j, bits, num_squares;
	mp_t base, tmp, oddpart, p_minus_1, recip;

	for (i = 0; i < sizeof(factors) / sizeof(uint32); i++)
		if (!mp_mod_1(p, factors[i]))
			return 0;

	if (p->nwords == 1 && p->val[0] < 65536)
		return 1;

	mp_sub_1(p, 1, &p_minus_1);
	mp_copy(&p_minus_1, &oddpart);
	mp_recip(p, &recip);
	bits = mp_bits(p);
	num_squares = mp_rjustify(&oddpart, &oddpart);

	for (i = 0; i < NUM_WITNESSES; i++) {
		mp_rand(bits, &base, seed1, seed2);
		while(mp_is_zero(&base) || mp_is_one(&base) || 
						!mp_cmp(&base, p))
			mp_rand(bits, &base, seed1, seed2);

		if (mp_cmp(&base, p) > 0)
			mp_sub(&base, p, &base);

		mp_expo(&base, &oddpart, p, &tmp);
		if (mp_is_one(&tmp) || !mp_cmp(&tmp, &p_minus_1)) {
		    	continue;
		}

		for (j = 0; j < num_squares - 1; j++) {
			mp_t square;
			mp_mul(&tmp, &tmp, &square);
			mp_mod(&square, p, &recip, &tmp);
			if (!mp_cmp(&tmp, &p_minus_1))
				break;
		}

		if (j == num_squares - 1)
			break;
	}

	if (i == NUM_WITNESSES)
		return 1;
	return 0;
}
		
/*---------------------------------------------------------------*/
void mp_random_prime(uint32 bits, mp_t *res,
			uint32 *seed1, uint32 *seed2) {

	mp_rand(bits, res, seed1, seed2);
	res->val[0] |= 1;
	res->nwords = (bits + 31) / 32;
	if (bits & 31)
		res->val[res->nwords - 1] |= 1 << ((bits & 31) - 1);
	else
		res->val[res->nwords - 1] |= 0x80000000;
	
	while (!mp_is_prime(res, seed1, seed2))
		mp_add_1(res, 2, res);
}
		
/*---------------------------------------------------------------*/
uint32 mp_next_prime(mp_t *p, mp_t *res,
		uint32 *seed1, uint32 *seed2) {

	uint32 inc;

	mp_copy(p, res);
	if (res->val[0] & 1) {
		mp_add_1(res, 2, res);
		inc = 2;
	}
	else {
		res->val[0] |= 1;
		inc = 1;
	}

	while (!mp_is_prime(res, seed1, seed2)) {
		mp_add_1(res, 2, res);
		inc += 2;
	}

	return inc;
}
