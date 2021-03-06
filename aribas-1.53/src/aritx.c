/****************************************************************/
/* file aritx.c

ARIBAS interpreter for Arithmetic
Copyright (C) 1996/2003 O.Forster

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Address of the author

	Otto Forster
	Math. Institut der LMU
	Theresienstr. 39
	D-80333 Muenchen, Germany

Email	forster@mathematik.uni-muenchen.de
*/
/****************************************************************/
/*
** aritx.c
** extended arithmetic functions
**
** date of last change
** 1995-01-07:	brill1
** 1995-03-05:	ithbit
** 1995-03-11:	mod_pemult, mod_coshmult
** 1995-04-09:	rab_primetest
** 1997-02-24:	rab_primetest, overflow handling
** 1997-03-02:	cf_factorize: small speedup (pivots)
** 1997-04-13:	reorg (newintsym)
** 1997-11-26:	fixed bug in Ffactorial
** 1997-12-09:	factorization routines moved to arity.c
** 1997-12-14:	changed function prime32
** 1998-10-17:  small change in gcdcx
** 1998-10-25:  allow interrupt in modpower
** 2002-02-24:  corrected sign in modpower when modulus negative
** 2003-03-01:  allow negative exponents in modpower
*/

#include "common.h"


PUBLIC void iniaritx	_((void));
PUBLIC truc modpowsym;

/*-------------------------------------------------------------------*/
/* setbit and testbit suppose that vv is an array of word2 */
#define setbit(vv,i)	vv[(i)>>4] |= (1 << ((i)&0xF))
#define testbit(vv,i)	(vv[(i)>>4] & (1 << ((i)&0xF)))
/*-------------------------------------------------------------------*/
PUBLIC int prime16	_((unsigned u));
PUBLIC int prime32	_((word4 u));
PUBLIC unsigned fact16    _((word4 u));
PUBLIC unsigned trialdiv  _((word2 *x, int n, unsigned u0, unsigned u1));
PUBLIC int jacobi	_((int sign, word2 *x, int n, word2 *y, int m,
			   word2 *hilf));
PUBLIC int jac		_((unsigned x, unsigned y));
PUBLIC int rabtest	_((word2 *x, int n, word2 *aux));
PUBLIC int nextprime32 _((word4 u, word2 *x));
PUBLIC int pemult	_((word2 *x, int n, word2 *ex, int exlen,
			   word2 *aa, int alen,
			   word2 *mm, int modlen, word2 *z, word2 *hilf));
PUBLIC int modinverse	_((word2 *x, int n, word2 *y, int m, word2 *zz,
			   word2 *hilf));
PUBLIC int modinv   _((int x, int mm));
PUBLIC int modpower	_((word2 *x, int n, word2 *ex, int exlen,
			   word2 *mm, int modlen, word2 *p, word2 *hilf));
PUBLIC unsigned modpow    _((unsigned x, unsigned n, unsigned mm));

PRIVATE void primsiev	_((word2 *vect, int n));
PRIVATE truc Fjacobi	_((void));

PRIVATE truc Ffact16	_((int argn));
PRIVATE truc Fprime32	_((void));
PRIVATE truc Frabtest	_((void));
PRIVATE int rabtest0 	_((word2 *x, int n, unsigned u, word2 *aux));

PRIVATE truc Fnextprime _((int argn));

PRIVATE truc Fgcd	_((int argn));
PRIVATE unsigned gcdfixnums  _((truc *argptr, int argn));
PRIVATE int gcdbignums	_((word2 *x, word2 *y, truc *argptr, int argn));
PRIVATE truc Fmodinv	_((void));
PRIVATE truc Sgcdx	_((void));
PRIVATE int gcdcx	_((word2 *x, int n, word2 *y, int m,
			   word2 *cx, int *cxlptr, word2 *hilf));
PRIVATE int gcdcxcy	_((word2 *x1, int n1, word2 *x, int n, word2 *y,int m,
			   word2 *cx, int cxlen, word2 *cy, word2 *hilf));

PRIVATE truc Fmodpower	_((int argn));
PRIVATE truc Fcoshmult	_((int argn));
PRIVATE int coshmult	_((word2 *x, int n, word2 *ex, int exlen,
			   word2 *mm, int modlen, word2 *z, word2 *hilf));
PRIVATE truc Fpemult	_((int argn));
PRIVATE truc Ffactorial	 _((void));
PRIVATE int factorial	_((unsigned a, word2 *x));

PRIVATE truc jacobsym, gcdsym, gcdxsym, modinvsym;
PRIVATE truc factorsym, primsym, rabtestsym;
PRIVATE truc nxtprimsym;
PRIVATE truc pemultsym, cshmultsym;
PRIVATE truc factsym;

