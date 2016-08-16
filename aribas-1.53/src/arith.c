/****************************************************************/
/* file arith.c

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

Email	forster@mathematik.uni-muenchen.de
*/
/****************************************************************/
/*
** arith.c
** arithmetic functions
**
** date of last change
** 1995-03-11:	set_floatprec, get_floatprec
** 1997-02-20:	fixed bug in Frandom()
** 1997-02-25:	modified Fmod()
** 1997-03-31:	error message in Gcompare
** 1997-04-13:	reorg (newintsym)
** 2001-01-06:  multiprec floats
** 2002-02-24:  vector arithmetic, 1st step
** 2002-04-19:  divide(x,y)
** 2003-02-11:  addition of gf2n_int's
** 2003-02-15:  Fdecode made safe wrt garbage collection
** 2003-11-21:  removed bug in vecdiv
*/

#include "common.h"

PUBLIC void iniarith	_((void));
PUBLIC truc addints		_((truc *ptr, int minflg));
PUBLIC truc scalintvec 	_((truc *ptr1, truc *ptr2));
PUBLIC truc Gvecmod     _((int flg));
PUBLIC unsigned random2 _((unsigned u));
PUBLIC unsigned random4 _((unsigned u));
PUBLIC int cmpnums		_((truc *ptr1, truc *ptr2, int type));


PUBLIC truc zero, constone;

PUBLIC truc sfloatsym, dfloatsym, lfloatsym, xfloatsym;
PUBLIC truc realsym, integsym, int_sym;
PUBLIC truc plussym, minussym, uminsym;
PUBLIC truc timessym, divsym, modsym, divfsym, powersym;
PUBLIC truc ariltsym, arigtsym, arilesym, arigesym;

PUBLIC long maxfltex, maxdecex, exprange;

/*-------------------------------------------------------------*/

PRIVATE truc Fplus		_((void));
PRIVATE truc addfloats	_((truc *ptr, int minflg));
PRIVATE truc Fminus		_((void));
PRIVATE truc Fnegate	_((void));
PRIVATE truc Sinc		_((void));
PRIVATE truc Sdec		_((void));
PRIVATE truc Sincaux	_((truc symb, int s));
PRIVATE truc Fabsolute	_((void));
PRIVATE truc Fmax		_((int argn));
PRIVATE truc Fmin		_((int argn));
PRIVATE truc Fmaxint    _((void));
PRIVATE truc Gminmax	_((truc symb, int argn));
PRIVATE truc Fodd		_((void));
PRIVATE truc Feven		_((void));
PRIVATE int odd			_((truc *ptr));
PRIVATE truc Ftimes		_((void));
PRIVATE truc multints	_((truc *ptr));
PRIVATE truc multfloats	_((truc *ptr));
PRIVATE truc Fsum		_((void));
PRIVATE truc sumintvec	_((truc *argptr, int argn));
PRIVATE truc sumfltvec  _((truc *argptr, int argn));
PRIVATE truc Fprod		_((void));
PRIVATE truc prodintvec	   _((truc *argptr, int argn));
PRIVATE truc prodfloatvec  _((truc *argptr, int argn));
PRIVATE truc Fdivf		_((void));
PRIVATE truc Fdiv		_((void));
PRIVATE truc Fdivide	_((void));
PRIVATE truc Fmod		_((void));
PRIVATE truc divide		_((truc *ptr, int tflag));
PRIVATE truc modout		_((truc *ptr));
PRIVATE truc divfloats	_((truc *ptr));
PRIVATE truc Ftrunc		_((void));
PRIVATE truc Fround		_((void));
PRIVATE truc Ffloor		_((void));
PRIVATE truc Ffrac		_((void));
PRIVATE truc Gtruncaux	_((truc symb));
PRIVATE void intfrac	_((numdata *npt1, numdata *np2));
PRIVATE int roundhalf	_((numdata *nptr));
PRIVATE void floshiftint  _((numdata *nptr));
PRIVATE truc Fpower		_((void));
PRIVATE truc exptints	_((truc *ptr, unsigned a));
PRIVATE truc exptfloats	_((truc *ptr));
PRIVATE int exptovfl	_((word2 *x, int n, unsigned a));
PRIVATE truc Fisqrt		_((void));
PRIVATE int cmpfloats	_((truc *ptr1, truc *ptr2, int prec));
PRIVATE truc Farilt		_((void));
PRIVATE truc Farigt		_((void));
PRIVATE truc Farile		_((void));
PRIVATE truc Farige		_((void));
PRIVATE int  Gcompare	_((truc symb));
PRIVATE void inirandstate  _((word2 *rr));
PRIVATE void nextrand	_((word2 *rr, int n));
PRIVATE truc Frandom	_((void));
PRIVATE truc Frandseed	_((int argn));
PRIVATE int objfltprec	_((truc obj));
PRIVATE truc Ffloat		_((int argn));
PRIVATE truc Fsetfltprec  _((int argn));
PRIVATE truc Fgetfltprec  _((int argn));
PRIVATE truc Fmaxfltprec  _((void));
PRIVATE truc Fsetprnprec  _((void));
PRIVATE truc Fgetprnprec  _((void));
PRIVATE int precdesc	_((truc obj));
PRIVATE truc Fdecode	_((void));
PRIVATE truc Fbitnot	_((void));
PRIVATE truc Fbitset	_((void));
PRIVATE truc Fbitclear	_((void));
PRIVATE truc Gbitset	_((truc symb));
PRIVATE truc Fbittest	_((void));
PRIVATE truc Fbitshift	_((void));
PRIVATE truc Fbitlength	  _((void));
PRIVATE truc Fbitcount	  _((void));
PRIVATE truc Fcardinal	_((void));
PRIVATE truc Finteger	_((void));
PRIVATE truc Gcardinal	_((truc symb));
PRIVATE truc Fbitand	_((void));
PRIVATE truc Fbitor		_((void));
PRIVATE truc Fbitxor	_((void));
PRIVATE truc Gboole		_((truc symb, ifunaa boolfun));

PRIVATE truc negatevec  _((truc *ptr));
PRIVATE truc addvecs    _((truc sym, truc *ptr));
PRIVATE truc addintvecs _((truc *ptr1, truc *ptr2));
PRIVATE truc addfltvecs _((truc *ptr1, truc *ptr2));
PRIVATE truc scalvec	_((truc *ptr1, truc *ptr2));
PRIVATE truc scalfltvec _((truc *ptr1, truc *ptr2));
PRIVATE truc vecdiv		_((truc *vptr, truc *zz));
PRIVATE truc vecdivfloat _((truc *vptr, truc *zz));

PRIVATE int chkplusargs	_((truc sym, truc *argptr));
PRIVATE int chktimesargs _((truc *argptr));
PRIVATE int chkmodargs  _((truc sym, truc *argptr));
PRIVATE int chkdivfargs _((truc sym, truc *argptr));

PRIVATE truc floatsym;
PRIVATE truc decodsym;
PRIVATE truc sumsym, prodsym;
PRIVATE truc bnotsym, bandsym, borsym, bxorsym, bshiftsym;
PRIVATE truc btestsym, bsetsym, bclrsym, blensym, bcountsym;
PRIVATE truc cardsym;

PRIVATE truc maxsym, minsym;
PRIVATE truc maxintsym;
PRIVATE truc setfltsym, getfltsym, maxfltsym, setprnsym, getprnsym;
PRIVATE truc incsym, decsym;
PRIVATE truc abssym, oddsym, evensym;
PRIVATE truc isqrtsym;
PRIVATE truc dividesym, div_sym, mod_sym;
PRIVATE truc truncsym, roundsym, fracsym, floorsym;
PRIVATE truc randsym, rseedsym;

PRIVATE word2 RandSeed[4];
PRIVATE word4 MaxBits;
PRIVATE int curFltPrec;

