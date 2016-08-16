/****************************************************************/
/* file arity.c

ARIBAS interpreter for Arithmetic
Copyright (C) 1996-2003 O.Forster

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

Email   forster@mathematik.uni-muenchen.de
*/
/****************************************************************/
/*
** arity.c
** factorization algorithms (Rho, continued fractions, quadratic sieve)
** and prime generation (next_prime)
**
** date of last change
** 1997-12-09:	created by splitting the file aritx.c
** 1997-12-22:	qs_factorize, big prime var
** 1998-05-31:	fixed bug in function mkquadpol
** 1998-10-11:	function next_prime
** 2000-04-26:	fixed bug in ATARI version due to ptr diffs > 32K
** 2003-05-30:  fixed bug in qs_factorize
** 2004-08-20:  Removed #define ETEST
*/

#include "common.h"

PUBLIC void iniarity    _((void));
PUBLIC void workmess   _((void));
PUBLIC void tick   	_((int c));

/*--------------------------------------------------------*/
/********* fuer rho-Faktorisierung **********/
typedef struct {
    word2 *x, *y, *Q;
    int xlen, ylen, Qlen;
} T_xyQ;

#define RHO_CYCLEN  256
#define RHO_INC 2

/********* fuer CF- und QS-Faktorisierung ************/

typedef byte sievitem;

typedef struct {
    word2 *bitmat;
    word2 *piv;
    word2 *Aarr;
    word2 *Qarr;
    word2 *fbas;
    int rank;
    int matinc;
    int vlen;
    int ainc;
    int qinc;
    int baslen;
} BMDATA;


#ifdef M_LARGE
#define ALEN        20      /* even! */
#define MAXMROWS    3200        /* was 2560 */
#define MAXSRANGE   160000      /* was 128000 */
#define MINSRANGE   6000
#define MEMCHUNK    64000
#define BIGPRIMES
#else
#define ALEN        12      /* even! */
#define MAXMROWS    512
#define MEMCHUNK    32000       /* < 2**15 */
#define STACKRES    4092
#endif

#define HLEN        (ALEN/2)

typedef struct {
    word2 kNlen;
    word2 kN[ALEN+1];
    word2 m2[HLEN+1];
    word2 R0[HLEN+1];
    word2 Q0[HLEN+1];
    word2 A0[ALEN+1];
    word2 QQ[HLEN+1];
    word2 AA[ALEN];
    int qrsign;
} CFDATA;

typedef struct {
    word2 NNlen;
    word2 NN[ALEN+1];
    word2 qq[HLEN+1];
    word2 qinv[ALEN+1];
    word2 aa[HLEN+1];
    word2 bb[HLEN+1];
    word2 cc[ALEN];
    int xi;
} QPOLY;

typedef struct {
    word2 *fbas;
    word2 *fbroot;
    sievitem *fblog;
    int baslen;
} QSFBAS;

#ifdef BIGPRIMES
/* for big prime variation in quadratic sieve factorization */
#define BPMINLEN    8   /* minimal length for using big prime var */
#define BPMAXIDX    5
PRIVATE unsigned QShtablen[BPMAXIDX] = {16000, 24000, 40000, 64000, 96000};
PRIVATE unsigned QShconst[BPMAXIDX]  = {31991, 47981, 79967, 127997,191999};
/* primes < 2*QShtablen[k] */
PRIVATE unsigned QSbpblen[BPMAXIDX]  = {8,9,10,12,14};
PRIVATE unsigned QSbpbmult[BPMAXIDX] = {1,2,3,4,6};
#define BIGPRIMEBOUND0   2000000

typedef struct {
    word4 bprime;
    word4 qdiff;
    int x;
} QSBP;

typedef struct {
    word2 q0len;
    word2 Q0[HLEN];
    word2 *hashtab;
    unsigned tablen;
    QSBP *QSBPdata;
    unsigned row;
    unsigned maxrows;
    word4 bpbound;
} QSBIGPRIMES;

typedef struct {
    sievitem *Sieve;
    int srange;
    int useBigprim;
    QPOLY *qpol;
    QSFBAS *fbp;
    QSBIGPRIMES *qsbig;
} SIEVEDATA;

#else /* !BIGPRIMES */

typedef struct {
    sievitem *Sieve;
    int srange;
    QPOLY *qpol;
    QSFBAS *fbp;
} SIEVEDATA;
#endif

/*----------------------------------------------------*/
#ifdef BIGPRIMES
#define ECMAXIDX    6
#define MAXDIFF     154
PRIVATE unsigned ECbpbound[ECMAXIDX] =
        {15000, 19500, 31000, 150000, 1300000, 0x1000000};
PRIVATE int ECmdiff[ECMAXIDX] =
        {36,    44,    52,    72,     114,     MAXDIFF};
#endif
/*----------------------------------------------------*/
/* elliptic curve c*y**2 = x**3 + a*x**2 + x mod N */
typedef struct {
    word2 *N;
    int nlen;
    word2 *aa;
    int alen;
    word2 *cc;
    int clen;
} ECN;
/*----------------------------------------------------*/
/*
** Points on elliptic curve are given by structure EPOINT
** Special points:
**    Origin: xlen = -1;
**    Partial origin: xlen = -2; (yy,ylen) a divisor of N
*/
typedef struct {
    word2 *xx;
    int xlen;
    word2 *yy;
    int ylen;
} EPOINT;
/*-----------------------------------------------------------------*/

#ifdef QTEST    /* only for testing */
#define DBGFILE
#endif
#ifdef ETEST
#define DBGFILE
#endif
#ifdef DBGFILE
FILE *dbgf;
#endif

/*-------------------------------------------------------------------*/
static char zeile[80];
/*-------------------------------------------------------------------*/
/* setbit and testbit suppose that vv is an array of word2 */
#define setbit(vv,i)    vv[(i)>>4] |= (1 << ((i)&0xF))
#define testbit(vv,i)   (vv[(i)>>4] & (1 << ((i)&0xF)))
/*-------------------------------------------------------------------*/
PRIVATE truc Frhofact   _((int argn));
PRIVATE int rhocycle    _((int anz, T_xyQ *xyQ, word2 *N, int len,
               word2 *hilf));
PRIVATE void rhomess    _((word4 i));

PRIVATE unsigned banalfact  _((word2 *N, int len));

PRIVATE truc Fcffact    _((int argn));
PRIVATE int brillmorr   _((word2 *N, int len, unsigned v, word2 *fact));
PRIVATE int bm_alloc    _((word2 *buf1, size_t len1, word2 *buf2, size_t len2,
               BMDATA *bmp, int alen, int qlen));
PRIVATE int brill1  	_((word2 *N, int len, unsigned u, word2 *fact,
               BMDATA *bmp, CFDATA *cfp, word2 *hilf));
PRIVATE unsigned factorbase  _((word2 *N, int len, word2 *prim, int anz,
                unsigned *pdivis));
PRIVATE int cfracinit   _((word2 *N, int len, unsigned u, CFDATA *cfp,
               word2 *hilf));
PRIVATE int cfracnext   _((word2 *N, int len, CFDATA *cfp, word2 *hilf));
PRIVATE int smoothea    _((word2 *QQ, word2 *fbas, int baslen, int easize));
PRIVATE word4 smooth    _((word2 *QQ, word2 *fbas, int baslen));

PRIVATE int bm_insert   _((BMDATA *bmp, word2 *QQ, int qsign, word2 *AA,
               word2 *hilf));
PRIVATE int gausselim   _((BMDATA *bmp));
PRIVATE int getfactor   _((word2 *N, int len, BMDATA *bmp, word2 *fact,
               word2 *hilf));
PRIVATE truc Fqsfact    _((int argn));
PRIVATE int mpqsfactor  _((word2 *N, int len, word2 *fact));
PRIVATE int qsfact1 _((word2 *N, int len, word2 *fact, BMDATA *bmp,
               SIEVEDATA *qsp, word2 *hilf));

PRIVATE int ppexpo	_((unsigned B1, unsigned B2, word2 *xx));

PRIVATE int startqq 	_((word2 *N, int len, unsigned srange,
               word2 *qq, word2 *hilf));

PRIVATE int nextqq  	_((word2 *N, int len, word2 *q0, int q0len,
                word2 *qq, word2 *hilf));
PRIVATE int dosieve 	_((SIEVEDATA *qsp));

PRIVATE int p2sqrt  	_((word2 *p, int plen, word2 *x, int xlen,
                word2 *z, word2 *hilf));

PRIVATE int mkquadpol   _((word2 *p, int plen, QPOLY *sptr, word2 *hilf));

PRIVATE int quadvalue   _((QPOLY *polp, word2 *QQ, int *signp));
PRIVATE int qresitem    _((QPOLY *polp, word2 *AA));

PRIVATE void counttick0  _((unsigned v));
PRIVATE void counttick  _((word4 v, BMDATA *bmp));
PRIVATE void cf0mess    _((int p, int blen));
PRIVATE void cf1mess    _((long n, int nf));
PRIVATE void qs0mess    _((int srange, int p, int blen));
PRIVATE void qs1mess    _((long n, int nf));
PRIVATE void ec0mess    _((unsigned bound));
PRIVATE void ec1mess    _((unsigned bound1, unsigned bound2));
PRIVATE void ec2mess    _((unsigned param, unsigned bound1));
PRIVATE void ec3mess    _((unsigned param, unsigned bigbound));

PRIVATE int is_square   _((word2 *N, int len, word2 *root, word2 *hilf));
PRIVATE int multlarr    _((word2 *x, int n, unsigned a, word2 *y));

#ifdef BIGPRIMES
PRIVATE int hashbigp    _((QSBIGPRIMES *qsbigp, word4 prim, QPOLY *qpolp,
               QPOLY *qpolp2, word2 *hilf));
PRIVATE int combinebp   _((word2 *N, int len, word4 prim, word2 *QQ, word2 *AA,
               word2 *QQ2, word2 *AA2, word2 *hilf));
PRIVATE void qs2mess    _((long n, int nf, int nf2));
#endif

PRIVATE truc Fecfactor	_((int argn));
PRIVATE int ecfacta _((word2 *N, int len, word2 *aa, int alen,
                       word2 *xx, int xlen, unsigned *pbound, word2 *hilf));
PRIVATE int ecbpvalloc   _((EPOINT *pEpoint, word2 *buf, size_t buflen,
                    int nlen, unsigned *pbound));
PRIVATE int ecfactbpv   _((ECN *pecN, EPOINT *pEpoint,
                    unsigned *pbound, int hdiff,
                    word2 *xx, int xlen, word2 *hilf));
PRIVATE int ECNx2c  _((ECN *pecN, word2 *xx, int xlen, word2 *hilf));
PRIVATE int ECNadd  _((ECN *pecN, EPOINT *pZ1, EPOINT *pZ2, word2 *hilf));
PRIVATE int ECNdup  _((ECN *pecN, EPOINT *pZ, word2 *hilf));
PRIVATE int ECNmult  _((ECN *pecN, EPOINT *pZ, word2 *ex, int exlen,
                word2 *hilf));

PRIVATE truc rhosym, cffactsym, qsfactsym;
PRIVATE truc ecfactsym;

#ifdef ETEST
PRIVATE truc eeesym, ee1sym, ee2sym;
PRIVATE truc Feee   _((void));
PRIVATE truc Fee1   _((void));
PRIVATE truc Fee2   _((void));
#endif

