/****************************************************************/
/* file aritz.c

ARIBAS interpreter for Arithmetic
Copyright (C) 1996-2004 O.Forster

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
WWW     http://www.mathematik.uni-muenchen.de
*/
/****************************************************************/
/*
** aritz.c
** functions for polynomials
** and for arithmetic in GF(2**n)
**
** date of last change
**
** 2002-04-07:  created
** 2003-03-05:  gf2n functions
*/
#include "common.h"

/*-----------------------------------------------------------------*/
/* field extension Fp[sqrt(D)] */
typedef struct {
    word2 *pp;
    int plen;
    word2 *D;
    int dlen;
} FP2D;

typedef struct {
    word2 *xx;
    int xlen;
    word2 *yy;
    int ylen;
} PAIRXY;

/*-----------------------------------------------------------------*/
/* setbit and testbit suppose that vv is an array of word2 */
#define setbit(vv,i)	vv[(i)>>4] |= (1 << ((i)&0xF))
#define testbit(vv,i)	(vv[(i)>>4] & (1 << ((i)&0xF)))
/*-----------------------------------------------------------------*/
#define MULTFLAG    0
#define DIVFLAG		1
#define MODFLAG		2
#define MODNFLAG    4
#define DDIVFLAG	(DIVFLAG | MODFLAG)
/*-----------------------------------------------------------------*/

PUBLIC void iniaritz	_((void));

PUBLIC truc gf2nzero, gf2none;
PUBLIC truc gf2nintsym, gf2n_sym;
PUBLIC truc polmultsym, polNmultsym;
PUBLIC truc polmodsym, polNmodsym, poldivsym, polNdivsym;
PUBLIC truc addgf2ns    _((truc *ptr));
PUBLIC truc multgf2ns   _((truc *ptr));
PUBLIC truc exptgf2n    _((truc *ptr));
PUBLIC truc divgf2ns    _((truc *ptr));
PUBLIC int fpSqrt   _((word2 *pp, int plen, word2 *aa, int alen,
                    word2 *zz, word2 *hilf));
PUBLIC int fp2Sqrt     _((word2 *pp, int plen, word2 *aa, int alen,
                word2 *zz, word2 *hilf));
PUBLIC unsigned fp_sqrt    _((unsigned p, unsigned a));


PRIVATE truc Fgf2nint       _((void));
PRIVATE truc Fpolmult       _((void));
PRIVATE truc FpolNmult      _((void));
PRIVATE truc Fpolmod        _((void));
PRIVATE truc FpolNmod       _((void));
PRIVATE truc Fpoldiv        _((void));
PRIVATE truc FpolNdiv       _((void));
PRIVATE int chkpolmultargs  _((truc sym, truc *argptr));
PRIVATE int chkpoldivargs   _((truc sym, truc *argptr));
PRIVATE truc multintpols    _((truc *argptr, int mode));
PRIVATE truc modintpols     _((truc *argptr, int mode));

PRIVATE int gf2polmod	_((word2 *x, int n, word2 *y, int m));
PRIVATE int gf2polmult	_((word2 *x, int n, word2 *y, int m, word2 *z));
PRIVATE int gf2ninverse _((word2 *x, int n, word2 *z, word2 *uu));
PRIVATE int gf2npower   _((word2 *x, int n, word2 *y, int m,
                    word2 *z, word2 *hilf));
PRIVATE int gf2polsquare    _((word2 *x, int n, word2 *z));
PRIVATE int gf2polgcd	_((word2 *x, int n, word2 *y, int m));
PRIVATE int gf2polgcdx  _((word2 *x, int n, word2 *y, int m,
                    word2 *z, int *zlenptr, word2 *hilf));
PRIVATE int gf2polirred _((word2 *x, int n, word2 *y, word2 *hilf));
PRIVATE int gf2ntrace   _((word2 *x, int n));
PRIVATE int shiftleft1  _((word2 *x, int n));
PRIVATE int bitxorshift	_((word2 *x, int n, word2 *y, int m, int s));
PRIVATE unsigned gf2polfindirr  _((int n));

PRIVATE truc gf2ndegsym, gf2npolsym, gf2ninisym, gf2ntrsym, maxgf2nsym;
PRIVATE truc Fgf2ninit  _((void));
PRIVATE truc Fgf2ndegree    _((void));
PRIVATE truc Fgf2nfieldpol  _((void));
PRIVATE truc Fgf2ntrace _((void));
PRIVATE truc Fmaxgf2n   _((void));
PRIVATE int gf2nmod _((word2 *x, int n));

/*----------------------------------------------------*/
PRIVATE void fp2Dmult _((FP2D *pfp2D, PAIRXY *pZ1, PAIRXY *pZ2, word2 *hilf));
PRIVATE void fp2Dsquare _((FP2D *pfp2D, PAIRXY *pZ, word2 *hilf));
PRIVATE void fp2Dpower _((FP2D *pfp2D, PAIRXY *pZ, word2 *ex, int exlen,
                        word2 *hilf));

PRIVATE long nonresdisc _((word2 *pp, int plen, word2 *aa, int alen,
                        word2 *hilf));

PRIVATE int fpSqrt58    _((word2 *pp, int plen, word2 *aa, int alen,
                    word2 *zz, word2 *hilf));
PRIVATE int fpSqrt14    _((word2 *pp, int plen, word2 *aa, int alen,
                    word2 *zz, word2 *hilf));
PRIVATE unsigned fp_sqrt14  _((unsigned p, unsigned a));
PRIVATE void fp2pow 	_((unsigned p, unsigned D, unsigned *uu, unsigned n));

PRIVATE truc Ffpsqrt  _((void));

PRIVATE truc fpsqrtsym;

/* #define NAUSKOMM */
#ifdef NAUSKOMM
PRIVATE truc gggsym, gg1sym, gg2sym;
PRIVATE truc Fggg	_((void));
PRIVATE truc Fgg1	_((void));
PRIVATE truc Fgg2	_((void));
#endif

