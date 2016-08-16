/****************************************************************/
/* file aritool0.c

ARIBAS interpreter for Arithmetic
Copyright (C) 1996 O.Forster

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

Email	forster@rz.mathematik.uni-muenchen.de
*/
/****************************************************************/
/*
** aritool0.c
** Hilfsprozeduren fuer Integer-Arithmetik (bignums)
** koennen zur Beschleunigung in Assembler geschrieben werden
*/
/*
** date of last change
** 95-02-01
** 97-02-24: pointer arithmetic in sumarr, diffarr, diff1arr
*/

#include "common.h"

int sumarr	_((word2 *x, int n, word2 *y));
int diffarr	_((word2 *x, int n, word2 *y));
int diff1arr	_((word2 *x, int n, word2 *y));
int incarr	_((word2 *x, int n, unsigned a));
int decarr	_((word2 *x, int n, unsigned a));
void cpyarr	_((word2 *x, int n, word2 *y));
void cpyarr1	_((word2 *x, int n, word2 *y));
int cmparr	_((word2 *x, int n, word2 *y, int m));
int shrarr	_((word2 *x, int n, int k));
int shlarr	_((word2 *x, int n, int k));
void setarr	_((word2 *x, int n, unsigned a));
void notarr	_((word2 *x, int n));
void andarr	_((word2 *x, int n, word2 *y));
void orarr	_((word2 *x, int n, word2 *y));
void xorarr	_((word2 *x, int n, word2 *y));
unsigned int2bcd	_((unsigned x));
unsigned bcd2int	_((unsigned x));
int big2bcd	_((word2 *x, int n, word2 *y));
int long2big	_((word4 u, word2 *x));
word4 big2long	_((word2 *x, int n));
word4 intsqrt	_((word4 u));
int bitlen	_((unsigned x));
int niblen	_((unsigned x));
int bitcount     _((unsigned u));


#if defined(MsDOS) || defined(DjGPP) || defined(SCOUNiX) || \
    defined(LiNUX) || defined(MsWIN32)
#ifndef NO_ASSEMB
#define ASSEMB86
#endif
#endif

#ifndef ASSEMB86
/*
** falls ASSEMB86 definiert ist, werden die folgenden Funktionen
** durch Assembler-Code ersetzt
*/

int multarr	_((word2 *x, int n, unsigned a, word2 *y));
int divarr	_((word2 *x, int n, unsigned a, word2 *restptr));
unsigned modarr _((word2 *x, int n, unsigned a));

/*-------------------------------------------------------------------*/
/*
** y = (x,n) * a
** funktioniert auch, wenn Platz y gleich oder <= Platz x
*/
int multarr(x,n,a,y)
word2 *x, *y;
unsigned a;
int n;
{
	register word4 u, carry = 0;
	int nn;

	if(a <= 1) {
		if(!a)
			return(0);
		else {
			cpyarr(x,n,y);
			return(n);
		}
	}
	nn = n;
	while(--n >= 0) {
		u = *x++;
		u *= a;
		u += carry;
		*y++ = u & 0xFFFF;
		carry = (u >> 16);
	}
	if(carry) {
		*y = carry;
		nn++;
	}
	return(nn);
}
/*-------------------------------------------------------------------*/
/*
** dividiert Array (x,n) destruktiv durch 16-bit-Zahl a
** und speichert Rest in *restptr
*/
int divarr(x,n,a,restptr)
word2 *x, *restptr;
unsigned a;
int n;
{
	register word4 u;
	word2 *xx;
	int nn;

	if(n == 0) {
		*restptr = 0;
		return(0);
	}
	xx = x += n;
	u = 0;
	nn = n;
	while(--n >= 0) {
		u <<= 16;
		u += *--x;
		*x = u/a;
		u %= a;
	}
	*restptr = u;
	return(*--xx ? nn : nn-1);
}
/*-------------------------------------------------------------------*/
/*
** Berechnet den Rest der Division von (x,n) durch 16-bit-Zahl a
** Das Array (x,n) bleibt erhalten
*/
unsigned modarr(x,n,a)
word2 *x;
int n;
unsigned a;
{
	register word4 u;

	if(n == 0 || a <= 1)
		return(0);
	x += n;
	u = 0;
	while(--n >= 0) {
		u <<= 16;
		u += *--x;
		u %= a;
	}
	return(u);
}
/*-------------------------------------------------------------------*/
#else
#undef ASSEMB86
#endif


