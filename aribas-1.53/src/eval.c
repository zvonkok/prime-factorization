/****************************************************************/
/* file eval.c

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
** eval.c
** evaluation functions
**
** date of last change
** 1995-03-20: sSYSTEMVAR
** 1995-03-25: fRECORD
** 1995-04-14: fixed bug in eval (case optional arguments)
** 2002-06-08   bugfix in evalargs and evalvargs
*/

#include "common.h"


#define STACKFAIL	(stkcheck() < 512)


/******* prototypes of exported functions ************/
PUBLIC void inieval	_((void));
PUBLIC truc eval	_((truc *ptr));
PUBLIC truc ufunapply	_((truc *fun, truc *arr, int n));
PUBLIC truc arreval	_((truc *arr, int n));

/******* module global variable *********/
PRIVATE truc evalsym;

/******* prototypes of functions internal to this module *****/
PRIVATE truc eval0	_((truc *ptr, int flg));
PRIVATE int stkevargs	_((truc *ptr));
PRIVATE void argvarspace   _((truc *argptr, int n, truc *vptr, int m));
PRIVATE int evalargs	_((truc *argptr, int n));
PRIVATE int evalvargs	_((truc parms, truc *argptr, int n));
PRIVATE truc vsymaux	_((truc *argptr, unsigned depth));
PRIVATE int lvarsini	_((truc *arr, int n));