/*------------------------------------------------------------------*/
PUBLIC void iniaritx()
{
	modpowsym = newintsym("** mod", sFBINARY,(wtruc)Fmodpower);

	gcdsym	  = newsymsig("gcd",sFBINARY,(wtruc)Fgcd, s_0uii);
	gcdxsym	  = newsymsig("gcdx",sSBINARY,(wtruc)Sgcdx, s_iiiII);
	modinvsym = newsymsig("mod_inverse",sFBINARY,(wtruc)Fmodinv, s_iii);

	jacobsym  = newsymsig("jacobi",	 sFBINARY,   (wtruc)Fjacobi, s_iii);
	primsym	  = newsymsig("prime32test",sFBINARY,(wtruc)Fprime32,s_ii);
	rabtestsym= newsymsig("rab_primetest",sFBINARY,(wtruc)Frabtest,s_1);
    nxtprimsym= newsymsig("next_prime",sFBINARY,(wtruc)Fnextprime, s_12);
	factorsym = newsymsig("factor16",sFBINARY,(wtruc)Ffact16, s_13);

	pemultsym = newsymsig("mod_pemult",sFBINARY,(wtruc)Fpemult, s_Viiii);
	cshmultsym= newsymsig("mod_coshmult",sFBINARY,(wtruc)Fcoshmult, s_iiii);
	factsym	  = newsymsig("factorial", sFBINARY, (wtruc)Ffactorial,s_ii);

	primsiev(PrimTab,PRIMTABSIZE);
}
/*-------------------------------------------------------------------*/
/*
** Setzt im Bit-Vektor vect der Laenge n*16 bit (8 <= n <= 2096)
** das bit i, falls 2*i + 1 eine Primzahl ist
*/
PRIVATE void primsiev(vect,n)
word2 *vect;
int n;
{
	word2 mask[16];
	unsigned i, k, inc;

	for(i=0; i<16; i++)
		mask[i] = ~(1 << i);
	setarr(vect,n,0xFFFF);
	n <<= 4;			/* mal 16 */
	vect[0] &= 0xFFFE;		/* da 1 keine Primzahl */
	for(i=1; i<128; i++) {		/* Zahlen 3 bis 255 */
		if(testbit(vect,i)) {	/* falls 2*i + 1 Primzahl */
			inc = i + i + 1;
			for(k=i+inc; k<n; k+=inc)
				vect[k >> 4] &= mask[k & 0xF];
		}
	}
}
/*-------------------------------------------------------------------*/
/*
** Stellt fest, ob die 16-bit-Zahl u prim ist
*/
PUBLIC int prime16(u)
unsigned u;
{
	if(!(u & 1))
		return(u == 2);
	u >>= 1;
	if(testbit(PrimTab,u))
		return(1);
	else
		return(0);
}
/*-------------------------------------------------------------------*/
/*
** Rueckgabewert 1, falls u Primzahl ist, sonst 0
*/
PUBLIC int prime32(u)
word4 u;
{
	unsigned mask, v, p;
	word2 *pbits;

	
	if(u <= 0xFFFF)
		return(prime16((unsigned)u));
	else if(!(u & 1))
		return(0);

	v = intsqrt(u);
	pbits = PrimTab;
	mask = 2;
	for(p=3; p<=v; p += 2) {
		if((*pbits & mask) && (u % p == 0))
			return(0);
		mask <<= 1;
		if((mask & 0xFFFF) == 0) {
			pbits++;
			mask = 1;
		}
	}
	return(1);
}
/*-------------------------------------------------------------------*/
/*
** Rueckgabewert 0, falls u Primzahl ist,
** sonst kleinster Primteiler
*/
PUBLIC unsigned fact16(u)
word4 u;
{
	unsigned mask, v, p;
	word2 *pbits;


	if(!(u & 1))
		return(2);

	v = intsqrt(u);
	pbits = PrimTab;
	mask = 2;
	for(p=3; p<=v; p += 2) {
		if((*pbits & mask) && (u % p == 0))
			return(p);
		mask <<= 1;
		if((mask & 0xFFFF) == 0) {
			pbits++;
			mask = 1;
		}
	}
	return(0);
}
/*------------------------------------------------------------------*/
PUBLIC int nextprime32(u,x)
word4 u;
word2 *x;
{
    int n;

    if(u <= 2) {
        *x = 2;
        return(1);
    }
    else if(u <= 0xFFFFFFFB) {
        if((u & 1) == 0)
            u++;
        while(prime32(u) == 0)
            u += 2;
        n = long2big(u,x);
        return(n);
    }
    else {
        x[0] = 0xF;
        x[1] = 0;
        x[2] = 1;
        return(3);
    }
}
/*------------------------------------------------------------------*/
PRIVATE truc Fnextprime(argn)
int argn;
{
#define ANZRAB  10
    truc *argptr;
    word2 *x;
    word4 u;
    int compos;
    int i, n, sign;
    int doreport;

    argptr = argStkPtr-argn+1;
    if(argn >= 2 && *argStkPtr == zero) {
        doreport = 0;
        argn--;
    }
    else {
        doreport = 1;
    }
    n = bigref(argptr,&x,&sign);
    if(n == aERROR) {
        error(nxtprimsym,err_int,*argptr);
        return(brkerr());
    }
    if(n >= aribufSize/9) {
        error(nxtprimsym,err_int,*argStkPtr);
        return(brkerr());
    }
    if(n <= 2) {
        u = big2long(x,n);
        n = nextprime32(u,AriBuf);
    }
    else {
        cpyarr(x,n,AriBuf);
        x = AriBuf;
        if((x[0] & 1) == 0)
            x[0]++;
        compos = 1;
        if(doreport)
            workmess();
        while(compos) {
            while(trialdiv(x,n,3,0xFFFB))
                n = incarr(x,n,2);
            for(compos=0,i=0; i<ANZRAB; i++) {
                if(rabtest(x,n,AriScratch) == 0) {
                    compos = 1;
                    if(doreport)
                        tick('.');
                    n = incarr(x,n,2);
                    break;
                }
                if(doreport)
                    tick(',');
            }
        }
        if(doreport)
            fprintstr(tstdout," probable prime:");
    }
    return(mkint(0,AriBuf,n));
}
/*------------------------------------------------------------------*/
PRIVATE truc Fjacobi()
{
	int n, m, sign1, sign2;
	int j;
	word2 *x, *y, *hilf;

	if(chkints(jacobsym,argStkPtr-1,2) == aERROR)
		return(brkerr());
	x = AriBuf;
	y = AriScratch;
	hilf = y + aribufSize;
	n = bigretr(argStkPtr-1,x,&sign1);
	m = bigretr(argStkPtr,y,&sign2);
	if(m == 0 || (*y & 1) == 0) {
		error(jacobsym,err_odd,*argStkPtr);
		return(brkerr());
	}
	j = jacobi(sign1,x,n,y,m,hilf);
	return(mksfixnum(j));
}
/*-------------------------------------------------------------------*/
/*
** Berechnet das Jacobi-Symbol von (x,n) ueber (y,m).
** Es wird vorausgesetzt, dass y ungerade ist.
** Falls x und y nicht teilerfremd sind, wird 0 zurueckgegeben.
** Arbeitet destruktiv auf x und y !!!
*/
PUBLIC int jacobi(sign,x,n,y,m,hilf)
word2 *x, *y, *hilf;
int sign, n, m;
{
	int res;
	int m8, tlen;
	word2 *temp;

	res = (sign && (y[0] & 2) ? -1 : 1);	/* (jacobi -1 y) */
	while(n >= 1) {
		while((x[0] & 1) == 0) {
			n = shrarr(x,n,1);
			m8 = y[0] & 0x7;
			if(m8 == 3 || m8 == 5)
				res = -res;
		}
		if(*x == 1 && n == 1)
			return(res);
		temp = x;
		tlen = n;
		x = y;
		n = m;
		y = temp;
		m = tlen;
		if((x[0] & 2) && (y[0] & 2))	/* beide = 3 mod 4 */
			res = -res;
		n = modbig(x,n,y,m,hilf);
	}
	return(0);
}
/*-------------------------------------------------------------------*/
/*
** Version von jacobi fuer kleine nichtnegative integers
** y muss ungerade sein
*/
PUBLIC int jac(x,y)
unsigned x, y;
{
	int res = 1;
	int m8;
	unsigned temp;

	while(x >= 1) {
		while((x & 1) == 0) {
			x >>= 1;
			m8 = y & 0x7;
			if(m8 == 3 || m8 == 5)
				res = -res;
		}
		if(x == 1)
			return(res);
		if((x & 2) && (y & 2))		/* beide = 3 mod 4 */
			res = -res;
		temp = x;
		x = y % temp;
		y = temp;
	}
	return(0);
}
/*------------------------------------------------------------------*/
PRIVATE truc Ffactorial()
{
	int n;
	word2 a;

	if(*FLAGPTR(argStkPtr) != fFIXNUM || *SIGNPTR(argStkPtr)) {
		error(factsym,err_pfix,*argStkPtr);
		return(brkerr());
	}
	a = *WORD2PTR(argStkPtr);
	if(a <= 1)
		return(constone);
	if(bitlen(a) > maxfltex/a + 1) {
		error(factsym,err_ovfl,voidsym);
		return(brkerr());
	}
	n = factorial(a,AriBuf);
	return(mkint(0,AriBuf,n));
}
/*------------------------------------------------------------------*/
/*
** Hypothesis: a > 1
** The factorial of a is stored in x (which must be large enough),
** its length is returned
*/
PRIVATE int factorial(a,x)
unsigned a;
word2 *x;
{
	word4 u;
	unsigned small = 718;
	unsigned i, k, b;
	int len, sh;

	if(a & 1) {
		x[0] = a;
		a--;
	}
	else
		x[0] = 1;
	len = 1;
#ifdef M_3264
	if(!(a & 2)) {
		u = a;
		u *= a-1;
		len = mult4arr(x,len,u,x);
		a -= 2;
	} /* now a = 2 mod 4 */
	b = (a <= small ? a : small);
	for(i=3, k=b>>1; i<b; i+=4, k-=2) {
		u = i*k;
		u *= (i+2)*(k-1);
		len = mult4arr(x,len,u,x);
	}
	for(i=small+1, k=(small>>1)+1; i<a; i+=2, k++) {
		u = i;
		u *= k;
		len = mult4arr(x,len,u,x);
	}
#else
	b = (a <= small ? a : small);
	for(i=3, k=b>>1; i<b; i+=2, k--)
		len = multarr(x,len,i*k,x);
	for(i=small+1, k=(small>>1)+1; i<a; i+=2, k++) {
		len = multarr(x,len,i,x);
		len = multarr(x,len,k,x);
	}
#endif
	sh = a >> 1;
	len = shiftarr(x,len,sh);
	return(len);
}
/*------------------------------------------------------------------*/
PRIVATE truc Ffact16(argn)
int argn;		/* 1 <= argn <= 3 */
{
	truc *argptr;
	word2 *x;
	unsigned u;
	unsigned u0 = 2, u1 = 0xFFF1;  /* largest prime <= 0xFFFF */
	int n, sign, flg;

	argptr = argStkPtr - argn + 1;
	if(chkints(factorsym,argptr,argn) == aERROR)
		return(brkerr());
	n = bigref(argptr,&x,&sign);
	if(argn >= 2) {
		argptr++;
		flg = *FLAGPTR(argptr);
		if(flg != fFIXNUM)
			return(zero);
		else
			u0 = *WORD2PTR(argptr);
	}
	if(argn >= 3 && *FLAGPTR(argStkPtr) == fFIXNUM)
		u1 = *WORD2PTR(argStkPtr);
	u = trialdiv(x,n,u0,u1);
	return(mkfixnum(u));
}
/*------------------------------------------------------------------*/
PRIVATE truc Fprime32()
{
	word4 u;
	word2 *x;
	int sign, n, res;

	n = bigref(argStkPtr,&x,&sign);
	if(n == aERROR) {
		error(primsym,err_int,*argStkPtr);
		return(brkerr());
	}
	if(n > 2)
		res = -1;
	else {
		u = big2long(x,n);
		res = prime32(u);
	}
	return(mksfixnum(res));
}
/*-------------------------------------------------------------------*/
/*
** Probedivision von (x,n) durch 16-bit Primzahlen
** zwischen u0 >= 2 und u1 <= (x,n)/2
** Der erste Teiler wird zurueckgegeben; 
** falls keiner gefunden, Rueckgabewert 0
*/
PUBLIC unsigned trialdiv(x,n,u0,u1)
word2 *x;
int n;
unsigned u0, u1;
{
	extern word2 *PrimTab;

	word2 *pbits;
	unsigned u, mask = 1;

	u = x[0];
	if(n == 1) {
		if(prime16(u))
			return(0);
		else if(u1 > (u >> 1))
			u1 = (u >> 1);
	}
	if(u1 > 0xFFF1)
		u1 = 0xFFF1;	  /* largest prime <= 0xFFFF */
	if(u0 > u1 || u1 < 2 || !n)
		return(0);
	if(u0 <= 2 && !(u & 1))
		return(2);
	u0 >>= 1;
	pbits = PrimTab + (u0 >> 4);
	mask <<= (u0 & 0xF);
	for(u = 2*u0 + 1; u <= u1; u += 2) {
		if((*pbits & mask) && (modarr(x,n,u) == 0))
			return(u);
		mask <<= 1;
		if((mask & 0xFFFF) == 0) {
			pbits++;
			mask = 1;
		}
	}
	return(0);
}
/*------------------------------------------------------------------*/
/*
** Strong-Pseudo-Primzahltest nach Rabin
*/
PRIVATE truc Frabtest()
{
/*
** TODO: optional argument base
*/
	word2 *x;
	int sign, n;
	unsigned u;

	n = bigref(argStkPtr,&x,&sign);
	if(n == aERROR) {
		error(rabtestsym,err_int,*argStkPtr);
		return(brkerr());
	}
	if(n == 0)
		return(false);
	else if(n <= 2) {
		return(prime32(big2long(x,n)) > 0 ? true : false);
	}
	else if((x[0] & 1) == 0)
		return(false);
	else if(n < scrbufSize/9 && n < aribufSize/2) {
		u = 2 + random2(64000);
		return(rabtest0(x,n,u,AriScratch) ? true : false);
	}
	else {
		error(rabtestsym,err_ovfl,*argStkPtr);
		return(brkerr());
	}
}
/*------------------------------------------------------------------*/
/*
** Puffer aux muss mindestens 8*n + 2 lang sein.
** u is a positive integer < 2**16
*/
PRIVATE int rabtest0(x,n,u,aux)
word2 *x, *aux;
unsigned u;
int n;
{
	word2 *base, *ex, *y, *x1, *hilf;
	int exlen, ylen, n1;
	int i, t;

	base = aux;
	y = aux + n;
	x1 = y + 2*n;
	ex = x1 + n;
	hilf = ex + n;

	base[0] = u;
	cpyarr(x,n,x1);
	n1 = decarr(x1,n,1);
	cpyarr(x1,n1,ex);
	exlen = n1;
	t = 0;
	while((ex[0] & 1) == 0) {
		t++;
		exlen = shrarr(ex,exlen,1);
	}
	ylen = modpower(base,1,ex,exlen,x,n,y,hilf);
	if((ylen == 1 && y[0] == 1) || (cmparr(y,ylen,x1,n1) == 0)) {
		return(1);
	}
	/* else */
	ex[0] = 2; exlen = 1;
	for(i=1; i<t; i++) {
		cpyarr(y,ylen,base);
		ylen = modpower(base,ylen,ex,exlen,x,n,y,hilf);
		if(cmparr(y,ylen,x1,n1) == 0)
			return(1);
	}
	return(0);
}
/*------------------------------------------------------------------*/
PUBLIC int rabtest(x,n,aux)
word2 *x, *aux;
int n;
{
	unsigned u;

	u = 2 + random2(64000);
	return(rabtest0(x,n,u,aux));
}
/*------------------------------------------------------------------*/
PRIVATE truc Fgcd(argn)
int argn;		/* 0 <= argn < infinity */
{
	truc *argptr;
	unsigned u;
	int n, flg;

	if(argn == 1 && *FLAGPTR(argStkPtr) == fVECTOR) {
		flg = chkintvec(gcdsym,argStkPtr);
		if(flg >= fFIXNUM) {
			argn = *VECLENPTR(argStkPtr);
			argptr = VECTORPTR(argStkPtr);
		}
    }
	else if(argn > 0) {
		argptr = argStkPtr - argn + 1;
		flg = chkints(gcdsym,argptr,argn);
    }
	else if(argn == 0)
		return(zero);

	if(flg == aERROR)
		return(brkerr());

	if(flg == fFIXNUM) {
		u = gcdfixnums(argptr,argn);
		return(mkfixnum(u));
	}
	else {	/* flg == fBIGNUM */
		n = gcdbignums(AriBuf,AriScratch,argptr,argn);
		return(mkint(0,AriBuf,n));
	}
}
/*------------------------------------------------------------------*/
PRIVATE unsigned gcdfixnums(argptr,argn)
truc *argptr;
int argn;	/* argn > 0 */
{
	unsigned x;

	x = *WORD2PTR(argptr++);
	while(--argn > 0) {
		x = shortgcd(x,*WORD2PTR(argptr++));
		if(x == 1)
			break;
	}
	return(x);
}
/*------------------------------------------------------------------*/
PRIVATE int gcdbignums(x,y,argptr,argn)
word2 *x, *y;
truc *argptr;
int argn;	/* argn > 0 */
{
	word2 *hilf;
	int n, m, sign;

	n = bigretr(argptr++,x,&sign);
	while(--argn > 0) {
		m = bigretr(argptr++,y,&sign);
		if(m == 0)
			continue;
		hilf = y + m;
		n = biggcd(x,n,y,m,hilf);
		if(*x == 1 && n == 1)
			break;
	}
	return(n);
}
/*-------------------------------------------------------------------*/
PRIVATE truc Fmodinv()
{
	word2 *x, *y;
	int n, m, bound, len;
	int sign1, sign2;

	if(chkints(modinvsym,argStkPtr-1,2) == aERROR) {
		return(brkerr());
	}

	n = bigref(argStkPtr-1,&x,&sign1);
	m = bigref(argStkPtr,&y,&sign2);
	if(n == 0 || m == 0) {
		return(zero);
	}
	bound = (aribufSize >> 1) - 2;
	if((n >= bound) || (m >= bound)) {
		error(modinvsym,err_ovfl,voidsym);
		return(brkerr());
	}
	len = modinverse(x,n,y,m,AriBuf,AriScratch);

	if(len == 0)	/* not relatively prime */
		return(zero);
	else if(sign1)
		len = sub1arr(AriBuf,len,y,m);
	return(mkint(0,AriBuf,len));
}
/*-------------------------------------------------------------------*/
PRIVATE truc Sgcdx()
{
	truc res;
	word2 *x, *y, *cx, *cy, *x1;
	int n, m, N, cxlen, cylen, n1;
	int sign1, sign2;
	int flg;

	res = eval(ARGNPTR(evalStkPtr,1));
	ARGpush(res);
	res = eval(ARGNPTR(evalStkPtr,2));
	ARGpush(res);
	flg = chkints(gcdxsym,argStkPtr-1,2);
	if(flg == aERROR) {
		res = brkerr();
		goto cleanup;
	}
	x1 = AriBuf;
	n = bigref(argStkPtr-1,&x,&sign1);
	m = bigref(argStkPtr,&y,&sign2);
        N = (n >= m ? n : m) + 2;
	if(N >= aribufSize/3 || N >= scrbufSize/10) {
		error(gcdxsym,err_ovfl,voidsym);
		return(brkerr());
	}
	cpyarr(x,n,x1);
	cx = x1 + N;
	cy = cx + N;
	if(n && m) {
		n1 = gcdcx(x1,n,y,m,cx,&cxlen,AriScratch);
		cylen = gcdcxcy(x1,n1,x,n,y,m,cx,cxlen,cy,AriScratch);
                if(cxlen)
		        sign2 = (sign2 ? 0 : MINUSBYTE);
	}
	else if(n) {	/* x != 0, y == 0 */
		n1 = n;
		cx[0] = 1;
		cxlen = 1;
		cylen = 0;
	}
	else if(m) {	/* x == 0, y != 0 */
		n1 = m;
		cpyarr(y,m,x1);
		cxlen = 0;
		cy[0] = 1;
		cylen = 1;
	}
	else {		/* x == 0, y == 0 */
		n1 = cxlen = cylen = 0;
	}
	Lvalassign(ARGNPTR(evalStkPtr,3),mkint(sign1,cx,cxlen));
	Lvalassign(ARGNPTR(evalStkPtr,4),mkint(sign2,cy,cylen));
	res = mkint(0,x1,n1);
  cleanup:
	ARGnpop(2);
	return(res);
}
/*-------------------------------------------------------------------*/
/*
** Berechnet den GGT von (x,n) und (y,m), destruktiv auf x
** Es wird vorausgesetzt, dass (y,m) ungleich Null.
** x wird durch den GGT ersetzt, seine Laenge ist der Rueckgabewert.
** y wird nicht beruehrt
** Ausserdem wird in (cx, *cxlptr) ein Koeffizient abgelegt,
** so dass (x,n) * (cx,*cxlptr) = GGT mod (y,m).
** Falls (x,n) = 0, wird (y,m) auf Platz x kopiert!
** Platz hilf muss 5 * max(n,m) + 10 lang sein
*/
PRIVATE int gcdcx(x,n,y,m,cx,cxlptr,hilf)
word2 *x, *y, *cx, *hilf;
int n, m;
int *cxlptr;
{
	word2 *q, *x1, *x2, *alfa, *beta, *prod, *temp;
	int n1, n2, N, qlen, rlen, alen, blen, plen, tlen;
	int count;

	N = (n > m ? n + 2 : m + 2);

	beta = cx;
	x1 = hilf;
	q = hilf + N;
	alfa = hilf + 2*N;
	prod = hilf + 3*N;
    hilf += 4*N;

	if(m == 0) {
		return(0);
	}
	n2 = modbig(x,n,y,m,hilf);
    if(n2 == 0) {
        cpyarr(y,m,x);
        *cxlptr = 0;
        return(m);
    }
	x2 = x;
	cpyarr(y,m,x1);
	n1 = m;
	alen = 0;
	blen = 1;
	beta[0] = 1;
/*
** Schleifeninvarianten:
** (x1,n1) = -+(alfa,alen)*(x,n) mod (y,m)
** (x2,n2) = +-(beta,blen)*(x,n) mod (y,m)
*/
    count = 0;
	while(qlen = divbig(x1,n1,x2,n2,q,&rlen,hilf), rlen) {
        count++;
		/* x1neu = x2alt, x2neu = rest */
		temp = x1;
		x1 = x2;
		n1 = n2;
		x2 = temp;
		n2 = rlen;
		plen = multbig(beta,blen,q,qlen,prod,hilf);
		/* alfaneu = betaalt, betaneu = alfaalt + q*betaalt */
		temp = alfa;
		tlen = alen;
		alfa = beta;
		alen = blen;
		beta = temp;
		blen = addarr(beta,tlen,prod,plen);
	}
	if(count & 1)
		blen = sub1arr(beta,blen,y,m);
	if(beta != cx)
		cpyarr(beta,blen,cx);
	*cxlptr = blen;
	if(x2 != x)
		cpyarr(x2,n2,x);
	return(n2);
}
/*------------------------------------------------------------------*/
/*
** Berechnet das Inverse von (x,n) modulo (y,m)
** Resultat wird in zz abgelegt, seine Laenge ist Rueckgabewert.
** x und y bleiben erhalten
** Falls x und y nicht teilerfremd, wird 0 zurueckgegeben.
*/
PUBLIC int modinverse(x,n,y,m,zz,hilf)
word2 *x, *y, *zz, *hilf;
int n, m;
{
	word2 *xx;
	int N, k, len;

	N = (n > m ? n + 2 : m + 2);
	xx = hilf;
	hilf += N;
	cpyarr(x,n,xx);
	k = gcdcx(xx,n,y,m,zz,&len,hilf);
	if((k != 1) || (xx[0] != 1))
		return(0);
	else
		return(len);
}
/*---------------------------------------------------------------*/
/*
** Calculates inverse of x mod mm
** If x is not invertible mod mm, then 0 is returned
*/
PUBLIC int modinv(x,mm)
int x,mm;
{
    int y, yold, q, q1, q2, q2old;

   	q1 = 1; q2 = 0;
	y = mm;
    while(y) {
		yold = y;
		q = x / y;
		y = x % y;
		x = yold;
		q2old = q2;
		q2 = q1 - q*q2;
		q1 = q2old;
    }
    if(x == 1) {
        return (q1 >= 0 ? q1 : mm+q1);
    }
    else
        return 0;
}
/*------------------------------------------------------------------*/
/*
** Berechnet den Koeffizienten cy fuer die Darstellung
**		 x1 = cx * x - cy * y.
** x, y, x1 und cx sind vorgegeben.
** Falls cx = 0, wird x1 = cy * y.
** Dabei wird vorausgesetzt, dass y /= 0 und x1 der GGT von x und y ist.
** Die Laenge von cy ist der Rueckgabewert.
*/
PRIVATE int gcdcxcy(x1,n1,x,n,y,m,cx,cxlen,cy,hilf)
word2 *x1, *x, *y, *cx, *cy, *hilf;
int n1, n, m, cxlen;
{
	word2 *temp;
	int tlen, len, rlen;

	temp = hilf;
	hilf += n + cxlen + 2;

    if(cxlen) {
        tlen = multbig(x,n,cx,cxlen,temp,hilf);
        tlen = subarr(temp,tlen,x1,n1);
    }
    else {
        cpyarr(x1,n1,temp);
        tlen = n1;
    }
	len = divbig(temp,tlen,y,m,cy,&rlen,hilf);
	return(len);
}
/*------------------------------------------------------------------*/
/*
** Base ** Ex mod Modulus
*/
PRIVATE truc Fmodpower(argn)
int argn;
{
	word2 *x, *y, *z, *hilf;
	int n, n1, n2, n3;
	int sign1, sign2, sign3;

	if(chkints(modpowsym,argStkPtr-2,3) == aERROR)
		return(brkerr());

	x = AriScratch;
	hilf = AriScratch + aribufSize;

	n1 = bigretr(argStkPtr-2,x,&sign1);
	n2 = bigref(argStkPtr-1,&y,&sign2);
	n3 = bigref(argStkPtr,&z,&sign3);
	if(!n3) {
		error(modpowsym,err_div,voidsym);
		return(brkerr());
	}
	/* overflow? */
	n = (n3 > n1 ? n3 : n1) + 3;
	if (n >= aribufSize/2 || n + aribufSize/3 >= scrbufSize/3) {
		error(modpowsym,err_ovfl,voidsym);
		return(brkerr());
	}
	if(sign2) {
        if(n1)
            n1 = modinverse(x,n1,z,n3,AriBuf,hilf);
        if(!n1) {
		    error(modpowsym,err_div,voidsym);
		    return(brkerr());
        }
        if(n2 == 1 && y[0] == 1) {
            n = n1;
            goto testsigns;
        }
        else {
            cpyarr(AriBuf,n1,x);
        }
	}
	n = modpower(x,n1,y,n2,z,n3,AriBuf,hilf);
	if(n == 0)
		return zero;
	/* else */
	if(n2 == 0)
		sign1 = 0;
	else if((*y & 1) == 0)
		sign1 = 0;
  testsigns:
	if(sign1 != sign3)
		n = sub1arr(AriBuf,n,z,n3);

	return(mkint(sign3,AriBuf,n));
}
/*-------------------------------------------------------------------*/
/*
** p = (x,n) hoch (ex,exlen) modulo (mm,modlen)
** (x,n) wird destruktiv modulo (mm,modlen) reduziert
** Der Puffer fuer p muss mindestens 2*modlen lang sein
** hilf ist Platz fuer Hilfsvariable,
** muss (2*modlen + max(n,modlen) + 2) lang sein.
*/
PUBLIC int modpower(x,n,ex,exlen,mm,modlen,p,hilf)
word2 *x, *ex, *mm, *p, *hilf;
int n, exlen, modlen;
{
	word2 *temp;
	int k, plen;
    int allowintr;

	temp = hilf + 2*modlen + 2;

	if(exlen == 0) {
		p[0] = 1;
		return(1);
	}
	n = modbig(x,n,mm,modlen,hilf);
	if(n == 0)
		return(0);

	cpyarr(x,n,p);
	plen = n;		/* plen <= modlen */

	k = ((exlen-1) << 4) + bitlen(ex[exlen-1]) - 1;
    allowintr = (modlen >= 16 && (k/16 + modlen >= 256) ? 1 : 0);
	while(--k >= 0) {
		plen = multbig(p,plen,p,plen,temp,hilf);
		cpyarr(temp,plen,p);
		plen = modbig(p,plen,mm,modlen,hilf);
		if(testbit(ex,k)) {
			plen = multbig(p,plen,x,n,temp,hilf);
			cpyarr(temp,plen,p);
			plen = modbig(p,plen,mm,modlen,hilf);
		}
        if(allowintr && INTERRUPT) {
            setinterrupt(0);
            reset(err_intr);
        }
	}
	return(plen);
}
/*---------------------------------------------------------------*/
/*
** Calculates x**n mod mm
** mm must be < 2**16
*/
PUBLIC unsigned modpow(x,n,mm)
unsigned x,n,mm;
{
    word4 u,z;

    if(n == 0)
        return(1);
    z = 1; u = x % mm;
    while(n > 1) {
        if(n & 1)
            z = (z*u) % mm;
        u = (u*u) % mm;
        n >>= 1;
    }
    return((z*u) % mm);
}
/*------------------------------------------------------------------*/
/*
** mod_coshmult(Base,Exponent,Module)
*/
PRIVATE truc Fcoshmult(argn)
int argn;	/* argn = 3 */
{
	word2 *x, *y, *z, *res, *hilf;
	int len, n1, n2, n3;
	int sign1, sign;

	if(chkints(cshmultsym,argStkPtr-2,3) == aERROR)
		return(brkerr());

	x = AriScratch;
	res = AriBuf;
	hilf = AriScratch + aribufSize;

	n1 = bigretr(argStkPtr-2,x,&sign1);
	n2 = bigref(argStkPtr-1,&y,&sign);
	n3 = bigref(argStkPtr,&z,&sign);
	if(!n3) {
		error(cshmultsym,err_div,voidsym);
		return(brkerr());
	}
	len = coshmult(x,n1,y,n2,z,n3,res,hilf);
	return(mkint(0,res,len));
}
/*------------------------------------------------------------------*/
/*
** Berechnet z = cosh(ex*xi) modulo mm, wobei x = cosh(xi).
** z = v(ex) berechnet sich durch folgende Rekursion:
** v(0) = 1; v(1) = x;
** v(2*n) = 2*v(n)*v(n) - 1;
** v(2*n+1) = 2*v(n+1)*v(n) - x.
*/
PRIVATE int coshmult(x,n,ex,exlen,mm,modlen,z,hilf)
word2 *x, *ex, *mm, *z, *hilf;
int n, exlen, modlen;
{
	word2 *v, *w, *u, *temp;
	int k, vlen, wlen, ulen, tlen, zlen;
	int bit, bit0;

	v = z;
	w = hilf;
	u = w + modlen + 2;
	hilf = u + 2*modlen + 2;

	if(exlen == 0) {
		z[0] = 1;
		return(1);
	}
	n = modbig(x,n,mm,modlen,hilf);
	v[0] = 1;
	vlen = 1;
	cpyarr(x,n,w);
	wlen = n;

	bit = 0;
	k = ((exlen-1) << 4) + bitlen(ex[exlen-1]);

	while(--k >= 0) {
		bit0 = bit;
		bit = (testbit(ex,k) ? 1 : 0);
		if(bit != bit0) {
			temp = v;	tlen = vlen;
			v = w;		vlen = wlen;
			w = temp;	wlen = tlen;
		}
			/* w := 2*v*w - x */
		ulen = multbig(w,wlen,v,vlen,u,hilf);
		ulen = modbig(u,ulen,mm,modlen,hilf);
		cpyarr(u,ulen,w);
		wlen = addarr(w,ulen,u,ulen);
		if(cmparr(w,wlen,x,n) < 0)
			wlen = addarr(w,wlen,mm,modlen);
		wlen = subarr(w,wlen,x,n);

			/* v := 2*v*v - 1 */
		ulen = multbig(v,vlen,v,vlen,u,hilf);
		ulen = modbig(u,ulen,mm,modlen,hilf);
		cpyarr(u,ulen,v);
		vlen = addarr(v,ulen,u,ulen);
		if(vlen == 0) {
			cpyarr(mm,modlen,v);
			vlen = modlen;
		}
		vlen = decarr(v,vlen,1);
	}
	zlen = (bit == 0 ? vlen : wlen);
	zlen = modbig(z,zlen,mm,modlen,hilf);
	return(zlen);
}
/*------------------------------------------------------------------*/
/*
** mod_pemult(Base,Exponent,Param,Module)
*/
PRIVATE truc Fpemult(argn)
int argn;	/* argn = 4 */
{
	truc *ptr;
	truc vector, obj;
	word2 *x, *y, *aa, *z, *res, *hilf;
	int len, n1, n2, n3, alen;
	int sign1, sign2, sign;

	if(chkints(pemultsym,argStkPtr-3,4) == aERROR)
		return(brkerr());

	x = AriScratch;
	res = AriBuf;
	hilf = AriScratch + aribufSize;

	n1 = bigretr(argStkPtr-3,x,&sign1);
	n2 = bigref(argStkPtr-2,&y,&sign);
	alen = bigref(argStkPtr-1,&aa,&sign2);
	n3 = bigref(argStkPtr,&z,&sign);
	if(!n3) {
		error(pemultsym,err_div,voidsym);
		return(brkerr());
	}
	
	len = pemult(x,n1,y,n2,aa,alen,z,n3,res,hilf);

	vector = mkvect0(2);
	WORKpush(vector);
	if(len >= 0) {
		obj = mkint(0,res,len);
	}
	else {
		obj = mkint(0,res,-len-1);
	}
	ptr = VECTORPTR(workStkPtr);
	ptr[0] = obj;
	ptr[1] = (len >= 0 ? constone : zero);
	vector = WORKretr();
	return(vector);
}
/*------------------------------------------------------------------*/
/*
** Berechnet fuer die elliptische Kurve B*y*y = x*x*x + aa*x*x + x
** in z die x-Koordinate modulo mm von ex*P,
** wobei P ein Punkt der Kurve mit x-Koordinate (x,n) ist.
** Falls die Berechnung scheitert, weil ein Teiler von
** mm auftaucht, wird dieser Teiler in z abgelegt;
** der Rueckgabewert ist dann -(lenz + 1).
*/
PUBLIC int pemult(x,n,ex,exlen,aa,alen,mm,modlen,z,hilf)
word2 *x, *ex, *aa, *mm, *z, *hilf;
int n, exlen, alen, modlen;
{
	

	
	word2 *yy1, *yy2, *pp1, *pp2, *uu1, *uu2;
	int zlen, y1len, y2len, p1len, p2len, u1len, u2len;
	int k, n0, n1, m0, m1;
	int sign0, sign1;
	int odd;
	int ll;
	int first;

	struct {
		word2 *num;
		word2 *den;
		int nlen;
		int dlen;
	} vv, ww, *act0, *act1;

	ll = 2*modlen + 2;
	vv.num = z;
	vv.den = hilf;
	ww.num = hilf + ll;
	ww.den = hilf + 2*ll;
	yy1 = hilf + 3*ll;
	yy2 = hilf + 4*ll;
	pp1 = hilf + 5*ll;
	pp2 = hilf + 6*ll;
	uu1 = hilf + 7*ll;
	uu2 = hilf + 8*ll;
	hilf = hilf + 9*ll;

	n = modbig(x,n,mm,modlen,hilf);
	if(exlen == 0) {
		cpyarr(mm,modlen,z);
		return(-modlen);
	}
	cpyarr(x,n,vv.num);
	vv.nlen = n;
	vv.den[0] = 1;
	vv.dlen = 1;
	cpyarr(x,n,ww.num);
	ww.nlen = n;
	ww.den[0] = 1;
	ww.dlen = 1;

	first = 1;
	
	k = ((exlen-1) << 4) + bitlen(ex[exlen-1]);
	
	
	
	while(--k >= 0) {
		odd = testbit(ex,k);
		act0 = (odd ? &ww : &vv);
		act1 = (odd ? &vv : &ww);
		
		n0 = act0->nlen;
		m0 = act0->dlen;
		
		n1 = act1->nlen;
		m1 = act1->dlen;

		if(first) {
			first = 0;
			goto duplic;
		}
/*
** i --> 2*i + 1
** X(2*i+1) = ((X(i+1)-Z(i+1))*(X(i)+Z(i)) + (X(i+1)+Z(i+1))*(X(i)-Z(i)))**2
** Z(2*i+1) = x*(((X(i+1)-Z(i+1))*(X(i)+Z(i))-(X(i+1)+Z(i+1))*(X(i)-Z(i)))**2
*/
		sign0 = cmparr(act0->num,n0,act0->den,m0);
		sign1 = cmparr(act1->num,n1,act1->den,m1);
		cpyarr(act1->num,n1,yy1);
		if(sign1 >= 0)
			y1len = subarr(yy1,n1,act1->den,m1);
		else
			y1len = sub1arr(yy1,n1,act1->den,m1);
		cpyarr(act0->num,n0,yy2);
		y2len = addarr(yy2,n0,act0->den,m0);
		p1len = multbig(yy1,y1len,yy2,y2len,pp1,hilf);
		p1len = modbig(pp1,p1len,mm,modlen,hilf);
		
/* pp1 = (X(i+1)-Z(i+1))*(X(i)+Z(i)) */

		cpyarr(act1->num,n1,yy1);
		y1len = addarr(yy1,n1,act1->den,m1);
		cpyarr(act0->num,n0,yy2);
		if(sign0 >= 0)
			y2len = subarr(yy2,n0,act0->den,m0);
		else
			y2len = sub1arr(yy2,n0,act0->den,m0);
		p2len = multbig(yy1,y1len,yy2,y2len,pp2,hilf);
		p2len = modbig(pp2,p2len,mm,modlen,hilf);
		
/* pp2 = (X(i+1)+Z(i+1))*(X(i)-Z(i)) */

		cpyarr(pp1,p1len,uu1);
		if(sign0 * sign1 >= 0)
			u1len = addarr(uu1,p1len,pp2,p2len);
		else if(cmparr(pp1,p1len,pp2,p2len) >= 0)
			u1len = subarr(uu1,p1len,pp2,p2len);
		else
			u1len = sub1arr(uu1,p1len,pp2,p2len);
		u2len = multbig(uu1,u1len,uu1,u1len,uu2,hilf);
		u2len = modbig(uu2,u2len,mm,modlen,hilf);
		cpyarr(uu2,u2len,act1->num);
		act1->nlen = u2len;

		cpyarr(pp1,p1len,uu1);
		if(sign0 * sign1 <= 0)
			u1len = addarr(uu1,p1len,pp2,p2len);
		else if(cmparr(pp1,p1len,pp2,p2len) >= 0)
			u1len = subarr(uu1,p1len,pp2,p2len);
		else
			u1len = sub1arr(uu1,p1len,pp2,p2len);
		u2len = multbig(uu1,u1len,uu1,u1len,uu2,hilf);
		u2len = modbig(uu2,u2len,mm,modlen,hilf);
		p1len = multbig(uu2,u2len,x,n,pp1,hilf);
		p1len = modbig(pp1,p1len,mm,modlen,hilf);
		cpyarr(pp1,p1len,act1->den);
		act1->dlen = p1len;

  duplic:
/*
** i --> 2*i
** X(2*i) = (X(i)*X(i) - Z(i)*Z(i))**2
** Z(2*i) = 4*X(i)*Z(i)*(X(i)*X(i) + A*X(i)*Z(i) + Z(i)*Z(i))
*/

/* yy1 = X(i)*X(i) */
		y1len = multbig(act0->num,n0,act0->num,n0,yy1,hilf);
		y1len = modbig(yy1,y1len,mm,modlen,hilf);
/* yy2 = Z(i)*Z(i) */
		y2len = multbig(act0->den,m0,act0->den,m0,yy2,hilf);
		y2len = modbig(yy2,y2len,mm,modlen,hilf);
/* uu1 = X(i)*Z(i) */
		u1len = multbig(act0->num,n0,act0->den,m0,uu1,hilf);
		u1len = modbig(uu1,u1len,mm,modlen,hilf);
/* uu2 = A*X(i)*Z(i) */
		u2len = multbig(uu1,u1len,aa,alen,uu2,hilf);
		u2len = modbig(uu2,u2len,mm,modlen,hilf);

		

		cpyarr(yy1,y1len,pp1);
		if(cmparr(yy1,y1len,yy2,y2len) >= 0)
			p1len = subarr(pp1,y1len,yy2,y2len);
		else
			p1len = sub1arr(pp1,y1len,yy2,y2len);
		
		p2len = multbig(pp1,p1len,pp1,p1len,pp2,hilf);
		p2len = modbig(pp2,p2len,mm,modlen,hilf);
		cpyarr(pp2,p2len,act0->num);
		act0->nlen = p2len;

		cpyarr(yy1,y1len,pp1);
		p1len = addarr(pp1,y1len,uu2,u2len);
		p1len = addarr(pp1,p1len,yy2,y2len);
		p1len = shlarr(pp1,p1len,2);
/* pp1 = 4*(X(i)*X(i) + A*X(i)*Z(i) + Z(i)*Z(i)) */
		p2len = multbig(pp1,p1len,uu1,u1len,pp2,hilf);
		p2len = modbig(pp2,p2len,mm,modlen,hilf);
		cpyarr(pp2,p2len,act0->den);
		act0->dlen = p2len;
	}
	m0 = vv.dlen;
	cpyarr(vv.den,m0,uu1);

	m0 = gcdcx(uu1,m0,mm,modlen,pp1,&p1len,hilf);
	if(m0 != 1 || uu1[0] != 1) {
		cpyarr(uu1,m0,z);
		return(-m0-1);
	}
	p2len = multbig(vv.num,vv.nlen,pp1,p1len,pp2,hilf);
	zlen = modbig(pp2,p2len,mm,modlen,hilf);
	cpyarr(pp2,zlen,z);
	return(zlen);
}
/********************************************************************/