/*-------------------------------------------------------------------*/
/*
** (x,n) := (x,n) + (y,n)
** returns 1, if carry is generated, else returns 0
*/
int sumarr(x,n,y)
word2 *x, *y;
int n;
{
	register word4 u;
	unsigned carry = 0;

	while(--n >= 0) {
		u = *x;
		u += *y++;
		u += carry;
		*x++ = u & 0xFFFF;
		carry = (u >= 0x10000 ? 1 : 0);
	}
	return(carry);
}
/*-------------------------------------------------------------------*/
/*
** (x,n) := (x,n) - (y,n)
** returns 1, if borrow is generated, else returns 0
*/
int diffarr(x,n,y)
word2 *x, *y;
int n;
{
	register word4 u;
	unsigned borrow = 0;

	while(--n >= 0) {
		u = *x;
		u -= *y++;
		u -= borrow;
		*x++ = u & 0xFFFF;
		borrow = (u & 0xFFFF0000 ? 1 : 0);
	}
	return(borrow);
}
/*-------------------------------------------------------------------*/
/*
** (x,n) := (y,n) - (x,n)
** returns 1, if borrow is generated, else returns 0
*/
int diff1arr(x,n,y)
word2 *x, *y;
int n;
{
	register word4 u;
	unsigned borrow = 0;

	while(--n >= 0) {
		u = *y++;
		u -= *x;
		u -= borrow;
		*x++ = u & 0xFFFF;
		borrow = (u & 0xFFFF0000 ? 1 : 0);
	}
	return(borrow);
}
/*-------------------------------------------------------------------*/
/*
** addiert zu Array (x,n) die 16-bit-Zahl a
** arbeitet destruktiv auf x
*/
int incarr(x,n,a)
word2 *x;
int n;
unsigned a;
{
	word4 u;
	int nn = n;

	while(a && --n >= 0) {
		u = *x;
		u += a;
		*x++ = u & 0xFFFF;
		a = (u >= 0x10000 ? 1 : 0);
	}
	if(a) {
		*x = a;
		nn++;
	}
	return(nn);
}
/*-------------------------------------------------------------------*/
/*
** subtrahiert von Array x die 16-bit-Zahl a
** arbeitet destruktiv auf x
** setzt voraus x >= a
*/
int decarr(x,n,a)
word2 *x;
int n;
unsigned a;
{
	register word4 u;
	word2 *xx;
	int nn;

	if(n == 0)
		return(0);
	xx = x + n - 1;
	nn = n;
	while(a && --n >= 0) {
		u = *x;
		u -= a;
		*x++ = u & 0xFFFF;
		a = (u & 0xFFFF0000 ? 1 : 0);
	}
	if(*xx == 0)
		nn--;
	return(nn);
}
/*-------------------------------------------------------------------*/
/*
** kopiert (x,n) nach y beginnend von unten
*/
void cpyarr(x,n,y)
word2 *x, *y;
int n;
{
	while(--n >= 0)
		*y++ = *x++;
}
/*-------------------------------------------------------------------*/
/*
** kopiert (x,n) nach y beginnend von oben
*/
void cpyarr1(x,n,y)
word2 *x, *y;
int n;
{
	x += n; y += n;

	while(--n >= 0)
		*--y = *--x;
}
/*-------------------------------------------------------------------*/
/*
** liefert +1, falls (x,n) > (y,m);
**	   -1, falls (x,n) < (y,m);
**	    0, falls (x,n) = (y,m);
*/
int cmparr(x,n,y,m)
word2 *x, *y;
int n, m;
{
	if(n != m)
		return(n > m ? 1 : -1);
	if(!n)
		return(0);
	x += n;
	y += n;
	while(--n >= 0)
		if(*--x != *--y)
			break;
	if(*x > *y)
		return(1);
	else if(*x < *y)
		return(-1);
	else
		return(0);
}
/*-------------------------------------------------------------------*/
/*
** Rechtsshift von (x,n) um k Bits; 0 <= k <= 15
** arbeitet destruktiv auf x
*/
int shrarr(x,n,k)
word2 *x;
int n, k;
{
	int i, k1 = 16 - k;
	word2 temp;

	if(!k || !n) 
		return(n);
	for(i=1; i<n; i++) {
		*x >>= k;
		temp = *(x+1) << k1;
		*x++ |= temp;
	}
	*x >>= k;
	return(*x ? n : n-1);
}
/*-------------------------------------------------------------------*/
/*
** Linksshift von (x,n) um k Bits; 0 <= k <= 15
** arbeitet destruktiv auf x
*/
int shlarr(x,n,k)
word2 *x;
int n, k;
{
	int i, k1 = 16 - k;
	word2 u = 0, temp;

	if(!k || !n) 
		return(n);
	for(i=0; i<n; i++) {
		temp = *x;
		*x++ = (temp << k) | u;
		u = (temp >> k1);
	}
	if(u) {
		*x = u;
		n++;
	}
	return(n);
}
/*-------------------------------------------------------------------*/
void setarr(x,n,a)
word2 *x;
int n;
unsigned a;
{
	while(--n >= 0)
		*x++ = a;
}
/*-------------------------------------------------------------------*/
void notarr(x,n)
word2 *x;
int n;
{
	while(--n >= 0) {
		*x = ~*x;
		x++;
	}
}
/*-------------------------------------------------------------------*/
void andarr(x,n,y)
word2 *x, *y;
int n;
{
	while(--n >= 0) {
		*x++ &= *y++;
	}
}
/*-------------------------------------------------------------------*/
void orarr(x,n,y)
word2 *x, *y;
int n;
{
	while(--n >= 0) {
		*x++ |= *y++;
	}
}
/*-------------------------------------------------------------------*/
void xorarr(x,n,y)
word2 *x, *y;
int n;
{
	while(--n >= 0) {
		*x++ ^= *y++;
	}
}
/*-------------------------------------------------------------------*/
unsigned int2bcd(x)
unsigned x;
{
	int i;
	word2 a[3];
	unsigned y;

	for(i=0; i<3; i++) {
		a[i] = x % 10;
		x /= 10;
	}
	y = x;			/* a[3] */
	for(i=2; i>=0; i--)
		y = (y<<4) + a[i];
	return(y);
}
/*-------------------------------------------------------------------*/
unsigned bcd2int(x)
unsigned x;
{
	int i;
	word2 a[3];
	unsigned y;

	for(i=0; i<3; i++) {
		a[i] = (0x000F & x);
		x >>= 4;
	}
	y = x;
	for(i=2; i>=0; i--)
		y = y*10 + a[i];
	return(y); 
}
/*-------------------------------------------------------------------*/
/*
** verwandelt big-Array (x,n) in bcd-Array y
** !!! arbeitet destruktiv auf x !!!
** Rueckgabewert Anzahl der Dezimalstellen von x
*/
int big2bcd(x,n,y)
word2 *x, *y;
int n;
{
	int k = -1;

	if(!n)
		return(0);
	while(n) {
		n = divarr(x,n,10000,y);
		*y = int2bcd(*y);
		y++;
		k++;
	}
	return(k*4 + niblen(*--y));
}
/*-------------------------------------------------------------------*/
int long2big(u,x)
word4 u;
word2 *x;
{
	if(u == 0)
		return(0);
	x[0] = u;
	if(u < 0x10000)
		return(1);
	else {
		x[1] = (u >> 16);
		return(2);
	}
}
/*-------------------------------------------------------------------*/
word4 big2long(x,n)
word2 *x;
int n;
{
	word4 u = 0;

	if(n >= 2) {
		u = x[1];
		u <<= 16;
	}
	if(n >= 1)
		u += x[0];
	return(u);
}
/*-------------------------------------------------------------------*/
/*
** berechnet die groesste ganze Zahl x mit x*x <= u
*/
word4 intsqrt(u)
word4 u;
{
	word4 v, x, x1, b;
	int n;

	if(!u)
		return(0);
	v = 0x40000000;
	x = 0x8000;
	n = 15;
	while(v > u) {
		v >>= 2;
		x >>= 1;
		n--;
	}
	b = x;
	u -= v;
	while(--n >= 0) {
		b >>= 1;
		x1 = x + b;
		v = (x + x1) << n;
		if(u >= v) {
			x = x1;
			u -= v;
		}
	}
	return(x);
}
/*------------------------------------------------------------------*/
/*
** bestimmt Laenge in Bits einer 16-Bit-Zahl x
** 0 <= bitlen <= 16; bitlen = 0 <==> x == 0;
*/
int bitlen(x)
unsigned x;
{
	int len;
	unsigned mask;

	if(x & 0xFF00) {
		len = 16;
		mask = 0x8000;
	}
	else if(x) {
		len = 8;
		mask = 0x0080;
	}
	else
		return(0);
	while(!(x & mask)) {
		mask >>= 1;
		len--;
	}
	return(len);
}
/*-------------------------------------------------------------------*/
int niblen(x)
unsigned x;
{
	int len = 4;
	unsigned mask = 0xF000;

	if(!x)
		return(0);
	while(!(x & mask)) {
		mask >>= 4;
		len--;
	}
	return(len);
}
/*-------------------------------------------------------------------*/
/*
** returns number of set bits in 16-bit number u
*/
PUBLIC int bitcount(u)
unsigned u;
{
    unsigned mask = 1;
    int count = 0;
    int k;

    for(k=0; k<16; k++) {
        if(u & mask)
            count++;
        else if(mask > u)
            break;
        mask <<= 1;
    }
    return count;
}
/*********************************************************************/