PRIVATE int doreport;
/*------------------------------------------------------------------*/
PUBLIC void iniarity()
{
    rhosym    = newsymsig("rho_factorize",sFBINARY,(wtruc)Frhofact, s_13);
    cffactsym = newsymsig("cf_factorize", sFBINARY,(wtruc)Fcffact, s_13);
    qsfactsym = newsymsig("qs_factorize", sFBINARY,(wtruc)Fqsfact, s_12);
    ecfactsym = newsymsig("ec_factorize", sFBINARY, (wtruc)Fecfactor,s_14);
#ifdef ETEST
    eeesym    = newsymsig("eee", sFBINARY, (wtruc)Feee, s_3);
    ee1sym    = newsymsig("ee1", sFBINARY, (wtruc)Fee1, s_3);
    ee2sym    = newsymsig("ee2", sFBINARY, (wtruc)Fee2, s_4);
#endif
}
/*-------------------------------------------------------------------*/
#ifdef ETEST
PRIVATE truc Feee()
{
    word2 *N, *aa, *xx;
    int alen, clen, xlen, sign, nlen;
    ECN ecN;

	if(chkints(eeesym,argStkPtr-2,3) == aERROR)
		return(brkerr());

    nlen = bigref(argStkPtr-2,&N,&sign);
    alen = bigref(argStkPtr-1,&aa,&sign);
    xlen = bigref(argStkPtr,&xx,&sign);

    ecN.cc = AriBuf;
    ecN.aa = aa;
    ecN.alen = alen;
    ecN.N = N;
    ecN.nlen = nlen;

    clen = ECNx2c(&ecN,xx,xlen,AriScratch);
    if(clen >= 0)
        return mkint(0,AriBuf,clen);
    else
        return zero;
}
/*-------------------------------------------------------------------*/
PRIVATE truc Fee1()
{
    word2 *N, *aa, *xx, *yy, *cc, *ex, *hilf;
    int len, alen, clen, xlen, exlen, sign, nlen;
    ECN ecN;
    EPOINT PP;

	if(chkints(eeesym,argStkPtr-2,3) == aERROR)
		return(brkerr());

    nlen = bigref(argStkPtr-2,&N,&sign);
    alen = bigref(argStkPtr-1,&aa,&sign);
    xlen = bigref(argStkPtr,&xx,&sign);

    cc = AriScratch;
    hilf = cc + 2*nlen + 2;

    ecN.cc = cc;
    ecN.aa = aa;
    ecN.alen = alen;
    ecN.N = N;
    ecN.nlen = nlen;

    clen = ECNx2c(&ecN,xx,xlen,hilf);
    if(clen < 0)
        return brkerr();
    cpyarr(xx,xlen,AriBuf);
    xx = AriBuf;
    yy = AriBuf + nlen + xlen;
    PP.xx = xx;
    PP.xlen = xlen;
    PP.yy = yy;
    yy[0] = 1;
    PP.ylen = 1;

    len = ECNdup(&ecN,&PP,hilf);

    if(len > 0)
        return mkint(0,AriBuf,len);
    else
        return mkinum(len);
}
/*-------------------------------------------------------------------*/
PRIVATE truc Fee2()
{
    word2 *N, *aa, *xx, *yy, *cc, *ex, *hilf;
    int len, alen, clen, xlen, exlen, sign, nlen;
    ECN ecN;
    EPOINT PP;

#ifdef ETEST1
dbgf = fopen("etest.log","w");
#endif
	if(chkints(eeesym,argStkPtr-3,4) == aERROR)
		return(brkerr());

    nlen = bigref(argStkPtr-3,&N,&sign);
    alen = bigref(argStkPtr-2,&aa,&sign);
    xlen = bigref(argStkPtr-1,&xx,&sign);
    exlen = bigref(argStkPtr,&ex,&sign);

    cc = AriScratch;
    hilf = cc + 2*nlen + 2;

    ecN.cc = cc;
    ecN.aa = aa;
    ecN.alen = alen;
    ecN.N = N;
    ecN.nlen = nlen;

    clen = ECNx2c(&ecN,xx,xlen,hilf);
    if(clen < 0)
        return brkerr();
    cpyarr(xx,xlen,AriBuf);
    xx = AriBuf;
    if(xlen > nlen)
        xlen = modbig(xx,xlen,N,nlen,hilf);
    yy = AriBuf + 2*nlen;
    PP.xx = xx;
    PP.xlen = xlen;
    PP.yy = yy;
    yy[0] = 1;
    PP.ylen = 1;

    len = ECNmult(&ecN,&PP,ex,exlen,hilf);

#ifdef ETEST1
fclose(dbgf);
#endif

    if(len > 0)
        return mkint(0,AriBuf,len);
    else
        return mkinum(len);
}
#endif /* ETEST */
/*-------------------------------------------------------------------*/
/*
** Messages for factorization algorithms
*/
PUBLIC void workmess()
{
    fnewline(tstdout);
    fprintstr(tstdout,"working ");
}
/*-------------------------------------------------------------------*/
PUBLIC void tick(c)
int c;
{
    char tt[2];

    tt[0] = c;
    tt[1] = 0;
    fprintstr(tstdout,tt);
    fflush(stdout);
}
/*-------------------------------------------------------------------*/
PRIVATE void counttick0(v)
unsigned v;
{
    char messbuf[80];

    s1form(messbuf,"~D",(wtruc)v);
    fprintstr(tstdout,messbuf);
    fflush(stdout);
}
/*-------------------------------------------------------------------*/
PRIVATE void counttick(v,bmp)
word4 v;
BMDATA *bmp;
{
    char messbuf[80];
    word4 z,w;
    int c;

    if(v&0x7F) {
        tick('_');
    }
    else {
        v >>= 7;
        c = v % 10;
        if(c) {
            tick('0' + c);
        }
        else {
            z = v/10;
            w = bmp->rank;
            w *= 100;
            w /= bmp->baslen;
            s2form(messbuf,"[~D/~D%]",(wtruc)z,(wtruc)w);
            fprintstr(tstdout,messbuf);
            fflush(stdout);
        }
    }
}
/*-------------------------------------------------------------------*/
PRIVATE void rhomess(anz)
word4 anz;
{
    char messbuf[80];

    s1form(messbuf,"~%factor found after ~D iterations",(wtruc)anz);
    fprintline(tstdout,messbuf);
    fnewline(tstdout);
}
/*-------------------------------------------------------------------*/
PRIVATE void cf0mess(p,blen)
int p, blen;
{
    char messbuf[80];

    s2form(messbuf,"~%CF-algorithm: factorbase 2 ... ~D of length ~D",
        (wtruc)p,(wtruc)blen);
    fprintline(tstdout,messbuf);
    fprintstr(tstdout,"working ");
}
/*-------------------------------------------------------------------*/
PRIVATE void cf1mess(n,nf)
long n;
int nf;
{
    char messbuf[80];

    s2form(messbuf,
        "~%~D quadratic residues calculated, ~D completely factorized",
        (wtruc)n,(wtruc)nf);
    fprintline(tstdout,messbuf);
    fnewline(tstdout);
}
/*-------------------------------------------------------------------*/
PRIVATE void qs0mess(srange,p,blen)
int srange, p, blen;
{
    char messbuf[80];

    s1form(messbuf,"~%quadratic sieve length = ~D, ",(wtruc)(2*srange));
    fprintstr(tstdout,messbuf);
    s2form(messbuf,"factorbase 2 ... ~D of length ~D",
        (wtruc)p,(wtruc)blen);
    fprintline(tstdout,messbuf);
    fprintstr(tstdout,"working ");
}
/*-------------------------------------------------------------------*/
PRIVATE void qs1mess(n,nf)
long n;
int nf;
{
    char messbuf[80];

    s2form(messbuf,
        "~%~D polynomials, ~D completely factorized quadratic residues",
        (wtruc)n,(wtruc)nf);
    fprintline(tstdout,messbuf);
    fnewline(tstdout);
}
/*------------------------------------------------------------------*/
#ifdef BIGPRIMES
/*------------------------------------------------------------------*/
PRIVATE void qs2mess(n,nf,nf2)
long n;
int nf, nf2;
{
    char messbuf[80];

    s2form(messbuf,"~%~D polynomials, ~D + ",(wtruc)n,(wtruc)(nf-nf2));
    fprintstr(tstdout,messbuf);

    s2form(messbuf,"~D = ~D factorized quadratic residues",
        (wtruc)nf2,(wtruc)nf);
    fprintline(tstdout,messbuf);
    fnewline(tstdout);
}
/*------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------*/
PRIVATE void ec0mess(bound)
unsigned bound;
{
    char messbuf[80];

    s1form(messbuf,"~%EC factorization with prime bound ~D ~%working ",
        (wtruc)bound);
    fprintstr(tstdout,messbuf);
}
/*------------------------------------------------------------------*/
PRIVATE void ec1mess(bound1, bound2)
unsigned bound1, bound2;
{
    char messbuf[80];

    s2form(messbuf,
    "~%EC factorization, prime bound ~D, bigprime bound ~D~%working ",
        (wtruc)bound1, (wtruc)bound2);
    fprintstr(tstdout,messbuf);
}
/*------------------------------------------------------------------*/
PRIVATE void ec2mess(u,bound1)
unsigned u,bound1;
{
    char messbuf[80];

    s2form(messbuf,
        "~%factor found with curve parameter ~D and prime bound ~D~%",
        (wtruc)u,(wtruc)bound1);
    fprintline(tstdout,messbuf);
}
/*------------------------------------------------------------------*/
PRIVATE void ec3mess(u,bound2)
unsigned u,bound2;
{
    char messbuf[80];

    s2form(messbuf,
        "~%factor found with curve parameter ~D and bigprime ~D~%",
        (wtruc)u,(wtruc)bound2);
    fprintline(tstdout,messbuf);
}
/*------------------------------------------------------------------*/
/*
** Pollardsche rho-Methode zur Faktorisierung;
** Aufruf rho_factorize(N,anz) oder rho_factorize(N);
** dabei ist anz die maximale Anzahl der Iterationen,
** default anz = 2**16
*/
PRIVATE truc Frhofact(argn)
int argn;
{
    T_xyQ xyQ;

    truc *argptr;
    word2 *N, *z, *d, *Q, *hilf;
    word4 u, i;
    size_t m;
    unsigned rr;
    int k, n, len, sign, ret;

    argptr = argStkPtr-argn+1;
    if(argn >= 2 && *argStkPtr == zero) {
        doreport = 0;
        argn--;
    }
    else {
        doreport = 1;
    }
    if(chkints(rhosym,argptr,argn) == aERROR)
        return(brkerr());

    len = bigref(argptr,&N,&sign);
    m = aribufSize/4;
    if(len >= (m-1)/2) {
        error(rhosym,err_ovfl,*argptr);
        return(brkerr());
    }

    d = AriBuf;
    xyQ.x = d + m;
    xyQ.y = d + 2*m;
    xyQ.Q = d + 3*m;
    Q = AriScratch;
    hilf = AriScratch + aribufSize;

    rr = random2(64000);
    xyQ.x[0] = xyQ.y[0] = rr;
    xyQ.xlen = xyQ.ylen = (rr ? 1 : 0);
    xyQ.Q[0] = 1;
    xyQ.Qlen = 1;

    if(argn == 2) {
        n = bigref(argptr+1,&z,&sign);
        if(n <= 2 && n)
            u = big2long(z,n);
        else
            u = 0x80000000;
    }
    else
        u = 0x10000;
    if(doreport)
        workmess();

    for(i=0; i<u; i+=RHO_CYCLEN) {
        if(doreport)
            tick('.');
        ret = rhocycle(RHO_CYCLEN,&xyQ,N,len,hilf);
        if(ret) {
            k = xyQ.Qlen;
            cpyarr(xyQ.Q,k,Q);
            cpyarr(N,len,d);
            k = biggcd(d,len,Q,k,hilf);
            if(k > 1 || *d > 1) {
                if(doreport)
                    rhomess(i+RHO_CYCLEN);
                return(mkint(0,d,k));
            }
        }
        if(INTERRUPT) {
            setinterrupt(0);
            break;
        }
    }
    return(zero);
}
/*-------------------------------------------------------------------*/
/*
** Berechnet anz mal
** x -> x*x+RHO_INC; x -> x*x+RHO_INC; y -> y*y+RHO_INC mod N
** Q -> Q*(x-y) mod N
** Rueckgabewert Laenge von Q
*/
PRIVATE int rhocycle(anz,xyQ,N,len,hilf)
int anz;
T_xyQ *xyQ;
word2 *N;
int len;
word2 *hilf;
{
    word2 *x1, *y1, *Q1, *z, *z1;
    int n, m, k, cmp;
    int zlen, z1len, nn;

    nn = 2*len + 2;
    z = hilf;
    z1 = hilf + nn;
    hilf = z1 + nn;

    x1 = xyQ->x;
    n = xyQ->xlen;
    y1 = xyQ->y;
    m = xyQ->ylen;
    Q1 = xyQ->Q;
    *Q1 = 1;
    k = 1;

    while(--anz >= 0) {
        zlen = multbig(x1,n,x1,n,z,hilf);
        zlen = incarr(z,zlen,RHO_INC);
        zlen = modbig(z,zlen,N,len,hilf);
        z1len = multbig(z,zlen,z,zlen,z1,hilf);
        z1len = incarr(z1,z1len,RHO_INC);
        n = modbig(z1,z1len,N,len,hilf);
        cpyarr(z1,n,x1);
        zlen = multbig(y1,m,y1,m,z,hilf);
        zlen = incarr(z,zlen,RHO_INC);
        m = modbig(z,zlen,N,len,hilf);
        cpyarr(z,m,y1);
        cmp = cmparr(z,m,z1,n);
        if(cmp > 0)
            zlen = subarr(z,m,z1,n);
        else if(cmp < 0)
            zlen = sub1arr(z,m,z1,n);
        else
            continue;
        z1len = multbig(Q1,k,z,zlen,z1,hilf);
        k = modbig(z1,z1len,N,len,hilf);
        cpyarr(z1,k,Q1);
    }
    xyQ->xlen = n;
    xyQ->ylen = m;
    xyQ->Qlen = k;
    return(k);
}
/*------------------------------------------------------------------*/
/*
** Continued fraction factorization
** cf_factorize(N: integer[; mult: integer]): integer;
*/
PRIVATE truc Fcffact(argn)
int argn;
{
    truc *argptr;
    word2 *N, *x;
    long mm;
    size_t buflen;
    unsigned u;
    int len0, len, n;
    int sign;

    argptr = argStkPtr-argn+1;
    if(argn >= 2 && *argStkPtr == zero) {
        doreport = 0;
        argn--;
    }
    else {
        doreport = 1;
    }
    if(chkints(cffactsym,argptr,argn) == aERROR)
        return(brkerr());

    len = bigref(argptr,&N,&sign);
    u = banalfact(N,len);
    if(u != (unsigned)-1)
        return(mkfixnum(u));

    buflen = auxbufSize * sizeof(word2);
#ifdef M_SMALL
    mm = stkcheck() - STACKRES;
    if(buflen < mm)
        mm = buflen;
#else
    mm = buflen;
#endif
    len0 = ALEN;
    if(mm < MEMCHUNK) {
        error(cffactsym,err_memev,voidsym);
        return(brkerr());
    }

    if(len > len0 || (len == len0 && bitlen(N[len-1]) > 4)) {
        error(cffactsym,err_2big,*argptr);
        return(brkerr());
    }
    if(argn >= 2) {
        n = bigref(argptr+1,&x,&sign);
        u = *x;
        if(n != 1 || u > 1023)
            u = 1;
    }
    else
        u = 1;
    n = brillmorr(N,len,u,AriBuf);

    return(mkint(0,AriBuf,n));
}
/*------------------------------------------------------------------*/
/*
** Falls (N,len) < 2**32 wird der kleinste Primfaktor zurueckgegeben.
** Falls N gerade, wird 2 zurueckgegeben.
** Andernfalls wird (unsigned)-1 zurueckgegeben.
*/
PRIVATE unsigned banalfact(N,len)
word2 *N;
int len;
{
    word4 u;
    unsigned v,d;

    if(len <= 2) {
        u = big2long(N,len);
        v = intsqrt(u);
        d = trialdiv(N,len,2,v);
        return(d);
    }
    else if(!(N[0] & 1))
        return(2);
    /* else */
    v = 15015;  /* 3*5*7*11*13 */
    d = modarr(N,len,v);
    d = shortgcd(d,v);
    if(d > 1)
        return d;
    else
        return((unsigned)-1);
}
/*------------------------------------------------------------------*/
PRIVATE int brillmorr(N,len,v,fact)
word2 *N, *fact;
int len;
unsigned v;
{
#ifdef M_SMALL
    word2 stackpiece[MEMCHUNK/sizeof(word2)];
#endif
    BMDATA bm;
    CFDATA cf;

    word2 *buf1, *buf2, *hilf;
    word4 u;
    size_t b1len, b2len;
    int k, alen, qlen, baslen, maxrows, b0, b1, b, ret;

    b1 = bitlen(N[len-1]);
    alen = (b1 > 4 ? len+1 : len);
    qlen = (alen + 1)/2;
#ifdef M_SMALL
    buf1 = AriScratch;
    b1len = scrbufSize; /* scrbufSize >= auxbufSize in M_SMALL */
    buf2 = stackpiece;
    b2len = MEMCHUNK/sizeof(word2);
    hilf = AuxBuf;
#else
    buf1 = AuxBuf;
    b1len = auxbufSize/2;
    buf2 = AuxBuf + b1len;
    b2len = b1len;
    hilf = AriScratch;
#endif
    maxrows = bm_alloc(buf1,b1len,buf2,b2len,&bm,alen,qlen);
    b0 = maxrows/16;

    u = (len - 1)*16 + b1;     /* bitlength of N */
    b = 1 + (u*u)/384;
    if(b > b0)
        b = b0;
    bm.vlen = b;

    bm.baslen = baslen = b * 16 - 2;
    for(k=0; k<=baslen; k++)
        bm.piv[k] = baslen-k;
        /* bm.piv[k] = k; */

    bm.fbas = hilf;
    hilf += b * 16;

    ret = brill1(N,len,v,fact,&bm,&cf,hilf);

    return(ret);
}
/*-------------------------------------------------------------------*/
/*
** In (buf1,len1) und (buf2,len2) werden zwei Puffer uebergeben
** Aus diesen wird der Struktur *bmp Speicher zugewiesen
**
** Bedarf fuer bmp:
**  bmp->bitmat Platz fuer eine Bitmatrix maxrows*(2*maxrows)
**  bmp->piv    Platz fuer maxrows word2's
**  bmp->fbas   Platz fuer maxrows word2's
**  bmp->Aarr   Platz fuer maxrows bigints der Laenge alen,
**          zuzueglich Laengen-Angabe
**  bmp->Qarr   Platz fuer maxrows bigints der Laenge qlen,
**          zuzueglich Laengen-Angabe
**
** Rueckgabewert ist maxrows; dies ist durch 16 teilbar
*/
PRIVATE int bm_alloc(buf1,len1,buf2,len2,bmp,alen,qlen)
word2 *buf1, *buf2;
size_t len1, len2;
BMDATA *bmp;
int alen, qlen;
{
    word2 *xx, *yy;
    word4 u;
    size_t ll;
    int maxrows;

    /* allocation for bmp->bitmat (from buf1) */
    u = len1;
    u *= 8;
    maxrows = (int)intsqrt(u);
    if(maxrows > MAXMROWS)
        maxrows = MAXMROWS;
    maxrows &= 0x7FF0;   /* make it a multiple of 16 */

    bmp->bitmat = buf1;
    bmp->matinc = maxrows/8;
    bmp->rank = 0;

    /* allocation for Aarr, Qarr, piv (from buf2) */
    alen++; qlen++;     /* one word2 for length specification */
    ll = len2 / (alen + qlen + 1);
    if(ll < maxrows) {
        maxrows = (ll & 0x7FF0);
    }
    bmp->Aarr = buf2 + 1;
    bmp->ainc = alen;

    xx = buf2 + alen * maxrows;
    bmp->Qarr = xx + 1;
    bmp->qinc = qlen;

    yy = xx + qlen * maxrows;
    bmp->piv = yy;

    return(maxrows);
}
/*-------------------------------------------------------------------*/
PRIVATE int brill1(N,len,u,fact,bmp,cfp,hilf)
word2 *N, *fact, *hilf;
unsigned u;
int len;
BMDATA *bmp;
CFDATA *cfp;
{
    word2 *zz, *fbase;
    word4 v;
    int k, qrlen, baslen, easize, res;
    int count, count1, maxshrieks;
    unsigned divis;

    qrlen = cfracinit(N,len,u,cfp,hilf);
    if(qrlen == 0) {
        k = cfp->AA[-1];
        cpyarr(cfp->AA,k,fact);
        zz = cfp->kN;
        cpyarr(N,len,zz);
        return(biggcd(fact,k,zz,len,hilf));
    }
    fbase = bmp->fbas;
    baslen = factorbase(cfp->kN,(int)cfp->kN[-1],fbase,bmp->baslen,&divis);
    bmp->baslen = baslen;

    if(doreport)
        cf0mess(fbase[baslen-1],baslen);

    easize = intsqrt(6*(word4)baslen);    /* ?! */
	maxshrieks = (baslen < 100 ? 20 : 20 + (baslen/2 - 50)/bitlen(baslen));
    for(v=1, count=count1=0; qrlen && count1<maxshrieks; v++) {
        if((v & 0xFF) == 0) {
	        if(INTERRUPT) {
	            setinterrupt(0);
	            break;
	        }
	        if((v & 0x3FF) == 0) {
	            if(doreport)
	                counttick(v>>10,bmp);
	        }
        }
        if(smoothea(cfp->QQ,fbase,baslen,easize)) {
	        if((++count & 0x3) == 1)
	            if(doreport)
	                tick('.');
	        res = bm_insert(bmp,cfp->QQ,cfp->qrsign,cfp->AA,hilf);
	        if(res && gausselim(bmp)) {
	            count1++;
	            if(doreport)
	                tick('!');
	            k = getfactor(N,len,bmp,fact,hilf);
	            if(k > 0) {
	                if(doreport)
	                    cf1mess(v,count);
	                return(k);
	            }
	        }
        }
        qrlen = cfracnext(N,len,cfp,hilf);
    }
    return(0);
}
/*------------------------------------------------------------------*/
/*
** Schreibt in das Array fbase die Primzahl 2 und weitere (anz-1)
** ungerade Primzahlen, fuer die jacobi((N,len),p) = 1
** Falls N durch eine Primzahl, die kleiner als das Maximum
** der Faktorbasis ist, teilbar ist, wird diese zurueckgegeben,
** andernfalls 0.
*/
PRIVATE unsigned factorbase(N,len,fbase,anz,pdivis)
word2 *N, *fbase;
int len, anz;
unsigned *pdivis;
{
    unsigned m, p;
    unsigned divisor = 0;
    int idx;

    fbase[0] = 2;
    for(idx=1, p=3; idx<anz; p+=2) {
        if(p > 0xFFF1) {
            anz = idx + 1;
            break;
        }
        if(!prime16(p))
            continue;
        m = modarr(N,len,p);
        if(m == 0 && divisor == 0) {
            divisor = p;
        }
        if(jac(m,p) >= 0) {
            fbase[idx] = p;
            idx++;
        }
    }
    *pdivis = divisor;
    return(anz);
}
/*-------------------------------------------------------------------*/
PRIVATE int cfracinit(N,len,u,cfp,hilf)
word2 *N;
int len;
unsigned u;
CFDATA *cfp;
word2 *hilf;
{
    word2 *temp, *temp1;
    int k, n, rlen;
    int ll = 2*len + 2;

    temp = hilf;
    temp1 = temp + ll;
    hilf = temp1 + ll;

    len = multarr(N,len,u,temp);
    cpyarr(temp,len,cfp->kN);
    cfp->kN[-1] = len;

    k = bigsqrt(temp,len,temp1,&rlen,hilf);
    cpyarr(temp1,k,cfp->AA);
    cfp->AA[-1] = k;

    n = multbig(temp1,k,temp1,k,temp,hilf);
    rlen = sub1arr(temp,n,cfp->kN,len);
    cpyarr(temp,rlen,cfp->QQ);
    cfp->QQ[-1] = rlen;

    k = shlarr(temp1,k,1);
    cpyarr(temp1,k,cfp->m2);
    cfp->m2[-1] = k;

    cfp->R0[-1] = 0;
    cfp->A0[0] = 1;
    cfp->A0[-1] = 1;
    cfp->Q0[0] = 1;
    cfp->Q0[-1] = 1;

    cfp->qrsign = -1;
    return(rlen);
}
/*-------------------------------------------------------------------*/
/*
** m2 - R0 = bb * QQ + rest;
** Qnew = Q0 + (rest - R0) * bb;
** Anew = (AA * bb + A0) mod N
** next R0 = rest
** next Q0 = QQ
** next A0 = AA
** next QQ = Qnew
** next AA = Anew
*/
PRIVATE int cfracnext(N,len,cfp,hilf)
word2 *N;
int len;
CFDATA *cfp;
word2 *hilf;
{
    static word2 rr[HLEN], bb[HLEN], temp1[HLEN];

    word2 *QQ, *Qtemp, *Atemp;
    int m2len, rlen, blen, qtlen, atlen, t1len;
    int cmp;

    Qtemp = Atemp = hilf;
    hilf += 2*ALEN;
    QQ = cfp->QQ;

    m2len = cfp->m2[-1];
    cpyarr(cfp->m2,m2len,rr);
    m2len = subarr(rr,m2len,cfp->R0,(int)cfp->R0[-1]);
    blen = divbig(rr,m2len,QQ,(int)QQ[-1],bb,&rlen,hilf);
    cpyarr(rr,rlen,temp1);
    t1len = rlen;

    cmp = cmparr(temp1,t1len,cfp->R0,(int)cfp->R0[-1]);
    if(cmp >= 0)
        t1len = subarr(temp1,t1len,cfp->R0,(int)cfp->R0[-1]);
    else
        t1len = sub1arr(temp1,t1len,cfp->R0,(int)cfp->R0[-1]);

    cpyarr(rr,rlen,cfp->R0);
    cfp->R0[-1] = rlen;

    qtlen = multbig(temp1,t1len,bb,blen,Qtemp,hilf);
    if(cmp >= 0)
        qtlen = addarr(Qtemp,qtlen,cfp->Q0,(int)cfp->Q0[-1]);
    else
        qtlen = sub1arr(Qtemp,qtlen,cfp->Q0,(int)cfp->Q0[-1]);

    cpyarr(QQ,(int)QQ[-1],cfp->Q0);
    cfp->Q0[-1] = QQ[-1];
    cpyarr(Qtemp,qtlen,QQ);
    QQ[-1] = qtlen;

    atlen = multbig(cfp->AA,(int)cfp->AA[-1],bb,blen,Atemp,hilf);
    atlen = addarr(Atemp,atlen,cfp->A0,(int)cfp->A0[-1]);
    atlen = modbig(Atemp,atlen,N,len,hilf);

    cpyarr(cfp->AA,(int)cfp->AA[-1],cfp->A0);
    cfp->A0[-1] = cfp->AA[-1];
    cpyarr(Atemp,atlen,cfp->AA);
    cfp->AA[-1] = atlen;

    cfp->qrsign = -cfp->qrsign;

    return(qtlen);
}
/*-------------------------------------------------------------------*/
/*
** Rueckgabe = 1, falls quadratischer Rest QQ smooth; sonst = 0
** QQ[-1] enthaelt Laengenangabe
** TODO: big prime variation
*/
PRIVATE int smoothea(QQ,fbas,baslen,easize)
word2 *QQ, *fbas;
int baslen, easize;
{
    word2 Q[ALEN];
    unsigned p;
    int qn, i, bitl, bound;
    word2 r;

    qn = QQ[-1];
    cpyarr(QQ,qn,Q);
    bitl = (qn - 1)*16 + bitlen(Q[qn-1]);
    bound = easize;
    i = 0;
  nochmal:
    while(++i <= bound) {
        p = *fbas++;
        while(modarr(Q,qn,p) == 0)
            qn = divarr(Q,qn,p,&r);
        if(qn == 1 && Q[0] == 1)
            return(1);
    }
    if(bound < baslen) {
        if(bitl - 16*(qn - 1) - bitlen(Q[qn-1]) >= 10) {
            bound = baslen;
            goto nochmal;
        }
        /* else early abort */
    }
    return(0);
}
/*-------------------------------------------------------------------*/
/*
** QQ[-1] contains length of QQ, which must be <= ALEN
** Extracts from QQ all prime factors in fbas
** returns last cofactor u if u < 2**32;
** else returns 0
*/
PRIVATE word4 smooth(QQ,fbas,baslen)
word2 *QQ, *fbas;
int baslen;
{
    word2 Q[ALEN];
    unsigned p;
    int qn, i;
    word2 r;

    qn = QQ[-1];
	if(!qn)
		return 0;
    cpyarr(QQ,qn,Q);
    for(i=0; i<baslen; i++) {
        p = *fbas++;
        while(modarr(Q,qn,p) == 0) {
            qn = divarr(Q,qn,p,&r);
		}
        if(qn == 1 && Q[0] == 1) 
            return(1);
    }
    if(qn <= 2)
        return(big2long(Q,qn));
    else
        return(0);
}
/*-------------------------------------------------------------------*/
/*
** qsign*QQ ist quadratischer Rest = AA**2 mod N,
** der sich mit der Faktorbasis faktorisieren laesst.
** QQ, AA und der zugehoerige Bitvektor werden in
** die Struktur *bmp eingetragen.
** Rueckgabewert: 1, falls QQ vollstaendig faktorisierbar,
**        0  sonst
*/
PRIVATE int bm_insert(bmp,QQ,qsign,AA,aux)
BMDATA *bmp;
word2 *QQ, *AA, *aux;
int qsign;
{
    word2 *Q, *A, *vect, *prime;
    word2 r;
    unsigned p;
    int qn, i, v, alen, baslen;
	size_t bmrk;

	bmrk = bmp->rank;
    Q = bmp->Qarr + bmrk*(bmp->qinc);
    qn = Q[-1] = QQ[-1];
    cpyarr(QQ,qn,Q);
    cpyarr(QQ,qn,aux);
    A = bmp->Aarr + bmrk*(bmp->ainc);
    alen = AA[-1];
    A[-1] = alen;
    cpyarr(AA,(int)A[-1],A);

    vect = bmp->bitmat + bmrk*(bmp->matinc);
    setarr(vect,bmp->vlen,0);
    if(qsign)
        vect[0] = 1;    /* setbit(vect,0); */
    prime = bmp->fbas;
    baslen = bmp->baslen;
    i = 0;
    while(++i<=baslen) {
        p = *prime++;
        v = 0;
        while(modarr(aux,qn,p) == 0) {
            v++;
            qn = divarr(aux,qn,p,&r);
        }
        if(v & 1)
            setbit(vect,i);
        if((qn == 1) && (aux[0] == 1)) {
            return(1);
        }
    }
    return(0);
}
/*-------------------------------------------------------------------*/
#ifdef DBGFILE
int showvect(vect,len)
word2 *vect;
int len;
{
	int i;
    fprintf(dbgf,"0x");
    for(i=len-1; i>=0; i--)
		fprintf(dbgf,"%04X",vect[i]);
	fprintf(dbgf,"\n");
	return len;
}
#endif
/*-------------------------------------------------------------------*/
/*
** Rueckgabe = 0, falls letzte Zeile unabhaengig; sonst = 1
*/
PRIVATE int gausselim(bmp)
BMDATA *bmp;
{
    word2 *v, *vect, *vectb, *piv;
    unsigned pivot, minc;
    int i;
    int vn, v2n, baslen;
	size_t rk;

    minc = bmp->matinc;
    rk = bmp->rank;
    vn = bmp->vlen;
    v2n = 2*vn;
    baslen = bmp->baslen;

    vect = bmp->bitmat + rk * minc;
    vectb = vect + vn;
    setarr(vectb,vn,0);
    setbit(vectb,rk);

    v = bmp->bitmat;
    piv = bmp->piv;
    for(i=0; i<rk; i++) {
        pivot = piv[i];
        if(testbit(vect,pivot))
            xorarr(vect,v2n,v);
        v += minc;
    }
    for(i=0; i<vn; i++) {
        if(vect[i])
            break;
    }
    if(i == vn)
		return(1);	/* then vect is identically zero */
    /* find new pivot */
    for(i=rk; i<=baslen; i++) {
        pivot = piv[i];
        if(testbit(vect,pivot)) {
            piv[i] = piv[rk];
            piv[rk] = pivot;
            break;
        }
    }
    bmp->rank = rk + 1;

    return(0);
}
/*-------------------------------------------------------------------*/
/*
** Tries to find a factor of N
** Hypothesis: The current row of the bit matrix in *bmp
** is linearly dependent from the previous rows.
** fact is a word2 array large enough to hold the factor
** If a factor is found, the number of linear dependent rows
** which contributed to finding the factor is stored in hilf[0].
*/
PRIVATE int getfactor(N,len,bmp,fact,hilf)
word2 *N, *fact, *hilf;
int len;
BMDATA *bmp;
{
    word2 *relat, *temp, *aux, *A, *AA, *Q, *QQ, *XX, *X1, *X2;
    int cmp, k;
	size_t rk, ainc, qinc;
    int count;
    int alen, qlen, q1len, tlen, xlen, x1len, x2len;
    int ll, lll;

    ll = 2*ALEN + 2;
    lll = 4*ALEN;

    A = hilf;
    XX = A + ll;
    X1 = XX + ll;
    X2 = X1 + ll;
    Q = X2 + ll;
    temp = Q + lll;
    aux = temp + lll;

    rk = bmp->rank;
    relat = bmp->bitmat + rk * (bmp->matinc) + (bmp->vlen);
    ainc = bmp->ainc;
    qinc = bmp->qinc;

    AA = bmp->Aarr + rk*ainc;
    alen = AA[-1];
    cpyarr(AA,alen,A);
    AA = bmp->Aarr;
    XX[0] = 1;
    xlen = 1;
    QQ = bmp->Qarr + rk*qinc;
    qlen = QQ[-1];
    cpyarr(QQ,qlen,Q);
    QQ = bmp->Qarr;

    for(count=1,k=0; k<rk; k++) {
        if(testbit(relat,k)) {
            count++;
            /* build AA */
            alen = multbig(A,alen,AA,(int)AA[-1],temp,aux);
            cpyarr(temp,alen,A);
            alen = modbig(A,alen,N,len,aux);

            /* build QQ */
            q1len = QQ[-1];
            cpyarr(QQ,q1len,X1);
            cpyarr(Q,qlen,temp);
            x1len = biggcd(X1,q1len,temp,qlen,aux);
            cpyarr(XX,xlen,X2);
            xlen = multbig(X2,xlen,X1,x1len,XX,aux);
            xlen = modbig(XX,xlen,N,len,aux);

            cpyarr(QQ,q1len,temp);
            x2len = divbig(temp,q1len,X1,x1len,X2,&tlen,aux);
            tlen = divbig(Q,qlen,X1,x1len,temp,&qlen,aux);
            qlen = multbig(temp,tlen,X2,x2len,Q,aux);
            if(qlen + ALEN >= lll) {
            lll += 2*ALEN;
            temp = Q + lll;
            aux = temp + lll;
            }
        }
        AA += ainc;
        QQ += qinc;
    }
    tlen = bigsqrt(Q,qlen,temp,&qlen,aux);
    qlen = multbig(temp,tlen,XX,xlen,Q,aux);
    xlen = modbig(Q,qlen,N,len,aux);
    cpyarr(Q,xlen,XX);

    cmp = cmparr(A,alen,XX,xlen);
    if(cmp == 0)
        return(0);
    cpyarr(A,alen,fact);
    if(cmp > 0)
        k = subarr(fact,alen,XX,xlen);
    else
        k = sub1arr(fact,alen,XX,xlen);
    cpyarr(N,len,temp);
    k = biggcd(fact,k,temp,len,aux);
    if(k > 1 || (k == 1 && fact[0] != 1)) {
        hilf[0] = count;
        return(k);
    }
    else
        return(0);
}
/*------------------------------------------------------------------*/
/*
** Multi-polynomial quadratic sieve factorization
*/
PRIVATE truc Fqsfact(argn)
int argn;
{
    truc *argptr;
    word2 *N;
    size_t buflen, buf2len;
    unsigned d;
    int len, n, sign, enough;

    argptr = argStkPtr-argn+1;
    if(argn >= 2 && *argStkPtr == zero) {
        doreport = 0;
    }
    else {
        doreport = 1;
    }
    len = bigref(argptr,&N,&sign);
    if(len == aERROR) {
        error(qsfactsym,err_int,*argptr);
        return(brkerr());
    }
    d = banalfact(N,len);
    if(d != (unsigned)-1)
        return(mkfixnum(d));

#ifdef M_SMALL
    buflen = stkcheck() - STACKRES;
#else
    buflen = scrbufSize * sizeof(word2);
#endif
    buf2len = auxbufSize * sizeof(word2);
    enough = (buflen >= MEMCHUNK && buf2len >= MEMCHUNK);
    if(!enough) {
        error(qsfactsym,err_memev,voidsym);
        return(brkerr());
    }
    if(len > ALEN) {
        error(qsfactsym,err_2big,*argptr);
        return(brkerr());
    }

#ifdef QTEST
dbgf = fopen("qtest.log","w");
fprintf(dbgf,"N := "); showvect(N,len);
#endif

    n = mpqsfactor(N,len,AriBuf);

#ifdef QTEST
fclose(dbgf);
#endif
    return(mkint(0,AriBuf,n));
}
/*-------------------------------------------------------------------*/
PRIVATE int mpqsfactor(N,len,fact)
word2 *N, *fact;
int len;
{
#ifdef M_SMALL
    word2 stackpiece[MEMCHUNK/sizeof(word2)];
#endif
#ifdef BIGPRIMES
    QSBIGPRIMES qsbp;
    int bpv, idx;
#endif
    BMDATA bm;
    SIEVEDATA sv;
    QSFBAS qsfb;
    word2 *buf1, *buf2, *hilf;
    size_t b1len, b2len, slen, restlen;
    long bitl;
    int k, alen, qlen, baslen, maxrows, srange, b0, b;
    int ret, again = 0;

    bitl = (len - 1)*16 + bitlen(N[len-1]);     /* bitlength of N */
    alen = len;
    qlen = (alen + 1)/2 + 2;        /* !! */

#ifdef M_SMALL
    srange = (MEMCHUNK/sizeof(sievitem))/2;
    sv.srange = srange;
    sv.Sieve = (sievitem *)stackpiece;
    buf1 = AuxBuf;
    b1len = auxbufSize;
    buf2 = AriScratch;
    b2len = scrbufSize/2;
    hilf = AriScratch + b2len;
#else /* M_LARGE */
    buf1 = AuxBuf;
    restlen = auxbufSize;
#ifdef BIGPRIMES
    idx = BPMAXIDX;
    while(--idx >= 0) {
        if(QSbpblen[idx] <= len)
            break;
    }
    b0 = sizeof(QSBP) + 2*sizeof(word2);
    b = ((2*auxbufSize/5)*sizeof(word2))/b0;
    while(idx > 0) {
        if(QShtablen[idx] <= b)
            break;
        else
            idx--;
    }
    bpv = (idx >= 0 ? 1 : 0);
    sv.useBigprim = bpv;
    if(bpv) {
        qlen = alen + 2;        /* !! */
        qsbp.maxrows = QShtablen[idx];
        qsbp.QSBPdata = (QSBP *)AuxBuf;
        b1len = (qsbp.maxrows*sizeof(QSBP))/sizeof(word2);
        buf1 = AuxBuf + b1len;
        qsbp.tablen = QShconst[idx];
        qsbp.hashtab = buf1;
        b2len = qsbp.tablen + 1;    /* even */
        buf1 += b2len;
        k = QSbpbmult[idx];
        qsbp.bpbound = k*BIGPRIMEBOUND0;
        sv.qsbig = &qsbp;
        restlen = auxbufSize - (b1len + b2len);
    }
#endif /* BIGPRIMES */
  nochmal:
	srange = MINSRANGE;
    if(bitl > 40)
        srange += (bitl-40)*(3*bitl)/2;
    if(bitl > 80)
        srange += (bitl-80)*(3*bitl)/2;
    if(again) {
        srange -= srange/5;
    }
/*
    srange = bitl*100;
	srange = MINSRANGE;
    if(bitl > 64)
        srange += (bitl-64)*(bitl/5)*16;
*/
    /* somewhat arbitrary */

    if(srange > MAXSRANGE)
        srange = MAXSRANGE;
    if(2*srange*sizeof(sievitem) > (restlen/3)*sizeof(word2))
        srange = (restlen/6)*sizeof(word2)/sizeof(sievitem);
    srange &= ~0x3;     /* make it a multiple of 4 */
    sv.srange = srange;
    sv.Sieve = (sievitem *)buf1;
    slen = (2*srange)/sizeof(word2);
    buf1 += slen;
    b1len = restlen - slen;

    buf2 = AriScratch;
    if(b1len/8 > scrbufSize) {
        b1len -= scrbufSize;
        hilf = buf1 + b1len;
        b2len = scrbufSize - 16;
    }
    else if((scrbufSize/2)*sizeof(word2) > MEMCHUNK) {
        b2len = scrbufSize - MEMCHUNK/sizeof(word2);
        hilf = buf2 + b2len;
    }
    else {
        b2len = scrbufSize/2;
        hilf = buf2 + b2len;
    }
#endif /* M_LARGE */

    maxrows = bm_alloc(buf1,b1len,buf2,b2len,&bm,alen,qlen);
    b0 = maxrows/16;
	b = 1 + (bitl*bitl)/384;
    if(bitl > 160)
        b += (b*(bitl-160))/80;
    if(again)
        b -= 1;
	if(b > b0)
        b = b0;
    bm.vlen = b;
    baslen = b*16 - 2;
    for(k=0; k<=baslen; k++)
        bm.piv[k] = baslen-k;
/*********bm.piv[k] = k;**********************/

    sv.fbp = &qsfb;
    qsfb.fbas = bm.fbas = hilf;
    qsfb.baslen = bm.baslen = baslen;
    qsfb.fbroot = hilf + b*16;
    qsfb.fblog = (sievitem *)(hilf + b*32);
    hilf += b * (32 + sizeof(sievitem)*16/sizeof(word2));

    ret = qsfact1(N,len,fact,&bm,&sv,hilf);
    if(ret == 0 && again == 0 && bitl <= 144) {
        again = 1;
        goto nochmal;
    }
    else if(ret >= 0)
        return ret;
    else
        return 0;
}
/*---------------------------------------------------------------*/
/*
** If factor is found, it is stored in fact and its
** length is returned;
** if no factor found, returns 0
** if interrupted, returns -1
*/
PRIVATE int qsfact1(N,len,fact,bmp,qsp,hilf)
word2 *N, *fact, *hilf;
int len;
BMDATA *bmp;
SIEVEDATA *qsp;
{
#ifdef BIGPRIMES
    QSBIGPRIMES *qsbigp;
    QPOLY Qpol2;
    word2 Work2[2*ALEN+4];
    word2 *hashtab, *tabptr, *QQ2, *AA2;
    word4 bigpbound, u;
    int sgn2;
    int count2 = 0;
    int useBigprim;
#endif
    QPOLY Qpol;
    QSFBAS *fbp;
    word2 Q0[HLEN], Q1[HLEN], Work[2*ALEN+4];
    word2 *fbase, *fbroot, *QQ, *AA;
	word2 *NN, *hilf1;
    sievitem *fblog, *sieve, *sptr;
    word4 cofac, v;
    int res, haveres, shrieks, maxshrieks;
    int k, n, n8, baslen, qlen, qvlen, srange, count, count1, sgn, xi;
    unsigned p,a;
    sievitem tol,target;

    QQ = Work + 1;
    AA = Work + (ALEN+4);

    fbp = qsp->fbp;
    fbase = fbp->fbas;
    baslen = factorbase(N,len,fbase,fbp->baslen,&p);
    if(p > 1) { /* found small prime divisor */
        fact[0] = p;
        return(1);
    }
    fbp->baslen = baslen;
    fbroot = fbp->fbroot;
    fbroot[0] = 1;
    fblog = fbp->fblog;
    n8 = N[0] & 0x7;
    fblog[0] = (n8 == 1 ? 3 : (n8 == 5 ? 2 : 1));

    for(k=1; k<baslen; k++) {
        p = fbase[k];
        a = modarr(N,len,p);
        fbroot[k] = fp_sqrt(p,a);
        fblog[k] = bitlen(p);
    }
    Qpol.NNlen = len;
    cpyarr(N,len,Qpol.NN);
    qsp->qpol = &Qpol;

    sieve = qsp->Sieve;
    srange = qsp->srange;
    qlen = startqq(N,len,srange,Q0,hilf);
    tol = fblog[baslen-1];

#ifdef BIGPRIMES
    useBigprim = qsp->useBigprim;
    if(useBigprim) {
        QQ2 = Work2 + 1;
        AA2 = Work2 + (ALEN+4);
        Qpol2.NNlen = len;
        cpyarr(N,len,Qpol2.NN);
        qsbigp = qsp->qsbig;
        bigpbound = qsbigp->bpbound;
        u = fbase[baslen-1];
        u *= u;
        if(bigpbound > u) {
            bigpbound = u;
        }
        tol = lbitlen(bigpbound) + (len-BPMINLEN);
        /* somewhat arbitrary */

        hashtab = qsbigp->hashtab;
        for(k=qsbigp->tablen, tabptr=hashtab; k>0; k--)
            *tabptr++ = 0xFFFF;
        qsbigp->row = 0;
        cpyarr(Q0,qlen,qsbigp->Q0);
        qsbigp->Q0[-1] = qlen;
    }   
#endif
    target = bit_length(N,len)/2 + lbitlen((word4)srange) - tol;

    if(doreport)
        qs0mess(srange,fbase[baslen-1],baslen);
	maxshrieks = (baslen < 64 ? 32 : 32 + (baslen/2 - 32)/bitlen(baslen));
    for(v=1, count=count1=shrieks=0; shrieks<maxshrieks; v++) {
        if(INTERRUPT) {
			setinterrupt(0);
        	return -1;
        }
        /* calculate polynomial for next sieving */
  nochmal:
        cpyarr(Q0,qlen,Q1);
        qlen = incarr(Q1,qlen,2);
        qlen = nextqq(N,len,Q1,qlen,Q0,hilf);
        res = mkquadpol(Q0,qlen,&Qpol,hilf);
        if(res > 0) {   /* Q0 divides N */
			cpyarr(Q0,qlen,fact);
			return(qlen);
        }
        else if(res < 0) {  /* possibly Q0 not prime */
			if(doreport)
				tick('\'');
			if(++shrieks>=maxshrieks)
				break;
			else
				goto nochmal;
        }
        dosieve(qsp);
        /* collect sieve results */
        for(sptr=sieve,xi=-srange; xi<srange; xi++,sptr++) {
	        if(*sptr >= target) {
	            haveres = 0;
	            Qpol.xi = xi;
	            qvlen = quadvalue(&Qpol,QQ,&sgn);
				if(!qvlen) {	/****** N square? ************/
					if(doreport)
						tick('`');
					continue;
				}
	            cofac = smooth(QQ,fbase,baslen);
	            if(cofac == 1) {
	            	if((++count1 & 0x3) == 1)
	                	if(doreport)
	                		tick('.');
	            	qresitem(&Qpol,AA);
	            	haveres = 1;
            	}
#ifdef BIGPRIMES
            	else if(useBigprim && (cofac < bigpbound) && (cofac > 1)) {
	            	res = hashbigp(qsbigp,cofac,&Qpol,&Qpol2,hilf);
	            	if(res > 0) {
	                	if((++count2 & 0x3) == 1)
	                		if(doreport)
	                    		tick(':');
	                	qresitem(&Qpol,AA);
	                	quadvalue(&Qpol2,QQ2,&sgn2);
	                	qresitem(&Qpol2,AA2);
	                	sgn = (sgn == sgn2 ? 0 : -1);
	                	combinebp(N,len,cofac,QQ,AA,QQ2,AA2,hilf);
	                	haveres = 1;
	            	}
            	}
#endif /* BIGPRIMES */
            	if(haveres) {
	            	count++;
	            	bm_insert(bmp,QQ,sgn,AA,hilf);
	            	if(gausselim(bmp)) {
	                	shrieks++;
	                	if(doreport)
	                		tick('!');
	                	n = getfactor(N,len,bmp,fact,hilf);
	                	if(n > 0) {
#ifdef BIGPRIMES
	                		if(useBigprim) {
	                    		if(doreport)
	                        		qs2mess(v,count,count2);
	                		}
	                		else
#endif
		                    	if(doreport)
		                    		qs1mess(v,count);
	                		return(n);
	                	}
						if(shrieks >= maxshrieks)
							break;
	            	}
            	}
        	}
    	}
    	if(doreport)
        	counttick(v,bmp);
	}
	/* check if N is a perfect square */
	NN = hilf + 1;
	hilf1 = hilf + len + 2;
	cpyarr(N,len,NN);
	k = is_square(NN,len,fact,hilf1);
    return(k);
}
/*---------------------------------------------------------------*/
PRIVATE int dosieve(qsp)
SIEVEDATA *qsp;
{
    QPOLY *qpol;
    QSFBAS *fbp;
    sievitem *sieve, *sptr, *fblog;
    word2 *fbas, *fbroot, *aa, *bb, *cc;
    word4 u;
    unsigned a1, ainv, b1, binv, p, r, r1, s, xi, xi0, srange, srange2;
    int k, alen, blen, clen, baslen;
    sievitem z;

    srange = qsp->srange;
    srange2 = 2*srange;
    sieve = qsp->Sieve;
    qpol = qsp->qpol;

    aa = qpol->aa; alen = aa[-1];
    bb = qpol->bb; blen = bb[-1];
    cc = qpol->cc; clen = cc[-1];

    fbp = qsp->fbp;
    fbas = fbp->fbas;
    baslen = fbp->baslen;
    fbroot = fbp->fbroot;
    fblog = fbp->fblog;

    z = fblog[0];
    sieve[0] = sieve[srange2-1] = 0;
    xi0 = ((cc[0]&1) == (srange&1) ? 0 : 1);
    for(sptr=sieve+xi0, xi=xi0+1; xi<srange2; xi+=2) {
        *sptr++ = z;
        *sptr++ = 0;
    }

    for(k=1; k<baslen; k++) {
        p = fbas[k];
        r = fbroot[k];
        z = fblog[k];
        s = srange % p;
        if(s)
            s = p - s;
        /* s = (-srange) mod p */
        a1 = modarr(aa,alen,p);
        b1 = modarr(bb,blen,p);
        if(a1) {
            ainv = modinv(a1,p);
            /* ainv = modpow(a1,p-2,p); */
            /* inverse of a1 mod p */
            u = r + (p - b1);
            u *= ainv;
            r1 = u % p;
            /* r1 = (r-b1)*ainv mod p,
            ** this is one root of qpol mod p
            */
            /* sieving with first root */
            xi0 = (r1 >= s ? r1 - s : r1 + (p-s));
            for(sptr=sieve+xi0,xi=xi0; xi<srange2; sptr+=p,xi+=p)
            	*sptr += z;
            /* now calculate the other root of qpol */
            u = (p - r) + (p - b1);
            u *= ainv;
            r1 = u % p;
        }
        else {
            /* qpol mod p is linear, we calculate
            ** the only root of qpol mod p
            ** r1 = cc/(2*bb) (mod p)
            */
            u = modarr(cc,clen,p);
            binv = modinv(2*b1,p);
            /* binv = modpow(2*b1,p-2,p); */
            u *= binv;
            r1 = u % p;
        }
        /* now sieving with second root */
        xi0 = (r1 >= s ? r1 - s : r1 + (p-s));
        for(sptr=sieve+xi0,xi=xi0; xi<srange2; sptr+=p,xi+=p)
            *sptr += z;
    }
    return(0);
}
/*---------------------------------------------------------------*/
#ifdef BIGPRIMES
/*
** inserts bigprime in hash table
** if insertion possible without collision, returns 0
** if matching bigprime is found, returns 1
** if there is collision without match, returns -1
*/
PRIVATE int hashbigp(qsbigp,prim,qpolp,qpolp2,hilf)
QSBIGPRIMES *qsbigp;
word4 prim;
QPOLY *qpolp, *qpolp2;
word2 *hilf;
{
    QSBP *qsdata;
    word2 *hashtab;
    word2 Qtemp[HLEN], pp[2];
    word2 *q0;
    word4 qdiff;
    unsigned idx, row, row0;
    int qlen, q0len, plen;

    qsdata = qsbigp->QSBPdata;
    hashtab = qsbigp->hashtab;
    idx = prim % (qsbigp->tablen);
    row0 = hashtab[idx];
    if(row0 == 0xFFFF) {
        row = qsbigp->row;
        if(row < qsbigp->maxrows)
        hashtab[idx] = row;
        qsdata[row].bprime = prim;
        qsdata[row].x = qpolp->xi;
        qlen = qpolp->qq[-1];
        cpyarr(qpolp->qq,qlen,Qtemp);
        q0 = qsbigp->Q0; q0len = q0[-1];
        qlen = subarr(Qtemp,qlen,q0,q0len);
        qsdata[row].qdiff = big2long(Qtemp,qlen);
        qsbigp->row = ++row;
        return(0);
    }
    else if(prim == qsdata[row0].bprime) {
        qpolp2->xi = qsdata[row0].x;
        qdiff = qsdata[row0].qdiff;
        plen = long2big(qdiff,pp);
        q0 = qsbigp->Q0; q0len = q0[-1];
        cpyarr(q0,q0len,Qtemp);
        qlen = addarr(Qtemp,q0len,pp,plen);
        mkquadpol(Qtemp,qlen,qpolp2,hilf);
        return(1);
    }
    return(-1);
}
/*---------------------------------------------------------------*/
/*
** Hypothesis: QQ and QQ2 smooth (with respect to factor base)
               upto the factor bigprim;
**             QQ = AA**2 mod N, QQ2 = AA2**2 mod N
** The function combines these data to produce (destructively)
** QQ and AA, such that QQ is smooth and QQ = AA**2 mod N
*/
PRIVATE int combinebp(N,len,bigprim,QQ,AA,QQ2,AA2,hilf)
word2 *N;
int len;
word4 bigprim;
word2 *QQ, *AA, *QQ2, *AA2, *hilf;
{
    word2 pp[2];
    word2 *xtemp, *ytemp, *ztemp;
    int alen, a2len, plen, pinvlen, qlen, q2len, rlen;

    qlen = QQ[-1];
    q2len = QQ2[-1];
    xtemp = hilf;
    ytemp = xtemp + 2*len;
    ztemp = ytemp + len;
    hilf += 5*len;

    plen = long2big(bigprim,pp);
    qlen = divbig(QQ,qlen,pp,plen,xtemp,&rlen,hilf);
    cpyarr(xtemp,qlen,QQ);
    q2len = divbig(QQ2,q2len,pp,plen,xtemp,&rlen,hilf);
    cpyarr(xtemp,q2len,QQ2);
    qlen = multbig(QQ,qlen,QQ2,q2len,xtemp,hilf);
    cpyarr(xtemp,qlen,QQ);
    QQ[-1] = qlen;
    alen = AA[-1];
    a2len = AA2[-1];
    alen = multbig(AA,alen,AA2,a2len,xtemp,hilf);
    alen = modbig(xtemp,alen,N,len,hilf);
    pinvlen = modinverse(pp,plen,N,len,ytemp,hilf);
    alen = multbig(xtemp,alen,ytemp,pinvlen,ztemp,hilf);
    alen = modbig(ztemp,alen,N,len,hilf);
    cpyarr(ztemp,alen,AA);
    return(AA[-1] = alen);
}
/*---------------------------------------------------------------*/
#endif  /* BIGPRIMES */
/*---------------------------------------------------------------*/
/*
** Calculates a start q-value, which is
** approx sqrt(sqrt(2*N)/srange) = sqrt(sqrt(N/2)/(srange/2))
*/
PRIVATE int startqq(N,len,srange,qq,hilf)
word2 *N,*qq,*hilf;
int len;
unsigned srange;
{
    word2 NN[ALEN],q0[HLEN];
    word2 rr;
    unsigned sroot;
    int k, len1, dum;

    cpyarr(N,len,NN);
    len1 = shrarr(NN,len,1);
    k = bigsqrt(NN,len1,q0,&dum,hilf);
    k = bigsqrt(q0,k,qq,&dum,hilf);
    sroot = intsqrt((word4)(srange/2))+1;   /* sroot < 2**16 */
    k = divarr(qq,k,sroot,&rr);
    return(k);
}
/*---------------------------------------------------------------*/
/*
** Calculates the smallest odd prime qq, which is >= q0
** and such that jacobi(N,qq) = 1.
** The result is stored in the buffer qq,
** return value is the length of qq.
** hilf is a buffer for auxiliary variables, whose length
** must be >= 11*max(len,q0len)
*/
PRIVATE int nextqq(N,len,q0,q0len,qq,hilf)
word2 *N, *q0, *qq, *hilf;
int len, q0len;
{
    word2 *NN, *QQ, *aux;
    unsigned bound;
    int qqlen, len0;

    qqlen = q0len;
    cpyarr(q0,q0len,qq);
    qq[0] |= 0x1;       /* make it odd */

    bound = (qqlen > 2 ? 0xFFFF : 0x7FF);
  nochmal:
    while(trialdiv(qq,qqlen,3,bound))
        qqlen = incarr(qq,qqlen,2);

    len0 = (len > qqlen ? len : qqlen+1);
    NN = hilf; QQ = hilf + len0; aux = QQ + len0;
    cpyarr(N,len,NN);
    cpyarr(qq,qqlen,QQ);
    if(jacobi(0,NN,len,QQ,qqlen,aux) == 1) {
        if(qqlen <= 2) {
            if(prime32(big2long(qq,qqlen)))
            return(qqlen);
        }
        else if(rabtest(qq,qqlen,aux)) {
            /* not 100% certain that qq is prime */
            return(qqlen);
        }
    }
    qqlen = incarr(qq,qqlen,2);
    goto nochmal;
}
/*---------------------------------------------------------------*/
/*
** Calculates the smallest prime qq = 3 mod 4, which is >= q0
** and such that jacobi(N,qq) = 1.
** The result is stored in the buffer qq,
** return value is the length of qq.
** hilf is a buffer for auxiliary variables, whose length
** must be >= 11*max(len,q0len)
*/
#if 0
PRIVATE int next_qq(N,len,q0,q0len,qq,hilf)
word2 *N, *q0, *qq, *hilf;
int len, q0len;
{
    word2 *NN, *QQ, *aux;
    unsigned bound;
    int qqlen, len0;

    qqlen = q0len;
    cpyarr(q0,q0len,qq);
    qq[0] |= 0x3;       /* make it = 3 mod 4 */

    bound = (qqlen > 2 ? 0xFFFF : 0x7FF);
  nochmal:
    while(trialdiv(qq,qqlen,3,bound))
        qqlen = incarr(qq,qqlen,4);

    len0 = (len > qqlen ? len : qqlen+1);
    NN = hilf; QQ = hilf + len0; aux = QQ + len0;
    cpyarr(N,len,NN);            
    cpyarr(qq,qqlen,QQ); 
    if(jacobi(0,NN,len,QQ,qqlen,aux) == 1) {
        if(qqlen <= 2) {
            if(prime32(big2long(qq,qqlen)))
            return(qqlen);
        }
        else if(rabtest(qq,qqlen,aux)) {
            /* not 100% certain that qq is prime */
            return(qqlen);
        }
    }
    qqlen = incarr(qq,qqlen,4);
    goto nochmal;
}
#endif
/*---------------------------------------------------------------*/
/*
** (p,plen) must be an odd prime such that jacobi(N,p) = 1.
** sptr is a pointer to a struct QPOLY which must contain in the
** fields NNlen and NN[] the number N to be factored.
** The function calculates the coefficients of a quadratic polynomial
**   Q(x) = a*x*x + 2*b*x - c
** such that a = p*p and N = b*b + a*c
** The inverse of (p,len) mod NN is also calculated.
** Return value:
**   0 if OK
**   1 if p and N are not relatively prime
**  -1 in case of error
*/
PRIVATE int mkquadpol(p,plen,sptr,hilf)
word2 *p, *hilf;
int plen;
QPOLY *sptr;
{
    word2 *N, *xx, *zz, *aux;
    int len, qilen, p2len, blen, b2len, clen, rlen, cmp;

    N = sptr->NN;
    len = sptr->NNlen;
    xx = hilf;
    zz = xx + len;
    aux = zz + 4*plen;

    sptr->qq[-1] = plen;
    cpyarr(p,plen,sptr->qq);

    /* now calculate inverse of (p,plen) mod (N,len) */
    qilen = modinverse(p,plen,N,len,xx,aux);
    if(qilen == 0) {
        /* p divides N */
        return(1);
    }
    else {
        cpyarr(xx,qilen,sptr->qinv);
        sptr->qinv[-1] = qilen;
    }

    /* coefficient a is square of p */
    p2len = multbig(p,plen,p,plen,sptr->aa,hilf);
    sptr->aa[-1] = p2len;

    /* coefficient b is square root of N mod p*p */
    blen = fp2Sqrt(p,plen,N,len,zz,aux);
/**    blen = p2sqrt(p,plen,N,len,zz,aux);  **/
    sptr->bb[-1] = blen;
    cpyarr(zz,blen,sptr->bb);

    /* calculate c as (N - b*b)/a */
    b2len = multbig(sptr->bb,blen,sptr->bb,blen,zz,aux);
    cpyarr(N,len,xx);
    cmp = cmparr(xx,len,zz,b2len);
    if(cmp < 0) {   /* this case should not happen */
        return(-2);
    }
    len = subarr(xx,len,zz,b2len);
    clen = divbig(xx,len,sptr->aa,p2len,sptr->cc,&rlen,aux);
    sptr->cc[-1] = clen;
    if(rlen != 0) { /* then probably p was not prime */
        return(-1);
    }
    return(0);
}
/*---------------------------------------------------------------*/
/*
** polp describes a polynomial F(X) = a*X*X + 2*b*X - c,
** polp->xi is an argument xi for this function
** quadvalue calculates F(xi).
** The value F(xi) is stored in QQ and *signp;
** the length of QQ is stored in QQ[-1]
** and is the return value.
*/
PRIVATE int quadvalue(polp,QQ,signp)
QPOLY *polp;
word2 *QQ;
int *signp;
{
    word2 ww1[ALEN], ww2[ALEN];
    word2 *aa, *bb, *cc;
    unsigned u;
    int x, sgn, sgnx, cmp, lenax, len, alen, blen, clen;

    x = polp->xi;
    sgnx = (x < 0 ? -1 : 0);
    u = (sgnx ? -x : x);
    aa = polp->aa; alen = aa[-1];
    bb = polp->bb; blen = bb[-1];
    cc = polp->cc; clen = cc[-1];

    lenax = multlarr(aa,alen,u,ww1);

    cpyarr(bb,blen,ww2);
    blen = shlarr(ww2,blen,1);
    if(!sgnx) {
        len = addarr(ww1,lenax,ww2,blen);
        sgn = 0;
    }
    else if(cmparr(ww1,lenax,ww2,blen) >= 0) {
        len = subarr(ww1,lenax,ww2,blen);
        sgn = -1;
    }
    else {
        len = sub1arr(ww1,lenax,ww2,blen);
        sgn = 0;
    }
    len = multlarr(ww1,len,u,ww1);
    if(len == 0)
        sgn = 0;
    else
        sgn = (sgnx ? -sgn-1 : sgn);

    if(!sgn) {
        cmp = cmparr(ww1,len,cc,clen);
        if(cmp >= 0) {
            len = subarr(ww1,len,cc,clen);
            sgn = 0;
        }
        else {
            len = sub1arr(ww1,len,cc,clen);
            sgn = -1;
        }
    }
    else {
        len = addarr(ww1,len,cc,clen);
        sgn = -1;
    }
    *signp = sgn;
    cpyarr(ww1,len,QQ);
    return(QQ[-1] = len);
}
/*------------------------------------------------------------------*/
/*
** Product of all primes B1 < p <= B2
** and all integers n with isqrt(B1) < n <= isqrt(B2)
** B1 < B2 must be 16-bit integers
*/
PRIVATE int ppexpo(B1,B2,xx)
unsigned B1, B2;
word2 *xx;
{
	unsigned m1,m2,k;
	int len;

	m1 = intsqrt(B1) + 1;
    if(m1 < 2)
        m1 = 2;
    m2 = intsqrt(B2);

	xx[0] = 1;
	len = 1;
	for(k=m1; k<=m2; k++)
		len = multarr(xx,len,k,xx);
	if(++B1 <= 2)
        B1 = 3;
	B1 |= 0x1;
	for(k=B1; k<=B2; k+=2) {
		if(prime16(k))
			len = multarr(xx,len,k,xx);
	}
	return len;
}
/*---------------------------------------------------------------*/
PRIVATE int multlarr(x,n,a,y)
word2 *x, *y;
int n;
unsigned a;
{
#ifdef M_LARGE
    word4 a0, a1, u, v, carry;
    int i;

    if(a <= 0xFFFF)
        return(multarr(x,n,a,y));
#ifdef M32_64
    return(mult4arr(x,n,a,y));
#else /* !M32_64 */
    carry = 0;
    a0 = a & 0xFFFF;
    a1 = a >> 16;
    for(i=0; i<n; i++) {
        u = v = *x++;
        u *= a0;
        v *= a1;
        u += (carry & 0xFFFF);
        *y++ = u & 0xFFFF;
        v += u >> 16;
        carry >>= 16;
        carry += v;
    }
    if(carry) {
        *y++ = carry & 0xFFFF;
        n++;
        if(carry >>= 16) {
            *y = carry;
            n++;
        }
    }
    return(n);
#endif /* ?M32_64 */

#else  /* !M_LARGE; in M_SMALL we always have a < 2**16 */
    return(multarr(x,n,a,y));
#endif /* ?M_LARGE */
}
/*---------------------------------------------------------------*/
/*
** polp describes a polynomial F(X) = a*X*X + 2*b*X - c,
** where a = q*q and b*b + a*c = N. Let qinv := q**-1 mod N.
** The following equation holds:
**      q*q*F(x) = (a*x + b)**2 mod N
** qresitem calculates AA := (a*x + b)*qinv mod N.
** the length of AA is stored in AA[-1].
*/
PRIVATE int qresitem(polp,AA)
QPOLY *polp;
word2 *AA;
{
    word2 ww0[2*ALEN], ww1[ALEN], ww2[ALEN], hilf[ALEN+1];
    word2 *aa, *bb, *qinv, *N;
    unsigned u;
    int x, sgnx, lenax, len, alen, blen, qlen, nlen;

    x = polp->xi;
    sgnx = (x < 0 ? -1 : 0);
    u = (sgnx ? -x : x);
    aa = polp->aa; alen = aa[-1];
    bb = polp->bb; blen = bb[-1];
    qinv = polp->qinv; qlen = qinv[-1];
    N = polp->NN; nlen = polp->NNlen;

    lenax = multlarr(aa,alen,u,ww1);
    cpyarr(ww1,lenax,ww2);

    if(!sgnx) {
        len = addarr(ww2,lenax,bb,blen);
    }
    else if(cmparr(ww2,lenax,bb,blen) >= 0) {
        len = subarr(ww2,lenax,bb,blen);
    }
    else {
        len = sub1arr(ww2,lenax,bb,blen);
    }
    len = multbig(ww2,len,qinv,qlen,ww0,hilf);
    len = modbig(ww0,len,N,nlen,hilf);
    cpyarr(ww0,len,AA); 
    return(AA[-1] = len);
}
/*---------------------------------------------------------------*/
/*
** Calculates a square root of x mod p**2
** Hypothesis: p prime = 3 mod 4, jacobi(x,p) = 1
** The result is stored in z, its length is returned
** The buffer z must have a length >= 4*plen.
**
** The square root z is calculated using the formula
**  z = x ** ((p*p - p + 2)/4) mod p**2
*/
#if 0
PRIVATE int p2sqrt(p,plen,x,xlen,z,hilf)
word2 *p, *x, *z, *hilf;
int plen, xlen;
{
    word2 *xx, *ex, *p2, *aux;
    int exlen, p2len, zlen;

    xx = hilf;
    p2len = 2*plen;
    p2 = xx + (xlen > p2len ? xlen : p2len);
    ex = p2 + p2len;
    aux = ex + p2len;

    p2len = multbig(p,plen,p,plen,p2,aux);
    cpyarr(x,xlen,xx);
    xlen = modbig(xx,xlen,p2,p2len,aux);
    cpyarr(p2,p2len,ex);
    exlen = subarr(ex,p2len,p,plen);
    exlen = incarr(ex,exlen,2);
    exlen = shrarr(ex,exlen,2);
    zlen = modpower(xx,xlen,ex,exlen,p2,p2len,z,aux);
    return(zlen);
}
#endif
/*------------------------------------------------------------------*/
/*
** Returns 0 if (N,len) is not a square
** If return value k > 0, then (N,len) is the square of (root,k)
*/
PRIVATE int is_square(N,len,root,hilf)
word2 *N, *root, *hilf;
int len;
{
	unsigned M = 15015; /* M=3*5*7*11*13 */
	unsigned a;
	int k, rlen;

	if(len <= 0)
		return 0;
	if((N[0] & 1) == 0) {
		if((N[0] & 0x3) != 0)
			return 0;
	}
	else if((N[0] & 0x7) != 1)
		return 0;
	a = modarr(N,len,M);
	if(jac(a,13) == -1 || jac(a,11) == -1 || jac(a,7) == -1 ||
	   jac(a,5) == -1 || jac(a,3) == -1)
		return 0;
	k = bigsqrt(N,len,root,&rlen,hilf);
	if(rlen == 0)
		return k;
	else
		return 0;
}
/*------------------------------------------------------------------*/
PRIVATE truc Fecfactor(argn)
int argn;
{
	struct vector *vec;
	truc *ptr;
    truc *argptr;
    word2 *N, *z, *xx, *x0, *cc, *hilf;
    word2 aa[2];
    unsigned u, v, bound, bigbound, bound2, anz;
    int k, m, n, nlen, alen, xlen, x0len, zlen, sign, bitl;

    EPOINT Epoint[MAXDIFF/2 + 1];
    ECN ecN;
    int  mhdiff, ret, usebpv;

    argptr = argStkPtr-argn+1;
    if(argn >= 2 && *argStkPtr == zero) {
        doreport = 0;
        argn--;
    }
    else {
        doreport = 1;
        if(argn == 4)
            argn--;
    }
    nlen = bigref(argptr,&N,&sign);
    if(nlen == aERROR) {
        error(ecfactsym,err_int,*argptr);
        return brkerr();
    }
    else if(nlen >= aribufSize/10) {
        error(ecfactsym,err_ovfl,*argptr);
        return brkerr();
    }
    else if(nlen == 0) {
        return zero;
    }
    else {
        u = banalfact(N,nlen);
        if(u != (unsigned)-1)
            return(mkfixnum(u));
    }
    bitl = (nlen-1)*16 + bitlen(N[nlen-1]);
    bound = 1;
    bigbound = 1;
    if(argn>=2 && *FLAGPTR(argptr+1) == fVECTOR) {
    	vec = (struct vector *)TAddress(argptr+1);
	    n = vec->len;
        if(n != 2) {
            error(ecfactsym,"vector of length 2 expected",argptr[1]);
            return brkerr();
        }
    	ptr = &(vec->ele0);
        if(chkints(ecfactsym,ptr,2) == aERROR)
            return(brkerr());
        n = bigref(ptr,&z,&sign);
        if(n <= 1 && n) {
            bound = *z;
            if(bound < 50)
                bound = 50;
        }
        n = bigref(ptr+1,&z,&sign);
        if(n <= 2) {
            bigbound = big2long(z,n);
            if(bigbound > 0 && bigbound < 97)
                bigbound = 100;
        }
        argptr += 2;
        argn -= 2;
    }
    else {
        argptr++;
        argn -= 1;
    }
    if(argn >= 1) {
        n = bigref(argptr,&z,&sign);
        if(n == aERROR) {
            error(ecfactsym,err_int,*argptr);
            return brkerr();
        }
        else if(n <= 1 && n) {
            anz = *z;
        }
        else {
            anz = 0xFFFF;
        }
    }
    else {
        anz = (bitl <= 64 ? 64 : bitl);
    }

    if(bound == 1) {
        bound = 100;
        if(bitl > 32)
            bound += (bitl-32)*16;
    }
    else if(bound < 50)
        bound = 50;
    else if(bound > 64000)
        bound = 64000;

    if(bigbound == 0) {
        usebpv = 0;
    }
    else if(bigbound == 1) {
        usebpv = (bitl > 64 ? 1 : 0);
    }
    else {
        usebpv = 1;
    }
    if(usebpv) {
        if(bigbound == 1) {
            m = 2 + (bound >> 9);
            if(m > 10)
                m = 10;
            bigbound = m*bound;
        }
        else if(bigbound > 0x1000000) {
            bigbound = 0x1000000;
        }
    }
    x0 = AriScratch;
    xx = x0 + 2*nlen + 2;
    cc = x0 + 4*nlen + 4;
    hilf = x0 + 6*nlen + 6;

    if(usebpv)
        mhdiff = ecbpvalloc(Epoint,AuxBuf,auxbufSize,nlen,&bigbound);
    ecN.N = N;
    ecN.nlen = nlen;
    ecN.aa = aa;
    ecN.cc = cc;

    if(doreport) {
        if(usebpv)
            ec1mess(bound, bigbound);
        else
            ec0mess(bound);
    }
    u = 3 + random4(0x1000000);
    alen = long2big(u,aa);
    v = 1 + random2(64000);
    /* here nlen > 1 */
    x0[nlen-1] = 1;
    for(k=0; k<=nlen-2; k++)
        x0[k] = random2(0xFFFF);
    x0len = nlen;
    for(k=0; k<anz; k++) {
        if(INTERRUPT) {
			setinterrupt(0);
        	break;
        }
        if(doreport) {
            if(k % 100 == 99) {
                counttick0((k+1)/100);
            }
            else
                tick('.');
        }

        cpyarr(x0,x0len,xx);
        xlen = x0len;
        zlen = ecfacta(N,nlen,aa,alen,xx,xlen,&bound,hilf);
        if(zlen < 0) {
            n = -zlen-1;
            cpyarr(xx,n,AriBuf);
            if(doreport) {
                u = big2long(aa,alen);
                ec2mess(u,bound);
            }
            return mkint(0,AriBuf,n);
        }
        /* goto big prime variation */
        if(usebpv == 0)
            goto skipbpv;
        if(doreport)
            tick(':');
        ecN.alen = alen;
        xlen = zlen;
        ret = ECNx2c(&ecN,xx,xlen,hilf);
        if(ret == 0) {
            goto skipbpv;
        }
        else if(ret < 0) {
            cpyarr(ecN.cc,ecN.clen,AriBuf);
            n = ecN.clen;
            return mkint(0,AriBuf,n);
        }
        bound2 = bigbound;
        ret = ecfactbpv(&ecN,Epoint,&bound2,mhdiff,xx,xlen,hilf);
        if(ret < 0) {
            n = -ret-1;
            cpyarr(xx,n,AriBuf);
            if(doreport) {
                u = big2long(aa,alen);
                ec3mess(u,bound2);
            }
            return mkint(0,AriBuf,n);
        }
  skipbpv:
        x0len = incarr(x0,x0len,v);
        alen = incarr(aa,alen,1);
    }
    return zero;
}
/*------------------------------------------------------------------*/
/*
** Calculates a multiple of the point with x-ccordinate (xx,xlen)
** on the elliptic curve with parameter (aa,alen).
** The new x-coordinate is stored in xx, its length is returned.
** If during the calculation a factor (zz,zlen) of N is detected,
** it is stored in xx; the return value is -zlen-1.
** Space xx must be at least have length len
*/
PRIVATE int ecfacta(N,len,aa,alen,xx,xlen,pbound,hilf)
word2 *N, *aa, *xx, *hilf;
int len, alen, xlen;
unsigned *pbound;
{
    unsigned inc = 128;
    int exmaxlen = inc/5;
    int exlen, zlen;
    unsigned B1, B2, bound;
    word2 *ex, *zz;

    ex = hilf;
    zz = ex + exmaxlen;
    hilf = zz + len + 1;

    bound = *pbound;
    for(B1=0; B1<bound; B1 += inc) {
        B2 = B1+inc;
        if(B2 > bound)
            B2 = bound;
        exlen = ppexpo(B1,B2,ex);
        zlen = pemult(xx,xlen,ex,exlen,aa,alen,N,len,zz,hilf);
        xlen = (zlen >= 0 ? zlen : -zlen-1);
        cpyarr(zz,xlen,xx);
        if(zlen < 0) {
            *pbound = B2;
            if(cmparr(N,len,zz,xlen) == 0 || (zz[0]==1 && xlen == 1))
                zlen = 0;
            break;
        }
    }
    return zlen;
}
/*-----------------------------------------------------------------*/
PRIVATE int ecbpvalloc(pEpoint, buf, buflen, nlen, pbound)
EPOINT *pEpoint;
word2 *buf;
size_t buflen;
int nlen;
unsigned *pbound;
{
    word2 *xx, *yy;
    size_t totsize, anz0;
    int maxdiff, k, m, n;
    unsigned bound;

    k = ECMAXIDX - 1;
    bound = *pbound;
    if(bound > ECbpbound[k]) {
        bound = ECbpbound[k];
    }
    else {
        while(k > 0 && bound <= ECbpbound[k-1])
            k--;
    }
    maxdiff = ECmdiff[k];
    anz0 = buflen/(nlen + 1);
    if(anz0 <= maxdiff) {
        if(anz0 <= ECmdiff[0])
            return -1;
        for(m=k-1; m>=0; m--) {
            if(anz0 > ECmdiff[m]) {
                maxdiff = ECmdiff[m];
                if(bound > ECbpbound[m])
                    bound = ECbpbound[m];
                break;
            }
        }
    }
    xx = buf;
    yy = buf + nlen + 1;
    m = 2*nlen + 2;
    for(n=0; n<=maxdiff/2; n++) {
        pEpoint[n].xx = xx;
        pEpoint[n].yy = yy;
        xx += m;
        yy += m;
    }
    *pbound = bound;
    return maxdiff/2;
}
/*-----------------------------------------------------------------*/
/*
** big prime variation for EC factoring
** In case a factor is found, it is stored in xx,
** with return value -xlen-1.
** If no factor found, return value is >= 0
*/
PRIVATE int ecfactbpv(pecN,pEpoint,pbound,hdiff,xx,xlen,hilf)
ECN *pecN;
EPOINT *pEpoint;
word2 *xx, *hilf;
int xlen, hdiff;
unsigned *pbound;
{
    EPOINT Z;
    EPOINT *pEtemp;
    word2 *zz;
    int k, ret, nlen;
    unsigned q, bound;
    int found = 0;

    nlen = pecN->nlen;
    Z.xx = hilf;
    Z.yy = hilf + nlen + 1;
    hilf += 2*nlen + 2;

    cpyarr(xx,xlen,Z.xx);
    Z.xlen = xlen;
    Z.yy[0] = 1;
    Z.ylen = 1;

    pEtemp = pEpoint + 1;
    cpyarr(xx,xlen,pEtemp->xx);
    pEtemp->xlen = xlen;
    pEtemp->yy[0] = 1;
    pEtemp->ylen = 1;

    ret = ECNdup(pecN,pEtemp,hilf);
    if(ret == -2) {
        zz = pEtemp->yy;
        xlen = pEtemp->ylen;
        found = 1;
        *pbound = 2;
        goto ausgang;
    }

    for(k=2; k<=hdiff; k++) {
        cpyarr(pEtemp->xx,pEtemp->xlen,pEpoint[k].xx);
        cpyarr(pEtemp->yy,pEtemp->ylen,pEpoint[k].yy);
        pEpoint[k].xlen = pEtemp->xlen;
        pEpoint[k].ylen = pEtemp->ylen;
    }
    ret = ECNdup(pecN,pEpoint+2,hilf);
    if(ret == -2) {
        zz = pEpoint[2].yy;
        xlen = pEpoint[2].ylen;
        found = 1;
        *pbound = 2;
        goto ausgang;
    }
    for(k=3; k<=hdiff; k++) {
        ret = ECNadd(pecN,pEpoint+k,pEpoint+(k-1),hilf);
        if(ret == -2) {
            zz = pEpoint[k].yy;
            xlen = pEpoint[k].ylen;
            found = 1;
            q = fact16(k);
            *pbound = (q ? q : k);
            goto ausgang;
        }
    }
    q = 1;
    bound = *pbound-2;
    while(q <= bound) {
        k = 1;
        q += 2;
        while(!prime32(q)) {
            q += 2;
            k++;
        }
        ret = ECNadd(pecN,&Z,pEpoint+k,hilf);
        if(ret == -2) {
            xlen = Z.ylen;
            zz = Z.yy;
            *pbound = q;
            found = 1;
            break;
        }
    }
  ausgang:
    if(found) {
        cpyarr(zz,xlen,xx);
        return(-xlen-1);
    }
    else
        return xlen;
}
/*-----------------------------------------------------------------*/
/*
** Given x, a an N, calculates c such that
**  c = x**3 + a*x**2 + x mod N
** a and N are handed in through pecN, the result
** c is stored in pecN
** Return value is the length of c
** If c == 0, a factor of N may be found. This factor
** is then stored in place of c and -2 is returned.
** If c == 0 and no factor is found, the return value is 0
*/
PRIVATE int ECNx2c(pecN,xx,xlen,hilf)
ECN *pecN;
word2 *xx, *hilf;
int xlen;
{
    word2 *yy, *zz, *N;
    int alen, ylen, zlen, nlen;

    if(xlen == 0)
        return 0;
    N = pecN->N;
    nlen = pecN->nlen;

    yy = hilf;
    zz = hilf + nlen + 2;
    hilf += 3*nlen + 4;
    if(xlen > nlen)
        xlen = modbig(xx,xlen,N,nlen,hilf);
    alen = pecN->alen;
    cpyarr(pecN->aa,alen,yy);
    ylen = addarr(yy,alen,xx,xlen);
    zlen = multbig(yy,ylen,xx,xlen,zz,hilf);
    zlen = modbig(zz,zlen,N,nlen,hilf);
    ylen = incarr(zz,zlen,1);
    cpyarr(zz,ylen,yy);
    /* yy contains x**2 + a*x + 1 */
    zlen = multbig(yy,ylen,xx,xlen,zz,hilf);
    zlen = modbig(zz,zlen,N,nlen,hilf);
    if(zlen == 0) {
        cpyarr(N,nlen,zz);
        if(ylen == 0) {
            cpyarr(xx,xlen,yy);
            ylen = xlen;
        }
        ylen = biggcd(yy,ylen,zz,nlen,hilf);
        if(yy[0] == 1 && ylen == 1)
            return 0;
        else {
            cpyarr(yy,ylen,pecN->cc);
            pecN->clen = ylen;
            return -2;
        }
    }
    cpyarr(zz,zlen,pecN->cc);
    return (pecN->clen = zlen);
}
/*-----------------------------------------------------------------*/
/*
** Calculates destructively Z1 := Z1 + Z2
** Returns pZ1->xlen
**   >= 0, if result is an affine point
**   -1, if Origin
**   -2, if divisor of N detected
**       This divisor is then stored in (pZ1->yy,pZ1->ylen)
*/
/*
    (x,y) := (x1,y1) add (x2,y2)

    m := mod_inverse(x2-x1,N);
    m := (y2 - y1)*m mod N;
    x := (c*m*m - a - x1 - x2) mod N;
    y := (- y1 - m*(x - x1)) mod N;
*/
PRIVATE int ECNadd(pecN,pZ1,pZ2,hilf)
ECN *pecN;
EPOINT *pZ1, *pZ2;
word2 *hilf;
{
    word2 *N, *xx, *yy, *zz, *slope;
    int x1len, x2len, zlen, nlen, slen;
    int xlen, ylen, cmp, cmp1;

    x1len = pZ1->xlen;
    x2len = pZ2->xlen;
    if(x2len < 0 || x1len < 0) {
        if(x2len == -1)
            return pZ1->xlen;
        else if(x2len == -2) {
            cpyarr(pZ2->yy,pZ2->ylen,pZ1->yy);
            pZ1->ylen = pZ2->ylen;
            return (pZ1->xlen = -2);
        }
        if(x1len == -1) {
            cpyarr(pZ2->yy,pZ2->ylen,pZ1->yy);
            pZ1->ylen = pZ2->ylen;
            cpyarr(pZ2->xx,x2len,pZ1->xx);
            return (pZ1->xlen = x2len);
        }
        else if(x1len == -2)
            return x1len;
    }

    N = pecN->N;
    nlen = pecN->nlen;
    xx = hilf;
    yy = hilf + 2*nlen + 2;
    zz = hilf + 4*nlen + 4;
    slope = hilf + 6*nlen + 6;
    hilf += 8*nlen + 8;

    xlen = x2len;
    cpyarr(pZ2->xx,xlen,xx);
    cmp = cmparr(pZ1->xx,x1len,pZ2->xx,x2len);
    if(cmp > 0) {
        xlen = sub1arr(xx,xlen,pZ1->xx,x1len);
        xlen = sub1arr(xx,xlen,N,nlen);
    }
    else if(cmp < 0)
        xlen = subarr(xx,xlen,pZ1->xx,x1len);
    /* xx contains (x2 - x1) */

    ylen = pZ2->ylen;
    cpyarr(pZ2->yy,ylen,yy);
    cmp1 = cmparr(pZ1->yy,pZ1->ylen,yy,ylen);
    if(cmp1 > 0) {
        ylen = sub1arr(yy,ylen,pZ1->yy,pZ1->ylen);
        ylen = sub1arr(yy,ylen,N,nlen);
    }
    else if(cmp1 < 0)
        ylen = subarr(yy,ylen,pZ1->yy,pZ1->ylen);
    /* yy contains (y2 - y1) */

    if(cmp == 0) {
        if(cmp1 == 0)
            return ECNdup(pecN,pZ1,hilf);
        else {
            cpyarr(N,nlen,xx);
            ylen = biggcd(yy,ylen,xx,nlen,hilf);
            cpyarr(yy,ylen,pZ1->yy);
            pZ1->ylen = ylen;
            return (pZ1->xlen = -2);
        }
    }
    slen = modinverse(xx,xlen,N,nlen,slope,hilf);
    if(slen == 0) {
        cpyarr(N,nlen,yy);
        xlen = biggcd(xx,xlen,yy,nlen,hilf);
        cpyarr(xx,xlen,pZ1->yy);
        pZ1->ylen = xlen;
        return (pZ1->xlen = -2);
    }
    slen = modmultbig(slope,slen,yy,ylen,N,nlen,xx,hilf);
    cpyarr(xx,slen,slope);
    /* slope = (y2-y1)/(x2-x1) */
    zlen = multbig(slope,slen,slope,slen,zz,hilf);
    zlen = modbig(zz,zlen,N,nlen,hilf);
    cpyarr(zz,zlen,xx);
    zlen = multbig(xx,zlen,pecN->cc,pecN->clen,zz,hilf);
    /* zz contains c*slope**2 */
    cpyarr(pZ1->xx,x1len,xx);
    xlen = addarr(xx,x1len,pZ2->xx,x2len);
    xlen = addarr(xx,xlen,pecN->aa,pecN->alen);
    xlen = modnegbig(xx,xlen,N,nlen,hilf);
    /* xx contains -a - x1 - x2 */
    zlen = addarr(zz,zlen,xx,xlen);
    zlen = modbig(zz,zlen,N,nlen,hilf);
    /* zz contains new x = c*slope**2 - a - x1 - x2 */
    cpyarr(pZ1->xx,x1len,xx);
    cmp = cmparr(zz,zlen,xx,x1len);
    if(cmp >= 0)
        xlen = sub1arr(xx,x1len,zz,zlen);
    else {
        xlen = subarr(xx,x1len,zz,zlen);
        xlen = sub1arr(xx,xlen,N,nlen);
    }
    /* xx contains (x - x1) */
    cpyarr(zz,zlen,pZ1->xx);
    pZ1->xlen = zlen;
    zlen = multbig(xx,xlen,slope,slen,zz,hilf);
    /* zz contains slope*(x - x1) */
    zlen = addarr(zz,zlen,pZ1->yy,pZ1->ylen);
    zlen = modnegbig(zz,zlen,N,nlen,hilf);
    /* zz contains (-y1 - slope*(x - x1)) mod N */
    cpyarr(zz,zlen,pZ1->yy);
    pZ1->ylen = zlen;
    return pZ1->xlen;
}
/*-----------------------------------------------------------------*/
/*
** Calculates destructively Z := Z + Z
** Returns pZ->xlen
*/
/*
    z := 2*c*y1;
    m := mod_inverse(z,N);
    Pprim := (((3*x1 + 2*a)*x1) + 1) mod N;
    m := Pprim*m mod N;
    x := (c*m*m - a - 2*x1) mod N;
    y := (- y1 - m*(x - x1)) mod N;
*/
PRIVATE int ECNdup(pecN,pZ,hilf)
ECN *pecN;
EPOINT *pZ;
word2 *hilf;
{
    word2 *N, *xx, *yy, *zz, *slope;
    int x1len, alen, nlen, slen;
    int xlen, ylen, zlen, cmp;

    if(pZ->ylen == 0)
        return (pZ->xlen = -1);
    else if(pZ->xlen < 0)
        return pZ->xlen;

    N = pecN->N;
    nlen = pecN->nlen;
    xx = hilf;
    yy = hilf + nlen + 2;
    zz = hilf + 2*nlen + 4;
    slope = hilf + 4*nlen + 6;
    hilf += 5*nlen + 8;
    zlen = multbig(pecN->cc,pecN->clen,pZ->yy,pZ->ylen,zz,hilf);
    zlen = shiftarr(zz,zlen,1);
    /* zz contains 2*c*y1 */
    slen = modinverse(zz,zlen,N,nlen,slope,hilf);
    /* slope contains 1/(2*c*y1) */
    if(slen == 0) {
        cpyarr(N,nlen,yy);
        zlen = biggcd(zz,zlen,yy,nlen,hilf);
        cpyarr(zz,zlen,pZ->yy);
        pZ->ylen = zlen;
        return (pZ->xlen = -2);
    }
    x1len = pZ->xlen;
    cpyarr(pZ->xx,x1len,xx);
    xlen = multarr(xx,x1len,3,xx);
    alen = pecN->alen;
    cpyarr(pecN->aa,alen,yy);
    ylen = shiftarr(yy,alen,1);
    xlen = addarr(xx,xlen,yy,ylen);
    /* xx contains (3*x1 + 2*a) */
    zlen = multbig(xx,xlen,pZ->xx,x1len,zz,hilf);
    zlen = incarr(zz,zlen,1);
    zlen = modbig(zz,zlen,N,nlen,hilf);
    /* zz contains Pprim = ((3*x1 + 2*a)*x1 + 1) mod N */
    cpyarr(zz,zlen,xx);
    slen = multbig(xx,zlen,slope,slen,zz,hilf);
    slen = modbig(zz,slen,N,nlen,hilf);
    cpyarr(zz,slen,slope);
    /* slope contains Pprim/(2*c*y1) */

    zlen = multbig(slope,slen,slope,slen,zz,hilf);
    zlen = modbig(zz,zlen,N,nlen,hilf);
    cpyarr(zz,zlen,xx);
    zlen = multbig(xx,zlen,pecN->cc,pecN->clen,zz,hilf);
    /* zz contains c*slope**2 */

    cpyarr(pZ->xx,x1len,xx);
    xlen = shiftarr(xx,x1len,1);
    xlen = addarr(xx,xlen,pecN->aa,alen);
    xlen = modnegbig(xx,xlen,N,nlen,hilf);
    /* xx contains -a - 2*x1 */
    zlen = addarr(zz,zlen,xx,xlen);
    zlen = modbig(zz,zlen,N,nlen,hilf);
    /* zz contains new x = c*slope**2 - a - 2*x1 */

    cpyarr(pZ->xx,x1len,xx);
    cmp = cmparr(zz,zlen,xx,x1len);
    if(cmp >= 0)
        xlen = sub1arr(xx,x1len,zz,zlen);
    else {
        xlen = subarr(xx,x1len,zz,zlen);
        xlen = sub1arr(xx,xlen,N,nlen);
    }
    /* xx contains (x - x1) */
    cpyarr(zz,zlen,pZ->xx);
    pZ->xlen = zlen;
    zlen = multbig(xx,xlen,slope,slen,zz,hilf);
    /* zz contains slope*(x - x1) */
    zlen = addarr(zz,zlen,pZ->yy,pZ->ylen);
    zlen = modnegbig(zz,zlen,N,nlen,hilf);
    /* zz contains new y = (-y1 - slope*(x - x1)) mod N */

    cpyarr(zz,zlen,pZ->yy);
    pZ->ylen = zlen;
    return pZ->xlen;
}
/*------------------------------------------------------------------*/
PRIVATE int ECNmult(pecN, pZ, ex, exlen, hilf)
ECN *pecN;
EPOINT *pZ;
word2 *ex, *hilf;
int exlen;
{
    word2 *xx, *yy;
    int xlen, ylen, nlen, m, bitl, k, ret;
    EPOINT Z0;

    if(exlen == 0) {
        pZ->xlen = -1;
        return -1;
    }
    nlen = pecN->nlen;
    xlen = Z0.xlen = pZ->xlen;
    ylen = Z0.ylen = pZ->ylen;
    m = (xlen > nlen ? xlen : nlen);
    m = (ylen > m ? ylen : m);
    xx = hilf;
    yy = hilf + m + 2;
    hilf += 2*m + 4;
    Z0.xx = xx;
    Z0.yy = yy;
    cpyarr(pZ->xx,xlen,xx);
    cpyarr(pZ->yy,ylen,yy);

    bitl = (exlen-1)*16 + bitlen(ex[exlen-1]);
    for(k=bitl-2; k>=0; k--) {
        ret = ECNdup(pecN,pZ,hilf);
        if(ret == -2)
            return ret;
        if(testbit(ex,k)) {
            ret = ECNadd(pecN,pZ,&Z0,hilf);
            if(ret == -2)
                return ret;
        }
    }
    return pZ->xlen;
}
/********************************************************************/