/* -------------------------------------------------------*/
PUBLIC void inieval()
{
	evalsym	   = newselfsym("eval", sINTERNAL);
}
/* -------------------------------------------------------*/
/*
** evaluates *ptr
*/
PUBLIC truc eval(ptr)
truc *ptr;
{
	static struct symbol *sptr;
	static truc *ptr1, *ptr2;
	static truc obj;
	static truc parms;
	static int i, n;
	static int chk;
	static int flg;
/* static, damit bei Rekursion nur einmal Speicher reserviert wird */

	funptr binfun;
	int k;

	if((flg = *FLAGPTR(ptr)) >= fSELFEVAL) {
		return(*ptr);
	}
	else if(flg < fFUNEXPR)
		return(eval0(ptr,flg));

/**** at this point flg >= fFUNEXPR && flg < fSELFEVAL *****/

	if(STACKFAIL)
		reset(err_rec);

	if(INTERRUPT) {
		setinterrupt(0);
		reset(err_intr);
	}

	EVALpush(*ptr);
  /********************/
  tailnrec:
	if(flg <= fSPECIALn) {
		if(flg == fSPECIAL0)
			binfun = (funptr)*SYMWBINDPTR(evalStkPtr);
		else
			binfun = (funptr)*SYMWBINDPTR(TAddress(evalStkPtr));
		obj = binfun();
	}
	else if(flg <= fBUILTINn) {
	    binfun = (funptr)*SYMWBINDPTR(TAddress(evalStkPtr));
	    switch(flg) {
	    case fBUILTIN1:
			obj = eval(ARG0PTR(evalStkPtr));
			ARGpush(obj);
			obj = binfun();
			ARGpop();
			break;
	    case fBUILTIN2:
			obj = eval(ARG0PTR(evalStkPtr));
			ARGpush(obj);
			obj = eval(ARG1PTR(evalStkPtr));
			ARGpush(obj);
			obj = binfun();
			ARGnpop(2);
			break;
	    default:	/* case fBUILTINn */
			goto fnbineval;
	    }
	}
	else switch(flg) {
	case fFUNCALL:
		ptr = TAddress(evalStkPtr);
		obj = eval0(ptr,*FLAGPTR(ptr));
		sptr = symptr(obj);
		flg = *FLAGPTR(sptr);
		if(flg == sFUNCTION || flg == sVFUNCTION) {
			ptr = Taddress(sptr->bind.t);
			/* ptr zeigt auf die Funktions-Definition */
		}
		else if(flg == sFBINARY || flg == sSBINARY) {
			k = *ARGCOUNTPTR(evalStkPtr);
			chk = chknargs(obj,k);
			if(chk == NARGS_FALSE) {
				error(obj,err_args,voidsym);
				obj = brkerr();
				goto cleanup;
			}
			binfun = (funptr)sptr->bind.w;
			if(k == 0 ||
			  ((flg==sSBINARY) && (chk==NARGS_VAR || k>=3))) {
			    obj = binfun();
			}
			else if(flg==sFBINARY && chk==NARGS_OK && k<=2) {
			    if(k == 1) {
					obj = eval(ARG1PTR(evalStkPtr));
					ARGpush(obj);
					obj = binfun();
					ARGpop();
			    }
			    else {	/* k == 2 */
					obj = eval(ARG1PTR(evalStkPtr));
					ARGpush(obj);
					obj = eval(ARGNPTR(evalStkPtr,2));
					ARGpush(obj);
					obj = binfun();
					ARGnpop(2);
			    }
			}
			else if(flg == sFBINARY) {
				goto fnbineval;
			}
			else {	/* flg == sSBINARY && (k == 1 || k == 2) */
				*evalStkPtr =
				mkspecnode(obj,ARG1PTR(evalStkPtr),k);
				obj = binfun();
			}
			goto cleanup;
		}
		else {
			error(evalsym,err_ufunc,*ptr);
			obj = brkerr();
			goto cleanup;
		}
		n = *WORD2PTR(ptr);
		/* number of formal function arguments */
		i = *ARGCOUNTPTR(evalStkPtr);
		/* number of actual function arguments */
		if(n != i) {
		    if(i < n && n-i <= *FLG2PTR(ptr)) {
				chk = 1;
		    }
		    else {
				error(*TAddress(evalStkPtr),err_args,voidsym);
				obj = brkerr();
				break;
		    }
		}
		else
		    chk = 0;
		SAVEpush(argStkPtr);

		k = *VARCPTR(ptr);
		if(chk) {	/* provide default optional arguments */
		    ptr1 = VECTORPTR(PARMSPTR(ptr));
		    argvarspace(ptr1,n,VARSPTR(ptr),k);
		    ptr1 = SAVEtop() + 1;
		    ptr2 = ARG1PTR(evalStkPtr);
		    while(--i >= 0)
			*ptr1++ = *ptr2++;
		}
		else {
		    argvarspace(ARG1PTR(evalStkPtr),n,VARSPTR(ptr),k);
		}
		*evalStkPtr = *(ptr + OFFS4body);

		if(flg == sVFUNCTION) {
		    parms = *PARMSPTR(ptr);
		    ptr = SAVEtop() + 1;
		    n = evalvargs(parms,ptr,n);
		}
		else {
		    ptr = SAVEtop() + 1;
		    n = evalargs(ptr,n);
		}
		SAVEpush(basePtr);
		basePtr = ptr;
		if(n == aERROR || lvarsini(ptr+n,k) == aERROR)
			obj = brkerr();
		else {
		    obj = zero;
		    k = *FUNARGCPTR(evalStkPtr);
		    while(--k >= 0) {
				obj = eval(ARGNPTR(evalStkPtr,k));
				if(obj == breaksym) {
				    if(*brkmodePtr == retsym) {
						obj = *brkbindPtr;
						*brkbindPtr = zero;
				    }
				    break;
				}
		    }
		}
		basePtr = SAVEretr();
		argStkPtr = SAVEretr();
		break;
	case fWHILEXPR:
		obj = Swhile();
		break;
	case fFOREXPR:
		obj = Sfor();
		break;
	case fIFEXPR:
		Sifaux();
		flg = *FLAGPTR(evalStkPtr);
		if(flg != fCOMPEXPR) {
			obj = eval(evalStkPtr);
			break;
		}
		/* else fall through */
	case fCOMPEXPR:
		obj = voidsym;
		k = *FUNARGCPTR(evalStkPtr);
		if(k == 0)
			goto cleanup;
		while(--k > 0) {	/* evaluate k-1 expressions */
			obj = eval(ARGNPTR(evalStkPtr,k));
			if(obj == breaksym)
				goto cleanup;
		}
		/* tail recursion elimination */
		*evalStkPtr = *ARG0PTR(evalStkPtr);

		flg = *FLAGPTR(evalStkPtr);
		if(flg >= fSELFEVAL)
			obj = *evalStkPtr;
		else if(flg < fFUNEXPR)
			obj = eval0(evalStkPtr,flg);
		else
			goto tailnrec;
		break;
	default:
		error(evalsym,err_case,mkfixnum(flg));
		obj = brkerr();
	}
	goto cleanup;
  fnbineval:
	SAVEpush(argStkPtr);
	ptr = TAddress(evalStkPtr);
	k = stkevargs(ptr+1);
	obj = (k == aERROR ? brkerr() : ((funptr1)binfun)(k));
	argStkPtr = SAVEretr();
  cleanup:
	EVALpop();
	return(obj);
}
/*------------------------------------------------------------*/
/*
** Wendet die benutzerdefinierte Funktion *fun auf die bereits
** ausgewertete Argumentliste (arr,n) an.
** Es wird vorausgesetzt, dass die Argumente der Funktion *fun
** alle Wert-Parameter sind und die Anzahl gleich n ist.
*/
PUBLIC truc ufunapply(fun,arr,n)
truc *fun;
truc *arr;
int n;
{
	truc *fundefptr;
	truc obj;
	int k;

	fundefptr = TAddress(fun);
	SAVEpush(argStkPtr);
	SAVEpush(basePtr);
	basePtr = argStkPtr + 1;

	k = *VARCPTR(fundefptr);
	argvarspace(arr,n,VARSPTR(fundefptr),k);
	EVALpush(*(fundefptr + OFFS4body));

	if(lvarsini(basePtr+n,k) == aERROR)
		obj = brkerr();
	else {	/* eval body, which is a compound statement */
	    obj = zero;
	    k = *FUNARGCPTR(evalStkPtr);
	    while(--k >= 0) {
			obj = eval(ARGNPTR(evalStkPtr,k));
			if(obj == breaksym) {
			    if(*brkmodePtr == retsym) {
					obj = *brkbindPtr;
					*brkbindPtr = zero;
			    }
			    break;
			}
	    }
	}
	EVALpop();
	basePtr = SAVEretr();
	argStkPtr = SAVEretr();
	return(obj);
}
/*------------------------------------------------------------*/
PRIVATE truc eval0(ptr,flg)
truc *ptr;
int flg;
{
	struct symbol *sptr;

	if(flg == fSYMBOL) {
		sptr = SYMPTR(ptr);

		switch(*FLAGPTR(sptr)) {
			case sCONSTANT:
			case sSCONSTANT:
			case sVARIABLE:
			case sINTERNAL:
			case sSYSTEMVAR:
				return(sptr->bind.t);
			case sUNBOUND:
				error(evalsym,err_ubound,*ptr);
				return(brkerr());
			default:
				return(*ptr);
		}

	}
	else if(flg == fLSYMBOL) {
		return(*LSYMBOLPTR(ptr));
	}
	else if(flg == fRSYMBOL) {
		ptr = LSYMBOLPTR(ptr);
		if((flg = *FLAGPTR(ptr)) == fSYMBOL)
			return(eval(ptr));
		else if(flg == fLRSYMBOL) {
			return(*LRSYMBOLPTR(ptr));
		}
		else if(flg == fBUILTIN2 || flg == fSPECIAL2
			|| flg == fSPECIAL1) {
		/* array access or record access or pointer reference */
			return(eval(ptr));
		}
		else {
			error(evalsym,err_case,mkfixnum(flg));
			return(brkerr());
		}
	}
	else if(flg == fLRSYMBOL) {
		return(*LRSYMBOLPTR(ptr));
	}
	else if(flg == fTMPCONST) {
		return(Lconsteval(ptr));
	}
	else
		return(*ptr);
}
/*------------------------------------------------------------*/
/*
** ptr[0] contains number of arguments,
** ptr[1],...,ptr[n] are expressions for arguments
** argStkPtr wird veraendert!
*/
PRIVATE int stkevargs(ptr)
truc *ptr;
{
	truc *argptr;
	int i,n;

	n = *WORD2PTR(ptr);
	ptr++;
	argptr = argStkPtr + 1;
	argStkPtr += n;
	if(argStkPtr >= saveStkPtr)
		reset(err_astk);
	for(i=0; i<n; i++)	/* expressions for arguments */
		argptr[i] = *ptr++;
	for(i=0; i<n; i++) {	/* evaluate arguments */
		if((*argptr = eval(argptr)) == breaksym)
			return(aERROR);
		else
			argptr++;
	}
	return(n);
}
/*------------------------------------------------------------*/
/*
** schafft Platz fuer Argumente und lokale Variable einer
** benutzerdefinierten Funktion
** argStkPtr wird veraendert!
*/
PRIVATE void argvarspace(argptr,n,vptr,m)
truc *argptr, *vptr;
int n, m;
{
	truc *ptr1;
	int i;

	ptr1 = argStkPtr + 1;
	argStkPtr += n + m;
	if(argStkPtr >= saveStkPtr)
		reset(err_astk);
	for(i=0; i<n; i++) {	 /* expressions for arguments */
		*ptr1++ = *argptr++;
	}
	if(!m)
		return;
	vptr = VECTORPTR(vptr);
	for(i=0; i<m; i++)	/* expressions for variable initialization */
		*ptr1++ = *vptr++;
}
/*------------------------------------------------------------*/
PRIVATE int evalargs(argptr,n)
truc *argptr;
int n;
{
	int i, flg;
	truc obj;

	for(i=0; i<n; i++) {
		if((flg = *FLAGPTR(argptr)) >= fBOOL) {
			argptr++;
			continue;
		}
		else if(flg < fFUNEXPR) {
			*argptr = eval0(argptr,flg);
		}
		if(*FLAGPTR(argptr) < fRECORD) {
			if((*argptr = eval(argptr)) == breaksym)
				return(aERROR);
		}
		if((flg = *FLAGPTR(argptr)) >= fRECORD && flg <= fVECTLIKE1) {
			*argptr = mkarrcopy(argptr);
		}
		argptr++;
	}
	return(n);
}
/*------------------------------------------------------------*/
PRIVATE int evalvargs(parms,argptr,n)
truc parms;
truc *argptr;
int n;
{
	int i, flg;
	unsigned depth;
	truc *ptr;

	depth = basePtr - ArgStack;
	WORKpush(parms);
	for(i=0; i<n; i++) {
		ptr = VECTORPTR(workStkPtr) + i;
		if(*FLAGPTR(ptr) == fSPECIAL1) {
		    /* es handelt sich um einen Variable-Parameter */
		    if((*argptr = vsymaux(argptr,depth)) == breaksym) {
				error(funcsym,err_vasym,voidsym);
				n = aERROR;
				goto cleanup;
		    }
		    else {
				argptr++;
				continue;
		    }
		}
		if((flg = *FLAGPTR(argptr)) >= fBOOL) {
			argptr++;
			continue;
		}
		else if(flg < fFUNEXPR) {
			*argptr = eval0(argptr,flg);
		}
		if(*FLAGPTR(argptr) < fRECORD) {
			if((*argptr = eval(argptr)) == breaksym) {
				n = aERROR;
				break;
			}
		}
		if((flg = *FLAGPTR(argptr)) >= fRECORD && flg <= fVECTLIKE1) {
			*argptr = mkarrcopy(argptr);
		}
		argptr++;
	}
  cleanup:
	WORKpop();
	return(n);
}
/*------------------------------------------------------------*/
PRIVATE truc vsymaux(argptr,depth)
truc *argptr;
unsigned depth;
{
	unsigned u;
	truc *ptr;
	truc sym, obj;

	switch(*FLAGPTR(argptr)) {
	case fSYMBOL:
	    return(*argptr);
	case fLSYMBOL:
	    u = depth + *WORD2PTR(argptr);
	    return(mklocsym(fLRSYMBOL,u));
	case fRSYMBOL:
	    return(*LSYMBOLPTR(argptr));
	case fBUILTIN2:		/* array access */
	case fSPECIAL2:		/* record access */
	    ptr = TAddress(argptr);
	    sym = *ptr;
	    ARGpush(ptr[1]);
	    ARGpush(ptr[2]);
	    if(sym == arr_sym || sym == subarrsym) {
		    argStkPtr[-1] = vsymaux(argStkPtr-1,depth);
		    argStkPtr[0] = eval(argStkPtr);
	    }
	    else if(sym == rec_sym) {
		    argStkPtr[-1] = vsymaux(argStkPtr-1,depth);
	    }
	    else {
		    ARGnpop(2);
		    break;
	    }
	    obj = mkbnode(sym);
	    ARGnpop(2);
	    return(obj);
	}
	return(brkerr());
}
/*------------------------------------------------------------*/
/*
** Initialisierung der lokalen Variablen
*/
PRIVATE int lvarsini(arr,n)
truc *arr;
int n;
{
	truc obj;

	while(--n >= 0) {
		if((obj = eval(arr)) == breaksym)
			return(aERROR);
		*arr++ = obj;
	}
	return(1);
}
/*------------------------------------------------------------*/
PUBLIC truc arreval(arr,n)
truc *arr;
int n;
{
	static truc res;

	res = voidsym;
	while(--n >= 0 && res != breaksym)
		res = eval(arr++);
	return(res);
}
/*********************************************************************/
