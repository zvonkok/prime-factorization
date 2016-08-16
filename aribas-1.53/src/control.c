/****************************************************************/
/* file control.c

ARIBAS interpreter for Arithmetic
Copyright (C) 1996-2002 O.Forster

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
#define TYPEIDENT
/*
** control.c
** function definition, logical and control functions
**
** date of last change
** 1995-02-22: lpbrksym
** 1995-03-15: const
** 1995-03-20: changed make_unbound
** 1995-03-31: pointer
** 1997-04-22: type symbol, reorg (newintsym), changed Sfor
** 1997-08-18: removed bug (discovered by M.Zimmer, Leipzig) in Sfor
** 1998-01-17: small change in Sfor (regarding toolong)
** 1998-10-07: continue statement
** 1999-06-21: make_unbound(user)
** 2002-03-27: small change in Lvalassign, new function is_lval()
** 2002-04-04: simultaneous assignment (x1,...,xn) := (val1,...,valn)
** 2002-04-27: gmtime
** 2003-02-28: case fGF2NINT in nulltest()
** 2004-06-20: function type_ident
** 2004-10-30: for-loop can now do more than 2**32 iterations
*/

#include "common.h"

PUBLIC void inicont	_((void));
PUBLIC int is_lval  _((truc *ptr));
PUBLIC int Lvaladdr	_((truc *ptr, trucptr *pvptr));
PUBLIC truc Lvalassign	_((truc *ptr, truc obj));
PUBLIC truc Swhile	_((void));
PUBLIC truc Sfor	_((void));
PUBLIC void Sifaux	_((void));
PUBLIC truc Sexit	_((void));
PUBLIC truc brkerr	_((void));

PUBLIC truc Lconsteval	_((truc *ptr));
PUBLIC int Lconstini	_((truc consts));
PUBLIC truc unbindsym	_((truc *ptr));
PUBLIC truc unbinduser  _((void));

PUBLIC truc boolsym;
PUBLIC truc truesym, falsesym, true, false, nil;
PUBLIC truc equalsym, nequalsym;
PUBLIC truc exitsym, exitfun, lpbrksym, lpbrkfun, lpcontsym, lpcontfun;
PUBLIC truc whilesym, dosym, ifsym, thensym, elsifsym, elsesym;
PUBLIC truc forsym, tosym, bysym;
PUBLIC truc constsym, varsym, var_sym, inivarsym, typesym;
PUBLIC truc not_sym, notsym;
PUBLIC truc voidsym, nullsym;
PUBLIC truc breaksym, contsym, contnsym, errsym;
PUBLIC truc retsym, ret_sym;
PUBLIC truc assignsym;
PUBLIC truc arisym, usersym;
PUBLIC truc symbsym;

PUBLIC truc funcsym, procsym;
PUBLIC truc extrnsym;
PUBLIC truc beginsym, endsym;

PUBLIC truc *brkbindPtr, *brkmodePtr;
/*--------------------------------------------------*/
PRIVATE truc symbolssym;
PRIVATE truc timersym, gmtimsym;
PRIVATE truc mkunbdsym;
PRIVATE truc andsym, orsym;

PRIVATE truc *constbindPtr;

PRIVATE truc Fequal	_((void));
PRIVATE truc Fnequal	_((void));
PRIVATE int equal	_((truc *ptr1, truc *ptr2));
PRIVATE truc Fnot	_((void));
PRIVATE truc Sand	_((void));
PRIVATE truc Sor	_((void));
PRIVATE int nulltest	_((truc obj));
PRIVATE truc Sinivars	_((void));
PRIVATE truc Svarparm	_((void));
PRIVATE truc Sassign	_((void));
PRIVATE int symbaddr	_((truc *ptr, trucptr *pvptr));
PRIVATE int increment	_((word2 *x, int n, int *signptr,
			   word2 *inc, int inclen, int s));
PRIVATE truc Freturn	_((void));
PRIVATE truc Slpbreak	_((void));
PRIVATE truc Slpcont	_((void));
PRIVATE truc Stimer	    _((void));
PRIVATE truc Fgmtime    _((int argn));
PRIVATE truc Smkunbound	  _((void));
PRIVATE truc Fsymbols	_((void));
PRIVATE int symbcmp	_((truc *ptr1, truc *ptr2));

