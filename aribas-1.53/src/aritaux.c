/****************************************************************/
/* file aritaux.c

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
** aritaux.c
** auxiliary procedures for arithmetic
**
** date of last change
** 1994-12-31
** 2000-12-30: multiprec floats
** 2002-02-16: changesign
** 2002-04-21: chkintnz
*/

#include "common.h"

PUBLIC int setfltprec	_((int prec));
PUBLIC int deffltprec	_((void));
PUBLIC int maxfltprec	_((void));
PUBLIC int fltpreccode	_((int prec));
PUBLIC int fltprec	    _((int type));
PUBLIC int refnumtrunc	_((int prec, truc *ptr, numdata *nptr));
PUBLIC int getnumtrunc	_((int prec, truc *ptr, numdata *nptr));
PUBLIC int getnumalign	_((int prec, truc *ptr, numdata *nptr));
PUBLIC int alignfloat	_((int prec, numdata *nptr));
PUBLIC int alignfix	    _((int prec, numdata *nptr));
PUBLIC void adjustoffs	_((numdata *npt1, numdata *npt2));
PUBLIC int normfloat	_((int prec, numdata *nptr));
PUBLIC int multtrunc	_((int prec, numdata *npt1, numdata *npt2,
			   word2 *hilf));
PUBLIC int divtrunc	_((int prec, numdata *npt1, numdata *npt2,
			   word2 *hilf));
PUBLIC int pwrtrunc	_((int prec, unsigned base, unsigned a,
			   numdata *nptr, word2 *hilf));
PUBLIC int float2bcd	_((int places, truc *p, numdata *nptr,
			   word2 *hilf));
PUBLIC int roundbcd	_((int prec, numdata *nptr));
PUBLIC int flodec2bin	_((int prec, numdata *nptr, word2 *hilf));
PUBLIC void int2numdat	_((int x, numdata *nptr));
PUBLIC void cpynumdat	_((numdata *npt1, numdata *npt2));
PUBLIC int numposneg	_((truc *ptr));
PUBLIC truc wipesign	_((truc *ptr));
PUBLIC truc changesign	_((truc *ptr));
PUBLIC long intretr	_((truc *ptr));
PUBLIC int bigref	_((truc *ptr, word2 **xp, int *sp));
PUBLIC int bigretr	_((truc *ptr, word2 *x, int *sp));
PUBLIC int twocretr	_((truc *ptr, word2 *x));
PUBLIC int and2arr	_((word2 *x, int n, word2 *y, int m));
PUBLIC int or2arr	_((word2 *x, int n, word2 *y, int m));
PUBLIC int xor2arr	_((word2 *x, int n, word2 *y, int m));
PUBLIC int xorbitvec	_((word2 *x, int n, word2 *y, int m));
PUBLIC long bit_length	_((word2 *x, int n));
PUBLIC int chkintnz _((truc sym, truc *ptr));
PUBLIC int chkints	_((truc sym, truc *argptr, int n));
PUBLIC int chkint   _((truc sym, truc *ptr));
PUBLIC int chkintt  _((truc sym, truc *ptr));
PUBLIC int chknums	_((truc sym, truc *argptr, int n));
PUBLIC int chknum	_((truc sym, truc *ptr));
PUBLIC int chkintvec	_((truc sym, truc *vptr));
PUBLIC int chknumvec	_((truc sym, truc *vptr));

PRIVATE long decdigs	_((numdata *nptr));
PRIVATE int malzehnhoch _((int prec, numdata *nptr, long d, word2 *hilf));
PRIVATE int twocadjust	_((word2 *x, int n));

PUBLIC int FltPrec[20] = 
	{2, 4, 8, 12, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 
	160, 192, 224, 256, 320};

#ifdef FPREC_HIGH
#define MAXFLTLEVEL 18
#else
#define MAXFLTLEVEL 10
#endif

PUBLIC int MaxFltLevel = MAXFLTLEVEL;

PRIVATE int floatprec = 2;