/*--------------------------------------------------------*/
PUBLIC void iniarith()
{
	word4 u;

	zero      = mkfixnum(0);
	constone  = mkfixnum(1);

	integsym  = newsym("integer", sTYPESPEC, zero);
	int_sym	  = new0symsig("integer", sFBINARY, (wtruc)Finteger, s_ii);
	realsym	  = newsym("real",    sTYPESPEC, fltzero(deffltprec()));

	sfloatsym = newsym("single_float",  sSYMBCONST, mkfixnum(FltPrec[0]));
	dfloatsym = newsym("double_float",  sSYMBCONST, mkfixnum(FltPrec[1]));
	lfloatsym = newsym("long_float",    sSYMBCONST, mkfixnum(FltPrec[2]));
	xfloatsym = newsym("extended_float",sSYMBCONST, mkfixnum(FltPrec[3]));

	floatsym  = newsymsig("float", sFBINARY, (wtruc)Ffloat, s_12rn);
	setfltsym = newsymsig("set_floatprec", sFBINARY,
				(wtruc)Fsetfltprec, s_12);
	getfltsym = newsymsig("get_floatprec", sFBINARY,
				(wtruc)Fgetfltprec, s_01);
	maxfltsym = newsymsig("max_floatprec", sFBINARY, (wtruc)Fmaxfltprec,s_0);
	setprnsym = newsymsig("set_printprec", sFBINARY, (wtruc)Fsetprnprec,s_1);
	getprnsym = newsymsig("get_printprec", sFBINARY, (wtruc)Fgetprnprec,s_0);

	decodsym  = newsymsig("decode_float",sFBINARY,(wtruc)Fdecode, s_vr);
	plussym	  = newintsym("+",  sFBINARY, (wtruc)Fplus);
	minussym  = newintsym("-",  sFBINARY, (wtruc)Fminus);
	uminsym   = newintsym("-",  sFBINARY, (wtruc)Fnegate);
	timessym  = newintsym("*",  sFBINARY, (wtruc)Ftimes);
	divfsym	  = newintsym("/",  sFBINARY, (wtruc)Fdivf);
	powersym  = newintsym("**", sFBINARY, (wtruc)Fpower);
	sumsym	  = newsymsig("sum",	sFBINARY, (wtruc)Fsum,  s_nv);
	prodsym	  = newsymsig("product",sFBINARY, (wtruc)Fprod, s_nv);

	divsym	  = newintsym("div",sFBINARY, (wtruc)Fdiv);
	div_sym	  = newsym("div", sINFIX, divsym);
    SYMcc1(div_sym) = DIVTOK;
	dividesym = newsymsig("divide",sFBINARY, (wtruc)Fdivide, s_2);

	modsym	  = newintsym("mod",sFBINARY, (wtruc)Fmod);
	mod_sym	  = newsym("mod", sINFIX, modsym);
    SYMcc1(mod_sym) = MODTOK;

	abssym	  = newsymsig("abs",	sFBINARY, (wtruc)Fabsolute,s_rr);
	maxsym	  = newsymsig("max",	sFBINARY, (wtruc)Fmax,s_1u);
	minsym	  = newsymsig("min",	sFBINARY, (wtruc)Fmin,s_1u);
	maxintsym = newsymsig("max_intsize", sFBINARY, (wtruc)Fmaxint,s_0);

	oddsym	  = newsymsig("odd",	sFBINARY, (wtruc)Fodd,s_ii);
	evensym	  = newsymsig("even",	sFBINARY, (wtruc)Feven,s_ii);
	incsym	  = newsymsig("inc",	sSBINARY, (wtruc)Sinc,s_12ii);
	decsym	  = newsymsig("dec",	sSBINARY, (wtruc)Sdec,s_12ii);
	isqrtsym  = newsymsig("isqrt",	sFBINARY, (wtruc)Fisqrt,s_ii);

	truncsym  = newsymsig("trunc", sFBINARY, (wtruc)Ftrunc, s_rr);
	roundsym  = newsymsig("round", sFBINARY, (wtruc)Fround, s_rr);
	floorsym  = newsymsig("floor", sFBINARY, (wtruc)Ffloor, s_rr);
	fracsym	  = newsymsig("frac",  sFBINARY, (wtruc)Ffrac,  s_rr);

	randsym	  = newsymsig("random", sFBINARY, (wtruc)Frandom, s_rr);
	rseedsym  = newsymsig("random_seed",sFBINARY,(wtruc)Frandseed, s_01);

	ariltsym  = newintsym("<",  sFBINARY, (wtruc)Farilt);
	arigtsym  = newintsym(">",  sFBINARY, (wtruc)Farigt);
	arilesym  = newintsym("<=", sFBINARY, (wtruc)Farile);
	arigesym  = newintsym(">=", sFBINARY, (wtruc)Farige);

	bnotsym	  = newsymsig("bit_not",  sFBINARY, (wtruc)Fbitnot, s_ii);
	bandsym	  = newsymsig("bit_and",  sFBINARY, (wtruc)Fbitand, s_iii);
	borsym	  = newsymsig("bit_or",	  sFBINARY, (wtruc)Fbitor,  s_iii);
	bxorsym	  = newsymsig("bit_xor",  sFBINARY, (wtruc)Fbitxor, s_iii);
	btestsym  = newsymsig("bit_test", sFBINARY, (wtruc)Fbittest,s_iii);
	bsetsym	  = newsymsig("bit_set",  sFBINARY, (wtruc)Fbitset, s_iii);
	bclrsym	  = newsymsig("bit_clear",sFBINARY, (wtruc)Fbitclear, s_iii);
	bshiftsym = newsymsig("bit_shift",sFBINARY, (wtruc)Fbitshift, s_iii);
	blensym	  = newsymsig("bit_length",sFBINARY,(wtruc)Fbitlength,s_ii);
	bcountsym = newsymsig("bit_count",sFBINARY, (wtruc)Fbitcount,s_ii);
	cardsym	  = newsymsig("cardinal", sFBINARY, (wtruc)Fcardinal,s_ii);

	u = aribufSize;
	u <<= 4;
	MaxBits = u;
	u -= 256;
	if(u > MAXFLTLIM)
		u = MAXFLTLIM;
	maxfltex = u;
	maxdecex = (u/10) * 3;
	exprange = u - u/3 + u/38;	/* log(2) = 1 - 1/3 + 1/38 */

	inirandstate(RandSeed);
	iniaritx();
	iniarity();
	iniaritz();
}
/*--------------------------------------------------------*/
#define DIVFLAG		1
#define MODFLAG		2
#define DDIVFLAG	(DIVFLAG | MODFLAG)
/*--------------------------------------------------------*/
PRIVATE truc Fplus()
{
	truc res;
	int type;

	type = chkplusargs(plussym,argStkPtr-1);
    if(type > fBIGNUM) {
	    curFltPrec = deffltprec();
	    res = addfloats(argStkPtr-1,0);
    }
    else switch(type) {
        case fFIXNUM:
	    case fBIGNUM:
		    res = addints(argStkPtr-1,0);
            break;
        case fVECTOR:
            res = addvecs(plussym,argStkPtr-1);
            break;
        case fGF2NINT:
		    res = addgf2ns(argStkPtr-1);
            break;
        case aERROR:
        default:
	    	res = brkerr();
            break;
	}
	return(res);
}
/*--------------------------------------------------------*/
PUBLIC truc addints(ptr,minflag)
truc *ptr;
int minflag;	/* if minflag != 0, subtract */
{
	word2 *y;
	int n1, n2, n;
	int sign1, sign2, sign, cmp;

	n1 = bigretr(ptr,AriBuf,&sign1);
	n2 = bigref(ptr+1,&y,&sign2);
	if(minflag) {
		sign2 = (sign2 ? 0 : MINUSBYTE);
	}
	if(sign1 == sign2) {
		sign = sign1;
		n = addarr(AriBuf,n1,y,n2);
	}
	else {
		cmp = cmparr(AriBuf,n1,y,n2);
		if(cmp > 0) {
			sign = sign1;
			n = subarr(AriBuf,n1,y,n2);
		}
		else if(cmp < 0) {
			sign = sign2;
			n = sub1arr(AriBuf,n1,y,n2);
		}
		else
			return(zero);
	}
	return(mkint(sign,AriBuf,n));
}
/*-------------------------------------------------------------*/
PRIVATE truc addfloats(ptr,minflag)
truc *ptr;
int minflag;
{
	numdata accum, temp;
	int prec, cmp;

	prec = curFltPrec + 1;
	if(prec < 3)
		prec++;
	accum.digits = AriBuf;
	temp.digits = AriScratch;
	getnumalign(prec,ptr,&accum);
	getnumalign(prec,ptr+1,&temp);
	if(minflag)
		temp.sign = (temp.sign ? 0 : MINUSBYTE);
	adjustoffs(&accum,&temp);
	if(accum.sign == temp.sign) {
		accum.len =
		addarr(accum.digits,accum.len,temp.digits,temp.len);
	}
	else {
		cmp = cmparr(accum.digits,accum.len,temp.digits,temp.len);
		if(cmp > 0) {
			accum.len =
			subarr(accum.digits,accum.len,temp.digits,temp.len);
		}
		else if(cmp < 0) {
			accum.sign = temp.sign;
			accum.len =
			sub1arr(accum.digits,accum.len,temp.digits,temp.len);
		}
		else {
			accum.len = 0;
		}
	}
	return(mkfloat(curFltPrec,&accum));
}
/*--------------------------------------------------------*/
PRIVATE truc Fminus()
{
	truc res;
	int type;

	type = chkplusargs(minussym,argStkPtr-1);
    if(type > fBIGNUM) {
	    curFltPrec = deffltprec();
	    res = addfloats(argStkPtr-1,-1);
    }
    else switch(type) {
        case fFIXNUM:
	    case fBIGNUM:
		    res = addints(argStkPtr-1,-1);
            break;
        case fVECTOR:
            res = addvecs(minussym,argStkPtr-1);
            break;
        case fGF2NINT:
		    res = addgf2ns(argStkPtr-1);
            break;
        case aERROR:
        default:
	    	res = brkerr();
            break;
	}
	return(res);
}
/*--------------------------------------------------------*/
PRIVATE truc Fnegate()
{
    int flg;
    truc res[1];

    flg = *FLAGPTR(argStkPtr);

    if(flg >= fFIXNUM) {
        res[0] = mkcopy(argStkPtr);
        changesign(res);
        return res[0];
    }
    else if(flg == fVECTOR) {
        flg = chknumvec(uminsym,argStkPtr);
        if(flg == aERROR)
            return brkerr();
        else
            return negatevec(argStkPtr);
    }
    else if(flg == fGF2NINT) {
        return *argStkPtr;
    }
    else {    /* flg == aERROR */
		error(uminsym,err_num,*argStkPtr);
        return brkerr();
	}
}
/*--------------------------------------------------------*/
PRIVATE truc Sinc()
{
	return(Sincaux(incsym,1));
}
/*--------------------------------------------------------*/
PRIVATE truc Sdec()
{
	return(Sincaux(decsym,-1));
}
/*--------------------------------------------------------*/
PRIVATE truc Sincaux(symb,s)
truc symb;
int s;
{
	truc res;
	long number;
	int argn;
	int flg;

	argn = *ARGCOUNTPTR(evalStkPtr);
	res = eval(ARG1PTR(evalStkPtr));
	ARGpush(res);
	if(argn == 2) {
		res = eval(ARGNPTR(evalStkPtr,2));
		ARGpush(res);
	}
	flg = chkints(symb,argStkPtr-argn+1,argn);
	if(flg == aERROR) {
		ARGnpop(argn);
		return(brkerr());
	}
	else if(argn == 1 && flg == fFIXNUM) {
		number = *WORD2PTR(argStkPtr);
		if(*SIGNPTR(argStkPtr))
			number = -number;
		res = mkinum(number + s);
		ARGpop();
	}
	else {
		if(argn == 1)
			ARGpush(constone);
		s = (s > 0 ? 0 : -1);
		res = addints(argStkPtr-1,s);
		ARGnpop(2);
	}
	return(Lvalassign(ARG1PTR(evalStkPtr),res));
}
/*--------------------------------------------------------*/
PRIVATE truc Ftimes()
{
	truc res;
	int type;

	type = chktimesargs(argStkPtr-1);
	if(type <= fBIGNUM) {
		switch(type) {
        case fFIXNUM:
        case fBIGNUM:
		    res = multints(argStkPtr-1);
            break;
        case fGF2NINT:
            res = multgf2ns(argStkPtr-1);
            break;
        default:	/* type == aERROR */
            res = brkerr();
	    }
	}
    else if((type & 0xFF00) == 0) {	/* float obj */
	    curFltPrec = deffltprec();
	    res = multfloats(argStkPtr-1);
    }
	else if((type >> 8) == fVECTOR) {
		type |= 0xFF;
	    if(type >= fFIXNUM) {
    	    res = scalvec(argStkPtr-1,argStkPtr);
    	}
    	else if(type == fGF2NINT) {
            error(timessym,err_imp,voidsym);
			res = brkerr();
    	}
		else
			res = brkerr();
	}
	else {
        error(timessym,err_case,mksfixnum(type));
		res = brkerr();
	}
	return(res);
}
/*----------------------------------------------------------*/
/*
** multiply integers in ptr[0] and ptr[1]
*/
PRIVATE truc multints(ptr)
truc *ptr;
{
	word2 *x, *y;
	int n1, n2, n, sign, sign2;

	n1 = bigref(ptr,&x,&sign);
	n2 = bigref(ptr+1,&y,&sign2);
	if(n1 + n2 >= aribufSize)
		goto errexit;
	else if(!n1 || !n2)
		return(zero);
	n = multbig(x,n1,y,n2,AriBuf,AriScratch);
	sign = (sign == sign2 ? 0 : MINUSBYTE);
	return(mkint(sign,AriBuf,n));
  errexit:
	error(timessym,err_ovfl,voidsym);
	return(brkerr());
}
/*---------------------------------------------------------*/
PRIVATE truc multfloats(ptr)
truc *ptr;
{
	numdata prod, temp;
	int prec;
	int n;

	prec = curFltPrec + 1;
	prod.digits = AriBuf;
	n = getnumtrunc(prec,ptr,&prod);
	refnumtrunc(prec,ptr+1,&temp);
	n = multtrunc(prec,&prod,&temp,AriScratch);

	if(n < 0) {
		error(timessym,err_ovfl,voidsym);
		return(brkerr());
	}
	return(mkfloat(curFltPrec,&prod));
}
/*--------------------------------------------------------*/
PRIVATE truc negatevec(ptr)
truc *ptr;
{
    truc tmp[1];
	int len,k;

    WORKpush(mkcopy(ptr));
    len = *VECLENPTR(workStkPtr);
    for(k=0; k<len; k++) {
        tmp[0] = mkcopy(VECTORPTR(workStkPtr)+k);
        *(VECTORPTR(workStkPtr)+k) = changesign(tmp);
    }
    return WORKretr();
}
/*--------------------------------------------------------*/
/*
** addition of two vectors (int or float) ptr[0] and ptr[1]
*/
PRIVATE truc addvecs(sym,ptr)
truc sym;
truc *ptr;
{
    truc tmp;
    int len1, len2, flg1, flg;

    flg1 = chknumvec(sym,ptr);
    if(flg1 == aERROR)
        return brkerr();
    flg = chknumvec(sym,ptr+1);
    if(flg == aERROR) {
        return brkerr();
    }
    if(flg < flg1)
        flg = flg1;

    if(sym == minussym) {
        ptr[1] = mkcopy(ptr+1);
        ptr[1] = negatevec(ptr+1);
    }
    len1 = *VECLENPTR(ptr);
	len2 = *VECLENPTR(ptr+1);
    if(len2 < len1) {
        tmp = mkcopy(ptr);
        ptr[0] = ptr[1];
        ptr[1] = tmp;
    }
    else if(sym == plussym) {
        ptr[1] = mkcopy(ptr+1);
    }
    if(flg <= fBIGNUM) {
        return addintvecs(ptr,ptr+1);
    }
    else {
    	curFltPrec = deffltprec();
        return addfltvecs(ptr,ptr+1);
    }
}
/*--------------------------------------------------------*/
/*
** adds two integer vectors *ptr1 and *ptr2
** length of *ptr1 must be <= length of *ptr2
*/
PRIVATE truc addintvecs(ptr1,ptr2)
truc *ptr1,*ptr2;
{
	truc obj;
    int len, k;

    len = *VECLENPTR(ptr1);
    WORKnpush(2);
    for(k=0; k<len; k++) {
        workStkPtr[-1] = *(VECTORPTR(ptr1) + k);
        workStkPtr[0] = *(VECTORPTR(ptr2) + k);
		obj = addints(workStkPtr-1,0);
        *(VECTORPTR(ptr2) + k) = obj;
    }
	WORKnpop(2);
    return *ptr2;
}
/*--------------------------------------------------------*/
/*
** adds two float vectors *ptr1 and *ptr2
** length of *ptr1 must be <= length of *ptr2
*/
PRIVATE truc addfltvecs(ptr1,ptr2)
truc *ptr1,*ptr2;
{
    truc obj;
    int len, k;

    len = *VECLENPTR(ptr1);
    WORKnpush(2);
    for(k=0; k<len; k++) {
        workStkPtr[-1] = *(VECTORPTR(ptr1) + k);
        workStkPtr[0] = *(VECTORPTR(ptr2) + k);
		obj = addfloats(workStkPtr-1,0);
        *(VECTORPTR(ptr2) + k) = obj;
    }
	WORKnpop(2);
	return *ptr2;
}
/*---------------------------------------------------------*/
/*
** multiplies vector given in *ptr2 by scalar given in *ptr1
*/
PRIVATE truc scalvec(ptr1,ptr2)
truc *ptr1, *ptr2;
{
    int flg, flg0;

    flg = chknumvec(timessym,ptr2);
    if(flg == aERROR)
        return brkerr();
    flg0 = *FLAGPTR(ptr1);
    if(flg0 > flg)
        flg = flg0;
    if(flg <= fBIGNUM) {
        return scalintvec(ptr1,ptr2);
	}
    else {
    	curFltPrec = deffltprec();
        return scalfltvec(ptr1,ptr2);
	}
}
/*---------------------------------------------------------*/
/*
** Multiplies int vector given in *ptr2 by scalar in *ptr1
*/
PUBLIC truc scalintvec(ptr1,ptr2)
truc *ptr1, *ptr2;
{
    truc obj;
    int len, k;

    WORKpush(*ptr1);
    WORKpush(constone);
    obj = mkcopy(ptr2);
    WORKpush(obj);
    len = *VECLENPTR(ptr2);
    for(k=0; k<len; k++) {
        *(workStkPtr-1) = *(VECTORPTR(workStkPtr) + k);
		obj = multints(workStkPtr-2);
        *(VECTORPTR(workStkPtr) + k) = obj;
    }
    obj = WORKretr();
    WORKnpop(2);
    return obj;
}
/*---------------------------------------------------------*/
/*
** Multiplies float vector given in *ptr2 by scalar in *ptr1
*/
PRIVATE truc scalfltvec(ptr1,ptr2)
truc *ptr1, *ptr2;
{
    truc obj;
    int len, k;

    WORKpush(*ptr1);
    WORKpush(constone);
    obj = mkcopy(ptr2);
    WORKpush(obj);
    len = *VECLENPTR(ptr2);
    for(k=0; k<len; k++) {
        *(workStkPtr-1) = *(VECTORPTR(workStkPtr) + k);
		obj = multfloats(workStkPtr-2);
        *(VECTORPTR(workStkPtr) + k) = obj;
    }
    obj = WORKretr();
    WORKnpop(2);
    return obj;
}
/*--------------------------------------------------------*/
PRIVATE truc Fabsolute()
{
	if(chknum(abssym,argStkPtr) == aERROR)
		return(brkerr());

	*argStkPtr = mkcopy(argStkPtr);
	wipesign(argStkPtr);
	return(*argStkPtr);
}
/*--------------------------------------------------------*/
PRIVATE truc Fmax(argn)
int argn;
{
	return(Gminmax(maxsym,argn));
}
/*--------------------------------------------------------*/
PRIVATE truc Fmin(argn)
int argn;
{
	return(Gminmax(minsym,argn));
}
/*--------------------------------------------------------*/
PRIVATE truc Fmaxint()
{
        long mxint = maxdecex - 8;

        return(mkinum(mxint));
}
/*--------------------------------------------------------*/
PRIVATE truc Gminmax(symb,argn)
truc symb;
int argn;
{
	struct vector *vptr;
	truc *ptr, *argptr;
	int cmp, type;

	if(argn == 1 && *FLAGPTR(argStkPtr) == fVECTOR) {
		vptr = (struct vector *)TAddress(argStkPtr);
		argn = vptr->len;
		if(argn == 0) {
			error(symb,err_args,*argStkPtr);
			goto errexit;
		}
		argptr = &(vptr->ele0);
	}
	else
		argptr = argStkPtr - argn + 1;
	if((type = chknums(symb,argptr,argn)) == aERROR)
		goto errexit;
	ptr = argptr++;
	while(--argn > 0) {
		cmp = cmpnums(ptr,argptr,type);
		if(symb == minsym)
			cmp = -cmp;
		if(cmp < 0)
			ptr = argptr;
		argptr++;
	}
	return(*ptr);
  errexit:
	return brkerr();
}
/*--------------------------------------------------------*/
PRIVATE truc Fodd()
{
	int ret = odd(argStkPtr);

	if(ret == aERROR)
		return(brkerr());
	return(ret ? true : false);
}
/*--------------------------------------------------------*/
PRIVATE truc Feven()
{
	int ret = odd(argStkPtr);

	if(ret == aERROR)
		return(brkerr());
	return(ret ? false : true);
}
/*--------------------------------------------------------*/
PRIVATE int odd(ptr)
truc *ptr;
{
	word2 *x;
	int sign;

	if(bigref(ptr,&x,&sign) == aERROR)
		return(aERROR);
	else
		return(x[0] & 1);
}
/*---------------------------------------------------------*/
PRIVATE truc Fsum()
{
	struct vector *vec;
	truc *ptr;
	long res;
	int flg;
	int len;

	if(*FLAGPTR(argStkPtr) != fVECTOR) {
		error(sumsym,err_vect,*argStkPtr);
		return(brkerr());
	}
	vec = (struct vector *)TAddress(argStkPtr);
	len = vec->len;
	ptr = &(vec->ele0);
	flg = chknums(sumsym,ptr,len);

	if(flg == fFIXNUM) {
		res = 0;
		while(--len >= 0) {
			if(*SIGNPTR(ptr))
				res -= *WORD2PTR(ptr);
			else
				res += *WORD2PTR(ptr);
			ptr++;
		}
		return(mkinum(res));
	}
	else if(flg == fBIGNUM) {
		return(sumintvec(ptr,len));
	}
	else if(flg >= fFLTOBJ) {
		curFltPrec = deffltprec();
		return(sumfltvec(ptr,len));
	}
	else		/* vector elements are not numbers */
		return(brkerr());
}
/*-------------------------------------------------------------*/
/*
** sum up all elements of integer vector *argptr of length len
*/
PRIVATE truc sumintvec(argptr,len)
truc *argptr;
int len;
{
	word2 *y;
	int sign, cmp, n0, n1, m;

	n0 = n1 = 0;
	while(--len >= 0) {
		m = bigref(argptr,&y,&sign);
		if(sign)
			n1 = addarr(AriScratch,n1,y,m);
		else
			n0 = addarr(AriBuf,n0,y,m);
		argptr++;
	}
	cmp = cmparr(AriBuf,n0,AriScratch,n1);
	if(cmp < 0) {
		sign = MINUSBYTE;
		m = sub1arr(AriBuf,n0,AriScratch,n1);
	}
	else {
		sign = 0;
		m = subarr(AriBuf,n0,AriScratch,n1);
	}
	return(mkint(sign,AriBuf,m));
}
/*-------------------------------------------------------------*/
/*
** sum up all elements of float vector *argptr of length len
*/
PRIVATE truc sumfltvec(argptr,len)
truc *argptr;
int len;
{
	numdata accum, negsum, temp;
	numdata *nptr;
	int prec;
	int cmp, sign;

	prec = curFltPrec + 1;
	accum.digits = AriBuf;
	negsum.digits = AriScratch;
	temp.digits = AriScratch + aribufSize;
	accum.len = 0;
	accum.expo = MOSTNEGEX;
	negsum.len = 0;
	negsum.expo = MOSTNEGEX;
	while(--len >= 0) {
		if(!getnumalign(prec,argptr++,&temp))
			continue;
		sign = temp.sign;
		nptr = (sign ? &negsum : &accum);
		adjustoffs(nptr,&temp);
		nptr->len =
		addarr(nptr->digits,nptr->len,temp.digits,temp.len);
	}
	adjustoffs(&accum,&negsum);
	cmp = cmparr(accum.digits,accum.len,negsum.digits,negsum.len);
	if(cmp < 0) {
		accum.sign = MINUSBYTE;
		accum.len =
		sub1arr(accum.digits,accum.len,negsum.digits,negsum.len);
	}
	else {
		accum.sign = 0;
		accum.len =
		subarr(accum.digits,accum.len,negsum.digits,negsum.len);
	}
	return(mkfloat(curFltPrec,&accum));
}
/*---------------------------------------------------------*/
PRIVATE truc Fprod()
{
	struct vector *vec;
	truc *ptr;
	int flg;
	int len;

	if(*FLAGPTR(argStkPtr) != fVECTOR) {
		error(prodsym,err_vect,*argStkPtr);
		return(brkerr());
	}
	vec = (struct vector *)TAddress(argStkPtr);
	len = vec->len;

	if(!len)
		return(constone);
	ptr = &(vec->ele0);
	flg = chknums(prodsym,ptr,len);
	if(flg == aERROR)
		return(brkerr());
	if(flg <= fBIGNUM)
		return(prodintvec(ptr,len));
	else {	/* flg >= fFLTOBJ */
		curFltPrec = deffltprec();
		return(prodfloatvec(ptr,len));
	}
}
/*----------------------------------------------------------*/
/*
** multiplies all elements of integer vector *argptr of length len
*/
PRIVATE truc prodintvec(argptr,len)
truc *argptr;
int len;
{
	word2 *y, *hilf;
	int n1, n, sign, sign1;
	unsigned a;

	if(len == 0)
		return(constone);

	n = bigref(argptr++,&y,&sign);
	cpyarr(y,n,AriBuf);
	hilf = AriScratch + aribufSize;

	while(--len > 0) {
		if(*FLAGPTR(argptr) == fFIXNUM) {
			a = *WORD2PTR(argptr);
			n = multarr(AriBuf,n,a,AriBuf);
			if(n >= aribufSize)
				goto errexit;
			sign1 = *SIGNPTR(argptr);
		}
		else {
			n1 = bigref(argptr,&y,&sign1);
			if(n + n1 >= aribufSize)
				goto errexit;
			cpyarr(AriBuf,n,AriScratch);
			n = multbig(AriScratch,n,y,n1,AriBuf,hilf);
		}
		if(n == 0) {
			sign = 0;
			break;
		}
		if(sign1)
			sign = (sign ? 0 : MINUSBYTE);
		argptr++;
	}
	return(mkint(sign,AriBuf,n));
  errexit:
	error(prodsym,err_ovfl,voidsym);
	return(brkerr());
}
/*---------------------------------------------------------*/
/*
** multiplies all elements of float vector *argptr of length len
*/
PRIVATE truc prodfloatvec(argptr,len)
truc *argptr;
int len;		/* len >= 1 */
{
	numdata prod, temp;
	int prec;
	int n;

	prec = curFltPrec + 1;
	prod.digits = AriBuf;
	n = getnumtrunc(prec,argptr++,&prod);
	while(--len > 0 && n > 0) {
		refnumtrunc(prec,argptr++,&temp);
		n = multtrunc(prec,&prod,&temp,AriScratch);
	}
	if(n < 0) {
		error(prodsym,err_ovfl,voidsym);
		return(brkerr());
	}
	return(mkfloat(curFltPrec,&prod));
}
/*---------------------------------------------------------*/
PRIVATE truc Fdivf()
{
	int type, flg;

	type = chkdivfargs(divfsym,argStkPtr-1);
	if(type == aERROR) {
		return brkerr();
	}
    else if(type == fGF2NINT) {
        return divgf2ns(argStkPtr-1);
    }
	if((*argStkPtr == zero) ||
       ((flg = *FLAGPTR(argStkPtr)) >= fFLTOBJ && (flg & FLTZEROBIT))) {
		error(divfsym,err_div,voidsym);
		return brkerr();
	}
	curFltPrec = deffltprec();
	if((type & 0xFF00) == 0) {
		return(divfloats(argStkPtr-1));
    }
	/* else  first argument is vector */
	flg = chknumvec(divfsym,argStkPtr-1);
	if(flg == aERROR) {
		return brkerr();
	}
    else {
	    return(vecdivfloat(argStkPtr-1,argStkPtr));
    }
}
/*----------------------------------------------------------*/
PRIVATE truc Fdiv()
{
	int type, flg;

	type = chkmodargs(divsym,argStkPtr-1);
	if(type == aERROR) {
		return(brkerr());
	}
	if(*argStkPtr == zero) {
		error(divsym,err_div,voidsym);
		return(brkerr());
	}
    if((type & 0xFF00) == 0)
		return(divide(argStkPtr-1,DIVFLAG));

    /* else first argument is vector */
	flg = chkintvec(divsym,argStkPtr-1);
	if(flg == aERROR) {
		return(brkerr());
	}
    else {
		return(vecdiv(argStkPtr-1,argStkPtr));
	}
}
/*----------------------------------------------------------*/
/*
** Divide integer vector given in *vptr by integer *zz
*/
PRIVATE truc vecdiv(vptr,zz)
truc *vptr;
truc *zz;
{
	truc *ptr;
	truc obj;
	word2 *z, *hilf;
	int k, len, len1, len2, rlen, sign1, sign2;

	hilf = AriScratch + aribufSize;
	len = *VECLENPTR(vptr);
	WORKpush(mkcopy(vptr));
	for(k=0; k<len; k++) {
		ptr = VECTORPTR(workStkPtr)+k;
		len1 = bigretr(ptr,AriScratch,&sign1);
        len2 = bigref(zz,&z,&sign2);
		len1 = divbig(AriScratch,len1,z,len2,AriBuf,&rlen,hilf);
		if(sign1 == sign2) {
			sign1 = 0;
		}
		else {
			sign1 = MINUSBYTE;
			if(rlen)
				len1 = incarr(AriBuf,len1,1);
		}
		obj = mkint(sign1,AriBuf,len1);
		*(VECTORPTR(workStkPtr)+k) = obj;
	}
	return(WORKretr());
}
/*----------------------------------------------------------*/
/*
** Divide integer vector given in *vptr by number *zz
*/
PRIVATE truc vecdivfloat(vptr,zz)
truc *vptr;
truc *zz;
{
    truc obj;
    int len, k;

	WORKpush(zero);
	WORKpush(*zz);
	obj = mkcopy(vptr);
	WORKpush(obj);
	len = *VECLENPTR(vptr);
	for(k=0; k<len; k++) {
		*(workStkPtr-2) = *(VECTORPTR(workStkPtr) + k);
		obj = divfloats(workStkPtr-2);
        *(VECTORPTR(workStkPtr) + k) = obj;
	}
    obj = WORKretr();
    WORKnpop(2);
    return obj;
}
/*----------------------------------------------------------*/
PRIVATE truc Fmod()
{
	int type, flg;

	type = chkmodargs(modsym,argStkPtr-1);
	if(type == aERROR) {
		return(brkerr());
	}
	if(*argStkPtr == zero) {
		error(modsym,err_div,voidsym);
		return(brkerr());
	}
    if((type & 0xFF00) == 0) 	/* then both arguments are integers */
		return(modout(argStkPtr-1));

    /* else first argument is vector */
	flg = chkintvec(modsym,argStkPtr-1);
	if(flg == aERROR) {
		return(brkerr());
	}
	type &= 0xFF;
    /* type == fFIXNUM or fBIGNUM */
    argStkPtr[-1] = mkcopy(argStkPtr-1);
    return Gvecmod(type);
}
/*----------------------------------------------------------*/
PRIVATE truc modout(ptr)
truc *ptr;
{
	word2 *y;
	int rlen, n1, n2, sign1, sign2;

	n1 = bigretr(ptr,AriBuf,&sign1);
	n2 = bigref(ptr+1,&y,&sign2);

	rlen = modbig(AriBuf,n1,y,n2,AriScratch);
	if(rlen && (sign1 != sign2))
		rlen = sub1arr(AriBuf,rlen,y,n2);

	return(mkint(sign2,AriBuf,rlen));
}
/*----------------------------------------------------------*/
/*
** mod out (destructively) vector given in argStkPtr[-1]
** by fixnum resp. bignum in argStkPtr[0]
*/
PUBLIC truc Gvecmod(flg)
int flg;    /* flg == fFIXNUM or fBIGNUM */
{
    truc *ptr;
    truc obj;
    word2 *zz, *arr;
    int k, len, len1, len2, rlen, sign1, sign2;
    unsigned z, x;
    int rest;

    len = *VECLENPTR(argStkPtr-1);
    if(flg == fFIXNUM) {
       	z = *WORD2PTR(argStkPtr);
        sign2 = *SIGNPTR(argStkPtr);
        ptr = VECTORPTR(argStkPtr-1);
        for(k=0; k<len; k++) {
            len1 = bigref(ptr,&arr,&sign1);
    		x = modarr(arr,len1,z);
            rest = ((sign1 == sign2) || (x==0) ? x : z-x);
            if(sign2)
            	rest = -rest;
            *ptr++ = mksfixnum(rest);
        }
    }
    else {  /* flg == fBIGNUM */
        len2 = *BIGLENPTR(argStkPtr);
        sign2 = *SIGNUMPTR(argStkPtr);
        for(k=0; k<len; k++) {
            ptr = VECTORPTR(argStkPtr-1) + k;
            len1 = bigretr(ptr,AriBuf,&sign1);
            zz = BIGNUMPTR(argStkPtr);
            rlen = modbig(AriBuf,len1,zz,len2,AriScratch);
		    if(rlen && (sign1 != sign2))
			    rlen = sub1arr(AriBuf,rlen,zz,len2);
		    obj = mkint(sign2,AriBuf,rlen);
		    *(VECTORPTR(argStkPtr-1)+k) = obj;
        }
    }
    return argStkPtr[-1];
}
/*----------------------------------------------------------*/
PRIVATE truc Fdivide()
{
	int type;

	type = chkmodargs(dividesym,argStkPtr-1);
	if(type == aERROR) {
		return(brkerr());
	}
	if(*argStkPtr == zero) {
		error(dividesym,err_div,voidsym);
		return(brkerr());
	}
    if((type & 0xFF00) == 0) 	/* then both arguments are integers */
		return(divide(argStkPtr-1,DDIVFLAG));
    else {/* else first argument is vector */
        error(dividesym,err_num,argStkPtr[-1]);
		return(brkerr());
    }
}
/*----------------------------------------------------------*/
PRIVATE truc divide(ptr,tflag)
truc *ptr;
int tflag;	/* one of DIVFLAG, MODFLAG, DDIVFLAG */
{
	truc res, vec;
    truc *vptr;
	word2 *y, *hilf;
	int len, rlen, n1, n2, sign1, sign2;

	n1 = bigretr(ptr,AriScratch,&sign1);
	n2 = bigref(ptr+1,&y,&sign2);
	hilf = AriScratch + aribufSize;
	len = divbig(AriScratch,n1,y,n2,AriBuf,&rlen,hilf);
	if(tflag & DIVFLAG) {
		if(sign1 != sign2) {
			if(rlen)
				len = incarr(AriBuf,len,1);
			sign1 = MINUSBYTE;
		}
		else
			sign1 = 0;
	}
	if(tflag & MODFLAG) {
		if(rlen && (sign1 != sign2))
			rlen = sub1arr(AriScratch,rlen,y,n2);
		y = AriBuf + len;
		cpyarr(AriScratch,rlen,y);
	}
	if(tflag == DIVFLAG) {
		return mkint(sign1,AriBuf,len);
	}
	else if(tflag == MODFLAG) {
		return mkint(sign2,y,rlen);
	}
	else {	/* tflag == DDIVFLAG */
		res = mkint(sign1,AriBuf,len);
        WORKpush(res);
		res = mkint(sign2,y,rlen);
        WORKpush(res);
	    vec = mkvect0(2);
	    vptr = VECTOR(vec);
	    vptr[1] = WORKretr();
	    vptr[0] = WORKretr();
		return vec;
	}
}
/*----------------------------------------------------------*/
/*
** divides ptr[0] by ptr[1].
** Hypothesis: ptr[1] is not equal to 0.
*/
PRIVATE truc divfloats(ptr)
truc *ptr;
{
	numdata quot, temp;
	int prec;
	int n, m;

	prec = curFltPrec + 1;
	quot.digits = AriBuf;

	n = getnumtrunc(prec,ptr,&quot);
	m = refnumtrunc(prec,ptr+1,&temp);
	n = divtrunc(prec,&quot,&temp,AriScratch);
	if(n < 0) {
		error(divfsym,err_ovfl,voidsym);
		return(brkerr());
	}
	return(mkfloat(curFltPrec,&quot));
}
/*------------------------------------------------------------------*/
PRIVATE truc Ftrunc()
{
	return Gtruncaux(truncsym);
}
/*------------------------------------------------------------------*/
PRIVATE truc Fround()
{
	return Gtruncaux(roundsym);
}
/*------------------------------------------------------------------*/
PRIVATE truc Ffloor()
{
	return Gtruncaux(floorsym);
}
/*------------------------------------------------------------------*/
PRIVATE truc Ffrac()
{
	return Gtruncaux(fracsym);
}
/*------------------------------------------------------------------*/
PRIVATE truc Gtruncaux(symb)
truc symb;
{
	numdata x1, y;
	int type;
	int prec, s1;

	type = chknum(symb,argStkPtr);
	if(type == aERROR)
		return(brkerr());
	curFltPrec = fltprec(type);
	prec = curFltPrec + 1;

	x1.digits = AriBuf;
	y.digits = AriScratch;

	if(type <= fBIGNUM) {
		if(symb == truncsym || symb == floorsym)
			return(*argStkPtr);
		else if(symb == fracsym) {
			x1.len = 0;
			return(mkfloat(curFltPrec,&x1));
		}
	}
	/* argument is a float */
	getnumtrunc(prec,argStkPtr,&x1);
	s1 = x1.sign;
	intfrac(&x1,&y);
	if(symb == fracsym) {
		cpyarr(y.digits,y.len,AriBuf);
		y.digits = AriBuf;
		y.sign = s1;
		return(mkfloat(curFltPrec,&y));
	}
	floshiftint(&x1);
	if(symb == roundsym) {
		if(roundhalf(&y) == 1) {
		    x1.len = incarr(x1.digits,x1.len,1);
		    x1.sign = s1;
		}
	}
	else if((symb == floorsym) && s1 && y.len) {
		x1.sign = s1;
		x1.len = incarr(x1.digits,x1.len,1);
	}
	return(mkint(x1.sign,x1.digits,x1.len));
}
/*------------------------------------------------------------------*/
/*
** Von der in npt1 gegebenen Zahl wird destruktiv der ganzzahlige
** Anteil gebildet. Der Rest wird in npt2 gespeichert.
** Rest erhaelt dasselbe Vorzeichen wie die Ausgangszahl.
*/
PRIVATE void intfrac(npt1,npt2)
numdata *npt1, *npt2;
{
	word2 *x;
	long expo;
	int n, k;

	n = npt1->len;
	if(n == 0 || npt1->expo >= 0) {
		int2numdat(0,npt2);
		return;
	}
	n = alignfloat(n+1,npt1);
	expo = npt1->expo;
	k = (-expo) >> 4;	/* div 16, geht auf */
	if(k >= n) {
		cpynumdat(npt1,npt2);
		int2numdat(0,npt1);
	}
	else {
		x = npt1->digits;
		while(k > 0 && x[k-1] == 0)
			k--;
		cpyarr(x,k,npt2->digits);
		setarr(x,k,0);
		npt2->len = k;
		npt2->expo = expo;
		npt2->sign = npt1->sign;
	}
}
/*------------------------------------------------------------------*/
/*
** Ergibt 1 oder 0, je nachdem die in *nptr gegebene Zahl absolut
** groesser oder kleiner 1/2 ist
*/
PRIVATE int roundhalf(nptr)
numdata *nptr;
{
	long nn;

	nn = bit_length(nptr->digits,nptr->len);
	return(nn >= -nptr->expo ? 1 : 0);
}
/*------------------------------------------------------------------*/
PRIVATE void floshiftint(nptr)
numdata *nptr;
{
	nptr->len = lshiftarr(nptr->digits,nptr->len,nptr->expo);
	nptr->expo = 0;
}
/*------------------------------------------------------------------*/
PRIVATE truc Fpower()
{
	truc res;
	int flg, flg2, sign, m;
	unsigned int a;
	word2 *aa;

    flg = *FLAGPTR(argStkPtr-1);
    flg2 = *FLAGPTR(argStkPtr);
    if(flg >= fFIXNUM) {
        if(flg2 >= fFIXNUM)
            flg = (flg2 >= flg ? flg2 : flg);
        else {
            error(powersym,err_num,*argStkPtr);
            flg = aERROR;
        }
    }
    else if(flg == fGF2NINT) {
        if(flg2 == fFIXNUM || flg2 == fBIGNUM) {
            return exptgf2n(argStkPtr-1);
        }
        else {
            error(powersym,err_int,*argStkPtr);
            flg = aERROR;
        }
    }
    else {
        error(powersym,err_intt,argStkPtr[-1]);
        flg = aERROR;
    }
	if(flg == aERROR)
		res = brkerr();
	else if(flg <= fBIGNUM) {
		if(argStkPtr[-1] == zero)
			res = (argStkPtr[0] == zero ? constone : zero);
		else if(argStkPtr[-1] == constone)
			res = constone;
		else {
			m = bigref(argStkPtr,&aa,&sign);
			if(sign) {
				flg = fFLTOBJ; /* vorlaeufig */
			}
			else if(m <= 2) {
				a = big2long(aa,m);
				res = exptints(argStkPtr-1,a);
			}
			else {
				error(powersym,err_ovfl,voidsym);
				res = brkerr();
			}
		}
	}
	if(flg >= fFLTOBJ) {
		curFltPrec = deffltprec();
		res = exptfloats(argStkPtr-1);
	}
	return(res);
}
/*----------------------------------------------------------------*/
PRIVATE truc exptints(ptr,a)
truc *ptr;
unsigned a;
{
	word2 *x, *temp, *hilf;
	int sign, n;

	n = bigref(ptr,&x,&sign);
	if(exptovfl(x,n,a)) {
		error(powersym,err_ovfl,voidsym);
		return(brkerr());
	}
	temp = AriScratch;
	hilf = temp + aribufSize;
	n = power(x,n,a,AriBuf,temp,hilf);
	sign = (a & 1 ? sign : 0);
	return(mkint(sign,AriBuf,n));
}
/*----------------------------------------------------------------*/
PRIVATE truc exptfloats(ptr)
truc *ptr;
{
	numdata acc, acc2;
	word2 *hilf;
	int prec, flg, len;
	int odd, sign = 0;

	acc.digits = AriBuf;
	acc2.digits = AriScratch;
	hilf = AriScratch + aribufSize;

	prec = curFltPrec + 1;
	len = getnumtrunc(prec,ptr,&acc);
	if(!len) {
		sign = numposneg(ptr+1);
		if(sign < 0) {
			goto errexit;
		}
		else {
			int2numdat((sign > 0 ? 0 : 1),&acc);
			goto ausgang;
		}
	}
	if(acc.sign) {
		flg = *FLAGPTR(ptr+1);
		if(flg > fBIGNUM) {
			goto errexit;
		}
		else {
			odd = (flg == fFIXNUM ?
				*WORD2PTR(ptr+1) & 1 :
				*BIGNUMPTR(ptr+1) & 1);
			sign = (odd ? MINUSBYTE : 0);
		}
		acc.sign = 0;
	}
	len = lognum(prec,&acc,hilf);
	if(len >= 0) {
		getnumtrunc(prec,ptr+1,&acc2);
		len = multtrunc(prec,&acc,&acc2,hilf);
	}
	if(len >= 0)
		len = expnum(prec,&acc,hilf);
	if(len == aERROR) {
		error(powersym,err_ovfl,voidsym);
		return(brkerr());
	}
	acc.sign = sign;
  ausgang:
	return(mkfloat(curFltPrec,&acc));
  errexit:
	error(powersym,err_pnum,*ptr);
	return(brkerr());
}
/*----------------------------------------------------------------*/
/*
** Stellt fest, ob (x,n)**a zu overflow fuehrt.
** Rueckgabe aERROR bei overflow, oder 0 sonst
*/
PRIVATE int exptovfl(x,n,a)
word2 *x;
int n;
unsigned a;
{
	numdata pow;
	word4 bitbound, b;

	if(n == 0 || a <= 1)
		return(0);
	bitbound = MaxBits/a;
	b = n - 1;
	b <<= 4;
	b += bitlen(x[n-1]);
	if(b <= bitbound)
		return(0);
	else if(n > 1 || b  > bitbound+1)
		return(aERROR);
	else if(n == 1 && x[0] == 2 && a < MaxBits)
		return(0);
	pow.digits = AriBuf;
	pwrtrunc(2,x[0],256,&pow,AriScratch);
		/******* oder mit log1_16() ****************/
	b = pow.expo;
	b += ((pow.len-1)<<4) + bitlen(pow.digits[pow.len-1]);
	bitbound = (MaxBits << 8)/a;
	return(b > bitbound ? aERROR : 0);
}
/*------------------------------------------------------------------*/
PRIVATE truc Fisqrt()
{
	word2 *x, *z, *hilf;
	int sign, n, rlen;

	z = AriBuf;
	x = AriScratch;
	hilf = x + aribufSize;

	n = bigretr(argStkPtr,x,&sign);
	if(n == aERROR) {
		error(isqrtsym,err_int,*argStkPtr);
		return(brkerr());
	}
	if(sign) {
		error(isqrtsym,err_p0num,*argStkPtr);
		return(brkerr());
	}
	n = bigsqrt(x,n,z,&rlen,hilf);
	return(mkint(0,z,n));
}
/*------------------------------------------------------------------*/
/*
** returns 1 (resp. 0, -1) if number in *ptr1 is
** bigger than (resp. equal, smaller than) number in *ptr2
*/
PUBLIC int cmpnums(ptr1,ptr2,type)
truc *ptr1, *ptr2;
int type;	/* must be an integer or float type */
{
	word2 *x, *y;
	int k, n1, n2, sign1, sign2, cmp;

	if(*ptr1 == *ptr2)
		return(0);
	if(type == fFIXNUM) {
		sign1 = *SIGNPTR(ptr1);
		sign2 = *SIGNPTR(ptr2);
		if(sign1 != sign2)
			return(sign1 ? -1 : 1);
		cmp = (*WORD2PTR(ptr1) > *WORD2PTR(ptr2) ? 1 : -1);
		return(sign1 ? -cmp : cmp);
	}
	else if(type == fBIGNUM) {
		n1 = bigref(ptr1,&x,&sign1);
		n2 = bigref(ptr2,&y,&sign2);
		if(sign1 != sign2)
			return(sign1 ? -1 : 1);
		cmp = cmparr(x,n1,y,n2);
		return(sign1 ? -cmp : cmp);
	}
    else if(type == fGF2NINT) {
		n1 = bigref(ptr1,&x,&sign1);
		n2 = bigref(ptr2,&y,&sign2);
        return cmparr(x,n1,y,n2);
    }
	else if(type >= fFLTOBJ) {
		return(cmpfloats(ptr1,ptr2,fltprec(type)+1));
	}
	else
		return(aERROR);
}
/*-------------------------------------------------------------------*/
PRIVATE int cmpfloats(ptr1,ptr2,prec)
truc *ptr1, *ptr2;
int prec;
{
	numdata npt1, npt2;
	long expo1, expo2;
	int cmp, sign1, n1, n2;

	npt1.digits = AriBuf;
	npt2.digits = AriScratch;
	getnumtrunc(prec,ptr1,&npt1);
	getnumtrunc(prec,ptr2,&npt2);
	sign1 = npt1.sign;
	if(sign1 != npt2.sign)
		return(sign1 ? -1 : 1);
	n1 = normfloat(prec,&npt1);
	n2 = normfloat(prec,&npt2);
	expo1 = npt1.expo;
	expo2 = npt2.expo;
	if(expo1 > expo2) {
		return(sign1 ? -1 : 1);
	}
	else if(expo1 < expo2) {
		return(sign1 ? 1 : -1);
	}
	else {
		cmp = cmparr(npt1.digits,n1,npt2.digits,n2);
		return(sign1 ? -cmp : cmp);
	}
}
/*-----------------------------------------------------------*/
/*
** Num1 < Num2
*/
PRIVATE truc Farilt()
{
	int cmp = Gcompare(ariltsym);

	if(cmp == aERROR)
		return(brkerr());
	else
		return(cmp < 0 ? true : false);
}
/*-------------------------------------------------------------*/
/*
** Num1 > Num2
*/
PRIVATE truc Farigt()
{
	int cmp = Gcompare(arigtsym);

	if(cmp == aERROR)
		return(brkerr());
	else
		return(cmp > 0 ? true : false);
}
/*-----------------------------------------------------------*/
/*
** Num1 <= Num2
*/
PRIVATE truc Farile()
{
	int cmp = Gcompare(arilesym);

	if(cmp == aERROR)
		return(brkerr());
	else
		return(cmp <= 0 ? true : false);
}
/*---------------------------------------------------------------*/
/*
** Num1 >= Num2
*/
PRIVATE truc Farige()
{
	int cmp = Gcompare(arigesym);

	if(cmp == aERROR)
		return(brkerr());
	else
		return(cmp >= 0 ? true : false);
}
/*---------------------------------------------------------------*/
PRIVATE int Gcompare(symb)
truc symb;
{
	truc obj;
	char *errmsg;
	char *str1, *str2;
	int type, type1;

	type = *FLAGPTR(argStkPtr);
	if(type >= fFIXNUM) {
		if((type1 = *FLAGPTR(argStkPtr-1)) >= fFIXNUM) {
			type = (type >= type1 ? type : type1);
			return(cmpnums(argStkPtr-1,argStkPtr,type));
		}
		else {
			errmsg = err_mism;
			obj = argStkPtr[-1];
		}
	}
	else if(type == fSTRING) {
		if(*FLAGPTR(argStkPtr-1) == fSTRING) {
			str1 = STRINGPTR(argStkPtr-1);
			str2 = STRINGPTR(argStkPtr);
			return(strcmp(str1,str2));
		}
		else {
			errmsg = err_mism;
			obj = argStkPtr[-1];
		}
	}
	else if(type == fCHARACTER) {
		if(*FLAGPTR(argStkPtr-1) == fCHARACTER) {
			return(*WORD2PTR(argStkPtr-1) - *WORD2PTR(argStkPtr));
		}
		else {
			errmsg = err_mism;
			obj = argStkPtr[-1];
		}
	}
	else {
		errmsg = err_type;
		obj = argStkPtr[0];
	}
	return(error(symb,errmsg,obj));
}
/*--------------------------------------------------------*/
PRIVATE void inirandstate(rr)
word2 *rr;
{
	rr[1] = sysrand();
	nextrand(rr,3);
	rr[0] = sysrand();
	nextrand(rr,3);
	rr[3] = 1;
}
/*--------------------------------------------------------*/
PRIVATE void nextrand(rr,n)
word2 *rr;
int n;
{
	/* for compilers which don't understand 57777U */
	static unsigned inc = 57777, scal = 56857;
	/* 57777 = 1 mod 4,  56857 prime */

	incarr(rr,n,inc);
	multarr(rr,n,scal,rr);
	rr[3] = 1;
}
/*------------------------------------------------------------------*/
/*
** 2-byte random integer
*/
PUBLIC unsigned random2(u)
unsigned u;
{
	nextrand(RandSeed,2);
	return(RandSeed[1] % u);
}
/*------------------------------------------------------------------*/
/*
** 4-byte random integer
*/
PUBLIC unsigned random4(u)
unsigned u;
{
    word4 v;

	nextrand(RandSeed,3);
    v = big2long(RandSeed+1,2);
	return(v % u);
}
/*------------------------------------------------------------------*/
/*
** random(bound)
*/
PRIVATE truc Frandom()
{
	numdata acc, acc2;
	word2 *x;
	unsigned a, b;
	int i, n, m, prec, type;

	type = chknum(randsym,argStkPtr);
	if(type == aERROR)
		return(brkerr());
	if(type == fFIXNUM) {
		nextrand(RandSeed,2);
		a = *WORD2PTR(argStkPtr);
		if(!a)
			return(zero);
		b = RandSeed[1] % a;
		return(mkfixnum(b));
	}
	else if(type >= fFLTOBJ) {
		curFltPrec = deffltprec();
		prec = curFltPrec + 1;
		for(x=AriBuf, i=0; i<curFltPrec; i+=2, x+=2) {
			nextrand(RandSeed,2);
			cpyarr(RandSeed,2,x);
		}
		acc.digits = AriBuf;
		acc.sign = 0;
		acc.len = curFltPrec;
		acc.expo = -(curFltPrec<<4);
		refnumtrunc(prec,argStkPtr,&acc2);
		n = multtrunc(prec,&acc,&acc2,AriScratch);
		return(mkfloat(curFltPrec,&acc));
	}
	else {			/* bignum */
		n = *BIGLENPTR(argStkPtr);
		for(x=AriBuf, i=0; i<n; i+=2, x+=2) {
			nextrand(RandSeed,3);
			cpyarr(RandSeed+1,2,x);
		}
		m = n;
		while(m > 0 && AriBuf[m-1] == 0)
			m--;
		x = BIGNUMPTR(argStkPtr);
		m = modbig(AriBuf,m,x,n,AriScratch);
		return(mkint(0,AriBuf,m));
	}
}
/*------------------------------------------------------------------*/
PRIVATE truc Frandseed(argn)
int argn;
{
	word2 *x;
	int sign, n, m;

	if(argn == 1) {
		n = bigref(argStkPtr,&x,&sign);
		if(n != aERROR) {
			m = (n > 3 ? 3 : n);
			cpyarr(x,m,RandSeed);
			setarr(RandSeed+m,3-m,0);
		}
	}
	return(mkint(0,RandSeed,4));
}
/*------------------------------------------------------------------*/
PRIVATE int objfltprec(obj)
truc obj;
{
	variant v;
	int flg, prec;

	if(obj == sfloatsym || obj == dfloatsym || 
         obj == lfloatsym || obj == xfloatsym) {
		v.xx = SYMbind(obj);
		prec = v.pp.ww;
	}
	else {
		v.xx = obj;
		flg = v.pp.b0;
		if(flg >= fFLTOBJ)
			prec = fltprec(flg);
		else
			prec = deffltprec();

	}
	return(prec);
}
/*------------------------------------------------------------------*/
PRIVATE truc Ffloat(argn)
int argn;	/* argn = 1 or 2 */
{
	truc *argptr;
	numdata xx;
	int prec;

	if(argn == 1)
		prec = deffltprec();
	else
		prec = precdesc(*argStkPtr);

	argptr = argStkPtr - argn + 1;
	if(chknum(floatsym,argptr) == aERROR)
		return(*argptr);

	xx.digits = AriBuf;
	getnumtrunc(prec+1,argptr,&xx);

	return(mkfloat(prec,&xx));
}
/*------------------------------------------------------------------*/
PRIVATE int precdesc(obj)
truc obj;
{
	variant v;
	int flg, prec, bits;

	if(obj == sfloatsym || obj == dfloatsym || 
         obj == lfloatsym || obj == xfloatsym) {
		v.xx = SYMbind(obj);
		prec = v.pp.ww;
	}
	else {
		v.xx = obj;
		flg = v.pp.b0;
		if(flg == fFIXNUM) {
			bits = v.pp.ww;
			prec = (bits + 15)/16;
		}
		else
			prec = deffltprec();
	}
	return(prec);
}
/*------------------------------------------------------------------*/
PRIVATE truc Fsetfltprec(argn)
int argn;
{
	int prec;
	truc obj;
	int prnflg = 1;

	if(argn == 2) {
		obj = argStkPtr[-1];
		if(*argStkPtr == zero)
			prnflg = 0;
	}
	else
		obj = *argStkPtr;
	prec = precdesc(obj);
	prec = setfltprec(prec);
	if(prnflg)
		setprnprec(prec);
	return(mkfixnum(16*prec));
}
/*------------------------------------------------------------------*/
PRIVATE truc Fsetprnprec()
{
	int prec;

	prec = precdesc(*argStkPtr);
	prec = setprnprec(prec);
	return(mkfixnum(16*prec));
}
/*------------------------------------------------------------------*/
PRIVATE truc Fgetprnprec()
{
	int prec;

	prec = setprnprec(-1);
	return(mkfixnum(16*prec));
}
/*------------------------------------------------------------------*/
PRIVATE truc Fgetfltprec(argn)
int argn;
{
	int prec;

	if(argn == 0)
		prec = deffltprec();
	else
		prec = objfltprec(*argStkPtr);

	return(mkfixnum(16*prec));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fmaxfltprec()
{
	int prec;

	prec = maxfltprec();
	return(mkfixnum(16*prec)); 
}
/*-------------------------------------------------------------*/
PRIVATE truc Fdecode()
{
	truc *ptr;
	truc vec, obj;
	numdata acc;
	long expo;
	int flg, len;

	flg = *FLAGPTR(argStkPtr);
	if(flg < fFLTOBJ) {
		if(flg >= fFIXNUM) {
			*argStkPtr = Ffloat(1);
			flg = *FLAGPTR(argStkPtr);
		}
		else {
			error(decodsym,err_float,*argStkPtr);
			return(brkerr());
		}
	}
	len = fltprec(flg);

	acc.digits = AriBuf;
	len = getnumtrunc(len,argStkPtr,&acc);
	expo = (len ? acc.expo : 0);
	obj = mkint(acc.sign,AriBuf,len);
    WORKpush(obj);
	obj = mkinum(expo);
    WORKpush(obj);
	vec = mkvect0(2);
	ptr = VECTOR(vec);
    ptr[1] = WORKretr();
    ptr[0] = WORKretr();
	return(vec);
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbitnot()
{
	int n, sign;

	if(chkints(bnotsym,argStkPtr,1) == aERROR)
		return(brkerr());

	n = bigretr(argStkPtr,AriBuf,&sign);
	if(sign) {
		n = decarr(AriBuf,n,1);
		sign = 0;
	}
	else {
		n = incarr(AriBuf,n,1);
		sign = 1;
	}
	return(mkint(sign,AriBuf,n));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbitset()
{
	return(Gbitset(bsetsym));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbitclear()
{
	return(Gbitset(bclrsym));
}
/*-------------------------------------------------------------*/
PRIVATE truc Gbitset(symb)
truc symb;
{
	word2 *x, *y;
	long index;
	int b, n, n1, m, sign, sign1;
	word2 u, mask = 1;

	if(chkints(symb,argStkPtr-1,2) == aERROR)
		return(brkerr());
	x = AriBuf;
	n = twocretr(argStkPtr-1,x);
	u = x[n];
	sign = (u == 0xFFFF ? MINUSBYTE : 0);

	n1 = bigref(argStkPtr,&y,&sign1);
	if(sign1) {
		error(symb,err_p0num,*argStkPtr);
		return(brkerr());
	}
	if(n1 > 2)
		index = 0xFFFF0;
	else
		index = big2long(y,n1);
	b = index & 0xF;
	index >>= 4;
	m = index;
	if(index >= n) {
		if((sign && symb == bsetsym) ||
			(!sign && symb == bclrsym)) {
			return(argStkPtr[-1]);	/* no action taken */
		}
		else if(index >= aribufSize) {
			error(symb,err_ovfl,voidsym);
			return(argStkPtr[-1]);
		}
		else {
			setarr(x+n+1,m-n,u);
			n = m + 1;
		}
	}
	mask <<= b;
	if(symb == bsetsym)
		x[m] |= mask;
	else
		x[m] &= ~mask;
	while((n > 0) && (x[n-1] == u))
		n--;
	if(sign) {
		notarr(x,n);
		n = incarr(x,n,1);
	}
	return(mkint(sign,x,n));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbittest()
{
	word2 *x;
	long index;
	int k, b, n, sign;
	word2 mask = 1;

    if(chkintt(btestsym,argStkPtr-1) == aERROR ||
	   chkint(btestsym,argStkPtr) == aERROR)
		return(brkerr());

	n = bigref(argStkPtr-1,&x,&sign);
	if(sign) {
		x = AriBuf;
		n = twocretr(argStkPtr-1,x);
	}
	index = intretr(argStkPtr);
	if(index == LONGERROR)
		return(sign ? constone : zero);
	else if(index < 0) {
		error(btestsym,err_p0num,*argStkPtr);
		return(brkerr());
	}
	k = index >> 4;
	if(index > 0x7FFF0 || k >= n)
		return(sign ? constone : zero);
	b = index & 0xF;
	mask <<= b;
	return(x[k] & mask ? constone : zero);
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbitshift()
{
	long sh, nn;
	int n, sign;

	if(chkints(bshiftsym,argStkPtr-1,2) == aERROR)
		return(brkerr());

	nn = n = bigretr(argStkPtr-1,AriBuf,&sign);
	sh = intretr(argStkPtr);
	if(sh == LONGERROR || sh >= maxfltex - (nn<<4)) {
		error(bshiftsym,err_ovfl,voidsym);
		return(brkerr());
	}
	if(sign && sh < 0) {
		n = decarr(AriBuf,n,1);
		n = lshiftarr(AriBuf,n,sh);
		n = incarr(AriBuf,n,1);
	}
	else
		n = lshiftarr(AriBuf,n,sh);
	return(mkint(sign,AriBuf,n));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbitlength()
{
	word2 *x;
	long len;
	int n, sign;

	if(chkintt(blensym,argStkPtr) == aERROR)
		return(brkerr());

	n = bigref(argStkPtr,&x,&sign);
	len = bit_length(x,n);
	return(mkinum(len));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbitcount()
{
	word2 *x;
	long count;
	int k, n, sign;

	if(chkintt(blensym,argStkPtr) == aERROR)
		return(brkerr());

	n = bigref(argStkPtr,&x,&sign);
    count = 0;
    for(k=0; k<n; k++)
        count += bitcount(x[k]);
	return(mkinum(count));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fcardinal()
{
	return(Gcardinal(cardsym));
}
/*-------------------------------------------------------------*/
PRIVATE truc Finteger()
{
	return(Gcardinal(int_sym));
}
/*-------------------------------------------------------------*/
PRIVATE truc Gcardinal(symb)
truc symb;
{
	word2 *x;
	byte *bpt;
	unsigned u;
	unsigned len;
	int i, n, flg;
	int sign = 0;

	flg = *FLAGPTR(argStkPtr);
    if(flg == fGF2NINT) {
        n = bigretr(argStkPtr,AriBuf,&sign);
        return mkint(0,AriBuf,n);
    }
	if(flg != fBYTESTRING && flg != fSTRING) {
		error(symb,err_bystr,*argStkPtr);
		return(brkerr());
	}
	len = *STRLENPTR(argStkPtr);
	if(len >= aribufSize*2 - 2) {
		error(symb,err_2long,mkfixnum(len));
		return(brkerr());
	}
	bpt = (byte *)STRINGPTR(argStkPtr);
	if(symb == int_sym && (bpt[len-1] & 0x80))
		sign = MINUSBYTE;
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
		if(sign)
			*x |= 0xFF00;
		n++;
	}
	if(sign) {
		notarr(AriBuf,n);
		n = incarr(AriBuf,n,1);
	}
	while(n > 0 && AriBuf[n-1] == 0)
		n--;
	return(mkint(sign,AriBuf,n));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbitand()
{
	return(Gboole(bandsym,and2arr));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbitor()
{
	return(Gboole(borsym,or2arr));
}
/*-------------------------------------------------------------*/
PRIVATE truc Fbitxor()
{
	return(Gboole(bxorsym,xor2arr));
}
/*------------------------------------------------------------------*/
PRIVATE truc Gboole(symb,boolfun)
truc symb;
ifunaa boolfun;
{
	int sign;
	int n, m;

	if(chkints(symb,argStkPtr-1,2) == aERROR)
		return(brkerr());
	n = twocretr(argStkPtr-1,AriBuf);
	m = twocretr(argStkPtr,AriScratch);
	n = boolfun(AriBuf,n,AriScratch,m);
	
	sign = (AriBuf[n] == 0xFFFF ? MINUSBYTE : 0);
	if(sign) {
		notarr(AriBuf,n);
		n = incarr(AriBuf,n,1);
	}
	return(mkint(sign,AriBuf,n));
}
/*--------------------------------------------------------*/
PRIVATE int chkplusargs(sym,argptr)
truc sym;
truc *argptr;
{
    int flg, flg1;

    flg = *FLAGPTR(argptr);
    flg1 = *FLAGPTR(argptr+1);
    if(flg >= fFIXNUM) {
        if(flg1 >= fFIXNUM)
            return (flg1 >= flg ? flg1 : flg);
        else
            return error(sym,err_num,argptr[1]);
    }
    else if((flg == flg1) && (flg == fVECTOR || flg == fGF2NINT))
        return flg;
    else
        return error(sym,err_num,*argptr);
}
/*--------------------------------------------------------*/
PRIVATE int chktimesargs(argptr)
truc *argptr;
{
    int flg, flg1;
    truc ele;

    flg = *FLAGPTR(argptr);
    flg1 = *FLAGPTR(argptr+1);

    if(flg < fFIXNUM) {
        if(flg == fVECTOR) {  /* then second argument must be a scalar */
            if(flg1 >= fFIXNUM || flg1 == fGF2NINT) { /* swap args */
                ele = *argptr;
                *argptr = argptr[1];
                *(argptr+1) = ele;
                return (flg1 | (flg << 8));
            }
            else
                return error(timessym,err_num,argptr[1]);
        }
        else if(flg == fGF2NINT) {
            if(flg1 == flg)
                return flg;
            else if(flg1 == fVECTOR)
                return (flg | (flg1 << 8));
        }
        return error(timessym,err_num,*argptr);
    }
    /* here flg >= fFIXNUM */
    if(flg1 >= fFIXNUM) {
        return (flg1 >= flg ? flg1 : flg);
    }
    else if(flg1 == fVECTOR)
        return (flg | (flg1 << 8));
    else
        return error(timessym,err_num,argptr[1]);
}
/*--------------------------------------------------------*/
PRIVATE int chkmodargs(sym,argptr)
truc sym;
truc *argptr;
{
    int flg, flg0;

    flg0 = *FLAGPTR(argptr+1);
    if(flg0 < fFIXNUM || flg0 > fBIGNUM)
        return error(sym,err_num,argptr[1]);
	flg = *FLAGPTR(argptr);
	if((flg < fFIXNUM && flg != fVECTOR) || (flg > fBIGNUM))
        return error(sym,err_num,argptr[0]);
	if(flg == fVECTOR)
		return((fVECTOR<<8) | flg0);
	else
		return(flg0);
}
/*--------------------------------------------------------*/
PRIVATE int chkdivfargs(sym,argptr)
truc sym;
truc *argptr;
{
    int flg, flg0;

    flg0 = *FLAGPTR(argptr+1);
	flg = *FLAGPTR(argptr);
    if(flg0 < fFIXNUM) {
        if(flg0 == fGF2NINT && flg == flg)
            return flg;
        else
            return error(sym,err_num,argptr[1]);
    }
	if(flg < fFIXNUM && flg != fVECTOR)
        return error(sym,err_num,argptr[0]);

	if(flg == fVECTOR)
		return((fVECTOR<<8) | flg0);
	else
		return(flg0);
}
/*********************************************************************/