#define GF2NINTEGER
/*------------------------------------------------------------------*/
PUBLIC void iniaritz()
{
#ifdef GF2NINTEGER
    word2 x[1];

    gf2nzero   = mk0gf2n(x,0);
    x[0] = 1;
    gf2none    = mk0gf2n(x,1);

    gf2nintsym = newsym   ("gf2nint",   sTYPESPEC, gf2nzero);
    gf2n_sym   = new0symsig("gf2nint",  sFBINARY, (wtruc)Fgf2nint, s_1);
    gf2ntrsym  = newsymsig("gf2n_trace",sFBINARY, (wtruc)Fgf2ntrace, s_1);
    gf2ninisym = newsymsig("gf2n_init", sFBINARY, (wtruc)Fgf2ninit, s_1);
    gf2ndegsym = newsymsig("gf2n_degree", sFBINARY, (wtruc)Fgf2ndegree, s_0);
    gf2npolsym = newsymsig("gf2n_fieldpol",sFBINARY,(wtruc)Fgf2nfieldpol,s_0);
    maxgf2nsym = newsymsig("max_gf2nsize",sFBINARY, (wtruc)Fmaxgf2n, s_0);
#endif GF2NINTEGER

#ifdef POLYARITH
    polmultsym = newsymsig("pol_mult",  sFBINARY, (wtruc)Fpolmult, s_2);
    polNmultsym= newintsym("pol_mult() mod", sFBINARY,(wtruc)FpolNmult);
    polmodsym  = newsymsig("pol_mod", sFBINARY, (wtruc)Fpolmod, s_2);
    polNmodsym = newintsym("pol_mod() mod", sFBINARY,(wtruc)FpolNmod);
    poldivsym  = newsymsig("pol_div", sFBINARY, (wtruc)Fpoldiv, s_2);
    polNdivsym = newintsym("pol_div() mod", sFBINARY,(wtruc)FpolNdiv);
#endif /* POLYARITH */

    fpsqrtsym = newsymsig("gfp_sqrt", sFBINARY, (wtruc)Ffpsqrt, s_2);

#ifdef NAUSKOMM
	gggsym = newsymsig("ggg", sFBINARY, (wtruc)Fggg, s_2);
	gg1sym = newsymsig("gg1", sFBINARY, (wtruc)Fgg1, s_2);
	gg2sym = newsymsig("gg2", sFBINARY, (wtruc)Fgg2, s_3);
#endif
}
/*-------------------------------------------------------------------*/
PRIVATE truc Ffpsqrt()
{
    word2 *pp, *aa, *hilf;
    int plen, alen, len, sign;

	if(chkints(fpsqrtsym,argStkPtr-1,2) == aERROR)
		return(brkerr());
    plen = bigref(argStkPtr-1,&pp,&sign);
    if(plen == 0 || (pp[0] & 1) == 0 || (plen == 1 && pp[0] <= 2)) {
        error(fpsqrtsym,err_oddprim,argStkPtr[-1]);
        return brkerr();
    }
    else if(plen > scrbufSize/16) {
        error(fpsqrtsym,err_ovfl,voidsym);
        return brkerr();
    }
    aa = AriScratch;
    alen = bigretr(argStkPtr,aa,&sign);
    if(sign)
        alen = modnegbig(aa,alen,pp,plen,AriBuf);
    else if(alen >= plen)
        alen = modbig(aa,alen,pp,plen,AriBuf);

    hilf = AriScratch + plen + 2;
    len = fpSqrt(pp,plen,aa,alen,AriBuf,hilf);
    if(len < 0) {
        error(scratch("gfp_sqrt(p,a)"),
        "p not prime or a not a square mod p",voidsym);
        return brkerr();
    }
    return mkint(0,AriBuf,len);
}
/*-------------------------------------------------------------------*/
#ifdef NAUSKOMM
/*-------------------------------------------------------------------*/
PRIVATE truc Fggg()
{
#if 1
    word2 *pp, *aa, *hilf;
    size_t N;
    int plen, alen, len, sign;

	if(chkints(gggsym,argStkPtr-1,2) == aERROR)
		return(brkerr());
    aa = AriScratch;
    plen = bigref(argStkPtr-1,&pp,&sign);
    alen = bigretr(argStkPtr,aa,&sign);
    N = 2*plen + alen;
    if(N > scrbufSize/16) {
        error(gggsym,err_ovfl,voidsym);
        return brkerr();
    }
    hilf = AriScratch + N;

    len = fp2Sqrt(pp,plen,aa,alen,AriBuf,hilf);
    if(len < 0) {
        error(gggsym,"sqrt mod p*p failed",voidsym);
        return brkerr();
    }
    return mkint(0,AriBuf,len);
#else
    return zero;
#endif
}
/*-------------------------------------------------------------------*/
PRIVATE truc Fgg1()
{
    word2 *pp, *aa, *hilf;
    size_t N;
    int plen, alen, len, sign;

	if(chkints(gg1sym,argStkPtr-1,2) == aERROR)
		return(brkerr());
    aa = AriScratch;
    plen = bigref(argStkPtr-1,&pp,&sign);
    alen = bigretr(argStkPtr,aa,&sign);
    N = plen + alen;
    if(N > scrbufSize/12) {
        error(gg1sym,err_ovfl,voidsym);
        return brkerr();
    }
    hilf = AriScratch + N;
    if(sign)
        alen = modnegbig(aa,alen,pp,plen,hilf);

    len = fpSqrt14(pp,plen,aa,alen,AriBuf,hilf);
    if(len < 0) {
        error(gg1sym,"sqrt mod p failed",voidsym);
        return brkerr();
    }
    return mkint(0,AriBuf,len);
}
/*-------------------------------------------------------------------*/
/*
** gg2(p,D,c) calculates (c + sqrt D)**(p+1)/2 in Fp[sqrt D]
** and returns its x-coordinate
*/
PRIVATE truc Fgg2()
{
    word2 *pp, *D, *ex, *xx, *yy, *hilf;
    int N, plen, dlen, exlen, xlen, sign;
    FP2D fp2D;
    PAIRXY Z;

	if(chkints(gg1sym,argStkPtr-2,3) == aERROR)
		return(brkerr());
    D = hilf = AriScratch;
    plen = bigref(argStkPtr-2,&pp,&sign);
    dlen = bigretr(argStkPtr-1,D,&sign);
    fp2D.pp = pp;
    fp2D.plen = plen;
    fp2D.D = D;
    fp2D.dlen = dlen;

    xx = AriBuf;
    N = plen + dlen + 2;
    ex = hilf + 2*N;
    yy = hilf + 4*N;
    hilf += 5*N;

    xlen = bigretr(argStkPtr,xx,&sign);
    Z.xx = xx;
    Z.xlen = xlen;
    Z.yy = yy; yy[0] = 1;
    Z.ylen = 1;
    cpyarr(pp,plen,ex);
    exlen = incarr(ex,plen,1);
    exlen = shiftarr(ex,exlen,-1);
    fp2Dpower(&fp2D,&Z,ex,exlen,hilf);
    xlen = Z.xlen;

    return mkint(0,xx,xlen);
}
/*-------------------------------------------------------------------*/
#endif /* NAUSKOMM */
/*-----------------------------------------------------------------*/
/*
** Hypothesis: (pp,plen) an odd prime, (aa, alen) a QR mod (pp,plen)
** Function calculates a square root (zz,zlen) of (aa,alen)
** (aa, alen) is reduced mod (pp,plen)
** space in zz must be at least 2*plen
** Return value is zlen
** In case of error, -1 is returned
*/
PUBLIC int fpSqrt(pp,plen,aa,alen,zz,hilf)
word2 *pp, *aa, *zz, *hilf;
int plen, alen;
{
    word2 *ex, *uu, *vv;
    int m8, exlen, zlen, ulen, vlen;

    if((plen < 1) || (pp[0] & 1) != 1)
        return -1;

    if(cmparr(pp,plen,aa,alen) <= 0) {
        alen = modbig(aa,alen,pp,plen,hilf);
    }
    if(!alen)
        return 0;

    m8 = (pp[0] & 0x7);
    if(m8 == 5) {
        return fpSqrt58(pp,plen,aa,alen,zz,hilf);
    }
    else if(m8 == 1) {
        return fpSqrt14(pp,plen,aa,alen,zz,hilf);
    }

    /******** else p = 3 mod 4 ********/
    ex = hilf;
    uu = hilf + plen + 2;
    vv = hilf + 3*plen + 4;
    hilf += 5*plen + 6;

    cpyarr(pp,plen,ex);
    exlen = shiftarr(ex,plen,-2);   /* ex = (p-3)/4 */
    ulen = modpower(aa,alen,ex,exlen,pp,plen,uu,hilf);  /* a**((p-3)/4) */
    zlen = modmultbig(uu,ulen,aa,alen,pp,plen,zz,hilf); /* a**((p+1)/4 */
    vlen = modmultbig(uu,ulen,zz,zlen,pp,plen,vv,hilf); /* a**((p-1)/2 */
    if(vv[0] != 1 || vlen != 1)
        return -1;
    return zlen;
}
/*-----------------------------------------------------------------*/
/*
** Hypothesis: (pp,plen) an odd prime = 5 mod 8,
** (aa, alen) is a QR mod (pp,plen)
** Function calculates a square root (zz,zlen) of (aa,alen)
** Return value is zlen
** In case of error, -1 is returned
*/
PRIVATE int fpSqrt58(pp,plen,aa,alen,zz,hilf)
word2 *pp, *aa, *zz, *hilf;
int plen, alen;
{
    word2 *ex, *uu, *vv, *ww;
    int zlen, exlen, ulen, vlen, wlen, cmp;

    ex = hilf;
    uu = hilf + plen + 2;
    vv = hilf + 3*plen + 4;
    ww = hilf + 5*plen + 6;
    hilf += 7*plen + 8;

    cpyarr(pp,plen,ex);
    exlen = shiftarr(ex,plen,-3);  /* ex = (p - 5)/8 */
    vlen = modpower(aa,alen,ex,exlen,pp,plen,vv,hilf);  /* v = a**((p-5)/8) */
    ulen = modmultbig(vv,vlen,aa,alen,pp,plen,uu,hilf); /* u = a**((p+3)/8) */
    wlen = modmultbig(uu,ulen,vv,vlen,pp,plen,ww,hilf); /* w = a**((p-1)/4) */
    if(ww[0] == 1 && wlen == 1) {
         cpyarr(uu,ulen,zz);
         zlen = ulen;
    }
    else {
        wlen = incarr(ww,wlen,1);
        cmp = cmparr(ww,wlen,pp,plen);
        if(cmp != 0)
            return -1;
        exlen = shiftarr(ex,exlen,1);
        exlen = incarr(ex,exlen,1);     /* ex = (p - 1)/4 */
        ww[0] = 2; wlen = 1;
        vlen = modpower(ww,wlen,ex,exlen,pp,plen,vv,hilf);
        zlen = modmultbig(uu,ulen,vv,vlen,pp,plen,ww,hilf);
        cpyarr(ww,zlen,zz);
    }
    if((zz[0] & 1) == 1)
        zlen = sub1arr(zz,zlen,pp,plen);
    return zlen;
}
/*-----------------------------------------------------------------*/
/*
** Hypothesis: (pp,plen) an odd prime = 1 mod 4,
** (aa, alen) is a QR mod (pp,plen)
** Function calculates a square root (zz,zlen) of (aa,alen)
** Return value is zlen
** In case of error, -1 is returned
*/
PRIVATE int fpSqrt14(pp,plen,aa,alen,zz,hilf)
word2 *pp, *aa, *zz, *hilf;
int plen, alen;
{
    long c;
    word2 *ex, *D, *xx, *yy;
    int exlen, dlen, xlen, zlen, cmp;
    FP2D fp2D;
    PAIRXY Z;

    c = nonresdisc(pp,plen,aa,alen,hilf);
    if(c < 0)
        return -1;
    xx = zz;
    yy = hilf;
    D = hilf + 2*plen + 2;
    ex = hilf + 4*plen + 4;
    hilf += 5*plen + 6;

    xlen = long2big(c,xx);
    Z.xx = xx;
    Z.xlen = xlen;
    yy[0] = 1;
    Z.yy = yy;
    Z.ylen = 1;

    dlen = multbig(xx,xlen,xx,xlen,D,hilf);     /* c**2 */
    cmp = cmparr(D,dlen,aa,alen);
    if(cmp < 0) {
        dlen = sub1arr(D,dlen,aa,alen);
        dlen = modnegbig(D,dlen,pp,plen,hilf);
    }
    else {
        dlen = subarr(D,dlen,aa,alen);
        dlen = modbig(D,dlen,pp,plen,hilf);
    }
    fp2D.pp = pp;
    fp2D.plen = plen;
    fp2D.D = D;
    fp2D.dlen = dlen;
    cpyarr(pp,plen,ex);
    exlen = incarr(ex,plen,1);
    exlen = shiftarr(ex,exlen,-1);
    fp2Dpower(&fp2D,&Z,ex,exlen,hilf);
    if(Z.ylen != 0)
        return -1;
    else {
        zlen = Z.xlen;
        if((zz[0] & 1) == 1)
            zlen = sub1arr(zz,zlen,pp,plen);
        return zlen;
    }
}
/*---------------------------------------------------------------*/
/*
** returns square root of a mod p, where p is a prime
** Hypothesis: jac(a,p) = 1.
** p and a should be 16-bit numbers.
*/
PUBLIC unsigned fp_sqrt(p,a)
unsigned p,a;
{
    if((p & 3) == 3)
        return(modpow(a,(p+1)/4,p));
    else
        return(fp_sqrt14(p,a));
}
/*---------------------------------------------------------------*/
/*
** returns square root of a mod p, where p is a prime = 1 mod 4.
** Hypothesis: jac(a,p) = 1.
** p and a should be 16-bit numbers.
*/
PRIVATE unsigned fp_sqrt14(p,a)
unsigned p,a;
{
    word4 c;
    unsigned D, u;
    unsigned uu[2];

    a = a % p;
    a = p - a;
    for(c=1; c < p; c++) {
        D = (unsigned)((c*c + a) % p);
        if(jac(D,p) == -1)
            break;
    }
    uu[0] = (unsigned)c;
    uu[1] = 1;
    fp2pow(p,D,uu,(p+1)/2);
    u = uu[0];
    if(u & 1)
        u = p-u;
    return u;
}
/*---------------------------------------------------------------*/
/*
** calculates uu**n in the field Fp(sqrt(D))
** Hypothesis: jac(D,p) = -1,
** p 16-bit prime
** (uu[0],uu[1]) is destructively replaced by result
*/
PRIVATE void fp2pow(p,D,uu,n)
unsigned p,D;
unsigned *uu;
unsigned n;
{
    word4 x,x0,y,y0,X,Y;

    if(n == 0) {
        uu[0] = 1;
        uu[1] = 0;
        return;
    }
    x = uu[0]; y = uu[1];
    X = 1; Y = 0;
    while(n > 1) {
        if(n & 1) {
            x0 = X;
            y0 = (Y*y) % p;
            /*
            ** X = (X*x + D*y0) % p;
            ** Y = (x0*y + Y*x) % p;
            */
            X *= x;    X %= p;
            X += D*y0; X %= p;
            Y *= x;    X %= p;
            Y += x0*y; Y %= p;
        }
        x0 = x;
        y0 = (y*y) % p;
        /*
        ** x = (x*x + D*y0) % p;
        ** y = (2*x0*y) % p;
        */
        x *= x;    x %= p;
        x += D*y0; x %= p;
        y *= x0;   y %= p;
        y += y;    y %= p;
        n >>= 1;
    }
    x0 = X;
    y0 = (Y*y) % p;
    /*
    ** uu[0] = (X*x + D*y0) % p;
    ** uu[1] = (X*y + Y*x) % p;
    */
    X *= x; X %= p;
    X += D*y0;
    uu[0] = X % p;
    Y *= x;
    Y += x0*y;
    uu[1] = Y % p;
}
/*-----------------------------------------------------------------*/
/*
** Calculates a square root of (aa,alen) mod (pp,plen)**2
** Hypothesis: pp odd prime, jacobi(aq,pq) = 1
*/
/*
    z := fp_sqrt(p,a);
    xi := (z*z - a) div p;
    eta := mod_inverse(2*z,p);
    delta := xi*eta mod p;
    z := z - delta*p;
    return (z mod p**2);
*/
PUBLIC int fp2Sqrt(pp,plen,aa,alen,zz,hilf)
word2 *pp, *aa, *zz, *hilf;
int plen, alen;
{
    word2 *xi, *eta, *delta, *ww;
    int m, xilen, elen, dlen, zlen, rlen, wlen, cmp, sign;

    m = 2*plen + 2;
    xi = hilf;
    eta = xi + m;
    delta = eta + m;
    ww = delta + m;
    hilf = ww + (alen >= m ? alen + 2: m);

    cpyarr(aa,alen,ww);
    wlen = modbig(ww,alen,pp,plen,hilf);
    zlen = fpSqrt(pp,plen,ww,wlen,zz,hilf);
    if(zlen < 0)    /* error */
        return zlen;
    xilen = multbig(zz,zlen,zz,zlen,xi,hilf);
    cmp = cmparr(xi,xilen,aa,alen);
    if(cmp >= 0) {
        xilen = subarr(xi,xilen,aa,alen);
        sign = 0;
    }
    else {
        xilen = sub1arr(xi,xilen,aa,alen);
        sign = MINUSBYTE;
    }
    xilen = divbig(xi,xilen,pp,plen,ww,&rlen,hilf);
    if(sign) {
        xilen = modnegbig(ww,xilen,pp,plen,hilf);
    }
    cpyarr(ww,xilen,xi);
        /* xi := (z*z - a) div p */
    cpyarr(zz,zlen,ww);
    elen = shiftarr(ww,zlen,1);
    elen = modinverse(ww,elen,pp,plen,eta,hilf);
        /* eta = mod_inverse(2*z,p) */
    dlen = modmultbig(xi,xilen,eta,elen,pp,plen,delta,hilf);
        /* delta := xi*eta mod p */
    wlen = multbig(delta,dlen,pp,plen,ww,hilf);
    cmp = cmparr(zz,zlen,ww,wlen);
    if(cmp >= 0) {
        zlen = subarr(zz,zlen,ww,wlen);
        sign = 0;
    }
    else {
        zlen = sub1arr(zz,zlen,ww,wlen);
        sign = MINUSBYTE;
    }
        /* z := z - delta*p, sign! */
    wlen = multbig(pp,plen,pp,plen,ww,hilf);
    if(sign == 0)
        zlen = modbig(zz,zlen,ww,wlen,hilf);
    else {
        zlen = modnegbig(zz,zlen,ww,wlen,hilf);
    }
    return zlen;
}
/*-----------------------------------------------------------------*/
/*
** returns a number c such that jacobi(c*c - (aa,alen),(pp,plen)) = -1
** In case of error (possibly (pp,plen) a square) returns -1
*/
PRIVATE long nonresdisc(pp,plen,aa,alen,hilf)
word2 *pp, *aa, *hilf;
int plen, alen;
{
    word4 c,v;
    word2 *xx, *yy;
    int k, vlen, xlen, cmp, sign, res;
    unsigned u;

    if(alen > plen)
        alen = modbig(aa,alen,pp,plen,hilf);
    if(!alen)
        return -1;

    xx = hilf;
    hilf += (plen >= alen ? plen : alen) + 2;
    yy = hilf;
    hilf += plen + 2;

    u = 0x1000;
    if(plen == 1 && pp[0] < u) {
        u = pp[0];
    }
    c = random2(u);
    for(k=1; k<=60000; k++,c++) {
        v = c*c;
        vlen = long2big(v,xx);
        cmp = cmparr(aa,alen,xx,vlen);
        if(cmp > 0) {
            xlen = sub1arr(xx,vlen,aa,alen);
            sign = MINUSBYTE;
        }
        else if(cmp < 0) {
            xlen = subarr(xx,vlen,aa,alen);
            sign = 0;
        }
        else
            continue;
        cpyarr(pp,plen,yy);
        res = jacobi(sign,xx,xlen,yy,plen,hilf);
        if(res < 0)
            return c;
        if((k & 0xFF) == 0) {
            if(!rabtest(pp,plen,hilf))
                break;
        }
    }
    return -1;
}
/*-----------------------------------------------------------------*/
/*
** Destructively calculates *pZ1 := (*pZ1) * (*pZ2)
** in the field given by *pfp2D
*/
PRIVATE void fp2Dmult(pfp2D,pZ1,pZ2,hilf)
FP2D *pfp2D;
PAIRXY *pZ1, *pZ2;
word2 *hilf;
{
    word2 *x0, *zz, *ww;
    int zlen, plen, wlen, x0len;

    plen = pfp2D->plen;
    x0 = hilf;
    zz = hilf + plen + 2;
    ww = hilf + 3*plen + 4;
    hilf += 5*plen + 6;

    /* x0 := x1 */
    x0len = pZ1->xlen;
    cpyarr(pZ1->xx,x0len,x0);
    /* x1 := x1*x2 + y1*y2*D */
    zlen = multbig(pZ1->yy,pZ1->ylen,pZ2->yy,pZ2->ylen,zz,hilf);
    zlen = modbig(zz,zlen,pfp2D->pp,plen,hilf);
    wlen = multbig(zz,zlen,pfp2D->D,pfp2D->dlen,ww,hilf);
    zlen = multbig(pZ1->xx,pZ1->xlen,pZ2->xx,pZ2->xlen,zz,hilf);
    wlen = addarr(ww,wlen,zz,zlen);
    wlen = modbig(ww,wlen,pfp2D->pp,plen,hilf);
    cpyarr(ww,wlen,pZ1->xx);
    pZ1->xlen = wlen;
    /* y1 := x0*y2 + y1*x2 */
    wlen = multbig(x0,x0len,pZ2->yy,pZ2->ylen,ww,hilf);
    zlen = multbig(pZ1->yy,pZ1->ylen,pZ2->xx,pZ2->xlen,zz,hilf);
    wlen = addarr(ww,wlen,zz,zlen);
    wlen = modbig(ww,wlen,pfp2D->pp,plen,hilf);
    cpyarr(ww,wlen,pZ1->yy);
    pZ1->ylen = wlen;
}
/*-----------------------------------------------------------------*/
/*
** Destructively calculates *pZ := (*pZ)**2
** in the field given by *pfp2D
*/
PRIVATE void fp2Dsquare(pfp2D,pZ,hilf)
FP2D *pfp2D;
PAIRXY *pZ;
word2 *hilf;
{
    word2 *x0, *zz, *ww;
    int zlen, plen, wlen, x0len;

    plen = pfp2D->plen;
    x0 = hilf;
    zz = hilf + plen + 2;
    ww = hilf + 3*plen + 4;
    hilf += 5*plen + 6;

    /* x0 := x */
    x0len = pZ->xlen;
    cpyarr(pZ->xx,x0len,x0);
    /* x := x*x + y*y*D */
    zlen = multbig(pZ->yy,pZ->ylen,pZ->yy,pZ->ylen,zz,hilf);
    zlen = modbig(zz,zlen,pfp2D->pp,plen,hilf);
    wlen = multbig(zz,zlen,pfp2D->D,pfp2D->dlen,ww,hilf);
    zlen = multbig(pZ->xx,pZ->xlen,pZ->xx,pZ->xlen,zz,hilf);
    wlen = addarr(ww,wlen,zz,zlen);
    wlen = modbig(ww,wlen,pfp2D->pp,plen,hilf);
    cpyarr(ww,wlen,pZ->xx);
    pZ->xlen = wlen;
    /* y := 2*x0*y */
    zlen = multbig(x0,x0len,pZ->yy,pZ->ylen,zz,hilf);
    zlen = shiftarr(zz,zlen,1);
    zlen = modbig(zz,zlen,pfp2D->pp,plen,hilf);
    cpyarr(zz,zlen,pZ->yy);
    pZ->ylen = zlen;
}
/*-----------------------------------------------------------------*/
/*
** Destructively calculates *pZ := (*pZ)**(ex,exlen)
** in the field given by *pfp2D
*/
PRIVATE void fp2Dpower(pfp2D,pZ,ex,exlen,hilf)
FP2D *pfp2D;
PAIRXY *pZ;
word2 *ex, *hilf;
int exlen;
{
    PAIRXY Z0;
    word2 *xx, *yy;
    int plen, bitl, k;
    int allowintr;

    if(exlen == 0) {
        pZ->xx[0] = 1;
        pZ->xlen = 1;
        pZ->ylen = 0;
        return;
    }

    plen = pfp2D->plen;
    xx = hilf;
    yy = hilf + 2*plen + 2;
    hilf += 4*plen + 4;

    /* z0 := z */
    cpyarr(pZ->xx,pZ->xlen,xx);
    cpyarr(pZ->yy,pZ->ylen,yy);
    Z0.xx = xx; Z0.yy = yy;
    Z0.xlen = pZ->xlen;
    Z0.ylen = pZ->ylen;
    bitl = (exlen-1)*16 + bitlen(ex[exlen-1]);
    allowintr = (plen >= 16 && (exlen + plen >= 256) ? 1 : 0);
    for(k=bitl-2; k>=0; k--) {
        fp2Dsquare(pfp2D,pZ,hilf);
    	if(testbit(ex,k))
            fp2Dmult(pfp2D,pZ,&Z0,hilf);
        if(allowintr && INTERRUPT) {
            setinterrupt(0);
            reset(err_intr);
        }
    }
    return;
}
/*-----------------------------------------------------------------*/
PRIVATE truc Fpolmult()
{
    int type;

    type = chkpolmultargs(polmultsym,argStkPtr-1);
    if(type == aERROR)
        return brkerr();
    if(type <= fBIGNUM) {
        return multintpols(argStkPtr-1,MULTFLAG);
    }
    else {
        error(polmultsym,err_imp,voidsym);
        return mkvect0(0);
    }
}
/*------------------------------------------------------------------*/
PRIVATE truc FpolNmult()
{
    int type;

    type = chkpolmultargs(polNmultsym,argStkPtr-2);
    if(type == aERROR || chkintnz(polNmultsym,argStkPtr) == aERROR)
        return brkerr();
    return multintpols(argStkPtr-2,MODNFLAG);
}
/*------------------------------------------------------------------*/
/*
** multiplies two integer polynomials given by argptr[0] and argptr[1]
*/
PRIVATE truc multintpols(argptr,mode)
truc *argptr;
int mode;
{
    truc *ptr1, *ptr2;
    truc *workptr, *w2ptr, *ptr, *wptr;
    struct vector *vecptr;
    truc obj;
	word2 *x, *y, *zz, *aa, *hilf;
    int len, len1, len2, k, i, j0, j1;
	int n1, n2, n3, n, m, sign, sign1, sign2, sign3;
    unsigned mlen, offshilf;

    len = *VECLENPTR(argptr);
    len2 = *VECLENPTR(argptr+1);
    if(len >= len2) {
        len1 = len;
        ptr1 = argptr;
        ptr2 = argptr + 1;
    }
    else {
        len1 = len2;
        len2 = len;
        ptr1 = argptr + 1;
        ptr2 = argptr;
    }
    /* now len1 >= len2, lenk = length of vector *ptrk */
    if(len2 == 0)
        return mkvect0(0);

    /* now len2 >= 1 */
    workptr = workStkPtr+1;
    if(!WORKspace(len1+len2)) {
        error(polmultsym,err_memev,voidsym);
        return brkerr();
    }
    mlen = aribufSize/3 - 6;
    ptr = VECTORPTR(ptr1);
    wptr = workptr;
    for(i=0; i<len1; i++) {      /* first vector stored in workptr[] */
        if(*FLAGPTR(ptr) == fBIGNUM && *BIGLENPTR(ptr) >= mlen)
            goto ovflexit;
        *wptr++ = *ptr++;
    }
    ptr = VECTORPTR(ptr2);
    wptr = w2ptr = workptr + len1;
    for(k=0; k<len2; k++) {     /* store second polynomial */
        if(*FLAGPTR(ptr) == fBIGNUM && *BIGLENPTR(ptr) >= mlen)
            goto ovflexit;
        *wptr++ = *ptr++;
    }
    len = len1 + len2 - 1;
    obj = mkvect0(len);
    WORKpush(obj);
    if(mode & MODNFLAG) {
        zz = AriBuf;
        n3 = bigretr(argStkPtr,zz,&sign3);
        if(n3 >= mlen)
            goto ovflexit;
    }
    aa = AriBuf + ((mlen + 6) & 0xFFFE);
    offshilf = (scrbufSize/2) & 0xFFFE;
    for(k=0; k<len; k++) {
        hilf = AriScratch + offshilf;
        n = 0; sign = 0;
        j0 = (k < len2 ? 0 : k+1-len2);
        j1 = (k < len1 ? k : len1-1);
        for(i=j0; i<=j1; i++) {
            n1 = bigref(workptr+i,&x,&sign1);
            n2 = bigref(w2ptr+(k-i),&y,&sign2);
            m = multbig(x,n1,y,n2,AriScratch,hilf);
           	sign1 = (sign1 == sign2 ? 0 : MINUSBYTE);
            n = addsarr(aa,n,sign,AriScratch,m,sign1,&sign);
        }
        if(mode & MODNFLAG) {
            n = modbig(aa,n,zz,n3,hilf);
            if(n && (sign != sign3)) {
		        n = sub1arr(aa,n,zz,n3);
                sign = sign3;
            }
        }
        obj = mkint(sign,aa,n);
        *(VECTORPTR(workStkPtr) + k) = obj;
    }
    vecptr = VECSTRUCTPTR(workStkPtr);
    ptr = &(vecptr->ele0) + len - 1;
    while(len > 0 && *ptr-- == zero)
        len--;
    vecptr->len = len;
    obj = WORKretr();
    workStkPtr = workptr-1;
    return obj;
  ovflexit:
    workStkPtr = workptr-1;
	error(polmultsym,err_ovfl,voidsym);
	return(brkerr());
}
/*------------------------------------------------------------------*/
PRIVATE truc Fpolmod()
{
    int type;

    type = chkpoldivargs(polmodsym,argStkPtr-1);
    if(type == aERROR)
        return brkerr();

    return modintpols(argStkPtr-1,MODFLAG);
}
/*------------------------------------------------------------------*/
PRIVATE truc FpolNmod()
{
    int type;

    type = chkpoldivargs(polNmodsym,argStkPtr-2);
    if(type == aERROR)
        return brkerr();

    return modintpols(argStkPtr-2, MODFLAG | MODNFLAG);
}
/*------------------------------------------------------------------*/
PRIVATE truc Fpoldiv()
{
    int type;

    type = chkpoldivargs(poldivsym,argStkPtr-1);
    if(type == aERROR)
        return brkerr();

    return modintpols(argStkPtr-1,DIVFLAG);
}
/*------------------------------------------------------------------*/
PRIVATE truc FpolNdiv()
{
    int type;

    type = chkpoldivargs(polNdivsym,argStkPtr-2);
    if(type == aERROR)
        return brkerr();

    return modintpols(argStkPtr-2, DIVFLAG | MODNFLAG);
}
/*------------------------------------------------------------------*/
PRIVATE truc modintpols(argptr,mode)
truc *argptr;
int mode;
{
    truc *workptr, *w1ptr, *w2ptr, *ptr1, *ptr2;
    truc obj;
    word2 *yy, *zz, *aa, *bb, *hilf;
    unsigned mlen, offsbb, offshilf;
    int sign, sign1, sign2, sign3;
    int j, k, m, m1, m2, n, len1, len2, len3;
    int mode1;

    len1 = *VECLENPTR(argptr);
    len2 = *VECLENPTR(argptr+1);
    if(!len2) {
        error(polmodsym,err_div,voidsym);
        return brkerr();
    }
    ptr2 = VECTORPTR(argptr+1);
    if(ptr2[len2-1] != constone) {
        error(polmodsym,"divisor must have leading coeff = 1",ptr2[len2-1]);
        return brkerr();
    }
    if(len2 > len1)
        return(*argptr);
    else if(len2 == 1)
        return mkvect0(0);
    workptr = workStkPtr+1;
    if(!WORKspace(len1 + len2)) {
        error(polmodsym,err_memev,voidsym);
        return brkerr();
    }
    mlen = aribufSize/3 - 6;
    offsbb = (mlen + 6) & 0xFFFE;
    offshilf = (scrbufSize/2) & 0xFFFE;
    w2ptr = workptr + len1;
    ptr1 = workptr;
    ptr2 = VECTORPTR(argptr);
    for(k=0; k<len1; k++) {     /* store first polynomial */
        if(*FLAGPTR(ptr2) == fBIGNUM && *BIGLENPTR(ptr2) >= mlen)
            goto ovflexit;
        *ptr1++ = *ptr2++;
    }
    ptr1 = w2ptr;
    ptr2 = VECTORPTR(argptr+1);
    for(k=0; k<len2; k++) {     /* store second polynomial */
        if(*FLAGPTR(ptr2) == fBIGNUM && *BIGLENPTR(ptr2) >= mlen)
            goto ovflexit;
        *ptr1++ = *ptr2++;
    }
    aa = AriBuf;
    bb = AriBuf + offsbb;
    len3 = len1 - len2;
    for(k=len3; k>=0; k--) {
        n = bigretr(workptr+len2+k-1,aa,&sign);
        if(mode & MODNFLAG) {
            m2 = bigref(argStkPtr,&zz,&sign2);
            n = modbig(aa,n,zz,m2,AriScratch);
            if(n && (sign != sign2)) {
		        n = sub1arr(aa,n,zz,m2);
                sign = sign2;
            }
        }
        if(!n)
            continue;
        for(j=0; j<=len2-2; j++) {
            hilf = AriScratch + offshilf;
            m = bigref(w2ptr+j,&yy,&sign1);
            if(!m)
                continue;
            m = multbig(aa,n,yy,m,AriScratch,hilf);
            sign1 = (sign == sign1 ? MINUSBYTE : 0);
            /* sign of the negative product */
            m1 = bigretr(workptr+k+j,bb,&sign2);
            m = addsarr(bb,m1,sign2,AriScratch,m,sign1,&sign3);
            obj = mkint(sign3,bb,m);
            workptr[k+j] = obj;
        }
    }
    mode1 = (mode & DDIVFLAG);
    if(mode1 == MODFLAG) {
        if(mode & MODNFLAG) {
            for(k=0; k<=len2-1; k++) {
                n = bigretr(workptr+k,aa,&sign);
                m2 = bigref(argStkPtr,&zz,&sign2);
                n = modbig(aa,n,zz,m2,AriScratch);
                if(n && (sign != sign2)) {
                    n = sub1arr(aa,n,zz,m2);
                    sign = sign2;
                }
                obj = mkint(sign,aa,n);
                workptr[k] = obj;
            }
        }
        k = len2-2;
        while(k >= 0 && workptr[k] == zero) {
            k--; len2--;
        }
        obj = mkvect0(len2-1);
        ptr1 = VECTOR(obj);
        ptr2 = workptr;
        for(k=0; k<=len2-2; k++)
            *ptr1++ = *ptr2++;
    }
    else if(mode1 == DIVFLAG) {
        w1ptr = workptr + len2 - 1;
        if(mode & MODNFLAG) {
            for(k=0; k<=len3; k++) {
                n = bigretr(w1ptr+k,aa,&sign);
                m2 = bigref(argStkPtr,&zz,&sign2);
                n = modbig(aa,n,zz,m2,AriScratch);
                if(n && (sign != sign2)) {
                    n = sub1arr(aa,n,zz,m2);
                    sign = sign2;
                }
                obj = mkint(sign,aa,n);
                w1ptr[k] = obj;
            }
        }
        while(len3>=0 && w1ptr[len3] == zero)
            len3--;
        obj = mkvect0(len3+1);
        ptr1 = VECTOR(obj);
        ptr2 = w1ptr;
        for(k=0; k<=len3; k++)
            *ptr1++ = *ptr2++;
    }
    workStkPtr = workptr-1;
    return obj;
  ovflexit:
    workStkPtr = workptr-1;
	error(polmodsym,err_ovfl,voidsym);
	return(brkerr());
}
/*------------------------------------------------------------------*/
PRIVATE int chkpolmultargs(sym,argptr)
truc sym;
truc *argptr;
{
    int flg1, flg2;
    truc *ptr;

    flg1 = *FLAGPTR(argptr);
    flg2 = *FLAGPTR(argptr+1);
    if(flg1 != fVECTOR || flg2 != fVECTOR) {
        ptr = (flg1 == fVECTOR ? argptr+1 : argptr);
        return error(sym,err_vect,*ptr);
    }
    if(sym == polmultsym) {
        flg1 = chknumvec(sym,argptr);
        if(flg1 != aERROR)
            flg2 = chknumvec(sym,argptr+1);
    }
    else {
        flg1 = chkintvec(sym,argptr);
        if(flg1 != aERROR)
            flg2 = chkintvec(sym,argptr+1);
    }
    if(flg1 == aERROR || flg2 == aERROR)
        return aERROR;

    return (flg1 >= flg2 ? flg1 : flg2);
}
/*------------------------------------------------------------------*/
PRIVATE int chkpoldivargs(sym,argptr)
truc sym;
truc *argptr;
{
    int flg1, flg2;
    truc *ptr;

    flg1 = *FLAGPTR(argptr);
    flg2 = *FLAGPTR(argptr+1);
    if(flg1 != fVECTOR || flg2 != fVECTOR) {
        ptr = (flg1 == fVECTOR ? argptr+1 : argptr);
        return error(sym,err_vect,*ptr);
    }
    flg1 = chkintvec(sym,argptr);
    if(flg1 != aERROR)
        flg2 = chkintvec(sym,argptr+1);
    if(flg1 == aERROR || flg2 == aERROR)
        return aERROR;


    return (flg1 >= flg2 ? flg1 : flg2);
}
/*******************************************************************/
typedef struct {
    int mode;
    unsigned deg;
    word2 ftail;
} GF2n_Field;

