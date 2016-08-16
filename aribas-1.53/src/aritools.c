/****************************************************************/
/* file aritools.c

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
** aritools.c
** tools fuer bignum-Arithmetik
**
** vorzeichenlose bignums werden als Paare (x,n) dargestellt,
** wobei n eine ganze Zahl >= 0 und x ein Array (x[0],...,x[n-1])
** von unsigned 16-bit-Zahlen ist; fuer die Zahl 0 ist n = 0,
** sonst ist stets n > 0 und x[n-1] != 0
**
** Rueckgabewert der Funktionen meist Laenge des Resultat-Arrays
**
** Typen: word2 ist unsigned 16-bit-Zahl, word4 unsigned 32-bit-Zahl
*/
/*-------------------------------------------------------------------*/
/*
** date of last change
** 1995-10-29:	modification of function biggcd
** 1997-02-25:	more M_3264 support in multbig, divbig and modbig
** 1999-06-03:  power for exponents a >= 2**16; return to old version of modbig
** 2002-04-20:  modmultbig, addsarr
** 2003-06-16:  modnegbig
** 2003-11-09:  bugfix in function divbig
*/

#include "common.h"

PUBLIC int shiftarr	_((word2 *x, int n, int sh));
PUBLIC int lshiftarr	_((word2 *x, int n, long sh));
PUBLIC int addarr	_((word2 *x, int n, word2 *y, int m));
PUBLIC int subarr	_((word2 *x, int n, word2 *y, int m));
PUBLIC int sub1arr	_((word2 *x, int n, word2 *y, int m));
PUBLIC int addsarr  _((word2 *x, int n, int sign1,
                word2 *y, int m, int sing2, int *psign));
PUBLIC int multbig	_((word2 *x, int n, word2 *y, int m, word2 *z,
                word2 *hilf));
PUBLIC int divbig	_((word2 *x, int n, word2 *y, int m, word2 *quot,
                int *rlenptr, word2 *hilf));
PUBLIC int modbig	_((word2 *x, int n, word2 *y, int m, word2 *hilf));
PUBLIC int modnegbig	_((word2 *x, int n, word2 *y, int m, word2 *hilf));
PUBLIC int modmultbig   _((word2 *xx, int xlen, word2 *yy, int ylen,
                word2 *mm, int mlen, word2 *zz, word2 *hilf));
PUBLIC int multfix	_((int prec, word2 *x, int n, word2 *y, int m,
                word2 *z, word2 *hilf));
PUBLIC int divfix	_((int prec, word2 *x, int n, word2 *y, int m,
			   word2 *z, word2 *hilf));
PUBLIC unsigned shortgcd  _((unsigned x, unsigned y));
PUBLIC int biggcd	_((word2 *x, int n, word2 *y, int m, word2 *hilf));
PUBLIC int power	_((word2 *x, int n, unsigned a, word2 *p,
			   word2 *temp, word2 *hilf));
PUBLIC int bigsqrt	_((word2 *x, int n, word2 *z, int *rlenptr,
			   word2 *temp));
PUBLIC int lbitlen	_((word4 x));
PUBLIC int bcd2big	_((word2 *x, int n, word2 *y));
PUBLIC int str2int	_((char *str, int *panz));
PUBLIC int str2big	_((char *str, word2 *arr, word2 *hilf));
PUBLIC int bcd2str	_((word2 *arr, int n, char *str));
PUBLIC int big2xstr	_((word2 *arr, int n, char *str));
PUBLIC int digval	_((int ch));
PUBLIC int xstr2big	_((char *str, word2 *arr));
PUBLIC int ostr2big	_((char *str, word2 *arr));
PUBLIC int bstr2big	_((char *str, word2 *arr));
PUBLIC int nibdigit	_((word2 *arr, int k));
PUBLIC int nibndigit	_((int n, word2 *arr, long k));
PUBLIC int nibascii	_((word2 *arr, int k));
PUBLIC int hexascii	_((int n));
PUBLIC int shiftbcd	_((word2 *arr, int n, int k));
PUBLIC int incbcd	_((word2 *x, int n, unsigned a));

/*-------------------------------------------------------------*/
PRIVATE int shlaux	_((word2 *x, int n, int k, int b));
PRIVATE int shraux	_((word2 *x, int n, int k, int b));
PRIVATE unsigned nibble _((unsigned x, int k));
PRIVATE int str2barr	_((char *str, int b, word2 *arr));
PRIVATE int nthbit	_((word2 *arr, long n));