/*------------------------------------------------------------*/
PUBLIC int setfltprec(prec)
int prec;
{
    int k;

    k = 0;
    while((k < MaxFltLevel) && (prec > FltPrec[k]))
        k++;
    floatprec = FltPrec[k];

    return(floatprec);
}
/*-------------------------------------------------------------*/
PUBLIC int fltpreccode(prec)
int prec;
{
    int k;

    k = 0;
    while((k < MaxFltLevel) && (prec > FltPrec[k]))
        k++;
	return(k);
}
/*-------------------------------------------------------------*/
PUBLIC int maxfltprec()
{
	return(FltPrec[MaxFltLevel]);
}
/*-------------------------------------------------------------*/
PUBLIC int deffltprec()
{
	return(floatprec);
}
/*------------------------------------------------------------*/
PUBLIC int fltprec(type)
int type;
{
    int k;

    if(type >= fFLTOBJ) {
        k = (type & PRECMASK) >> 1;
        return FltPrec[k];
    }
    else
        return(floatprec);
}
/*------------------------------------------------------------*/
/*
** Erzeugt Referenz auf Zahl in *p mit prec 16-bit-Stellen (ohne Kopie)
** Fuer die Zahl 0 wird nptr->len = 0 und nptr->expo = MOSTNEGEX
*/
PUBLIC int refnumtrunc(prec,p,nptr)
int prec;		/* should be >= 2 */
truc *p;
numdata *nptr;
{
	static word2 ddd[2];

	struct bigcell *big;
	struct floatcell *fl;
	long ex;
	int len, diff, pcode;
	int flg = *FLAGPTR(p);

	if(flg == fFIXNUM) {
		if(!*WORD2PTR(p))
			goto zeroexit;
		nptr->digits = WORD2PTR(p);
		nptr->sign = *SIGNPTR(p);
		nptr->len = 1;
		nptr->expo = 0;
	}
	else if(flg == fBIGNUM) {
		big = (struct bigcell *)TAddress(p);
		nptr->sign = big->signum;
		len = big->len;
		ex = diff = (len > prec ? len - prec : 0);
		nptr->digits = &(big->digi0) + diff;
		nptr->len = len - diff;
		nptr->expo = (ex << 4);
	}
	else if(flg >= fFLTOBJ) {
		if(flg & FLTZEROBIT)
			goto zeroexit;
		pcode = (flg & PRECMASK);
        len = FltPrec[pcode>>1];
		fl = (struct floatcell *)TAddress(p);
		nptr->sign = (fl->signum & FSIGNBIT ? MINUSBYTE : 0);
		diff = (len > prec ? len - prec : 0);
		nptr->digits = &(fl->digi0) + diff;
		nptr->len = len - diff;
		ex = fl->expo;
		if(flg >= fHUGEFLOAT) {
			ex <<= 7;
			ex += (fl->signum & HUGEMASK);
		}
		nptr->expo = ex + (diff << 4);
	}

	else {
		error(voidsym,err_case,voidsym);
		return(0);
	}
	return(nptr->len);
  zeroexit:
	nptr->digits = ddd;
	nptr->len = 0;
	nptr->sign = 0;
	nptr->expo = MOSTNEGEX;
	return(0);
}
/*------------------------------------------------------------------*/
/*
** holt Zahl aus *ptr nach *nptr mit prec 16-bit-Stellen (mit Kopie)
*/
PUBLIC int getnumtrunc(prec,ptr,nptr)
int prec;
truc *ptr;
numdata *nptr;
{
	int len;
	word2 *saveptr;

	saveptr = nptr->digits;
	len = refnumtrunc(prec,ptr,nptr);
	cpyarr(nptr->digits,len,saveptr);
	nptr->digits = saveptr;
	return(len);
}
/*------------------------------------------------------------------*/
/*
** holt Zahl aus *p und normalisiert sie
*/
PUBLIC int getnumalign(prec,p,nptr)
int prec;
truc *p;
numdata *nptr;
{
	getnumtrunc(prec,p,nptr);
	return(alignfloat(prec,nptr));
}
/*-------------------------------------------------------------------*/
/*
** Die in nptr gegebene Zahl wird so normalisiert, dass nptr->expo
** durch 16 teilbar wird.
** Es werden genau prec 16-bit-Stellen benuetzt, falls Zahl /= 0
** Fuer die Zahl 0 ist der Rueckgabewert 0
*/
PUBLIC int alignfloat(prec,nptr)
int prec;
numdata *nptr;
{
	int len, b; 
	long sh;

	len = nptr->len;
	if(len == 0)
		return(0);
	b = nptr->expo & 0xF;	      /* b != 0 only for floats */
	sh = prec - len;
	sh <<= 4;
	sh -= (b ? 16-b : 0);
	nptr->expo -= sh;
	nptr->len = lshiftarr(nptr->digits,len,sh);
	return(nptr->len);		/* nptr->len = prec */
}
/*-----------------------------------------------------------------*/
/*
** Die Zahl in nptr wird so normalisiert, dass
** nptr->expo gleich -(prec<<4) wird.
** Falls aber nptr->len groesser als 2*prec wuerde,
** wird nptr unveraendert gelassen und aERROR zurueckgegeben.
** Sonst ist der Rueckgabewert nptr->len
*/
PUBLIC int alignfix(prec,nptr)
int prec;
numdata *nptr;
{
	int len; 
	long sh;

	if(!(len = nptr->len))
		return(0);
	sh = nptr->expo;
	if(len + (sh>>4) > prec)
		return(aERROR);
	sh += prec<<4;
	nptr->expo -= sh;
	nptr->len = lshiftarr(nptr->digits,len,sh);
	return(nptr->len);
}
/*-------------------------------------------------------------------*/
/*
** Die in npt1 und npt2 gegebenen Zahlen werden so normalisiert,
** dass beide expos gleich werden
** Es erfolgt ein Rechtsshift auf npt1->digits oder npt2->digits
** Nur npt1->expo wird auf den korrekten Wert gesetzt
** Es wird vorausgesetzt, dass beide expos durch 16 teilbar sind
*/
PUBLIC void adjustoffs(npt1,npt2)
numdata *npt1, *npt2;
{
	int diff;

	if(npt1->len == 0)
		npt1->expo = npt2->expo;
	else if(npt2->len == 0)
		;
	else if((diff = (npt1->expo - npt2->expo) >> 4) > 0) {
		if(diff >= npt2->len)
			npt2->len = 0;
		else {
			npt2->len -= diff;
			cpyarr(npt2->digits+diff,npt2->len,npt2->digits);
		}
	}
	else if(diff < 0) {
		npt1->expo = npt2->expo;
		if(-diff >= npt1->len)
			npt1->len = 0;
		else {
			npt1->len += diff;
			cpyarr(npt1->digits-diff,npt1->len,npt1->digits);
		}
	}
}
/*------------------------------------------------------------------*/
/*
** Die durch nptr gegebene float-Zahl wird normalisiert, so dass
** die mantisse genau (prec*16) Bits lang wird.
** nptr->expo wird der Exponent bzgl. Basis 2
** Rueckgabewert: 0 fuer die Zahl 0, sonst prec
** !!! arbeitet destruktiv auf nptr !!!
*/
PUBLIC int normfloat(prec,nptr)
int prec;
numdata *nptr;
{
	int len;
	long sh;

	len = nptr->len;
	if(len == 0) {
		nptr->sign = 0;
		nptr->expo = MOSTNEGEX;
		return(0);
	}
	nptr->len = prec;
	sh = prec - len + 1;
	sh <<= 4;
	sh -= bitlen(nptr->digits[len-1]);
	lshiftarr(nptr->digits,len,sh);
	nptr->expo -= sh;
	return(prec);
}
/*-------------------------------------------------------------------*/
/*
** Die durch npt1 gegebene Zahl wird mit der durch npt2 gegebenen
** Zahl multipliziert. Produkt in npt1
** Die Laenge des Produkts wird auf <= prec 16-bit-Stellen gekuerzt
** Platz hilf muss 3*prec + 4 lang sein
*/
PUBLIC int multtrunc(prec,npt1,npt2,hilf)
int prec;
numdata *npt1, *npt2;
word2 *hilf;
{
	int n, n1, n2;
	word2 *x, *y, *z;
	long exponent, diff;

	n1 = npt1->len;
	n2 = npt2->len;
	x = npt1->digits;
	y = npt2->digits;

	diff = n1 - prec;
	if(diff > 0) {
		x += diff;
		n1 -= diff;
		npt1->expo += diff << 4;
	}
	diff = n2 - prec;
	if(diff > 0) {
		y += diff;
		n2 -= diff;
		npt2->expo += diff << 4;
	}

	z = hilf + prec + 2;
	n = multbig(x,n1,y,n2,z,hilf);
	diff = (n > prec ? n - prec : 0);
	n -= diff;
	cpyarr(z+diff,n,npt1->digits);
	exponent = npt1->expo + npt2->expo + (diff << 4);
	if(exponent >= maxfltex)
		return(aERROR);
	else if(exponent <= -maxfltex) {
		exponent = MOSTNEGEX;
		n = 0;
	}
	npt1->len = n;
	npt1->expo = exponent;
	if(n == 0)
		npt1->sign = 0;
	else if((npt1->sign == npt2->sign) || (npt1->sign && npt2->sign))
		npt1->sign = 0;
	else
		npt1->sign = MINUSBYTE;
	return(n);
}
/*-------------------------------------------------------------------*/
PUBLIC int divtrunc(prec,npt1,npt2,hilf)
int prec;
numdata *npt1, *npt2;
word2 *hilf;
{
	int n, n1, n2;
	int rlen;
	long exponent, diff;
	word2 *x, *y, *z;

	n1 = npt1->len;
	if(n1 == 0)
		return(0);
	n2 = npt2->len;
	if(n2 == 0)
		return(aERROR);	/* Division durch 0 */

	x = hilf + prec + 2;
	y = x + (prec << 1);
	z = y + prec;

	diff = n2 - prec;
	if(diff > 0) {
		y = npt2->digits + diff;
		npt2->expo += diff << 4;
		n2 -= diff;
	}
	else
		y = npt2->digits;

	diff = n2 + prec - n1;
	if(diff > 0) {
		setarr(x,(int)diff,0);
		cpyarr(npt1->digits,n1,x+diff);
	}
	else
		cpyarr(npt1->digits-diff,(int)(n1+diff),x);
	n1 += diff;
	npt1->expo -= diff << 4;
	n = divbig(x,n1,y,n2,z,&rlen,hilf);
	diff = n - prec;	/* diff is 0 or 1 */
	n -= diff;
	cpyarr(z+diff,n,npt1->digits);
	exponent = npt1->expo - npt2->expo + (diff << 4);
	if(exponent >= maxfltex)
		return(aERROR);
	else if(exponent <= -maxfltex) {
		exponent = MOSTNEGEX;
		n = 0;
	}
	npt1->expo = exponent;
	npt1->len = n;
	if(n == 0)
		npt1->sign = 0;
	else if(npt1->sign == npt2->sign || (npt1->sign && npt2->sign))
		npt1->sign = 0;
	else
		npt1->sign = MINUSBYTE;
	return(n);
}
/*-------------------------------------------------------------------*/
/*
** die durch *nptr gegebene Zahl ist etwa 10**decdigs(nptr)
*/
PRIVATE long decdigs(nptr)
numdata *nptr;
{
	static unsigned a[3] = {3,31,22088};
		/* log(2)/log(10) = 1/3 - 1/31 - 1/22088 */
	word4 u;
	long b;
	int n, sign;

	b = n = nptr->len - 1;
	if(n < 0)
		return(MOSTNEGEX);
	b <<= 4;
	b += nptr->expo + bitlen(nptr->digits[n]);
	sign = (b < 0);
	u = (sign ? -b : b);
	b = (u/a[0] - u/a[1] - u/a[2]);
	b = (sign ? -b : b);
	return(b);
}
/*-------------------------------------------------------------------*/
/*
** berechnet base**a in *nptr mit prec 16-bit-Stellen
*/
PUBLIC int pwrtrunc(prec,base,a,nptr,hilf)
int prec;
unsigned base, a;
numdata *nptr;
word2 *hilf;
{
	word2 *pow, *temp;
	long offs, offs1;
	unsigned bb;
	int len;

	temp = hilf + prec + 2;

	nptr->sign = 0;
	pow = nptr->digits;
	pow[0] = (a ? base : 1);
	len = 1;
	offs = 0;
        if(a <= 1) {
		bb = 0;
        }
	else if(a <= 0xFFFF) {
		bb = 1;
		bb <<= (bitlen(a)-2);
	}
	else {
		bb = 0x8000;
		bb <<= (bitlen(a>>16)-1);
	}
	while(bb) {
		len = multbig(pow,len,pow,len,temp,hilf);
		offs += offs;
		if((offs1 = len-prec) > 0) {
			offs += offs1;
			cpyarr(temp+offs1,prec,temp);
			len = prec;
		}
		if(a & bb) {
			len = multarr(temp,len,base,pow);
			if(len > prec) {	/* dann len = prec+1 */
				offs++;
				cpyarr(pow+1,prec,pow);
				len = prec;
			}
		}
		else
			cpyarr(temp,len,pow);
		bb >>= 1;
	}
	nptr->len = len;
	nptr->expo = (offs << 4);
	return(len);
}
/*-------------------------------------------------------------------*/
/*
** Verwandelt float-Zahl (gegeben in *p) in bcd-Zahl mit places Stellen
** Ist Rueckgabewert len != 0, so ist len = places und das
** Ergebnis gleich
**	   (arr,len) * 10**offs
** wobei arr = nptr->digits, offs = nptr->expo.
** Rueckgabewert 0 bedeutet die Zahl 0.0
** arr muss mindestens die word2-Laenge (2 + places/4) haben
** Platz hilf muss word2-Laenge >= places + places/4 + 8 haben
*/
PUBLIC int float2bcd(places,p,nptr,hilf)
int places;
truc *p;
numdata *nptr;
word2 *hilf;
{
	word2 *arr;
	long d, d1;
	int prec, prec1, n, sh;
	int flg = *FLAGPTR(p);

	prec = fltprec(flg);
	prec1 = places/4;
	if(prec > prec1)
		prec = prec1;
    prec +=1;

	if(getnumtrunc(prec,p,nptr) == 0)
		return(0);
	d = decdigs(nptr);
	d1 = places - d + 3; 	/* +3 wegen Rundungsfehlern */

	n = malzehnhoch(prec,nptr,d1,hilf);
	arr = nptr->digits;
	cpyarr(arr,n,hilf);
	sh = nptr->expo;	      /* sh ist int! */
	n = shiftarr(hilf,n,sh);
	nptr->len = big2bcd(hilf,n,arr);
	nptr->expo = -d1;
	return(roundbcd(places,nptr));
}
/*--------------------------------------------------------------*/
/*
** Die in *nptr gegebene Float-Zahl wird auf prec signifikante
** Stellen gerundet
*/
PUBLIC int roundbcd(prec,nptr)
int prec;
numdata *nptr;
{
	word2 *arr;
	int sh, len;
	int carry, dig;

	len = nptr->len;
	sh = len - prec;
	if(sh <= 0 || prec <= 0)
		return(len);
	arr = nptr->digits;
	dig = nibdigit(arr,sh-1);
	carry = (dig >= 5 ? 1 : 0);
	len = shiftbcd(arr,len,-sh);
	nptr->expo += sh;
	if(carry)
		len = incbcd(arr,len,carry);
	if(len > prec) {
		len = shiftbcd(arr,len,-1);
		nptr->expo++;
	}
	return(nptr->len = len);
}
/*------------------------------------------------------------------*/
/*
** Multipliziert die in *nptr gegebene Zahl mit 10**d
** auf prec 16-Bit-Stellen genau
*/
PRIVATE int malzehnhoch(prec,nptr,d,hilf)
int prec;
long d;
numdata *nptr;
word2 *hilf;
{
	numdata p10;
	word2 *hilf1;
	unsigned int dd;
	int n;

	p10.digits = hilf;
	hilf1 = hilf + prec + 2;

	dd = (d >= 0 ? d : -d);
	pwrtrunc(prec,10,dd,&p10,hilf1);
	if(d >= 0)
		n = multtrunc(prec,nptr,&p10,hilf1);
	else
		n = divtrunc(prec,nptr,&p10,hilf1);
	return(n);
}
/*------------------------------------------------------------------*/
/*
** verwandelt die in *nptr gegebene Zahl, wobei sich nptr->expo
** auf die Basis 10 bezieht, in Zahl bezueglich Basis 2
*/
PUBLIC int flodec2bin(prec,nptr,hilf)
int prec;
numdata *nptr;
word2 *hilf;
{
	long d;

	d = nptr->expo;
	nptr->expo = 0;
	return(malzehnhoch(prec,nptr,d,hilf));
}
/*------------------------------------------------------------------*/
/*
** verwandelt integer x (darf nur 16 bit enthalten) in numdata
*/
PUBLIC void int2numdat(x,nptr)
int x;
numdata *nptr;
{
	if(x < 0) {
		x = -x;
		nptr->sign = MINUSBYTE;
	}
	else
		nptr->sign = 0;
	*nptr->digits = x;
	nptr->len = (x ? 1 : 0);
	nptr->expo = (x ? 0 : MOSTNEGEX);
}
/*------------------------------------------------------------------*/
PUBLIC void cpynumdat(npt1,npt2)
numdata *npt1, *npt2;
{
	npt2->sign = npt1->sign;
	npt2->len = npt1->len;
	cpyarr(npt1->digits,npt1->len,npt2->digits);
	npt2->expo = npt1->expo;
}
/*-------------------------------------------------------------------*/
/*
** Ergibt +1, 0, -1, je nachdem Zahl in *ptr positiv, null oder
** negativ ist.
*/
PUBLIC int numposneg(ptr)
truc *ptr;
{
	int flg = *FLAGPTR(ptr);

	if(((flg == fFIXNUM) && *SIGNPTR(ptr)) ||
         ((flg == fBIGNUM) && *SIGNUMPTR(ptr)) ||
         ((flg >= fFLTOBJ) && (*SIGNUMPTR(ptr) & FSIGNBIT)))
		return(-1);
	else if((*ptr == zero) || ((flg >= fFLTOBJ) && (flg & FLTZEROBIT)))
		return(0);
	else
		return(1);
}
/*------------------------------------------------------------------*/
/*
** Loescht destruktiv das Vorzeichen der in *ptr gegebenen Zahl.
*/
PUBLIC truc wipesign(ptr)
truc *ptr;
{
	int flg = *FLAGPTR(ptr);

	if(flg == fFIXNUM)
		*SIGNPTR(ptr) = 0;
	else if(flg >= fFLTOBJ) {
		if((flg & FLTZEROBIT) == 0)
			*SIGNUMPTR(ptr) &= ~FSIGNBIT;
	}
	else		/* bignums */
		*SIGNUMPTR(ptr) = 0;
	return(*ptr);
}
/*------------------------------------------------------------------*/
/*
** Aendert destruktiv das Vorzeichen der in *ptr gegebenen Zahl.
*/
PUBLIC truc changesign(ptr)
truc *ptr;
{
    int sign, flg;

    sign = numposneg(ptr);
    if(sign == 0)
        return *ptr;
    else if(sign < 0)
        return wipesign(ptr);

    /* now *ptr is positive */
	flg = *FLAGPTR(ptr);
	if(flg == fFIXNUM)
		*SIGNPTR(ptr) = MINUSBYTE;
	else if(flg >= fFLTOBJ) {
			*SIGNUMPTR(ptr) |= FSIGNBIT;
	}
	else		/* bignums */
		*SIGNUMPTR(ptr) = MINUSBYTE;
	return(*ptr);
}
/*------------------------------------------------------------------*/
/*
** holt long aus *ptr;
** falls abs(Zahl) >= 2 ** 31, Returnwert: LONGERROR
*/
PUBLIC long intretr(ptr)
truc *ptr;
{
	word2 *x;
	word4 u;
	long res;
	int n, sign;

	n = bigref(ptr,&x,&sign);
	if(n < 0 || n > 2 || (u = big2long(x,n)) >= 0x80000000) 
		return(LONGERROR);
	res = u;
	return(sign ? -res : res);
}
/*------------------------------------------------------------------*/
/*
** Erzeugt Referenz auf Integer (bignum, fixnum oder gf2n_int) ohne Kopie
** Rueckgabe aERROR, falls *ptr kein fixnum oder bignum
*/
PUBLIC int bigref(ptr,xp,sp)
truc *ptr;
word2 **xp;
int *sp;
{
	struct bigcell *big;
	word2 *x;
	int flg = *FLAGPTR(ptr);

	if(flg == fFIXNUM) {
		*xp = x = WORD2PTR(ptr);
		*sp = *SIGNPTR(ptr);
		return(*x ? 1 : 0);
	}
	else if(flg == fBIGNUM || flg == fGF2NINT) {
		big = (struct bigcell *)TAddress(ptr);
		*sp = big->signum;
		*xp = &(big->digi0);
		return(big->len);
	}
	else
		return(aERROR);
}
/*------------------------------------------------------------------*/
/*
** holt Integer (fixnum oder bignum) aus *ptr nach x
** Vorzeichen in *sp
** Rueckgabe aERROR, falls *ptr kein fixnum oder bignum
*/
PUBLIC int bigretr(ptr,x,sp)
truc *ptr;
word2 *x;
int *sp;
{
	word2 *z;
	int n;

	n = bigref(ptr,&z,sp);
	if(n != aERROR)
		cpyarr(z,n,x);
	return(n);
}
/*-------------------------------------------------------------*/
/*
** holt Integer (fixnum oder bignum) aus *ptr nach x
** in Zweier-Komplement-Darstellung
** x[n] = 0, falls positiv;
** x[n] = 0xFFFF, falls negativ.
*/
PUBLIC int twocretr(ptr,x)
truc *ptr;
word2 *x;
{
	int sign, n;

	n = bigretr(ptr,x,&sign);
	if(n < 0)
		return(n);
	if(sign) {
		n = decarr(x,n,1);
		notarr(x,n);
		x[n] = 0xFFFF;
	}
	else
		x[n] = 0;
	return(n);
}
/*-------------------------------------------------------------*/
PRIVATE int twocadjust(x,n)
word2 *x;
int n;
{
	word2 u = x[n];

	while((n > 0) && (x[n-1] == u))
		n--;
	return(n);
}
/*-------------------------------------------------------------*/
/*
** Bitwise and of two bignums (x,n), (y,m) 
** in two's complement representation
** Destructively replaces (x,n) by result
*/
PUBLIC int and2arr(x,n,y,m)
word2 *x,*y;
int n,m;
{
	if(n < m && x[n]) {
		setarr(x+n+1,m-n,0xFFFF);
		n = m;
	}
	else if(m < n && !y[m]) {
		n = m;
	}
	andarr(x,m+1,y);
	n = twocadjust(x,n);
	return(n);
}
/*-------------------------------------------------------------*/
/*
** Bitwise or of two bignums (x,n), (y,m) 
** in two's complement representation
** Destructively replaces (x,n) by result
*/
PUBLIC int or2arr(x,n,y,m)
word2 *x,*y;
int n,m;
{
	if(n < m && !x[n]) {
		setarr(x+n+1,m-n,0);
		n = m;
	}
	else if(m < n && y[m]) {
		n = m;
	}
	orarr(x,m+1,y);
	n = twocadjust(x,n);
	return(n);
}
/*-------------------------------------------------------------*/
/*
** Bitwise xor of two bignums (x,n), (y,m) 
** in two's complement representation
** Destructively replaces (x,n) by result
*/
PUBLIC int xor2arr(x,n,y,m)
word2 *x,*y;
int n,m;
{
	if(n < m) {
		setarr(x+n+1,m-n,x[n]);
		n = m;
	}
	else if(m < n && y[m]) {
		setarr(y+m+1,n-m,0xFFFF);
		m = n;
	}
	xorarr(x,m+1,y);
	n = twocadjust(x,n);
	return(n);
}
/*-------------------------------------------------------------*/
/*
** Bitwise xor of two bitvectors (x,n), (y,m) 
** Destructively replaces (x,n) by result
*/
PUBLIC int xorbitvec(x,n,y,m)
word2 *x,*y;
int n,m;
{
	if(n < m) {
		setarr(x+n,m-n,0);
		n = m;
	}
	xorarr(x,m,y);
	while(n > 0 && x[n-1] == 0)
		n--;
	return(n);
}
/*-------------------------------------------------------------*/
PUBLIC long bit_length(x,n)
word2 *x;
int n;
{
	long b;

	if(n > 0) {
		b = n - 1;
		b <<= 4;
		b += bitlen(x[n-1]);
	}
	else 
		b = 0;
	return(b);
}
/*-------------------------------------------------------------*/
/*
** Prueft, ob die Elemente argptr[i], 0 <= i < n, Integers sind
** Rueckgabewert: fFIXNUM, fBIGNUM oder aERROR
*/
PUBLIC int chkints(sym,argptr,n)
truc sym;
truc *argptr;
int n;
{
	int flg, flg0 = fFIXNUM;

	while(--n >= 0) {
		flg = *FLAGPTR(argptr);
		if(flg < fFIXNUM || flg > fBIGNUM)
			return(error(sym,err_int,*argptr));
		else if(flg > flg0)
			flg0 = flg;
		argptr++;
	}
	return(flg0);
}
/*------------------------------------------------------------------*/
/*
** checks whether element *ptr is an integer
** Return value: fFIXNUM, fBIGNUM or aERROR
*/
PUBLIC int chkint(sym,ptr)
truc sym;
truc *ptr;
{
    int flg;

    flg = *FLAGPTR(ptr);
	if(flg < fFIXNUM || flg > fBIGNUM)
		return(error(sym,err_int,*ptr));
    else
        return flg;
}
/*------------------------------------------------------------------*/
/*
** checks whether element *ptr is an integer or gf2nint
** Return value: fFIXNUM, fBIGNUM, fGF2NINT or aERROR
*/
PUBLIC int chkintt(sym,ptr)
truc sym;
truc *ptr;
{
    int flg;

    flg = *FLAGPTR(ptr);
	if(flg < fINTTYPE0 || flg > fINTTYPE1)
		return(error(sym,err_intt,*ptr));
    else
        return flg;
}
/*------------------------------------------------------------------*/
/*
** checks whether element *ptr is a nonzero integer
** Return value: fFIXNUM, fBIGNUM or aERROR
*/
PUBLIC int chkintnz(sym,ptr)
truc sym;
truc *ptr;
{
    int flg;

    flg = *FLAGPTR(ptr);
	if(flg < fFIXNUM || flg > fBIGNUM)
		return(error(sym,err_int,*ptr));
    else if(*ptr == zero)
        return(error(sym,err_div,voidsym));
    else
        return flg;
}
/*-------------------------------------------------------------*/
/*
** Argument *vptr must be a vector.
** The function checks whether all components of this
** vector are integers.
** Return value: fFIXNUM, fBIGNUM or aERROR
*/
PUBLIC int chkintvec(sym,vptr)
truc sym;
truc *vptr;
{
	struct vector *vec;
	truc *ptr;
	int len;
	int flg, flg0 = fFIXNUM;

	vec = (struct vector *)TAddress(vptr);
	len = vec->len;
	ptr = &(vec->ele0);

	while(--len >= 0) {
		flg = *FLAGPTR(ptr);
		if(flg < fFIXNUM || flg > fBIGNUM)
			return(error(sym,err_int,*ptr));
		else if(flg > flg0)
			flg0 = flg;
		ptr++;
	}
	return(flg0);
}
/*-------------------------------------------------------------*/
/*
** Prueft, ob die Elemente argptr[i], 0 <= i < n, Integers oder
** Floats sind
** Rueckgabewert: fFIXNUM, fBIGNUM, Float-Flag oder aERROR
*/
PUBLIC int chknums(sym,argptr,n)
truc sym;
truc *argptr;
int n;
{
	int flg, flg0 = fFIXNUM;

	while(--n >= 0) {
		flg = *FLAGPTR(argptr);
		if(flg < fFIXNUM)
			return(error(sym,err_num,*argptr));
		else if(flg > flg0)
			flg0 = flg;
		argptr++;
	}
	return(flg0);
}
/*-------------------------------------------------------------*/
PUBLIC int chknum(sym,ptr)
truc sym;
truc *ptr;
{
	int flg = *FLAGPTR(ptr);

	if(flg < fFIXNUM)
		return(error(sym,err_num,*ptr));
	return(flg);
}
/*-------------------------------------------------------------*/
/*
** Argument *vptr must be a vector.
** The function checks whether all components of this
** vector are integers.
** Return value: fFIXNUM, fBIGNUM, Float-flag or aERROR
*/
PUBLIC int chknumvec(sym,vptr)
truc sym;
truc *vptr;
{
	struct vector *vec;
	truc *ptr;
	int len;
	int flg, flg0 = fFIXNUM;

	vec = (struct vector *)TAddress(vptr);
	len = vec->len;
	ptr = &(vec->ele0);

	while(--len >= 0) {
		flg = *FLAGPTR(ptr);
		if(flg < fFIXNUM)
			return(error(sym,err_num,*ptr));
		else if(flg > flg0)
			flg0 = flg;
		ptr++;
	}
	return(flg0);
}
/***************************************************************/