static GF2n_Field gf2nField = {1, 8, 0x1B};
static int MaxGf2n = 4099;

/*-------------------------------------------------------------*/
/*
** if k = sum(b_i * 2**i), then
** spreadbyte[k] = sum(b_i * 4**i).
*/
static word2 spreadbyte[256] = {
0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015,
0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055,
0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115,
0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155,
0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415,
0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455,
0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515,
0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555,
0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015,
0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115,
0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155,
0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415,
0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455,
0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555,
0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015,
0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115,
0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415,
0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455,
0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515,
0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555,
0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055,
0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115,
0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155,
0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415,
0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515,
0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555};
/*-------------------------------------------------------------------*/
/*
** Adds two gf2n_int's in ptr[0] and ptr[1]
*/
PUBLIC truc addgf2ns(ptr)
truc *ptr;
{
	word2 *y;
	int n, m, deg;
	int sign;

	n = bigretr(ptr,AriBuf,&sign);
	m = bigref(ptr+1,&y,&sign);
    deg = gf2nField.deg;
    if(deg < bit_length(AriBuf,n) || deg < bit_length(y,m)) {
        error(plussym,"gf2nint summand too big",voidsym);
        return brkerr();
    }
    n = xorbitvec(AriBuf,n,y,m);
    return(mkgf2n(AriBuf,n));
}
/*-------------------------------------------------------------------*/
/*
** Multiplies two gf2n_int's in ptr[0] and ptr[1]
*/
PUBLIC truc multgf2ns(ptr)
truc *ptr;
{
    int n, m, sign, deg;
    word2 *x, *y;

    n = bigref(ptr,&x,&sign);
    m = bigref(ptr+1,&y,&sign);
    deg = gf2nField.deg;
    if(deg < bit_length(x,n) || deg < bit_length(y,m)) {
        error(timessym,"gf2nint factor too big",voidsym);
        return brkerr();
    }
    n = gf2polmult(x,n,y,m,AriBuf);
    n = gf2nmod(AriBuf,n);
    return mkgf2n(AriBuf,n);
}
/*-------------------------------------------------------------------*/
/*
** Divide gf2nint ptr[0] by gf2nint ptr[1]
*/
PUBLIC truc divgf2ns(ptr)
truc *ptr;
{
    word2 *x, *y, *z;
    int n, m, sign, deg;

    n = bigref(ptr,&x,&sign);
    deg = gf2nField.deg;
    if(deg < bit_length(x,n)) {
        error(divfsym,"gf2nint argument too big",*ptr);
        return brkerr();
    }
    y = AriBuf;
    m = bigretr(ptr+1,y,&sign);
    if(deg < bit_length(y,m)) {
        error(divfsym,"gf2nint argument too big",ptr[1]);
        return brkerr();
    }
    z = y + m + 1;
    m = gf2ninverse(y,m,z,AriScratch);
    if(m == 0) {
        error(divfsym,err_div,voidsym);
        return brkerr();
    }
    n = gf2polmult(z,m,x,n,AriScratch);
    cpyarr(AriScratch,n,AriBuf);
    n = gf2nmod(AriBuf,n);
    return mkgf2n(AriBuf,n);
}
/*-------------------------------------------------------------------*/
/*
** gf2nint in ptr[0] is raised to power ptr[1], which may
** be a positive or negative integer
*/
PUBLIC truc exptgf2n(ptr)
truc *ptr;
{
    word2 *x, *y, *z;
    int n, m, N, deg, sign;

    n = bigref(ptr,&x,&sign);
    deg = gf2nField.deg;
    if(deg < bit_length(x,n)) {
        error(powersym,"gf2nint argument too big",*ptr);
        return brkerr();
    }
    m = bigref(ptr+1,&y,&sign);
    if(sign) {
        cpyarr(x,n,AriBuf);
        x = AriBuf;
        z = AriBuf + n + 1;
        n = gf2ninverse(AriBuf,n,z,AriScratch);
        if(n == 0) {
            error(powersym,err_div,voidsym);
            return brkerr();
        }
        else {
            cpyarr(z,n,x);
            z = AriBuf + n + 1;
        }
        if(m == 1 && y[0] == 1) {
            return mkgf2n(x,n);
        }
    }
    else if(m == 0) {
        return gf2none;
    }
    else {
        z = AriBuf;
    }
    N = gf2npower(x,n,y,m,z,AriScratch);
    return mkgf2n(z,N);
}
/*-------------------------------------------------------------------*/
/*
** Transforms object in *argStkPtr to data type gf2nint
*/
PRIVATE truc Fgf2nint()
{
	word2 *x;
	byte *bpt;
	unsigned u;
	unsigned len;
	int i, n, flg, sign;

    flg = *FLAGPTR(argStkPtr);

    if(flg == fFIXNUM || flg == fBIGNUM || flg == fGF2NINT) {
        n = bigretr(argStkPtr,AriBuf,&sign);
    }
    else if(flg == fBYTESTRING) {
    	len = *STRLENPTR(argStkPtr);
	    if(len >= aribufSize*2 - 2) {
		    error(gf2n_sym,err_2long,mkfixnum(len));
    		return(brkerr());
	    }
    	bpt = (byte *)STRINGPTR(argStkPtr);
	    n = len / 2;
    	x = AriBuf;
	    for(i=0; i<n; i++) {
		    u = bpt[1];
    		u <<= 8;
	    	u += bpt[0];
		    *x++ = u;
    		bpt += 2;
	    }
    	if(len & 1) {
	    	*x = *bpt;
		    n++;
    	}
    }
    else {
        error(gf2n_sym,err_int,voidsym);
        return brkerr();
    }
    n = gf2nmod(AriBuf,n);
    return mkgf2n(AriBuf,n);
}
/*-------------------------------------------------------------------*/
PRIVATE truc Fgf2ninit()
{
    int n, m;
    unsigned u;
    word2 *x;

    if(*FLAGPTR(argStkPtr) != fFIXNUM) {
        error(gf2ninisym,err_pfix,voidsym);
        return brkerr();
    }
    n = *WORD2PTR(argStkPtr);
    if(n < 2) {
        error(gf2ninisym,"degree >= 2 expected",*argStkPtr);
        return brkerr();
    }
    else if(n > MaxGf2n) {
        error(gf2ninisym,"maximal degree is",mkfixnum(MaxGf2n));
        return brkerr();
    }
    u = gf2polfindirr(n);
    if(!u) {
        error(gf2ninisym, "no irreducible polynomial found", voidsym);
        return brkerr();
    }
    gf2nField.deg = n;
    gf2nField.ftail = u;

    m = (n+1)/16 + 1;
    x = AriBuf;
    setarr(x,m,0);
    x[0] = u;
    setbit(x,n);

    return mkint(0,x,m);
}
/*-------------------------------------------------------------------*/
PRIVATE truc Fgf2ndegree()
{
    unsigned deg = gf2nField.deg;

    return mkfixnum(deg);
}
/*-------------------------------------------------------------------*/
PRIVATE truc Fgf2nfieldpol()
{
    int n;
    unsigned deg;
    word2 *x;

    deg = gf2nField.deg;

    n = (deg+1)/16 + 1;
    x = AriBuf;
    setarr(x,n,0);
    x[0] = gf2nField.ftail;
    setbit(x,deg);
    return mkint(0,x,n);
}
/*-------------------------------------------------------------------*/
#if 0
PRIVATE truc Fgf2nparms()
{
    int mode, n;
    unsigned deg;
    word2 *x;
    truc fdeg, fpol, vec;
    truc *ptr;

    mode = gf2nField.mode;
    deg = gf2nField.deg;

    if(mode == 1) {
        n = (deg+1)/16 + 1;
        x = AriBuf;
        setarr(x,n,0);
        x[0] = gf2nField.ftail;
        setbit(x,deg);
        fpol = mkint(0,x,n);
        WORKpush(fpol);
        fdeg = mkfixnum(deg);
        vec = mkvect0(2);
        ptr = VECTOR(vec);
        ptr[0] = fdeg;
        ptr[1] = WORKretr();
        return vec;
    }
    else {
        error(gf2parmsym,err_imp,voidsym);
        return brkerr();
    }
}
#endif
/*-------------------------------------------------------------------*/
PRIVATE truc Fmaxgf2n()
{
    return mkfixnum(MaxGf2n);
}
/*-------------------------------------------------------------------*/
PRIVATE truc Fgf2ntrace()
{
	int n, t, sign;
    word2 *x;

    if(*FLAGPTR(argStkPtr) != fGF2NINT) {
        error(gf2ntrsym,"gfnint expected",*argStkPtr);
        return brkerr();
    }
    x = AriBuf;
    n = bigretr(argStkPtr,x,&sign);
	if(n == aERROR)
		return brkerr();
    if(gf2nField.deg < bit_length(x,n)) {
        error(gf2ntrsym,"gf2nint argument too big",voidsym);
        return brkerr();
    }
    t = gf2ntrace(x,n);
    return mkfixnum(t);
}
/*------------------------------------------------------------------*/
/*
** P := (x,n) and Q := (y,m) are considered as polynomials over GF(2)
** Calculates P mod Q; works destructively on x; returns new length
*/
PRIVATE int gf2polmod(x,n,y,m)
word2 *x, *y;
int n,m;
{
	int N, M, k, s;

    if(m == 0)
		return n;

    N = bit_length(x,n) - 1;
    M = bit_length(y,m) - 1;
    if(M > N)
		return n;
	for(k=N; k>=M; --k) {
		if(testbit(x,k)) {
			s = k-M;
			n = bitxorshift(x,n,y,m,s);
		}
	}
	return n;
}
/*------------------------------------------------------------------*/
PRIVATE int gf2ntrace(x,n)
word2 *x;
int n;
{
    int deg = gf2nField.deg;
    unsigned ftail = gf2nField.ftail;
    int k, m, t;

    m = (deg/16) + 1;
    for(k=n; k<m; k++)
        x[k] = 0;
    t = (x[0] & 1);
    for(k=1; k<deg; k++) {
        n = shiftleft1(x,n);
        if(n > m)
            n = m;
        if(testbit(x,deg))
            x[0] ^= ftail;
        if(testbit(x,k))
            t++;
    }
    return (t & 1);
}
/*------------------------------------------------------------------*/
/*
** Shift (x,n) to the left by 1 bit
** Works destructively on x
*/
PRIVATE int shiftleft1(x,n)
word2 *x;
int n;
{
    int k, blen;
    unsigned maskhi = 0x8000, u = 1;

    if(n == 0)
        return n;
    blen = bitlen(x[n-1]);
    for(k=n-1; k>=1; k--) {
        x[k] <<= 1;
        if(x[k-1] & maskhi)
            x[k] |= u;
    }
    x[0] <<= 1;
    if(blen == 15) {
        x[n] = u;
        n++;
    }
    return n;
}
/*------------------------------------------------------------------*/
/*
** (y,m) is shifted by s >= 0 and xored with (x,n)
** If bitlength of (x,n) < bitlength of (y,m) plus s,
** higher entries of x must be 0.
*/
PRIVATE int bitxorshift(x,n,y,m,s)
word2 *x, *y;
int n,m,s;
{
    int s0, s1, t1, k, k1;
    unsigned u;

    if(!m)
        return n;

	s0 = (s >> 4);
    s1 = (s & 0xF);

	if(s1 == 0) {
		for(k=0, k1=s0-1; k<m; k++) {
			x[++k1] ^= y[k];
		}
    }
    else {
		t1 = 16 - s1;
        x[s0] ^= (y[0] << s1);
    	for(k=1, k1=s0; k<m; k++) {
            x[++k1] ^= ((y[k] << s1) | (y[k-1] >> t1));
        }
        u = (y[m-1] >> t1);
        if(u) {
			x[++k1] ^= u;
        }
	}
    if(k1 >= n)
        n = k1+1;
    while((n > 0) && (x[n-1] == 0))
        n--;
	return n;
}
/*------------------------------------------------------------------*/
/*
** Reduce (x,n) modulo the field polynomial given in gf2nField
** Works destructively on (x,n)
*/
PRIVATE int gf2nmod(x,n)
word2 *x;
int n;
{
    int N, m, s, s0, s1;
    int deg = gf2nField.deg;
    unsigned ftail = gf2nField.ftail;
    unsigned mask = 0xFFFF;
    unsigned t1, t2;

    N = bit_length(x,n) - 1;
    if(N < deg)
        return n;

    m = (deg >> 4);
    mask >>= (16 - (deg & 0xF));

    for(s = N-deg; s >= 0; s--) {
        if(testbit(x,deg+s)) {
            s0 = (s >> 4);
            s1 = (s & 0xF);
            t1 = (ftail << s1);
            x[s0] ^= t1;
            if(s1 && (t2 = (ftail >> (16-s1))))
                x[s0+1] ^= t2;
        }
    }
    x[m] &= mask;
    while(m >= 0 && (x[m] == 0))
        m--;
    return m+1;
}
/*------------------------------------------------------------------*/
/*
** Multiplies gf2pols (x,n) and (y,m);
** does not touch (x,n) or (y,m)
** Result returned in z
*/
PRIVATE int gf2polmult(x,n,y,m,z)
word2 *x, *y, *z;
int n,m;
{
    int k, M, n1;

    M = bit_length(y,m) - 1;
    if(M < 0)
        return 0;
    cpyarr(x,n,z);
    n1 = shiftarr(z,n,M);
    for(k=M-1; k>=0; k--) {
        if(testbit(y,k)) {
            n1 = bitxorshift(z,n1,x,n,k);
        }
    }
	return n1;
}
/*------------------------------------------------------------------*/
PRIVATE int gf2polsquare(x,n,z)
word2 *x, *z;
int n;
{
    byte *bptr;
    int k, N;
    unsigned u;

    if(n == 0)
        return 0;
    N = 2*n - 2;
    bptr = (byte *)x;
    for(k=0; k<N; k++) {
        u = *bptr++;
        z[k] = spreadbyte[u];
    }
    u = x[n-1];
    z[N] = spreadbyte[u & 0x00FF];
    N++;
    u >>= 8;
    if(u) {
        z[N] = spreadbyte[u];
        N++;
    }
    return N;
}
/*-------------------------------------------------------------------*/
/*
** Calculates inverse of (x,n).
** Result in z; if (x,n) is not invertible, 0 is returned
** uu is an auxiliary array needed for intermediate calculations
*/
PRIVATE int gf2ninverse(x,n,z,uu)
word2 *x, *z, *uu;
int n;
{
    int deg, m, zlen;
    word2 *y;

    y = uu;
    deg = gf2nField.deg;
    m = (deg+1)/16 + 1;
    setarr(y,m,0);
    y[0] = gf2nField.ftail;
    setbit(y,deg);

    uu = y + m + 1;
	n = gf2polgcdx(x,n,y,m,z,&zlen,uu);
    if(x[0] != 1 || n != 1) {
        return 0;
    }
    return zlen;
}
/*------------------------------------------------------------------*/
/*
** gf2nint (x,n) is raised to power (y,n)
** Result in z; hilf is an auxiliary array
*/
PRIVATE int gf2npower(x,n,y,m,z,hilf)
word2 *x,*y,*z,*hilf;
int n, m;
{
    int N, k, exlen;

    if(m == 0) {
        z[0] = 1;
        return 1;
    }
    else if(n == 0)
        return 0;
    exlen = bit_length(y,m);
    cpyarr(x,n,z);
    N = n;
    for(k=exlen-2; k>=0; k--) {
        cpyarr(z,N,hilf);
        N = gf2polsquare(hilf,N,z);
        N = gf2nmod(z,N);
        if(testbit(y,k)) {
            cpyarr(z,N,hilf);
            N = gf2polmult(hilf,N,x,n,z);
            N = gf2nmod(z,N);
        }
    }
    return N;
}
/*------------------------------------------------------------------*/
/*
** Calculates greatest common divisor of gf2pols (x,n) and (y,m);
** Works destructively on x and y
** Result is returned in x
*/
PRIVATE int gf2polgcd(x,n,y,m)
word2 *x, *y;
int n,m;
{
    while(m > 0) {
        n = gf2polmod(x,n,y,m);
        if(n == 0) {
            cpyarr(y,m,x);
            n = m;
            break;
        }
        m = gf2polmod(y,m,x,n);
    }
	return n;
}
/*-----------------------------------------------------------------*/
/*
** Calculates the gcd d of (x,n) and (y,m) (considered as polynomials
** over GF(2)) and calculates a coefficient lambda such that
** d = lambda*(x,n) mod (y,m).
** Works destructively on x and y.
** The gcd is stored in x, its length is the return value;
** lambda = (z, *zlenptr)
** uu is an auxiliary array needed for intermediate calculations
*/
PRIVATE int gf2polgcdx(x,n,y,m,z,zlenptr,uu)
word2 *x, *y, *z, *uu;
int n,m;
int *zlenptr;
{
    int s, N, M, zlen, ulen, nn, m0;
    word2 *y0;

    nn = (n >= m ? n : m);
    setarr(z,nn,0);
    setarr(uu,nn,0);
    z[0] = 1;
    zlen = 1;
    ulen = 0;
    m0 = m;
    y0 = uu + nn + 1;
    cpyarr(y,m0,y0);

    N = bit_length(x,n);
    M = bit_length(y,m);

    while(M > 0) {
    /* loop invariants:
        (x,n) =  z*(x0,n0) mod (y0,m0);
        (y,m) = uu*(x0,n0) mod (y0,m0);
       where (x0,n0) and (y0,m0) are the initial
       values of (x,n) and (y,m)
    */
        if(N >= M) {
            s = N - M;
            n = bitxorshift(x,n,y,m,s);
            N = bit_length(x,n);
            zlen = bitxorshift(z,zlen,uu,ulen,s);
            if(zlen > m0)
                zlen = gf2polmod(z,zlen,y0,m0);
        }
        else {
            s = M - N;
            m = bitxorshift(y,m,x,n,s);
            M = bit_length(y,m);
            ulen = bitxorshift(uu,ulen,z,zlen,s);
            if(ulen > m0)
                ulen = gf2polmod(uu,ulen,y0,m0);
        }
        if(N == 0) {
            cpyarr(y,m,x);
            n = m;
            cpyarr(uu,ulen,z);
            zlen = ulen;
            break;
        }
    }
    if(bit_length(z,zlen) >= bit_length(y0,m0))
        zlen = gf2polmod(z,zlen,y0,m0);
    *zlenptr = zlen;
    return n;
}
/*-----------------------------------------------------------------*/
/*
** Tests whether the gf2 polynomial given by (x,n) is irreducible.
** yy and zz are auxiliary arrays needed for intermediate calculations
*/
PRIVATE int gf2polirred(x,n,yy,zz)
word2 *x, *yy, *zz;
int n;
{
    int m, m1, N, k;
    word2 xi;
    word2 *x2k;

    if(n == 0)
        return 0;
    N = bit_length(x,n) - 1;
    if(N <= 1)
        return N;
    x2k = zz + n + 1;
    xi = 2;
    m = 1;
    yy[0] = xi;
    N = N/2;
    for(k=1; k<=N; k++) {
        m = gf2polmult(yy,m,yy,m,x2k);
        cpyarr(x,n,zz);
        m = gf2polmod(x2k,m,zz,n);
        if(m == 1 && x2k[0] == xi)
            return 0;
        cpyarr(x2k,m,yy);
        x2k[0] ^= xi;
        if(m == 0)
            m = 1;
        m1 = gf2polgcd(x2k,m,zz,n);
        if(m1 != 1 || x2k[0] != 1) {
            return 0;
        }
    }
    return 1;
}
/*-----------------------------------------------------------------*/
PRIVATE unsigned gf2polfindirr(n)
int n;
{
    unsigned u;
    int m;
    word2 *x, *yy, *zz;

    if(n < 2)
        return 0;
    m = (n+1)/16 + 1;
    x = AriBuf;
    yy = AriBuf + m + 1;
    zz = AriScratch;

    for(u=3; u<0xFFF0; u+=2) {
        if(bitcount(u) & 1)
            continue;
        setarr(x,m,0);
        x[0] = u;
        setbit(x,n);
        if(gf2polirred(x,m,yy,zz))
            return u;
    }
    return 0;
}
/*******************************************************************/