#define DECBASE 10000
/*-------------------------------------------------------------------*/
/*
** Shift von (x,n) um sh bit-Stellen
** sh > 0: Links-Shift; sh < 0: Rechts-Shift
** arbeitet destruktiv auf x
** x muss genuegend lang sein
*/
PUBLIC int shiftarr(x,n,sh)
word2 *x;
int n, sh;
{
	int k, b;

	if(!n || !sh)
		return(n);
	k = (sh > 0 ? sh : -sh);
	b = k & 0x000F;
	k >>= 4;
	if(sh > 0)
		return(shlaux(x,n,k,b));
	else
		return(shraux(x,n,k,b));
}
/*-------------------------------------------------------------------*/
PRIVATE int shlaux(x,n,k,b)
word2 *x;
int n,k,b;
{
	cpyarr1(x,n,x+k);
	n = k + shlarr(x+k,n,b);
	setarr(x,k,0);
	return(n);
}
/*-------------------------------------------------------------------*/
PRIVATE int shraux(x,n,k,b)
word2 *x;
int n,k,b;
{
	if(k >= n)
		return(0);
	n -= k;
	cpyarr(x+k,n,x);
	return(shrarr(x,n,b));
}
/*-------------------------------------------------------------------*/
/*
** Shift von (x,n) um sh bit-Stellen
** sh ist long, aber abs(sh) >> 4 muss integer sein
** sh > 0: Links-Shift; sh < 0: Rechts-Shift
** arbeitet destruktiv auf x
** x muss genuegend lang sein
*/
PUBLIC int lshiftarr(x,n,sh)
word2 *x;
int n;
long sh;
{
	int k, b;
	word4 a;

	if(!n || !sh)
		return(n);
	a = (sh > 0 ? sh : -sh);
	k = a >> 4;
	b = a & 0x0000000F;
	if(sh > 0)
		return(shlaux(x,n,k,b));
	else
		return(shraux(x,n,k,b));
}
/*-------------------------------------------------------------------*/
/*
** x := (x,n) + (y,m)
*/
PUBLIC int addarr(x,n,y,m)
word2 *x, *y;
int n, m;
{
	int k;

	if(n < m) {
		cpyarr(y+n,m-n,x+n);
		k = m;			/* swap m, n */
		m = n;
		n = k;
	}
	if(sumarr(x,m,y))
		n = m + incarr(x+m,n-m,1);
	return(n);
}
/*-------------------------------------------------------------------*/
/*
** x := (x,n) - (y,m)
** setzt voraus, dass (x,n) >= (y,m)
*/
PUBLIC int subarr(x,n,y,m)
word2 *x, *y;
int n, m;
{
	if(diffarr(x,m,y))
		n = m + decarr(x+m,n-m,1);
	while(n > 0 && x[n-1] == 0)
		n--;
	return(n);
}
/*-------------------------------------------------------------------*/
/*
** x := -(x,n) + (y,m)
** setzt voraus, dass (y,m) >= (x,n)
*/
PUBLIC int sub1arr(x,n,y,m)
word2 *x, *y;
int n, m;
{
	if(n < m)
		cpyarr(y+n,m-n,x+n);
	if(diff1arr(x,n,y))
		m = n + decarr(x+n,m-n,1);
	while(m > 0 && x[m-1] == 0)
		m--;
	return(m);
}
/*-------------------------------------------------------------------*/
/*
** Addition of signed bigintegers
** non-negative numbers: sign = 0;
** negative numbers:     sign = MINUSBYTE;
**
** array x is overwritten with the result; must be long enough
** sign of result in *psign
*/
PUBLIC int addsarr(x,n,sign1,y,m,sign2,psign)
word2 *x, *y;
int sign1, sign2;
int *psign;
{
    int k;
    int cmp;

    if(sign1 == sign2) {
        *psign = sign1;
	    if(n < m) {
		    cpyarr(y+n,m-n,x+n);
		    k = m;			/* swap m, n */
		    m = n;
		    n = k;
	    }
	    if(sumarr(x,m,y))
		    n = m + incarr(x+m,n-m,1);
	    return(n);
    }
    /* else */
    cmp = cmparr(x,n,y,m);
    if(cmp > 0) {
        *psign = sign1;
    	if(diffarr(x,m,y))
	    	n = m + decarr(x+m,n-m,1);
    	while(n > 0 && x[n-1] == 0)
	    	n--;
    	return(n);
    }
    else if(cmp < 0) {
        *psign = sign2;
    	if(n < m)
	    	cpyarr(y+n,m-n,x+n);
    	if(diff1arr(x,n,y))
	    	m = n + decarr(x+n,m-n,1);
    	while(m > 0 && x[m-1] == 0)
	    	m--;
    	return(m);
    }
    else {
        *psign = 0;
        return 0;
    }
}
/*-------------------------------------------------------------------*/
/*
** z := (x,n) * (y,m)
** Rueckgabewert Laenge von z
** hilf ist Platz fuer Hilfsvariable;
** muss mindestens max(n,m) + 2 lang sein
*/
PUBLIC int multbig(x,n,y,m,z,hilf)
word2 *x, *y, *z, *hilf;
int n, m;
{
	int hilflen, zlen;
	word4 u;

	if(!n || !m)
		return(0);
	if(m == 1)
		return(multarr(x,n,(unsigned)*y,z));
	else if(n == 1)
		return(multarr(y,m,(unsigned)*x,z));
#ifdef M_3264
	else if(m == 2) {
		u = big2long(y,2);
		return(mult4arr(x,n,u,z));
	}
	else if(n == 2) {
		u = big2long(x,2);
		return(mult4arr(y,m,u,z));
	}
	setarr(z,m-1,0);
	if(m & 1) {
		y += m-1;
		z += m-1;
		zlen = multarr(x,n,(unsigned)*y,z);
	}
	else {
		y += m;
		z += m;
		zlen = -2;
	}
	while(m >= 2) {
		m -= 2;
		z -= 2;
		zlen += 2;
		u = *--y; 
		u <<= 16;
		u += *--y;
		hilflen = mult4arr(x,n,u,hilf);
		zlen = addarr(z,zlen,hilf,hilflen);
	}
	return(zlen);
#else
	setarr(z,m-1,0);
	y += m;
	z += m;
	zlen = -1;
	while(--m >= 0) {
		hilflen = multarr(x,n,(unsigned)*--y,hilf);
		zlen = addarr(--z,++zlen,hilf,hilflen);
	}
	return(zlen);
#endif
}
/*-------------------------------------------------------------------*/
/*
** Ersetzt (x,n) destruktiv durch (x,n) mod (y,m)
*/
PUBLIC int modbig(x,n,y,m,hilf)
word2 *x, *y, *hilf;
int n, m;
{
	word4 u, v;
	unsigned a;
	int cmp, b, b1;
	int k, hilflen;

	if(!m)		/* Division durch 0 ohne Fehlermeldung */
		return(0);
	if(m == 1) {
		a = y[0];
		a = modarr(x,n,a);
		*x = a;
		return(a ? 1 : 0);
	}
#ifdef M_3264
	if(m == 2) {
		u = big2long(y,m);
		v = mod4arr(x,n,u);
		k = long2big(v,x);
		return(k);
	}
#endif
	k = n - m;
	if(k >= 0)
		cmp = cmparr(x+k,n-k,y,m);
	if(k < 0 || (k == 0 && cmp < 0))
		return(n);		/* (y,m) > (x,n) */
	else if(k > 0 && cmp < 0)
		k--;

	b = bitlen(y[m-1]);
	b1 = 16 - b;
	v = (y[m-1] << b1) + (y[m-2] >> b);
	u = 0xFFFFFFFF;
	u /= (v + 1);

	k++;
	x += k;
	n -= k;
	while(--k >= 0) {
		--x;
		if(n || *x)
			n++;
		if(n >= m) {
			v = (x[m-1] >> b);
			if(n > m)	/* dann n = m+1 */
				v += (x[m] << b1);
			a = (u*v) >> 16;
			hilflen = multarr(y,m,a,hilf);
			n = subarr(x,n,hilf,hilflen);
			while(cmparr(x,n,y,m) >= 0)
				n = subarr(x,n,y,m);
		}
	}
	return(n);
}
/*-------------------------------------------------------------------*/
/*
** Ersetzt (x,n) destruktiv durch -(x,n) mod (y,m)
*/
PUBLIC int modnegbig(x,n,y,m,hilf)
word2 *x, *y, *hilf;
int n, m;
{
    int cmp;

    cmp = cmparr(y,m,x,n);
    if(cmp == 0)
        n = 0;
    else if(cmp < 0)
        n = modbig(x,n,y,m,hilf);
    if(n > 0)
        return sub1arr(x,n,y,m);
    else
        return 0;
}
/*-------------------------------------------------------------------*/
/*
** (zz,ret) = (xx,xlen)*(yy,ylen) mod (mm,mlen)
** TODO: optimize!
*/
PUBLIC int modmultbig(xx,xlen,yy,ylen,mm,mlen,zz,hilf)
word2 *xx,*yy,*mm,*zz,*hilf;
int xlen, ylen, mlen;
{
    int len;

    len = multbig(xx,xlen,yy,ylen,zz,hilf);
    len = modbig(zz,len,mm,mlen,hilf);
    return len;
}
/*-------------------------------------------------------------------*/
#ifdef OLDVERSION
/*
** quot = (x,n) / (y,m); Voraussetzung (y,m) != null
** Arbeitet destruktiv auf x; x wird rest, seine Laenge in *rlenptr
** hilf ist Platz fuer Hilfsvariable; muss mindestens m+1 lang sein
** Funktioniert fuer Bestimmung des Restes auch, falls
** Platz quot und Platz hilf gleich sind und der Quotient
** nicht interessiert
*/
PUBLIC int divbig(x,n,y,m,quot,rlenptr,hilf)
word2 *x, *y, *quot, *hilf;
int n, m, *rlenptr;
{
	word4 u, v;
	word2 a;
	int k, quotlen, hilflen;
	int cmp, b, b1;

	if(!m) {	/* Division durch 0 ohne Fehlermeldung */
		*rlenptr = 0;
		return(0);
	}
	if(m == 1) {
		quotlen = divarr(x,n,(unsigned)y[0],&a);
		cpyarr(x,quotlen,quot);
		*x = a;
		*rlenptr = (a ? 1 : 0);
		return(quotlen);
	}
#ifdef M_3264
	if(m == 2) {
		u = big2long(y,m);
		quotlen = div4arr(x,n,u,&v);
		cpyarr(x,quotlen,quot);
		*rlenptr = long2big(v,x);
		return(quotlen);
	}
#endif
	k = n - m;
	if(k >= 0)
		cmp = cmparr(x+k,n-k,y,m);
	if(k < 0 || (k == 0 && cmp < 0)) {
		*rlenptr = n;
		return(0);
	}
	else if(k > 0 && cmp < 0)
		k--;

	b = bitlen(y[m-1]);
	b1 = 16 - b;
	v = (y[m-1] << b1) + (y[m-2] >> b);
	u = 0xFFFFFFFF;
	u /= (v + 1);

	k++;
	x += k;
	n -= k;
	quot += k;
	quotlen = k;
	while(--k >= 0) {
		--x;
		if(n || *x)
			n++;
		if(n >= m) {
			v = (x[m-1] >> b);
			if(n > m)	/* dann n = m+1 */
				v += (x[m] << b1);
			a = (u*v) >> 16;
			hilflen = multarr(y,m,a,hilf);
			n = subarr(x,n,hilf,hilflen);
			while(cmparr(x,n,y,m) >= 0) {
				a++;
				n = subarr(x,n,y,m);
			}
		}
		else
			a = 0;
		*--quot = a;
	}
	*rlenptr = n;
	return(quotlen);
}
/*-------------------------------------------------------------------*/
#else /* NEWVERSION */
/*
** quot = (x,n) / (y,m); Voraussetzung (y,m) != null
** Arbeitet destruktiv auf x; x wird rest, seine Laenge in *rlenptr
** hilf ist Platz fuer Hilfsvariable; muss mindestens m+1 lang sein
** Funktioniert fuer Bestimmung des Restes auch, falls
** Platz quot und Platz hilf gleich sind und der Quotient
** nicht interessiert
*/
PUBLIC int divbig(x,n,y,m,quot,rlenptr,hilf)
word2 *x, *y, *quot, *hilf;
int n, m, *rlenptr;
{
	word4 u, v;
	word2 a;
	int k, quotlen, hilflen;
	int cmp, b, b1;
#ifdef M_3264
	int kappa, nu, mu;
	word2 ww[6];
#endif

	if(!m) {	/* Division durch 0 ohne Fehlermeldung */
		*rlenptr = 0;
		return(0);
	}
	if(m == 1) {
		quotlen = divarr(x,n,(unsigned)y[0],&a);
		cpyarr(x,quotlen,quot);
		*x = a;
		*rlenptr = (a ? 1 : 0);
		return(quotlen);
	}
#ifdef M_3264
	if(m == 2) {
		u = big2long(y,m);
		quotlen = div4arr(x,n,u,&v);
		cpyarr(x,quotlen,quot);
		*rlenptr = long2big(v,x);
		return(quotlen);
	}

	/* else if(m >= 3) */
	k = n - m;
	if(k & 1) k++;	    /* k must be even */
	if(k >= 0)
		cmp = cmparr(x+k,n-k,y,m);
	if(k < 0 || (k == 0 && cmp < 0)) {     /* (y,m) > (x,n) */
		*rlenptr = n;
		return(0);
	}
	else if(cmp >= 0)
		k += 2;
	/* now k >= 2 and (x+k,n-k) < (y,m) */
	b = bitlen(y[m-1]);
	b1 = 16 - b;

	v = y[m-1]; v <<= 16;
    v += y[m-2]; v <<= b1;
    v += (y[m-3] >> b);
	if(v < 0xFFFFFFFF)
	    v++;
	ww[3] = 0x7FFF; ww[2] = 0xFFFF; ww[1] = 0xFFFF;
	/* don't care for ww[0] */
	nu = div4arr(ww,4,v,&u);
	v = big2long(ww,nu);

	b--;
	for(kappa = k-2; kappa >= 0; kappa -= 2) {
		mu = m + kappa - 1;
		if(n <= mu) {
		    quot[kappa] = quot[kappa+1] = 0;
		    continue;
		}
		nu = n - mu;
		cpyarr(x+mu,nu,ww);
		nu = mult4arr(ww,nu,v,ww);
		nu = shrarr(ww+2,nu-2,b);
		u = big2long(ww+2,nu);
		hilflen = mult4arr(y,m,u,hilf);
		n = subarr(x+kappa,n-kappa,hilf,hilflen);
		while(cmparr(x+kappa,n,y,m) >= 0) {
		    u++;
		    n = subarr(x+kappa,n,y,m);
		}
		quot[kappa] = (word2)u;
		quot[kappa+1] = (word2)(u >> 16);
		if(n == 0) {
		    n = kappa;
		    while(n > 0 && !x[n-1])
                n--;
		}
		else
		    n += kappa;
	}
	*rlenptr = n;
	if(quot[k-1])
	    quotlen = k;
	else if(quot[k-2])
	    quotlen = k - 1;
	else
	    quotlen = k - 2;

	return(quotlen);
#else
	k = n - m;
	if(k >= 0)
		cmp = cmparr(x+k,n-k,y,m);
	if(k < 0 || (k == 0 && cmp < 0)) {
		*rlenptr = n;
		return(0);
	}
	else if(k > 0 && cmp < 0)
		k--;

	b = bitlen(y[m-1]);
	b1 = 16 - b;
	v = (y[m-1] << b1) + (y[m-2] >> b);
	u = 0xFFFFFFFF;
	u /= (v + 1);

	k++;
	x += k;
	n -= k;
	quot += k;
	quotlen = k;
	while(--k >= 0) {
		--x;
		if(n || *x)
			n++;
		if(n >= m) {
			v = (x[m-1] >> b);
			if(n > m)	/* dann n = m+1 */
				v += (x[m] << b1);
			a = (u*v) >> 16;
			hilflen = multarr(y,m,a,hilf);
			n = subarr(x,n,hilf,hilflen);
			while(cmparr(x,n,y,m) >= 0) {
				a++;
				n = subarr(x,n,y,m);
			}
		}
		else
			a = 0;
		*--quot = a;
	}
	*rlenptr = n;
	return(quotlen);
#endif
}
#endif /* ?OLDVERSION */
/*-------------------------------------------------------------------*/
/*
** Berechnet Produkt von (x,n)*(2^16)^-prec mit (y,m)*(2^16)^-prec
** Ist len der Rueckgabewert so erhaelt man
**	Ergebnis = (z,len)*(2^16)^-prec
** Platz z muss Laenge len + prec haben
** Platz hilf muss Laenge m + 1 haben
*/
PUBLIC int multfix(prec,x,n,y,m,z,hilf)
int prec, n, m;
word2 *x, *y, *z, *hilf;
{
	int len;

	if(n+m < prec || !n || !m)
		return(0);
	len = multbig(x,n,y,m,z,hilf) - prec;
	if(len <= 0)
		return(0);
	else {
		cpyarr(z+prec,len,z);
		return(len);
	}
}
/*-------------------------------------------------------------------*/
/*
** Berechnet Quotient von (x,n)*(2^16)^-prec und (y,m)*(2^16)^-prec
** Ist len der Rueckgabewert so erhaelt man
**	Ergebnis = (z,len)*(2^16)^-prec
** Platz z muss Laenge len + 1 haben
** Platz hilf muss Laenge n + m + prec + 1 haben
*/
PUBLIC int divfix(prec,x,n,y,m,z,hilf)
int prec, n, m;
word2 *x, *y, *z, *hilf;
{
	int i, len;
	word2 *temp;

	temp = hilf;
	hilf += prec + n;
	setarr(temp,prec,0);
	cpyarr(x,n,temp+prec);
	len = divbig(temp,prec+n,y,m,z,&i,hilf);
	return(len);
}
/*------------------------------------------------------------------*/
PUBLIC unsigned shortgcd(x,y)
unsigned x, y;
{
	unsigned r;

	while(y) {
		r = x % y;
		x = y;
		y = r;
	}
	return(x);
}
/*------------------------------------------------------------------*/
/*
** bestimmt den gcd von (x,n) und (y,m), Resultat in x
** arbeitet destruktiv auf x und y
*/
PUBLIC int biggcd(x,n,y,m,hilf)
word2 *x, *y, *hilf;
int n, m;
{
	int rlen;
	word2 *savex, *temp;

	savex = x;

	while(m > 0) {
		rlen = modbig(x,n,y,m,hilf);
		temp = x;
		x = y;
		n = m;
		y = temp;
		m = rlen;
	}
	if(savex != x)
		cpyarr(x,n,savex);
	return(n);
}
/*-------------------------------------------------------------------*/
/*
** p = (x,n) hoch a
** temp und hilf sind Plaetze fuer Hilfsvariable,
** muessen so gross wie das Resultat p sein
*/
PUBLIC int power(x,n,a,p,temp,hilf)
word2 *x, *p, *temp, *hilf;
int n;
unsigned a;
{
	int plen;
	unsigned mask, b;

	if(a == 0) {
		p[0] = 1;
		return(1);
	}
	else if(n == 0)
		return(0);

	cpyarr(x,n,p);
	plen = n;
    if((b = (a >> 16))) {
        mask = 0x8000;
        mask <<= bitlen(b);
    }
    else {
        mask = 1 << (bitlen(a)-1);
    }
	while(mask >>= 1) {
		plen = multbig(p,plen,p,plen,temp,hilf);
		if(a & mask) {
			plen = multbig(temp,plen,x,n,p,hilf);
		}
		else
			cpyarr(temp,plen,p);
	}
	return(plen);
}
/*-------------------------------------------------------------------*/
/*
** Berechnet groesste ganze Zahl z mit z*z <= x
** Arbeitet destruktiv auf x, x wird rest, seine Laenge in *rlenp
** Platz x muss mindestens n+1 lang sein,
** Platz z um eins groesser als die Wurzel,
** Platz temp um 2 groesser als die Wurzel
*/
PUBLIC int bigsqrt(x,n,z,rlenp,temp)
word2 *x, *z, *temp;
int n;
int *rlenp;
{
	int sh, len, restlen, templen;
	word4 v, vv, rr;
	unsigned xi;

	if(n <= 2) {
		v = big2long(x,n);
		v = intsqrt(v);
		return(long2big(v,z));
	}
	sh = (n&1 ? 8 : 0);
	sh += (16 - bitlen(x[n-1])) >> 1;
	n = shiftarr(x,n,sh+sh);	/* n is always even */
	x += n - 2;
	rr = big2long(x,2);
	v = intsqrt(rr);
	restlen = long2big(rr-v*v,x);
	len = n >> 1;
	setarr(z,len-1,0);
	z += len - 1;
	z[0] = v;
	n = incarr(z,1,(unsigned)v);
	vv = v << 16;
	while(--len > 0) {
		z--;
		n++;
		x -= 2;
		restlen += 2;
		if(restlen == 2) {
			while(restlen > 0 && x[restlen-1] == 0)
				restlen--;
		}
		if(restlen < n)
			continue;
		rr = big2long(x+n-2,2) >> 1;
		if(restlen > n)	    /* dann x[restlen-1] = 1 */
			rr += 0x80000000;
		if(rr >= vv)
			xi = 0xFFFF;
		else
			xi = rr / v;
		n = incarr(z,n,xi);
		templen = multarr(z,n,xi,temp);
		n = incarr(z,n,xi);
		while(cmparr(x,restlen,temp,templen) < 0) {
			n = decarr(z,n,1);
			templen = subarr(temp,templen,z,n);
			n = decarr(z,n,1);
		}
		restlen = subarr(x,restlen,temp,templen);
		while(cmparr(x,restlen,z,n) > 0) {
			n = incarr(z,n,1);
			restlen = subarr(x,restlen,z,n);
			n = incarr(z,n,1);
		}
	}
	*rlenp = shiftarr(x,restlen,-sh-sh);
	return(shiftarr(z,n,-sh-1));
}
/*-------------------------------------------------------------------*/
/*
** bestimmt Laenge in Bits einer 32-Bit-Zahl x
** 0 <= bitlen <= 32; bitlen = 0 <==> x == 0;
*/
PUBLIC int lbitlen(x)
word4 x;
{
	unsigned x0;

	if(x >= 0x10000) {
		x0 = x >> 16;
		return(16 + bitlen(x0));
	}
	else {
		x0 = x;
		return(bitlen(x0));
	}
}
/*-------------------------------------------------------------------*/
/*
** berechnet Nibble k (0 <= k <= 3) einer 16-Bit-Zahl x
*/
PRIVATE unsigned nibble(x,k)
unsigned x;
int k;
{
	k <<= 2;
	x >>= k;

	return(x & 0x000F);
}
/*-------------------------------------------------------------------*/
/*
** verwandelt Array von bcd-Zahlen (x,n) in big-Array y;
** n ist Anzahl der word2-Stellen von x
** Rueckgabewert Laenge von y.
*/
PUBLIC int bcd2big(x,n,y)
word2 *x, *y;
int n;
{
	int m;

	if(!n)
		return(0);
	x += n;
	y[0] = bcd2int(*--x);
	m = 1;
	while(--n > 0) {
		m = multarr(y,m,DECBASE,y);
		m = incarr(y,m,bcd2int(*--x));
	}
	return(m);
}
/*-------------------------------------------------------------------*/
#ifdef NAUSKOMM
/********* not used *********/
/*
** verwandelt String in Array von bcd-Zahlen;
** Rueckgabewert Anzahl der Ziffern
*/
PRIVATE int str2bcd(str,arr)
char *str;
word2 *arr;
{
	int n = str2barr(str,4,arr);

	return(n ? ((n-1)<<2) + niblen(arr[n-1]) : 0);
}
#endif
/*-------------------------------------------------------------------*/
PUBLIC int str2int(str,panz)
char *str;
int *panz;
{
	int i,x;
	int ch;

	for(i=0,x=0; isdecdigit(ch = *str); i++,str++)
		x = x * 10 + (ch - '0');
	*panz = i;
	return(x);
}
/*-------------------------------------------------------------------*/
/*
** verwandelt String str in big-integer arr
** Rueckgabewert Laenge von arr
** Platz hilf muss strlen(str)/2 + 1 lang sein
*/
PUBLIC int str2big(str,arr,hilf)
char *str;
word2 *arr, *hilf;
{
	int n;

	n = str2barr(str,4,hilf);
	n = bcd2big(hilf,n,arr);
	return(n);
}
/*-------------------------------------------------------------------*/
/*
** verwandelt Array (arr,n) von bcd-Zahlen in String;
** Rueckgabewert Stringlaenge
*/
PUBLIC int bcd2str(arr,n,str)
word2 *arr;
int n;
char *str;
{
	int i = n;

	if(n == 0) {
		*str++ = '0';
		*str = 0;
		return(1);
	}
	while(--i >= 0)
		*str++ = nibascii(arr,i);
	*str = 0;
	return(n);
}
/*-------------------------------------------------------------------*/
PUBLIC int big2xstr(arr,n,str)
word2 *arr;
int n;
char *str;
{
	n = (n ? (n-1)*4 + niblen(arr[n-1]) : 0);
	return(bcd2str(arr,n,str));
}
/*-------------------------------------------------------------------*/
/*
** ch muss ein hex-Character sein
*/
PUBLIC int digval(ch)
int ch;
{
	if(ch >= '0' && ch <= '9')
		return(ch - '0');
	else if(ch >= 'A' && ch <= 'Z')
		return(ch - 'A' + 10);
	else
		return(ch - 'a' + 10);
}
/*-------------------------------------------------------------------*/
/*
*/
PRIVATE int str2barr(str,b,arr)
char *str;
int b;		/* bits per digit: 1=bin, 3=oct, 4=hex */
word2 *arr;
{
	int n, i, k, m, len = 0;
	unsigned dig;

	while(*str == '0')
		str++;
	while(*str++)
		len++;
	--str;		/* jetzt zeigt str auf '\0' */
	n = (len*b + 15) >> 4;
	setarr(arr,n,0);
	for(i=0; --len>=0; i+=b) {
		dig = digval(*--str);
		k = i >> 4;
		m = i & 0xF;
		arr[k] += (dig << m);
		if(m+b > 16)
			arr[k+1] += (dig >> (16-m));
	}
	return(n);
}
/*-------------------------------------------------------------------*/
/*
** Verwandelt String in bignum (arr,n); n ist Rueckgabewert.
*/
PUBLIC int xstr2big(str,arr)
char *str;
word2 *arr;
{
	return(str2barr(str,4,arr));
}
/*-------------------------------------------------------------------*/
PUBLIC int ostr2big(str,arr)
char *str;
word2 *arr;
{
	return(str2barr(str,3,arr));
}
/*-------------------------------------------------------------------*/
PUBLIC int bstr2big(str,arr)
char *str;
word2 *arr;
{
	return(str2barr(str,1,arr));
}
/*-------------------------------------------------------------------*/
/*
** gibt k-te Dezimalstelle des bcd-Arrays arr;
** 0 <= k < Stellenzahl von arr
*/
PUBLIC int nibdigit(arr,k)
word2 *arr;
int k;
{
	return(nibble(arr[k>>2],k&3));
}
/*-------------------------------------------------------------------*/
/*
** Das Array arr wird als Folge von Nibbles zu je n bit (n=1,3,4)
** aufgefasst. Die Funktion gibt das k-te Nibble zurueck.
** Vorsicht! Die gueltige Laenge von arr ist nicht bekannt.
*/
PUBLIC int nibndigit(n,arr,k)
int n;		/* bits per digit */
word2 *arr;
long k;
{
	int x, i;
	long k3;

	if(n == 4)
		return(nibdigit(arr,(int)k));
	else if(n == 1)
		return(nthbit(arr,k));
	else if(n == 3) {
		k3 = k+k+k;
		x = 0;
		for(i=2; i>=0; i--) {
			x <<= 1;
			if(nthbit(arr,k3+i))
				x |= 1;
		}
		return(x);
	}
	else	/* this case should not happen */
		return(0);
}
/*-------------------------------------------------------------------*/
PRIVATE int nthbit(arr,n)
word2 *arr;
long n;		/* zero based */
{
	int k, b;
	word2 mask = 1;

	k = n >> 4;
	b = n & 0xF;
	return(arr[k] & (mask << b) ? 1 : 0);
}
/*-------------------------------------------------------------------*/
/*
** gibt Ascii-code der k-ten Dezimalstelle des bcd-Arrays arr;
** 0 <= k < Stellenzahl von arr
*/
PUBLIC int nibascii(arr,k)
word2 *arr;
int k;
{
	return(hexascii(nibdigit(arr,k)));
}
/*-------------------------------------------------------------------*/
PUBLIC int hexascii(n)
int n;
{
	if(0 <= n && n <= 9)
		return('0' + n);
	else if(n >= 10 && n <= 15)
		return(('A'-10) + n);
	else	/* this case should not happen */
		return('0');
}
/*-------------------------------------------------------------------*/
/*
** verschiebt bcd-array (arr,n) um k Stellen
** k > 0: Links-Shift; k < 0: Rechts-Shift
** Rueckgabewert: Anzahl der Dezimal-Stellen
*/
PUBLIC int shiftbcd(arr,n,k)
word2 *arr;
int n, k;
{
	int b, len;

	if(-k >= n)
		return(0);
	b = k<<2;
	len = (n + 3)>>2;
	len = shiftarr(arr,len,b);
	return(n + k);
}
/*-------------------------------------------------------------------*/
PUBLIC int incbcd(x,n,a)
word2 *x;
int n;
unsigned a;
{
	int i, len;
	unsigned u;

	if(a == 0)
		return(n);
	len = (n + 3)>>2;
	for(i=0; a && i<len; i++) {
		u = bcd2int(x[i]);
		u += a;
		if(u < DECBASE)
			a = 0;
		else {
			a = 1;
			u -= DECBASE;
		}
		x[i] = int2bcd(u);
	}
	if(a)
		x[len++] = a;
	n = ((len-1)<<2) + niblen(x[len-1]);
	return(n);
}
/*********************************************************************/