#ifdef TYPEIDENT
PRIVATE truc typeidsym;
PRIVATE truc Ftypeident _((void));
PRIVATE int typevalue   _((truc symb));
#endif
/*----------------------------------------------------------------------*/
PUBLIC void inicont()
{
	truc temp;
	truc and_sym, or_sym, const_sym;
	variant v;

	v.pp.b0	   = fBOOL;
	v.pp.b1	   = 0;
	v.pp.ww	   = 1;
	true	   = v.xx;
	v.pp.ww	   = 0;
	false	   = v.xx;

	boolsym	   = newsym("boolean", sTYPESPEC, false);

	voidsym	   = newselfsym("",	sINTERNAL);
	nullsym	   = newselfsym("",	sINTERNAL);
	contsym	   = newselfsym("cont", sINTERNAL);
	contnsym   = newselfsym("contn",sINTERNAL);
	errsym	   = newselfsym("error",sINTERNAL);

	exitsym	   = newsym("exit",   sPARSAUX, nullsym);
	temp	   = newintsym("exit",sSBINARY, (wtruc)Sexit);
	exitfun	   = mk0fun(temp);
	lpbrksym   = newsym("break",  sPARSAUX, nullsym);
	temp	   = newintsym("break",sSBINARY, (wtruc)Slpbreak);
	lpbrkfun   = mk0fun(temp);
	lpcontsym  = newsym("continue",  sPARSAUX, nullsym);
	temp	   = newintsym("continue",sSBINARY, (wtruc)Slpcont);
	lpcontfun  = mk0fun(temp);

	breaksym   = newsym("$break", sINTERNVAR, voidsym);
	brkbindPtr = SYMBINDPTR(&breaksym);
	brkmodePtr = (truc *)SYMCCPTR(&breaksym);
	*brkmodePtr = breaksym;

	equalsym   = newintsym("=",  sFBINARY, (wtruc)Fequal);
	nequalsym  = newintsym("/=", sFBINARY, (wtruc)Fnequal);

	assignsym  = newintsym(":= ", sSBINARY, (wtruc)Sassign);

	funcsym	   = newsym("function", sPARSAUX, nullsym);
	procsym	   = newsym("procedure",sPARSAUX, nullsym);
	extrnsym   = newsym("external", sDELIM,	  nullsym);
	varsym	   = newsym("var",	sPARSAUX, nullsym);
	var_sym	   = newintsym("_var",	sSBINARY,(wtruc)Svarparm);
	inivarsym  = newintsym("var",	sSBINARY,(wtruc)Sinivars);
	constsym   = newsym("const",	sPARSAUX, nullsym);
	const_sym  = newsym("$const", sINTERNVAR, voidsym);
	constbindPtr = SYMBINDPTR(&const_sym);
	typesym	   = newsym("type",	sPARSAUX, nullsym);

	whilesym   = newsym("while", sPARSAUX, nullsym);
	forsym	   = newsym("for",   sPARSAUX, nullsym);
	ifsym	   = newsym("if",    sPARSAUX, nullsym);
	tosym	   = newsym("to",    sDELIM,   nullsym);
	bysym	   = newsym("by",    sDELIM,   nullsym);
	dosym	   = newsym("do",    sDELIM,   nullsym);
	thensym	   = newsym("then",  sDELIM,   nullsym);
	elsifsym   = newsym("elsif", sDELIM,   nullsym);
	elsesym	   = newsym("else",  sDELIM,   nullsym);
	beginsym   = newsym("begin", sDELIM,   nullsym);
	endsym	   = newsym("end",   sDELIM,   nullsym);

	not_sym	   = newintsym("not",sFBINARY, (wtruc)Fnot);
	notsym	   = newsym("not",   sPARSAUX, not_sym);

	ret_sym	   = newintsym("return",sFBINARY, (wtruc)Freturn);
	retsym	   = newsym("return",	sPARSAUX, ret_sym);

	and_sym	   = newintsym("and",sSBINARY, (wtruc)Sand);
	andsym	   = newsym("and",	sINFIX, and_sym);
    SYMcc1(andsym) = ANDTOK;

	or_sym	   = newintsym("or", sSBINARY, (wtruc)Sor);
	orsym	   = newsym("or",	sINFIX, or_sym);
    SYMcc1(orsym) = ORTOK;

	truesym	   = newsym("true",  sSCONSTANT, true);
	falsesym   = newsym("false", sSCONSTANT, false);

	nil	       = newreflsym("nil",	  sSCONSTANT);
	arisym	   = newreflsym("aribas", sSYSSYMBOL);
	usersym	   = newreflsym("user",	  sSYSSYMBOL);
#ifdef DEVEL
	symbsym	   = newreflsym("symbol", sTYPESPEC);
#else
	symbsym = nullsym;
#endif
	timersym   = newsymsig("timer", sSBINARY, (wtruc)Stimer, s_0);
    gmtimsym   = newsymsig("gmtime",sFBINARY, (wtruc)Fgmtime,s_01);
	mkunbdsym  = newsymsig("make_unbound", sSBINARY,
				(wtruc)Smkunbound, s_bV);
	symbolssym = newsymsig("symbols", sFBINARY, (wtruc)Fsymbols, s_1);
#ifdef TYPEIDENT
	typeidsym  = newsymsig("type_ident", sFBINARY, (wtruc)Ftypeident, s_1);
#endif
}
/*----------------------------------------------------------------------*/
PRIVATE truc Fequal()
{
	return(equal(argStkPtr-1,argStkPtr) ? true : false);
}
/*----------------------------------------------------------*/
PRIVATE truc Fnequal()
{
	return(equal(argStkPtr-1,argStkPtr) ? false : true);
}
/*----------------------------------------------------------*/
PRIVATE int equal(ptr1,ptr2)
truc *ptr1, *ptr2;
{
	char *cpt1, *cpt2;
	unsigned n, i;
	int flg, flg2;

	if(*ptr1 == *ptr2)
		return(1);

	flg = *FLAGPTR(ptr1);
	flg2 = *FLAGPTR(ptr2);

	if(flg >= fFIXNUM && flg2 >= fFIXNUM) {
		if(flg2 > flg)
			flg = flg2;
		return(cmpnums(ptr1,ptr2,flg) ? 0 : 1);
	}
	else if(flg != flg2) {
		if(*ptr2 != nil && *ptr1 != nil)
			return(0);
		else if(*ptr2 == nil) {
			if(flg == fPOINTER && *PTARGETPTR(ptr1) == nil)
				return(1);
		}
		else {	/* *ptr1 == nil */
			if(flg2 == fPOINTER && *PTARGETPTR(ptr2) == nil)
				return(1);
		}
		return(0);
	}
	else switch(flg) {  /* here flg == flg2 */
    case fGF2NINT:
		return(cmpnums(ptr1,ptr2,flg) ? 0 : 1);
	case fSTRING:
	case fBYTESTRING:
		n = *STRLENPTR(ptr1);
		if(n != *STRLENPTR(ptr2))
			return(0);
		cpt1 = STRINGPTR(ptr1);
		cpt2 = STRINGPTR(ptr2);
		for(i=0; i<n; i++)
			if(*cpt1++ != *cpt2++)
				return(0);
		return(1);
	case fVECTOR:
	case fTUPLE:
	case fRECORD:
		n = *VECLENPTR(ptr1);
		if(n != *VECLENPTR(ptr2))
			return(0);
		ptr1 = VECTORPTR(ptr1);
		ptr2 = VECTORPTR(ptr2);
		if(flg == fRECORD)
			n++;
		for(i=0; i<n; i++)
			if(!equal(ptr1++,ptr2++))
				return(0);
		return(1);
	case fPOINTER:
		return(*PTARGETPTR(ptr1) == *PTARGETPTR(ptr2));
	default:
		return(0);
	}
}
/*----------------------------------------------------------*/
PRIVATE truc Fnot()
{
	int val;

	val = nulltest(*argStkPtr);
	if(val > 0)
		return(false);
	else if(!val)
		return(true);
	else {
		error(notsym,err_bool,*argStkPtr);
		return(brkerr());
	}
}
/*-----------------------------------------------------------*/
PRIVATE truc Sand()
{
	truc obj;
	int val;

	obj = eval(ARG0PTR(evalStkPtr));
	val = nulltest(obj);
	if(!val)
		return(false);
	else if(val > 0) {
		obj = eval(ARG1PTR(evalStkPtr));
		val = nulltest(obj);
		if(!val) 
			return(false);
		else if(val > 0)
			return(true);
	}
	error(andsym,err_bool,obj);
	return(brkerr());
}
/*-----------------------------------------------------------*/
PRIVATE truc Sor()
{
	truc obj;
	int val;

	obj = eval(ARG0PTR(evalStkPtr));
	val = nulltest(obj);
	if(val > 0)
		return(true);
	else if(!val) {
		obj = eval(ARG1PTR(evalStkPtr));
		val = nulltest(obj);
		if(val > 0)
			return(true);
		else if(!val)
			return(false);
	}
	error(orsym,err_bool,obj);
	return(brkerr());
}
/*-----------------------------------------------------------*/
/*
** returns 0 if obj represents false, 1 if true
** returns aERROR in case of error
*/
PRIVATE int nulltest(obj)
truc obj;
{
	variant v;

	v.xx = obj;
	switch(v.pp.b0) {	/* flag */
	case fBOOL:
	case fFIXNUM:
	case fCHARACTER:
		return(v.pp.ww ? 1 : 0);
	case fBIGNUM:		/* bignum is not zero */
		return(1);
    case fGF2NINT:
        return (obj == gf2nzero ? 0 : 1);
	default:
		return(aERROR);
	}
}
/*----------------------------------------------------------*/
PRIVATE truc Sinivars()
{
	struct symbol *sptr;
	truc *ptr;
	truc obj;
	int i,n;

	ptr = TAddress(evalStkPtr);
	WORKpush(ptr[1]);		/* list of variables */
	ARGpush(ptr[2]);		/* list of initial values */
	ptr = TAddress(argStkPtr);
	n = *WORD2PTR(ptr);		/* number of variables */
	for(i=1; i<=n; i++) {
		ptr = TAddress(argStkPtr) + i;
		obj = eval(ptr);
		ptr = TAddress(workStkPtr) + i;
		sptr = SYMPTR(ptr);
		*FLAGPTR(sptr) = sVARIABLE;
		sptr->bind.t = obj;
	}
	ARGpop();
	WORKpop();
	return(varsym);
}
/*-----------------------------------------------------------*/
/*
** Initialisierung der lokalen Konstanten (nach dem
** Lesen der const-Deklaration)
** Werte werden an das Symbol const_sym als fTUPLE gebunden
*/
PUBLIC int Lconstini(consts)
truc consts;
{
	truc *ptr;
	truc obj;
	unsigned i, n;
	int res = 0;

	if(consts == voidsym) {
		*constbindPtr = voidsym;
		return(0);
	}
	ptr = Taddress(consts);
	*constbindPtr = ptr[2];
	ptr = TAddress(constbindPtr);
	n = *WORD2PTR(ptr);
	for(i=1; i<=n; i++) {
		obj = eval(++ptr);
		if(obj == breaksym) {
			res = aERROR;
			break;
		}
		else
			res = i;
		ptr = TAddress(constbindPtr) + i;
		/* ptr must be evaluated again, since gc may have occurred */
		*ptr = obj;
	}
	return(res);
}
/*-----------------------------------------------------------*/
/*
** bestimmt den Wert einer lokalen Konstanten
** (waehrend des Compilierens einer benutzerdefinierten Funktion)
*/
PUBLIC truc Lconsteval(ptr)
truc *ptr;
{
	truc *vec;
	unsigned n,len;

	n = *WORD2PTR(ptr);
	if(*FLAGPTR(constbindPtr) == fTUPLE)  {
		len = *VECLENPTR(constbindPtr);
		if(n < len) {
			vec = VECTORPTR(constbindPtr);
			return(vec[n]);
		}
	}
	error(constsym,err_case,mkfixnum(n));
	return(zero);
}
/*-----------------------------------------------------------*/
PRIVATE truc Svarparm()
{
	truc obj;

	obj = eval(ARG0PTR(evalStkPtr));
	return(obj);
}
/*-----------------------------------------------------------*/
PRIVATE truc Sassign()
{
	truc obj;
	int flg;

	obj = eval(ARG1PTR(evalStkPtr));
	flg = Tflag(obj);
	if(flg <= fVECTLIKE1 && flg >= fRECORD) {
		WORKpush(obj);
		if(flg < fCONSTLIT) {	/* fRECORD or fVECTOR */
			obj = mkarrcopy(workStkPtr);
		}
		else {	/* flg == fSTRING || flg == fBYTESTRING */
			obj = mkcopy(workStkPtr);
		}
		WORKpop();
	}
	return(Lvalassign(ARG0PTR(evalStkPtr),obj));
}
/*-----------------------------------------------------------*/
/*
** Moegliche lvals sind entweder Symbole mit den flags
** fSYMBOL (globales Symbol)
** fLSYMBOL (lokales Symbol)
** fRSYMBOL (Referenz auf Symbol [global oder lokal] bei var-Parametern)
** sowie Array-Elemente, Sub-Arrays, Record-Felder, Pointer-Referenzen
**
** In *pvptr wird entweder ein Pointer auf die bind-Zelle eines Symbols
** abgelegt (Rueckgebewert vBOUND oder vUNBOUND)
** oder ein Pointer ptr auf eine Funktion zur Beschreibung des lvals:
** Rueckgabewert vARRELE:
** ptr[0] = arr_sym, ptr[1] = Array, ptr[2] = Index
** vSUBARRAY:
** ptr[0] = subarrsym, ptr[1] = Array, ptr[2] = Paar mit Subarray-Grenzen
** vRECFIELD:
** ptr[0] = rec_sym, ptr[1] = Record, ptr[2] = field
** vPOINTREF:
** ptr[0] = derefsym, ptr[1] = Pointer
** vVECTOR:
** ptr[0] = vectorsym, ptr[1] = len, ptr[2] = ele0, ptr[3] = ele1, ..
** Die Argumente in ptr[1] bzw. ptr[2] sind jeweils unausgewertet.
*/
PUBLIC int Lvaladdr(ptr,pvptr)
truc *ptr;
trucptr *pvptr;
{
	int flag;

	flag = *FLAGPTR(ptr);
	if(flag == fLSYMBOL) {
		*pvptr = LSYMBOLPTR(ptr);
		return(vBOUND);
	}
	else if(flag == fSYMBOL) {
		return(symbaddr(ptr,pvptr));
	}
	else if(flag == fRSYMBOL) {
		ptr = LSYMBOLPTR(ptr);
		if((flag = *FLAGPTR(ptr)) == fSYMBOL)
			return(symbaddr(ptr,pvptr));
		else if(flag == fLRSYMBOL) {
			*pvptr = LRSYMBOLPTR(ptr);
			return(vBOUND);
		}
		/* else fall through */
	}
	if(flag >= fSPECIAL1 && flag <= fBUILTINn) {
		/* array access or record access or pointer reference or vector*/
		*pvptr = ptr = TAddress(ptr);
		if(*ptr == arr_sym) {
			return(vARRELE);
		}
		else if(*ptr == subarrsym) {
			return(vSUBARRAY);
		}
		else if(*ptr == rec_sym) {
			return(vRECFIELD);
		}
		else if(*ptr == derefsym) {
			return(vPOINTREF);
		}
		else if(*ptr == vectorsym) {
			return(vVECTOR);
		}
	}
	/* else aERROR */
	*pvptr = NULL;
	return(aERROR);
}
/*-----------------------------------------------------------*/
/*
** stripped down version of Lvaladdr
*/
PUBLIC int is_lval(ptr)
truc *ptr;
{
	struct symbol *sptr;
	int flag;

	flag = *FLAGPTR(ptr);
	if(flag == fLSYMBOL) {
		return(vBOUND);
	}
	else if(flag == fSYMBOL) {
        goto symbol;
	}
	else if(flag == fRSYMBOL) {
		ptr = LSYMBOLPTR(ptr);
		if((flag = *FLAGPTR(ptr)) == fSYMBOL)
			goto symbol;
		else if(flag == fLRSYMBOL) {
			return(vBOUND);
		}
		/* else fall through */
	}
	if(flag == fBUILTIN2 || flag == fSPECIAL2 || flag == fSPECIAL1) {
		/* array access or record access or pointer reference */
		ptr = TAddress(ptr);
		if(*ptr == arr_sym) {
			return(vARRELE);
		}
		else if(*ptr == subarrsym) {
			return(vSUBARRAY);
		}
		else if(*ptr == rec_sym) {
			return(vRECFIELD);
		}
		else if(*ptr == derefsym) {
			return(vPOINTREF);
		}
		/* else fall through */
	}
	/* else aERROR */
	return(aERROR);
  symbol:
	sptr = SYMPTR(ptr);
	switch(*FLAGPTR(sptr)) {
    case sUNBOUND:
		return(vUNBOUND);
	case sVARIABLE:
		return(vBOUND);
	case sCONSTANT:
	case sSCONSTANT:
		return(vCONST);
	default:
		break;
	}
	return(aERROR);
}
/*-----------------------------------------------------------*/
/*
** Legt in *pvptr die Adresse, in der der Wert des Symbols
** gespeichert ist, falls es sich um eine Variable oder ein
** ungebundenes Symbol handelt
** Return-Wert:
** vBOUND, falls Bindung vorhanden,
** vUNBOUND, falls noch ungebunden,
** vCONST, falls Konstante
** aERROR falls keine Variable
*/
PRIVATE int symbaddr(ptr,pvptr)
truc *ptr;
trucptr *pvptr;
{
	struct symbol *sptr;

	sptr = SYMPTR(ptr);
	switch(*FLAGPTR(sptr)) {
    case sUNBOUND:
		*FLAGPTR(sptr) = sVARIABLE;
		*pvptr = &(sptr->bind.t);
		return(vUNBOUND);
	case sVARIABLE:
		*pvptr = &(sptr->bind.t);
		return(vBOUND);
	case sCONSTANT:
	case sSCONSTANT:
		*pvptr = NULL;
		return(vCONST);
	default:
		*pvptr = NULL;
		return(aERROR);
	}
}
/*-----------------------------------------------------------*/
PUBLIC truc Lvalassign(ptr,obj)
truc *ptr;
truc obj;
{
	truc *vptr;
	truc *ptr1, *work0ptr;
    truc ele;
	int flg, ret, len, k;

	if(obj == nil)
		return(Pdispose(ptr));
	ret = Lvaladdr(ptr,&vptr);
	if(ret == vBOUND || ret == vUNBOUND) {
		flg = *FLAGPTR(vptr);
		switch(flg) {
		case fRECORD:
			return(fullrecassign(vptr,obj));
		case fPOINTER:
        default:
/******* type check unvollstaendig **********/
    		return(*vptr = obj);
		}
	}
	/* else */
	WORKpush(obj);
	switch(ret) {
    case vARRELE:
	case vSUBARRAY:
        ARGpush(vptr[1]);
		ARGpush(vptr[2]);
		argStkPtr[-1] = eval(argStkPtr-1);
		argStkPtr[0] = eval(argStkPtr);
		if(ret == vARRELE)
			obj = arrassign(argStkPtr-1,*workStkPtr);
		else
			obj = subarrassign(argStkPtr-1,*workStkPtr);
		ARGnpop(2);
		break;
	case vRECFIELD:
		ARGpush(vptr[1]);
		*argStkPtr = eval(argStkPtr);
		obj = recfassign(argStkPtr,vptr[2],*workStkPtr);
			/* vptr[2] = field */
		ARGpop();
		break;
	case vPOINTREF:
		ARGpush(vptr[1]);
		*argStkPtr = eval(argStkPtr);
		flg = *FLAGPTR(argStkPtr);
		if(flg == fPOINTER) {
			ptr1 = TAddress(argStkPtr);
			if(ptr1[2] == nil) {
				error(assignsym,err_nil,voidsym);
				obj = brkerr();
			}
			else {
				obj = fullrecassign(ptr1+2,*workStkPtr);
			}
		}
		else {
			obj = brkerr();
		}
		ARGpop();
		break;
    case vVECTOR:
        if(*FLAGPTR(workStkPtr) != fVECTOR) {
            error(assignsym,err_vect,obj);
            goto errexit;
        }
        len = *WORD2PTR(vptr+1);
        if(len != *VECLENPTR(workStkPtr)) {
            error(assignsym,"vectors must have same length",mkfixnum(len));
            goto errexit;
        }
        work0ptr = workStkPtr;
        for(k=0; k<len; k++)
            WORKpush(vptr[k+2]);
        for(k=0; k<len; k++) {
			ele = *(VECTORPTR(work0ptr)+k);
            ele = Lvalassign(work0ptr+k+1,ele);
            if(ele == breaksym)
                goto errexit;
        }
        workStkPtr = work0ptr;
		break;
	case vCONST:
		error(assignsym,err_sym2,*ptr);
		goto errexit;
	default:
		error(assignsym,err_vsym,*ptr);
  errexit:
		obj = brkerr();
	} /* end switch */
	WORKpop();
	return(obj);
}
/*-----------------------------------------------------------*/
PUBLIC truc Swhile()
{
	truc *ptr, *arr;
	truc res;
	int val;
	int i, n;

	res = eval(ARG0PTR(evalStkPtr));       /* boolean expression */
	val = nulltest(res);
	if(val <= 0)
		return(voidsym);

	ptr = TAddress(evalStkPtr);
	n = *WORD2PTR(ptr);
	ARGpush(ptr[1]);			/* boolean expression */
	arr = workStkPtr + 1;
	for(i=2; i<=n; i++) {
		WORKpush(ptr[i]);
	}
	while(val > 0) {
		for(res=voidsym, ptr=arr, i=0; ++i<n && res != breaksym; )
			res = eval(ptr++);
		if((res == breaksym) && (*brkmodePtr != lpcontsym)) {
			if(*brkmodePtr == lpbrksym)
				res = voidsym;
			goto cleanup;
		}
		res = eval(argStkPtr);
		val = nulltest(res);
	}
	res = voidsym;
  cleanup:
	workStkPtr = arr - 1;
	ARGpop();
	return(res);
}
/*-----------------------------------------------------------*/
PUBLIC truc Sfor()
{
	struct fornode *fptr;
	struct symbol *sptr;
	truc *runvar;
	truc *ptr, *arr;
	truc *argptr0, *saveptr0;
	truc obj;
	truc res = voidsym;

	word2 *x, *y, *lauf, *inc, *zaehler;
	word4 anz;
	int flg;
	int i, n, n0, m, slen, bodylen, inclen, zlen, rlen;
	int sign, sign1;
	int toolong = 0;

	fptr = (struct fornode *)TAddress(evalStkPtr);
	bodylen = fptr->len - 4;
	ptr = &(fptr->runvar);
	if((flg = *FLAGPTR(ptr)) == fSYMBOL) {
		sptr = SYMPTR(ptr);
		*FLAGPTR(sptr) = sVARIABLE;
		runvar = &(sptr->bind.t);
	}
	else if(flg == fLSYMBOL) {
		runvar = LSYMBOLPTR(ptr);
	}
	else {
		error(forsym,err_case,mkfixnum(flg));
		return(brkerr());
	}
	argptr0 = argStkPtr;
    saveptr0 = saveStkPtr;
	arr = workStkPtr + 1;
	ARGpush(fptr->inc);
	ARGpush(fptr->end);
	ARGpush(fptr->start);
	ptr = &(fptr->body0);
	for(i=0; i<bodylen; i++)
		WORKpush(*ptr++);
	argStkPtr[-2] = eval(argStkPtr-2);	/* inc */
	argStkPtr[-1] = eval(argStkPtr-1);	/* end */
	argStkPtr[0] = eval(argStkPtr);		/* start */
	if(chkints(forsym,argStkPtr-2,3) == aERROR) {
		res = brkerr();
		goto cleanup;
	}
	/* Berechnung der Anzahl der Iterationen */
	argStkPtr[-1] = addints(argStkPtr-1,-1); /* end - start */
	n = bigref(argStkPtr-1,&x,&sign);
	inclen = bigref(argStkPtr-2,&y,&sign1);
	if(n == 0)
		anz = 1;
	else if(sign1 != sign) {
		goto cleanup;		/* leere for-Schleife */
	}
	else if(inclen == 0) {
		error(forsym,err_div,voidsym);
		goto cleanup;
	}
	else {
		n = divbig(x,n,y,inclen,AriBuf,&rlen,AriScratch);
		if(n > 2) {
			toolong = 1;
		}
		else {
			anz = big2long(AriBuf,n);
			if(anz < 0xFFFFFFFF) {
                anz++;
            }
			else {
				toolong = 1;
			}
		}
        if(toolong) {
            zaehler = SAVEspace(n/2+2);
            if(zaehler) {
                cpyarr(AriBuf,n,zaehler);
                zlen = incarr(zaehler,n,1);
            }
            else {
        		error(forsym,err_savstk,voidsym);
		        goto cleanup;
            }
        }
	}
	n0 = bigref(argStkPtr,&x,&sign);	/* start */
	m = (n0 < inclen ? inclen : n0) + 3;
	slen = (m + inclen)/2 + 2;	/* unit of SaveStack is word4 */
	lauf = (word2 *)SAVEspace(slen);
	if(lauf) {
		cpyarr(x,n0,lauf);
		inc = lauf + m;
		cpyarr(y,inclen,inc);
	}
	else {
		error(forsym,err_savstk,voidsym);
		goto cleanup;
	}
    if(!toolong) {
    	while(anz) {
	    	*runvar = mkint(sign,lauf,n0);
		    obj = arreval(arr,bodylen);
    		if((obj == breaksym) && (*brkmodePtr != lpcontsym)) {
	    		if(*brkmodePtr != lpbrksym)
		    		res = obj;
			    /* else res = voidsym; */
    			break;
	    	}
		    n0 = increment(lauf,n0,&sign,inc,inclen,sign1);
    		anz--;
	    }
    }
    else {
    	while(zlen) {
	    	*runvar = mkint(sign,lauf,n0);
		    obj = arreval(arr,bodylen);
    		if((obj == breaksym) && (*brkmodePtr != lpcontsym)) {
	    		if(*brkmodePtr != lpbrksym)
		    		res = obj;
			    /* else res = voidsym; */
    			break;
	    	}
		    n0 = increment(lauf,n0,&sign,inc,inclen,sign1);
    		zlen = decarr(zaehler,zlen,1);
	    }
    }
  cleanup:
    saveStkPtr = saveptr0;
	argStkPtr = argptr0;
	workStkPtr = arr - 1;
	return(res);
}
/*-----------------------------------------------------------*/
PRIVATE int increment(x,n,signptr,inc,inclen,s)
word2 *x, *inc;
int n, inclen;
int *signptr;
int s;
{
	int cmp;

	if(*signptr == s)
		return(addarr(x,n,inc,inclen));
	/* else */
	cmp = cmparr(x,n,inc,inclen);
	if(cmp > 0)
		return(subarr(x,n,inc,inclen));
	else if(cmp < 0) {
		*signptr = s;
		return(sub1arr(x,n,inc,inclen));		
	}
	else {
		*signptr = 0;
		return(0);
	}
}
/*-----------------------------------------------------------*/
/*
** Bestimmt in einem if-elsif-...-else-Ausdruck durch Auswertung
** der Bedingungen, welcher Zweig ausgewertet werden muss
** und legt diesen in *evalStkPtr ab
*/
PUBLIC void Sifaux()
{
	truc *ptr;
	int val;
	int i, n;

	ptr = TAddress(evalStkPtr);
	n = *WORD2PTR(ptr);
	for(i=1; i<n; i+=2) {
		val = nulltest(eval(ptr+i));
		if(val > 0) {
			*evalStkPtr = *ARGNPTR(evalStkPtr,i);
			return;
		}
		else if(val == 0) {
			ptr = TAddress(evalStkPtr);
			/* this may have been changed */
		}
		else {	/* val == aERROR */
			*evalStkPtr = brkerr();
			return;
		}
	}
	*evalStkPtr = *ARGNPTR(evalStkPtr,n-1);	   /* else statement */
	return;
}
/*-----------------------------------------------------------*/
PRIVATE truc Freturn()
{
	if((*brkbindPtr = *argStkPtr) == breaksym)
		*brkmodePtr = errsym;
	else {
		*brkmodePtr = retsym;
	}
	return(breaksym);
}
/*-----------------------------------------------------------*/
PUBLIC truc Sexit()
{
	*brkmodePtr = exitsym;
	return(breaksym);
}
/*-----------------------------------------------------------*/
PRIVATE truc Slpbreak()
{
	*brkmodePtr = lpbrksym;
	return(breaksym);
}
/*-----------------------------------------------------------*/
PRIVATE truc Slpcont()
{
	*brkmodePtr = lpcontsym;
	return(breaksym);
}
/*-----------------------------------------------------------*/
PUBLIC truc brkerr()
{
	*brkmodePtr = errsym;
	return(breaksym);
}
/*-----------------------------------------------------------*/
PRIVATE truc Stimer()
{
	return mkinum(timer());
}
/*----------------------------------------------------------*/
PRIVATE truc Fgmtime(argn)
int argn;
{
    int tim[6];
    long secs;
    char *str;
    word4 x,y;

    secs = datetime(tim);

    if(argn == 1 && *argStkPtr == zero) {
        return mkinum(secs);
    }
    /* else */
    x = tim[0] + 1900;
    y = tim[1] + 1;
    str = OutBuf;
    str += s2form(str,"~04D:~02D:", (wtruc)x,(wtruc)y);
    x = tim[2];
    y = tim[3];
    str += s2form(str,"~02D:~02D:",(wtruc)x,(wtruc)y);
    x = tim[4];
    y = tim[5];
    s2form(str,"~02D:~02D",(wtruc)x,(wtruc)y);

    return mkstr(OutBuf);
}
/*----------------------------------------------------------*/
PRIVATE truc Smkunbound()
{
	truc *ptr;
	int flg;

	ptr = ARG0PTR(evalStkPtr);
	if((flg = *FLAGPTR(ptr)) != fSYMBOL) {
/** arrays of symbols? **/
		error(mkunbdsym,err_gsym,(flg==fLSYMBOL ? voidsym : *ptr));
		return(false);
	}
        else if(*ptr == usersym) {
                return(unbinduser());
        }
	else if(*SYMFLAGPTR(ptr) >= sFBINARY) {
		error(mkunbdsym,err_bltin,*ptr);
		return(false);
	}
	else
		return(unbindsym(ptr));
}
/*----------------------------------------------------------*/
/*
** unbind all user defined symbols
*/
PUBLIC truc unbinduser()
{
        truc *ptr;
        truc obj;
        int flg;
        int i = 0;

	while((ptr = nextsymptr(i++))) {
		obj = symbobj(ptr);
		if(!inpack(obj,usersym))
			continue;
		flg = *FLAGPTR(ptr);
		if(flg >= sVARIABLE && flg < sINTERNAL) {
                        unbindsym(ptr);
		}
		else
			continue;
	}
        return(true);
}
/*----------------------------------------------------------*/
/*
** unbinds symbol *ptr
** (used also by globtypedef [file parser.c] in case of error)
** ! if *ptr is not a symbol, this may have bad consequences !
*/
PUBLIC truc unbindsym(ptr)
truc *ptr;
{
	struct symbol *sptr;

	sptr = SYMPTR(ptr);
	sptr->bind.t = zero;
	*FLAGPTR(sptr) = sUNBOUND;
	return(true);
}
/*----------------------------------------------------------*/
PRIVATE truc Fsymbols()
{
	truc *arr, *ptr;
	truc vec, pack, obj;
	int flg;
	int i = 0;
	int count = 0;

	pack = *argStkPtr;
/***
	if(pack != usersym && pack != arisym)
		pack = usersym;
***/
	arr = workStkPtr + 1;
	while((ptr = nextsymptr(i++))) {
		obj = symbobj(ptr);
		if(!inpack(obj,pack))
			continue;
		flg = *FLAGPTR(ptr);
		if(flg >= sVARIABLE && flg < sINTERNAL) {
			WORKpush(obj);
			count++;
		}
		else
			continue;
	}
	sortarr(arr,count,symbcmp);
	vec = mkvect0(count);
	ptr = VECTOR(vec);
	while(--count >= 0)
		*ptr++ = WORKretr();
	return(vec);
}
/*----------------------------------------------------------*/
PRIVATE int symbcmp(ptr1,ptr2)
truc *ptr1, *ptr2;
{
	return(strcmp(SYMNAMEPTR(ptr2),SYMNAMEPTR(ptr1)));
}
/*----------------------------------------------------------*/
PRIVATE truc Ftypeident()
{
	int flag, flg1, val;
	truc obj, symb;

	flag = *FLAGPTR(argStkPtr);

	if(flag==1) {
		flg1 = *SYMFLAGPTR(argStkPtr);
		switch(flg1) {
		case sFUNCTION:
		case sVFUNCTION:
		case sFBINARY:
		case sSBINARY:
			symb = funcsym;
			break;
		case sTYPEDEF:
            *argStkPtr = *SYMBINDPTR(argStkPtr);
            flag = *FLAGPTR(argStkPtr);
                goto weiter;
		case sPARSAUX:
			symb = *argStkPtr;
            if(symb == funcsym || symb == procsym)
                symb = funcsym;
            else
                symb = voidsym;
			break;
		case sTYPESPEC:
			symb = *argStkPtr;
			break;
		case sSCONSTANT:
			symb = *argStkPtr;
			if(symb == nil)
				symb = pointrsym;		
			break; 
		default:
			symb = voidsym;
			break;
		}
        goto ausgang;
	}
  weiter:
	switch(flag) {
		case fBOOL:
			symb = boolsym;
			break;
		case fFIXNUM:
		case fBIGNUM:
			symb = integsym;
			break;
		case fGF2NINT:
			symb = gf2nintsym;
			break;
		case fCHARACTER:
			symb = charsym;
			break;
		case fSTRING:
			symb = stringsym;
			break;
		case fBYTESTRING:
			symb = bstringsym;
			break;
		case fVECTOR:
			symb = arraysym;
			break;
		case fSTREAM:
			symb = filesym;
			break;
		case fSTACK:
			symb = stacksym;
			break;
		case fRECORD:
			symb = recordsym;
			break;
		case fPOINTER:
			symb = pointrsym;
			break;
        case fBUILTIN1:
            obj = *OPNODEPTR(argStkPtr);
            if(obj == mkrecsym) {
                symb = recordsym;
                break;
            }
            /* else fall through */
		default:
			if ((flag & fFLTOBJ) == fFLTOBJ)
				symb = realsym;
			else
				symb = errsym;
			break;
	}
  ausgang:
    val = typevalue(symb);
	return mkinum(val);
}
/*----------------------------------------------------------*/
PRIVATE int typevalue(symb)
truc symb;
{
    int val;

    if(symb == boolsym) {
        val = 1;
    }
    else if(symb == integsym) {
        val = 2;
    }
    else if(symb == gf2nintsym) {
        val = 3;
    }
    else if(symb == realsym) {
        val = 4;
    }
    else if(symb == charsym) {
        val = 10;
    }
    else if(symb == stringsym) {
        val = 11;
    }
    else if(symb == bstringsym) {
        val = 12;
    }
    else if(symb == arraysym) {
        val = 20;
    }
    else if(symb == recordsym) {
        val = 21;
    }
    else if(symb == pointrsym) {
        val = 22;
    }
    else if(symb == stacksym) {
        val = 23;
    }
    else if(symb == filesym) {
        val = 30;
    }
    else if(symb == funcsym) {
        val = 40;
    }
    else if(symb == voidsym) {
        val = 0;
    }
    else
        val = -1;
    return val;
}
/*********************************************************************/
