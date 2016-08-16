/****************************************************************/
/* file parser.c

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
*/
/****************************************************************/

/*
** parser.c
** parser
**
** date of last change
** 1995-02-22	lpbrksym
** 1995-03-11	fixed problems with tread
** 1995-03-20	const, type
** 1995-03-29	record, pointer to
** 1997-04-13	type symbol, reorg (newintsym)
** 1998-10-07	continue statement
** 2001-03-11	improved some error messages
** 2002-02-16   uminsym in negate
** 2002-02-24   Z1TOK
** 2003-02-27   enddotsym
** 2004-06-13	better error handling in readwhile, readif; ARGLIST
** 2004-08-27   removed bug in previous handling of ARGLIST
*/

#include "common.h"


PUBLIC void iniparse	_((void));
PUBLIC truc tread	_((truc *strom, int mode));
PUBLIC void clearcompile  _((void));

PUBLIC truc parserrsym;

/*------------------------------------------------------------*/
PRIVATE int operprec	_((int tok));
PRIVATE int rightass	_((int tok));
PRIVATE int binop	_((int tok));
PRIVATE truc readexpr	_((truc *strom, int prec));
PRIVATE truc optbnode	_((truc op));
PRIVATE truc primary	_((truc *strom, int *cflgptr));
PRIVATE int parsedsym	_((char *name, truc *pobj));
PRIVATE truc prochistory  _((char *sbuf, truc *strom));
PRIVATE truc readwrite	_((truc wsym, truc *strom));
PRIVATE truc readformat	 _((truc *strom, int *tokptr));
PRIVATE truc readvector	 _((int flg, truc *strom));
PRIVATE truc readbrack	_((truc *strom, int *pmode));
PRIVATE truc funcall	_((truc fun, truc *strom));
PRIVATE truc arraccess	_((truc arr, truc *strom));
PRIVATE truc recaccess	_((truc arr, truc *strom));
PRIVATE truc compfunc	_((truc *strom));
PRIVATE truc varssyms	_((truc symbs));
PRIVATE void vars2push	_((truc symbs));
PRIVATE truc getfuname	_((truc *strom));
PRIVATE int markparms	_((truc symbs, truc kind));
PRIVATE int marksymbs	_((truc symbs));
PRIVATE int markglobs	_((truc symbs));
PRIVATE void unmarksymbs  _((truc symbs));
PRIVATE truc parmlist	_((truc *strom, int *vflgptr, int *optcptr));
PRIVATE truc varlist	_((truc *strom, truc *endptr));
PRIVATE truc lconstlist _((truc *strom, truc *endptr));
PRIVATE int varsection	_((truc *strom, truc *endptr));
PRIVATE int parmsection	 _((truc *strom, truc *endptr, int *vflgptr));
PRIVATE int parmsaux  _((truc *strom, truc *typptr, truc *endptr, int *ofp));
PRIVATE int varsaux    _((int tok, truc *strom, truc *typptr, truc *endptr));
PRIVATE truc typespec	_((truc *strom));
PRIVATE truc pointertype  _((truc *strom));
PRIVATE truc ptrtypeval	 _((truc typ));
PRIVATE truc globconstdef _((truc *strom));
PRIVATE truc globtypedef _((truc *strom));
PRIVATE truc globvardef	 _((truc *strom));
PRIVATE int decldelim	_((truc delim));
PRIVATE int arglist	_((truc *strom, int endtok));
PRIVATE truc readrecdef	 _((truc *strom));
PRIVATE truc readwhile	_((truc *strom));
PRIVATE truc readfor	_((truc *strom));
PRIVATE int obligsym	_((truc fun, truc *strom, truc symb));
PRIVATE void errexpect	_((truc fun, truc symb, truc esym));
PRIVATE int statements	_((truc *strom, truc *endptr));
PRIVATE truc readif	_((truc *strom));
PRIVATE truc negate	_((truc obj));
PRIVATE void recoverr	_((truc *strom));
PRIVATE int pstat	_((int mode, unsigned flag));
PRIVATE truc tmptoksym	_((int tok));

PRIVATE truc scansym, enddotsym;

PRIVATE unsigned locsymbs = 0;

#ifdef PARSLOG
char *plogfile = "pars.log";
FILE *Plog = NULL;
char pmess[80];
#endif
/*--------------------------------------------------------------------*/
PUBLIC void iniparse()
{
	parserrsym = newselfsym("syntax error",sINTERNAL);
	scansym    = newselfsym("scanning",sINTERNAL);
    enddotsym  = newselfsym(".",sINTERNAL);
#ifdef PARSLOG
    Plog = fopen(plogfile,"w");
#endif
}
/*--------------------------------------------------------------------*/
/*
** Funktion zur Verwaltung des Pars-Status
*/
#define PQUERY		1
#define PSET		2
#define PCLEAR		3
#define PCLEARALL	4

#define COMPILING	0x01
#define CONSTDECL	0x02
#define EXTERNDECL	0x04
#define POINTRECURS	0x08
#define PLAINSYM	0x10
#define ARGLIST     0x20
#define ALLFLAG	    0xFFFF

PRIVATE int pstat(mode,flag)
int mode;
unsigned flag;
{
	static unsigned pstatus = 0;

	switch(mode) {
		case PQUERY:
		    return(pstatus & flag);
		case PSET:
		    return(pstatus |= flag);
		case PCLEAR:
		    return(pstatus &= ~flag);
		case PCLEARALL:
		    pstatus = 0;
		    return(0);
		default:
		    return(pstatus);
	}
}
/*--------------------------------------------------------------------*/
/*
** operator precedence
*/
#define POWERPREC	3000
#define UMINUSPREC	2500
#define MULTPREC	2000
#define ADDPREC		1000
#define RELPREC		 500
#define NOTPREC		 400
#define ANDPREC		 300
#define ORPREC		 200
#define ASSIGNPREC	  10

PRIVATE int operprec(tok)
int tok;
{
	if(tok >= PLUSTOK) {
		if(tok <= MINUSTOK)
			return(ADDPREC);
		if(tok <= MODTOK)
			return(MULTPREC);
		if(tok == UMINUSTOK)
			return(UMINUSPREC);
		if(tok == POWERTOK)
			return(POWERPREC);
	}
	else if(tok >= EQTOK)	     /* relational ops */
		return(RELPREC);
	else switch(tok) {
		case ASSIGNTOK:
			return(ASSIGNPREC);
		case ANDTOK:
			return(ANDPREC);
		case ORTOK:
			return(ORPREC);
		case NOTTOK:
			return(NOTPREC);
	}
	rerror(parserrsym,err_case,mkfixnum(tok));
	return(0);
}
/*--------------------------------------------------------------------*/
/*
** returns associativity of operator (left = 0, right = 1)
*/
PRIVATE int rightass(tok)
int tok;
{
	return(tok & 1);
}
/*--------------------------------------------------------------------*/
/*
** returns 1 if tok is binary operator, else 0
*/
PRIVATE int binop(tok)
int tok;
{
	if(tok >= EQTOK && tok <= MODTOK)
		return(1);
	else if(tok >= ASSIGNTOK && tok <= ANDTOK)
		return(1);
	else if(tok == POWERTOK)
		return(1);
	else
		return(0);
}
/*--------------------------------------------------------------------*/
#define READBUFSIZE	64
/*
** Read expressions from strom
** Expressions are buffered
** If mode == TERMINALINP, returns if DOTTOK is encountered
** (called from mainloop)
** if mode == FILEINPUT, returns if EOLTOK is encountered after
** a complete expression
** (called from loadaux, used by ARIBAS function load)
** Before returning, all but the last expressions are evaluated
** The last expression read is returned
** Place strom should be protected w/r to garbage collection
*/
PUBLIC truc tread(strom,mode)
truc *strom;
int mode;	/* mode = TERMINALINP or FILEINPUT */
{
	truc *arr;
	truc obj;
	int tok;
	int count = 0;
	arr = workStkPtr;
	while(++count <= READBUFSIZE) {
		obj = readexpr(strom,0);
		if(obj == parserrsym || obj == breaksym) {
			recoverr(strom);
			obj = brkerr();
			goto cleanup;
		}
		WORKpush(obj);
		if(obj == exitfun)
			break;
		if(mode == TERMINALINP) {
		    if(obj == historsym) {
			if(count != 1) {
				rerror(parserrsym,"",historsym);
				obj = brkerr();
				goto cleanup;
			}
			break;
		    }
		    tok = skipeoltok(strom);
		    if(tok == SEMICOLTOK) {
				tok = nexttok(strom,0);
		    }
		    else if(tok == DOTTOK || tok == QUESTIONTOK) {
				tok = nexttok(strom,0);
				break;
		    }
		}
		else {		/* FILEINPUT */
/**************************/
		    if((tok = curtok(strom)) != SYMBOLTOK)
			tok = nexttok(strom,0);
		    if(tok == EOLTOK || tok == EOFTOK)
			break;
		}
	}
	if(arreval(arr+1,count-1) == breaksym) {
		obj = brkerr();
		goto cleanup;
	}
	obj = *workStkPtr;
  cleanup:
	workStkPtr = arr;
	return(obj);
}
#undef READBUFSIZE
/*--------------------------------------------------------------------*/
PRIVATE truc readexpr(strom,prec)
truc *strom;
int prec;
{
	truc obj, op;
	int tok;
	int prec1;
	int complete;

	obj = primary(strom,&complete);
	if(complete || obj == parserrsym) {
		return(obj);
	}
	while(1) {
		tok = skipeoltok(strom);
		if(tok == SYMBOLTOK) {
			/* look for infix operator mod, div, and, or .. */
			if(lookupsym(SymBuf,&op) == sINFIX) {
				tok = tokenvalue(op);
				op = SYMbind(op);
			}
			else
				break;
		}
		else if(binop(tok))
			op = Curop;	/* from scanner */
		else if(tok == LPARENTOK) {
			if(obj == helpsym) {
				pstat(PSET,PLAINSYM);
				obj = funcall(obj,strom);
				pstat(PCLEAR,PLAINSYM);
			}
			else
				obj = funcall(obj,strom);
			continue;
		}
		else if(tok == LBRACKTOK) {
			obj = arraccess(obj,strom);
			continue;
		}
		else if(tok == RECDOTTOK) {
			obj = recaccess(obj,strom);
			continue;
		}
		else if(tok == DEREFTOK) {
			PARSpush(obj);
			obj = mkunode(derefsym);
			PARSpop();
			tok = nexttok(strom,1);
			continue;
		}
		else
			break;
		prec1 = operprec(tok);
		if(prec1 < prec)
			break;
		else if(prec1 == prec) {
			if(prec == RELPREC) {
/* Fehler-Behandlung noch mangelhaft */
				rerror(parserrsym,"RELPREC",Curop);
				obj = parserrsym;
				break;
			}
			else if(!rightass(tok))
				break;
		}
		PARSpush(obj);
		nexttok(strom,0);
		obj = readexpr(strom,prec1);
		if(obj == parserrsym || obj == breaksym) {
			PARSpop();
			break;
		}
		PARSpush(obj);
		obj = optbnode(op);
		PARSnpop(2);
	}
	return(obj);
}
/*--------------------------------------------------------------------*/
#define isCONSTANT(p)	(*FLAGPTR(p) >= fCONSTLIT)
/*--------------------------------------------------------------------*/
PRIVATE truc optbnode(op)
truc op;
{
	truc obj, sym, sym1;
	truc *ptr;
    int doopt;

	if(op == modsym) {
	/* Erkennt einen Ausdruck der Form Base ** Expo mod Modul
	** und stellt einen Knoten modpower(Base,Expo,Modul) her
    ** Analog fuer pol_mult(P1,P2) mod N
	*/
	    if(*FLAGPTR(argStkPtr-1) == fBUILTIN2) {
            doopt = 0;
			ptr = TAddress(argStkPtr-1);
            sym = *ptr;
            if(sym == powersym) {
                doopt = 1;
                sym1 = modpowsym;
            }
#ifdef POLYARITH
            else if(sym == polmultsym) {
                doopt = 1;
                sym1 = polNmultsym;
            }
            else if(sym == polmodsym) {
                doopt = 1;
                sym1 = polNmodsym;
            }
            else if(sym == poldivsym) {
                doopt = 1;
                sym1 = polNdivsym;
            }
#endif /* POLYARITH */
            if(doopt) {
				obj = *argStkPtr;	    /* Modul */
				argStkPtr[-1] = ptr[1]; /* arg1 */
				*argStkPtr = ptr[2];	/* arg2 */
				PARSpush(obj);
				obj = mkfunode(sym1,3);
				PARSpop();
				return(obj);
			}
	    }
	}
	/* Binaere Ausdruecke mit konstanten Operanden
	** werden ausgewertet
	*/
	if(op != powersym && isCONSTANT(argStkPtr-1)
                      && isCONSTANT(argStkPtr)) {
		*argStkPtr = mkbnode(op);
		return(eval(argStkPtr));
	}
	return(mkbnode(op));
}
/*--------------------------------------------------------------------*/
/*
** read a primary expression from strom
*/
PRIVATE truc primary(strom,cflgptr)
truc *strom;
int *cflgptr;
{
	truc obj;
	int tok, weiter = 1;
	int sflg;

	*cflgptr = 0;
  nochmal:
	tok = skipeoltok(strom);

	switch(tok) {
	case SYMBOLTOK:
		sflg = parsedsym(SymBuf,&obj);
		if(sflg == aERROR) {
			obj = parserrsym;
		}
		else if(sflg == sPARSAUX) {
			if(obj == whilesym) {
				obj = readwhile(strom);
				*cflgptr = 1;
			}
			else if(obj == ifsym) {
				obj = readif(strom);
				*cflgptr = 1;
			}
			else if(obj == forsym) {
				obj = readfor(strom);
				*cflgptr = 1;
			}
			else if(obj == funcsym || obj == procsym) {
				if (pstat(PQUERY,ARGLIST) == ARGLIST) {
					obj = funcsym;
                    nexttok(strom,0);
				}
				else {
					obj = compfunc(strom);
				}
				*cflgptr = 1;
			}
			else if(obj == varsym) {
				obj = globvardef(strom);
				*cflgptr = 1;
			}
			else if(obj == notsym) {
				nexttok(strom,0);
				obj = readexpr(strom,NOTPREC);
/* Fehlerbehandlung fehlt */
				PARSpush(obj);
				obj = mkunode(not_sym);
				PARSpop();
			}
			else if(obj == retsym) {
				nexttok(strom,0);
				obj = readexpr(strom,0);
/* Fehlerbehandlung fehlt */
				PARSpush(obj);
				obj = mkunode(ret_sym);
				PARSpop();
				*cflgptr = 1;
			}
			else if(obj == exitsym) {
				nexttok(strom,0);
				obj = exitfun;
				*cflgptr = 1;
			}
			else if(obj == lpbrksym) {
				nexttok(strom,0);
				obj = lpbrkfun;
				*cflgptr = 1;
			}
			else if(obj == lpcontsym) {
				nexttok(strom,0);
				obj = lpcontfun;
				*cflgptr = 1;
			}
			else if(obj == writesym || obj == writlnsym) {
				tok = nexttok(strom,0);
				if(tok != LPARENTOK) {
					rerror(obj,err_0lparen,voidsym);
					obj = parserrsym;
				}
				else {
					obj = readwrite(obj,strom);
					*cflgptr = 1;
				}
			}
			else if(obj == typesym) {
				obj = globtypedef(strom);
				*cflgptr = 1;
			}
			else if(obj == constsym) {
				obj = globconstdef(strom);
				*cflgptr = 1;
			}
			else {
				rerror(parserrsym,"",obj);
				obj = parserrsym;
			}
			return(obj);
		}
		else if(sflg == sEXTFUNCTION) {
			/* external function, during compiling */
			tok = nexttok(strom,1);
			if(tok != LPARENTOK) {
				rerror(funcsym,err_unvar,obj);
				return(parserrsym);
			}
			else
				weiter = 0;
		}
		break;
	case INUMTOK:
		obj = mkint(Curnum.sign,Curnum.digits,Curnum.len);
		break;
    case GF2NTOK:
		obj = mkgf2n(Curnum.digits,Curnum.len);
		break;
	case FLOATTOK:
		obj = mkfloat(fltreadprec(),&Curnum);
		break;
	case CHARTOK:
		obj = mkchar(StrBuf[0]);
		break;
	case STRINGTOK:
		obj = mkstr(StrBuf);
		break;
	case BSTRINGTOK:
		obj = mkbstr((byte*)Curnum.digits,Curnum.len);
		break;
	case LPARENTOK:
	case LBRACETOK:
		obj = readvector(tok,strom);
		weiter = 0;
		break;
	case PLUSTOK:		/* skip unary + */
		nexttok(strom,1);
		goto nochmal;
	case MINUSTOK:
		nexttok(strom,0);
		obj = readexpr(strom,operprec(UMINUSTOK));
		obj = negate(obj);
		weiter = 0;
		break;
	case SEMICOLTOK:
		obj = voidsym;
		*cflgptr = 1;
		break;
	case RPARENTOK:
	case RBRACETOK:
		rerror(parserrsym,err_rparen,voidsym);
		obj = parserrsym;
		break;
	case HISTORYTOK:
		obj = prochistory(SymBuf,strom);
		*cflgptr = 1;
		break;
	case QUESTIONTOK:
		if(pstat(PQUERY,COMPILING)) {
			nexttok(strom,1);
			goto nochmal;
		}
		obj = mkfunode(helpsym,0);
		weiter = 0;
		*cflgptr = 1;
		break;
	case DOTTOK:
		obj = enddotsym;
		*cflgptr = 1;
		weiter = 0;
		break;
	case Z1TOK:
        *cflgptr = 1;
        error(scansym,"input too long",bufovflsym);
		obj = brkerr();
        break;
	case EOFTOK:
		obj = eofsym;
		weiter = 0;
		break;
	default:
		rerror(parserrsym,"while parsing",tmptoksym(tok));
/****************************/
		obj = parserrsym;
	}
	if(weiter)
		nexttok(strom,0);
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE int parsedsym(name,pobj)
char *name;
truc *pobj;
{
	struct symbol *sptr;
	truc obj;
	unsigned n;
	int sflg;

	obj = mksym(name,&sflg);
	if(pstat(PQUERY,PLAINSYM))
		sflg = sVARIABLE;
	if(pstat(PQUERY,COMPILING) && sflg < sFBINARY) {
		n = SYMcc0(obj);
		if(n == mGLOBAL) {
			if(sflg == sCONSTANT) {
				sptr = symptr(obj);
				obj = sptr->bind.t;
			}
		}
		else if(n & mGLOBAL) {
		/* obj is a reference variable */
			n &= ~mGLOBAL;
			obj = mklocsym(fRSYMBOL,n-1);
		}
		else if(n >= mLOCCONST) {
			n -= mLOCCONST;
			obj = mklocsym(fTMPCONST,n);
			if(!pstat(PQUERY,CONSTDECL)) {
				obj = Lconsteval(&obj);
			}
		}
		else if(n) {
		/* obj is a local variable */
			obj = mklocsym(fLSYMBOL,n-1);
		}
		else if(sflg != sFUNCTION) {
		/* identifier must be an external, not yet defined function
		** otherwise generates syntax error
		*/
			sflg = sEXTFUNCTION;
		}
	}
	*pobj = obj;
	return(sflg);
}
/*--------------------------------------------------------------*/
/*
** process commandline of the form !, !!, !!!, !a, !b, !c.
*/
PRIVATE truc prochistory(sbuf,strom)
char *sbuf;
truc *strom;
{
	int tok, ind, ch;

	ch = sbuf[1];
	if('a' <= ch && ch <= 'c')
		ind = ch;
	else
		ind = strlen(sbuf);
/*
** Indices: ! = 1, !! = 2, !!! = 3, !a = 'a', !b = 'b', !c = 'c'
** used by function historydisp() in module terminal.c
*/
	tok = nexttok(strom,0);
	if(tok == EOLTOK) {
		SYMbind(historsym) = mkfixnum(ind);
		return(historsym);
	}
	else
		return(brkerr());
}
/*--------------------------------------------------------------*/
/*
** Lese Argument-Liste fuer write und writeln-Funktion
*/
PRIVATE truc readwrite(wsym,strom)
truc wsym;		/* writesym or writlnsym */
truc *strom;
{
	truc *savptr;
	truc fun;
	truc obj;
	int tok, chk;
	int n = 0;

	savptr = argStkPtr;
	tok = nexttok(strom,1);		/* ueberlese linke Klammer */
	while(tok != RPARENTOK) {
		obj = readexpr(strom,0);
		if(obj == parserrsym)
			goto cleanup;
		else {
			PARSpush(obj);
			n++;
		}
		tok = skipeoltok(strom);
		if(tok == COLONTOK) {
			obj = readformat(strom,&tok);
			if(obj == parserrsym)
				goto cleanup;
			PARSpush(obj);
			obj = mkbnode(formatsym);
			PARSpop();
			*argStkPtr = obj;
		}
		if(tok == COMMATOK)
			nexttok(strom,1);
		else if(tok != RPARENTOK) {
			rerror(parserrsym,err_0rparen,voidsym);
			obj = parserrsym;
			goto cleanup;
		}
	}
	nexttok(strom,0);

	fun = SYMbind(wsym);	/* write_sym or writln_sym */
	chk = chknargs(fun,n);

	if(chk == NARGS_VAR)
		obj = mkfunode(fun,n);
	else {
		rerror(fun,err_args,voidsym);
		obj = parserrsym;
	}
  cleanup:
	argStkPtr = savptr;
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** Lese Format-Angaben in write- oder writeln-Funktion
** nach dem Doppelpunkt
*/
PRIVATE truc readformat(strom,tokptr)
truc *strom;
int *tokptr;
{
	truc *arr;
	truc obj;
	int m = 0;

	arr = workStkPtr + 1;
	while(*tokptr == COLONTOK) {
		nexttok(strom,1);
		obj = readexpr(strom,0);
		if(obj == parserrsym)
			goto cleanup;
		WORKpush(obj);
		m++;
		*tokptr = skipeoltok(strom);
	}
	obj = mkntuple(fTUPLE,arr,m);
  cleanup:
	workStkPtr = arr - 1;
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** read vector or parentized expression
*/
PRIVATE truc readvector(flg,strom)
int flg;	/*  flg == LPARENTOK or LBRACETOK */
truc *strom;
{
	truc obj;
	int endtok;
	int n;

	endtok = (flg == LBRACETOK ? RBRACETOK : RPARENTOK);
	n = arglist(strom,endtok);
	if(n == aERROR)
		obj = parserrsym;
	else if(n == 1 && flg == LPARENTOK)
		obj = PARSretr();
	else {
		obj = mkfunode(vectorsym,n);
		PARSnpop(n);
	}
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** read expression between [ and ]
** either [<index>]		(*pmode = 0)
** or [<start> .. <end>]	(*pmode = 1)
*/
PRIVATE truc readbrack(strom,pmode)
truc *strom;
int *pmode;
{
	truc obj;
	int tok;

	nexttok(strom,0);		/* ueberlese linke Klammer */
	obj = readexpr(strom,0);
	tok = skipeoltok(strom);
	if(tok == RBRACKTOK) {
		*pmode = 0;
		goto ausgang;
	}
	else if(tok == DOTDOTTOK) {
		*pmode = 1;
		PARSpush(obj);
		tok = nexttok(strom,1);	 /* ueberlese .. */
		if(tok == RBRACKTOK) {
			PARSpush(endsym);
		}
		else {
			obj = readexpr(strom,0);
			PARSpush(obj);
			tok = skipeoltok(strom);
		}
		if(tok == RBRACKTOK) {
			obj = mkbnode(pairsym);
			PARSnpop(2);
			goto ausgang;
		}
		else {
			PARSnpop(2);
			goto errexit;
		}
	}
  errexit:
	 /* genauere Fehlermeldung wuenschenswert */
	 rerror(parserrsym,err_0rbrack,voidsym);
	 return(parserrsym);
  ausgang:
	nexttok(strom,0);
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** parse function call
*/
PRIVATE truc funcall(fun,strom)
truc fun;
truc *strom;
{
	truc obj;
	int n, chk;

	/* ? check function name !!! */
/*** case of functions as local variables ? ****/
	if(Tflag(fun) == fSYMBOL && Symflag(fun) == sTYPESPEC) {
		if(fun == bstringsym) {
			fun = bstr_sym;
		}
		else if(fun == integsym) {
			fun = int_sym;
		}
		else if(fun == stringsym) {
			fun = str_sym;
		}
        else if(fun == gf2nintsym) {
            fun = gf2n_sym;
        }
		else {
			rerror(voidsym,err_funame,fun);
			return(parserrsym);
		}
	}
	n = arglist(strom,RPARENTOK);
	if(n == aERROR)
		return(parserrsym);
	chk = chknargs(fun,n);

	if(chk == NARGS_OK) {
		if(n == 1)
			obj = mkunode(fun);
		else if(n == 2)
			obj = mkbnode(fun);
		else if(n == 0)
			obj = mk0fun(fun);
		else
			obj = mkfunode(fun,n);
	}
	else if(chk == NARGS_VAR)
		obj = mkfunode(fun,n);
	else {
		rerror(fun,err_args,voidsym);
		obj = parserrsym;
	}
	PARSnpop(n);
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE truc arraccess(arr,strom)
truc arr;
truc *strom;
{
	truc obj;
	truc sym;
	int mode;

	/* ? check array name !!! */

	PARSpush(arr);
	obj = readbrack(strom,&mode);
	if(obj != parserrsym) {
		sym = (mode == 0 ? arr_sym : subarrsym);
		PARSpush(obj);
		obj = mkbnode(sym);
		PARSpop();
	}
	PARSpop();
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE truc recaccess(rr,strom)
truc rr;
truc *strom;
{
	truc obj;
	int tok;

	PARSpush(rr);
	tok = nexttok(strom,0);		/* ueberlese dot */
	if(tok != SYMBOLTOK || lookupsym(SymBuf,&obj) == aERROR) {
		rerror(recordsym,err_field,voidsym);
		obj = parserrsym;
		goto ausgang;
	}
	nexttok(strom,0);
	PARSpush(obj);
	obj = mkbnode(rec_sym);
	PARSpop();
  ausgang:
	PARSpop();
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** parse function definition
*/
PRIVATE truc compfunc(strom)
truc *strom;
{
	struct symbol *sptr;
	truc *parssav, *worksav;
	truc fun, parms, typ, globs, consts, vars, delim, body;
	truc obj = parserrsym;
	int tok, sflg, vflg;
	int numstats;
	int numparms = 0, numvars = 0, numopts = 0;
	int errflg = 0;

	if(pstat(PQUERY,COMPILING)) {
		rerror(parserrsym,err_funest,voidsym);
		goto ausgang;
	}
	else
		pstat(PSET,COMPILING);

	parssav = argStkPtr;
	worksav = workStkPtr;

	/******** function head *********/
	fun = getfuname(strom);
	if(fun == parserrsym)
		goto ausgang;
	parms = parmlist(strom,&vflg,&numopts);
	if(parms == parserrsym)
		goto ausgang;
	vars2push(parms);
	numparms = markparms(*workStkPtr,*argStkPtr);
	if(numparms == aERROR)
		goto cleanup1;

	tok = skipeoltok(strom);
	if(tok == COLONTOK) {	/* return type of function */
		typ = typespec(strom);
		if(typ == parserrsym)
			goto cleanup1;
		tok = skipeoltok(strom);
/****** no action taken in this version ******/
	}
	if(tok == SEMICOLTOK) {
		tok = nexttok(strom,1);
	}

	/******** declaration of global variables *********/
	if(tok != SYMBOLTOK || lookupsym(SymBuf,&delim) == aERROR) {
		rerror(funcsym,err_synt,voidsym);
		goto cleanup1;
	}
	if(delim == extrnsym) {
		pstat(PSET,EXTERNDECL);
		globs = varlist(strom,&delim);
		pstat(PCLEAR,EXTERNDECL);
		if(globs == parserrsym)
			goto cleanup1;
		globs = varssyms(globs);
	}
	else {
		globs = voidsym;
	}
	WORKpush(globs);
	if(markglobs(globs) == aERROR)
		goto cleanup2;

	/********** declaration of constants *********/
	if(delim == constsym) {
		pstat(PSET,CONSTDECL);
		consts = lconstlist(strom,&delim);
		if(consts == parserrsym)
		    goto cleanup2;
		pstat(PCLEAR,CONSTDECL);
		WORKpush(varssyms(consts));
		if(Lconstini(consts) == aERROR)
			goto cleanup3;
	}
	else {
		WORKpush(voidsym);
	}
	/******** declaration of local variables ********/
	if(delim == varsym) {
		vars = varlist(strom,&delim);
		if(vars == parserrsym)
			goto cleanup3;
	}
	else {
		vars = voidsym;
	}
	if(delim != beginsym) {
		errexpect(funcsym,beginsym,delim);
		goto cleanup3;
	}
	vars2push(vars);
	numvars = marksymbs(*workStkPtr);
	if(numvars == aERROR)
		goto cleanup4;

	/******** function body *********/
	nexttok(strom,1);
	numstats = statements(strom,&delim);
	if(numstats == aERROR || delim != endsym) {
		errflg = 1;
	}
	else if(curtok(strom) == SYMBOLTOK) {
		sflg = lookupsym(SymBuf,&delim);
		if(sflg != aERROR && delim == fun)
			nexttok(strom,0);
		else
			errflg = 1;
	}
	if(errflg) {
		rerror(funcsym,err_synt,voidsym);
		goto cleanup4;
	}
	body = mkcompnode(fCOMPEXPR,numstats);
	PARSnpop(numstats);
	PARSpush(body);
	sptr = symptr(fun);
	*FLAGPTR(sptr) = (vflg ? sVFUNCTION : sFUNCTION);
	sptr->bind.t = mkfundef(numparms,numopts,numvars);
	obj = fun;

  cleanup4:
	vars = worksav[4];
	unmarksymbs(vars);
  cleanup3:
	consts = worksav[3];
	unmarksymbs(consts);
	Lconstini(voidsym);
  cleanup2:
	globs = worksav[2];
	unmarksymbs(globs);
  cleanup1:
	parms = worksav[1];
	unmarksymbs(parms);
	locsymbs = 0;
	argStkPtr = parssav;
	workStkPtr = worksav;
  ausgang:
	pstat(PCLEAR,COMPILING);
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** symbs ist ein bnode, wie er von varlist geliefert wird.
** Das erste Argument (Symbolliste) wird zurueckgegeben
*/
PRIVATE truc varssyms(symbs)
truc symbs;
{
	if(symbs == voidsym)
		return(symbs);
	else
		return(NODEarg0(symbs));
}
/*--------------------------------------------------------------*/
/*
** symbs ist ein bnode, wie er von varlist geliefert wird.
** Das erste Argument (Symbolliste) wird auf den WorkStack gelegt,
** das zweite (Liste der Initialisierungen) auf den ParsStack
*/
PRIVATE void vars2push(symbs)
truc symbs;
{
	truc *ptr;

	if(symbs == voidsym) {
		WORKpush(symbs);
		PARSpush(symbs);
	}
	else {
		ptr = Taddress(symbs);
		WORKpush(ptr[1]);
		PARSpush(ptr[2]);
	}
}
/*--------------------------------------------------------------*/
PRIVATE truc getfuname(strom)
truc *strom;
{
	truc fun;
	int tok, sflg;

	tok = nexttok(strom,1);
	if(tok != SYMBOLTOK) {
		rerror(funcsym,err_sym,voidsym);
		return(parserrsym);
	}
	fun = mksym(SymBuf,&sflg);
	if(sflg >= sSYSTEM) {
		rerror(funcsym,err_funame,fun);
		return(parserrsym);
	}
	return(fun);
}
/*--------------------------------------------------------------*/
/*
** Die formalen Parameter einer Funktion (Liste symbs) werden mit
** fortlaufenden Nummern markiert. Aus der Liste kind ist
** abzulesen, ob es sich um einen Variablen-Parameter handelt.
** In diesem Fall wird die Nummer um mGLOBAL erhoeht
*/
PRIVATE int markparms(symbs,kind)
truc symbs, kind;
{
	truc *ptr, *ptr1;
	word2 *p0;
	int i, n;

	if(symbs == voidsym)
		return(0);
	ptr = Taddress(symbs);
	n = *WORD2PTR(ptr);
	ptr1 = Taddress(kind);
	for(i=1; i<=n; i++) {
		p0 = SYMCC0PTR(ptr+i);
		if(*p0) {
			rerror(funcsym,err_2ident,ptr[i]);
			return(aERROR);
		}
		*p0 = ++locsymbs;
		if(*FLAGPTR(ptr1 + i) == fSPECIAL1) {
			/* dann Variable-Parameter */
			*p0 |= mGLOBAL;
		}
	}
	return(n);
}
/*--------------------------------------------------------------*/
/*
** In einer Funktion vorkommende lokale Variable werden mit
** fortlaufenden Nummern markiert
** Die letzte benutzte Nummer steht in der globalen Variablen locsymbs
*/
PRIVATE int marksymbs(symbs)
truc symbs;
{
	truc *ptr;
	word2 *p0;
	int i, n;

	if(symbs == voidsym)
		return(0);
	ptr = Taddress(symbs);
	n = *WORD2PTR(ptr);
	for(i=1; i<=n; i++) {
		p0 = SYMCC0PTR(ptr+i);
		if(*p0) {
			rerror(funcsym,err_2ident,ptr[i]);
			return(aERROR);
		}
		*p0 = ++locsymbs;
	}
	return(n);
}
/*--------------------------------------------------------------*/
/*
** In einer Funktion vorkommende globale Variable werden in der
** Symbol-Tabelle mit der Marke mGLOBAL markiert.
*/
PRIVATE int markglobs(symbs)
truc symbs;
{
	truc *ptr;
	word2 *p0;
	int i, n;

	if(symbs == voidsym)
		return(0);
	ptr = Taddress(symbs);
	n = *WORD2PTR(ptr);
	for(i=1; i<=n; i++) {
		p0 = SYMCC0PTR(ptr+i);
		if(*p0) {
			rerror(funcsym,err_2ident,ptr[i]);
			return(aERROR);
		}
		*p0 = mGLOBAL;
	}
	return(n);
}
/*--------------------------------------------------------------*/
/*
** Markierung von Symbolen waehrend der Compilation wird wieder geloescht
*/
PRIVATE void unmarksymbs(symbs)
truc symbs;
{
	truc *ptr;
	int i, n;

	if(symbs == voidsym)
		return;
	ptr = Taddress(symbs);
	n = *WORD2PTR(ptr);
	for(i=1; i<=n; i++) {
		*SYMCC0PTR(ptr+i) = 0;
	}
}
/*--------------------------------------------------------------*/
/*
** falls waehrend der Compilation ein Reset vorkommt,
** muessen alle Markierungen mit clearcompile geloescht werden
*/
PUBLIC void clearcompile()
{
	truc *ptr;
	int i;

	if(pstat(PQUERY,COMPILING)) {
		i = 0;
		while((ptr = nextsymptr(i++)) != NULL)
			*SYMCC0PTR(ptr) = 0;
	}
	locsymbs = 0;
	pstat(PCLEARALL,0);
}
/*--------------------------------------------------------------*/
/*
** Liest die formale Parameterliste einer Funktions-Definition
** einschliesslich der Klammern '(' und ')' .
*/
PRIVATE truc parmlist(strom,vflgptr,optcptr)
truc *strom;
int *vflgptr, *optcptr;
{
	truc *parssav, *worksav;
	truc obj, delim;
	int varflg, optc;
	int tok;
	int k = 0, n = 0;

	tok = nexttok(strom,1);
	if(tok != LPARENTOK) {
		rerror(funcsym,err_0lparen,voidsym);
		return(parserrsym);
	}

	parssav = argStkPtr;
	worksav = workStkPtr;
	*vflgptr = 0;
	tok = nexttok(strom,1);		/* ueberlese LPARENTOK */
	if(tok == RPARENTOK) {
		nexttok(strom,0);	/* ueberlese RPARENTOK */
		k = -1;
	}
	optc = 0;
	while(k >= 0) {
		k = parmsection(strom,&delim,&varflg);
		if(optc && varflg != -1) {
			k = aERROR;
		}
		else if(varflg == -1)
			optc += k;
		else if(varflg == 1)
			*vflgptr = 1;
		if(k >= 0) {
			n += k;
			if(delim == nullsym) {	/* denotes RPARENTOK */
				nexttok(strom,0);
				break;
			}
		}
		else if(k == aERROR) {
			rerror(funcsym,err_parl,voidsym);
			obj = parserrsym;
			goto cleanup;
		}
	}
	obj = mkntuple(fTUPLE,parssav+1,n);
	PARSpush(obj);
	obj = mkntuple(fTUPLE,worksav+1,n);
	PARSpush(obj);
	obj = mkbnode(inivarsym);
  cleanup:
	*optcptr = optc;
	argStkPtr = parssav;
	workStkPtr = worksav;
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE truc varlist(strom,endptr)
truc *strom;
truc *endptr;
{
	truc *parssav, *worksav;
	truc obj;
	int k, n;

	nexttok(strom,1);		/* ueberspringe var oder record */

	parssav = argStkPtr;
	worksav = workStkPtr;
	n = 0;
	while(1) {
		k = varsection(strom,endptr);
		if(k >= 0) {
			n += k;
		}
		else if(k == aERROR) {
			obj = parserrsym;
			goto cleanup;
		}
		else
			break;
	}
	obj = mkntuple(fTUPLE,parssav+1,n);
	PARSpush(obj);
	obj = mkntuple(fTUPLE,worksav+1,n);
	PARSpush(obj);
	obj = mkbnode(inivarsym);
  cleanup:
	argStkPtr = parssav;
	workStkPtr = worksav;
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** Parst eine lokale Konstantendeklaration.
** Die benutzten Konstantennamen werden markiert.
** Zurueckgegeben wird ein bnode mit zwei n-tuplen aus
** Konstantennamen und Liste der Initialisierungen.
** Ist die Konstantendeklaration leer, wird voidsym zurueckgegeben.
** Im Fehlerfall wird parserrsym zurueckgegeben und die
** Markierungen werden wieder geloescht.
*/
PRIVATE truc lconstlist(strom,endptr)
truc *strom;
truc *endptr;
{
	truc *parssav, *worksav;
	truc obj;
	word2 *pt2;
	int tok, sflg;
	int n;

	parssav = argStkPtr;
	worksav = workStkPtr;

	tok = nexttok(strom,1);		/* ueberspringe const */
	n = 0;
	while(1) {
		if(tok != SYMBOLTOK) {
			rerror(constsym,err_sym,voidsym);
			goto unmarkcleanup;
		}
		obj = mksym(SymBuf,&sflg);
		if(sflg >= sSYSTEM) {
			*endptr = obj;
			break;
		}
		PARSpush(obj);
		tok = nexttok(strom,1);
		if(tok != EQTOK) {
			errexpect(constsym,equalsym,voidsym);
			goto unmarkcleanup;
		}
		tok = nexttok(strom,0);
		obj = readexpr(strom,0);
		if(obj == parserrsym) {
			goto unmarkcleanup;
		}
		WORKpush(obj);

		/** mark local constant **/
		pt2 = SYMCC0PTR(argStkPtr);
		if(*pt2) {
			rerror(constsym,err_2ident,*argStkPtr);
			goto unmarkcleanup;
		}
		else {
			*pt2 = mLOCCONST + n;
			n++;
		}
		tok = skipeoltok(strom);
		if(tok == SEMICOLTOK)
			tok = nexttok(strom,1);
	}
	if(n > 0) {
		obj = mkntuple(fTUPLE,parssav+1,n);
		PARSpush(obj);
		obj = mkntuple(fTUPLE,worksav+1,n);
		PARSpush(obj);
		obj = mkbnode(inivarsym);
	}
	else
		obj = voidsym;
	goto cleanup;
  unmarkcleanup:
	obj = mkntuple(fTUPLE,parssav+1,n);
	unmarksymbs(obj);
	obj = parserrsym;
  cleanup:
	argStkPtr = parssav;
	workStkPtr = worksav;
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** Liest eine Zeile ein Variablen-Deklaration der Gestalt
**	Sym1, ..., Symk: Type;
** bzw.
**	Sym1, ..., Symk;
** und legt die Symbole auf den ParsStack; gleichzeitig
** werden die Initialisierungen auf den WorkStack gelegt.
** Die Stacks werden nicht aufgeraeumt!!
** Rueckgabewert:
** Anzahl der Symbole, falls erfolgreich
** -1,	  falls Liste zu Ende. 
**	  In diesem Fall wird das Endsymbol in *endptr abgelegt.
** aERROR, falls Fehler
*/
PRIVATE int varsection(strom,endptr)
truc *strom;
truc *endptr;
{
	truc typ;
	int tok;
	int i, n;

	tok = skipeoltok(strom);
	n = varsaux(tok,strom,&typ,endptr);
	if(n == aERROR) {
/* Fehlermeldung nicht immer zutreffend, da auch von
** globtypedef aus benutzt
*/
		rerror(varsym,err_varl,voidsym);
	}
	else for(i=0; i<n; i++) {
		WORKpush(typ);
	}
	return(n);
}
/*--------------------------------------------------------------*/
/*
** Liest eine Zeile einer formalen Parameter-Deklaration der Gestalt
**	Sym1, ..., Symk: Type;
** bzw.
**	Sym1, ..., Symk;
** bzw.
**	var Sym1, ..., Symk: Type;
** bzw.
**	var Sym1, ..., Symk;
** und legt die Symbole auf den ParsStack; gleichzeitig
** werden die Typ-Initialisierungen auf den WorkStack gelegt. 
** Falls das Symbol var vorkommt, wird statt der Typ-Initialisierung
** das Paar (var_sym, typ) verwendet.
** Die Stacks werden nicht aufgeraeumt!!
** Rueckgabewert:
** Anzahl der Symbole, falls erfolgreich
** -1,	  falls Liste zu Ende, d.h. nur eine Klammer ) vorhanden.
** aERROR, falls Fehler
**
** In der Variablen vflgptr wird 1 abgelegt, falls das Symbol var
** vorkommt, -1, falls es sich um einen optionalen Parameter handelt,
** sonst 0.
*/
PRIVATE int parmsection(strom,endptr,vflgptr)
truc *strom;
truc *endptr;
int *vflgptr;
{
	truc obj, typ;
	int tok;
	int i, n = 0;
	int oflg;

	*vflgptr = 0;
	tok = skipeoltok(strom);
	if(tok == SYMBOLTOK && lookupsym(SymBuf,&obj) == sPARSAUX) {
		if(obj == varsym) {
			*vflgptr = 1;
			tok = nexttok(strom,1);
		}
		else {
			n = aERROR;
			goto ausgang;
		}
	}
	if(tok != SYMBOLTOK)
		n = aERROR;
	else
		n = parmsaux(strom,&typ,endptr,&oflg);
	if(n != aERROR) {
		if(oflg) {
			typ = eval(&typ);
			if(typ == breaksym || *vflgptr != 0) {
				n = aERROR;
				goto ausgang;
			}
			else {
				*vflgptr = -1;
			}
		}
		if(*vflgptr == 1) {
			typ = mkpair(fSPECIAL1,var_sym,typ);
		}
		for(i=0; i<n; i++)
			WORKpush(typ);
	}
  ausgang:
	return(n);
}
/*--------------------------------------------------------------*/
PRIVATE int parmsaux(strom,typptr,endptr,ofptr)
truc *strom;
truc *typptr, *endptr;
int *ofptr;
{
	truc obj;
	int tok;
	int sflg;
	int n = 0;

	*typptr = zero;		/* default type integer */
	*endptr = voidsym;
	*ofptr = 0;
	tok = SYMBOLTOK;
	while(tok == SYMBOLTOK) {
		obj = mksym(SymBuf,&sflg);
		if(sflg >= sSYSTEM) {
			if(n == 0) {
				*endptr = obj;
				n = -1;
			}
			else
				n = aERROR;
			break;
		}
		PARSpush(obj);
		n++;
		tok = nexttok(strom,1);
		if(tok == COMMATOK) {
			tok = nexttok(strom,1);
			continue;
		}
		else if(tok == COLONTOK) {
			*typptr = typespec(strom);
			if(*typptr == parserrsym) {
				n = aERROR;
				break;
			}
			tok = skipeoltok(strom);
		}
		else if(tok == ASSIGNTOK) {
			nexttok(strom,0);
			if(n != 1) {
				n = aERROR;
				break;
			}
			*typptr = readexpr(strom,0);
			if(*typptr == parserrsym) {
				n = aERROR;
				break;
			}
			*ofptr = 1;
			tok = skipeoltok(strom);
		}
		if(tok == SEMICOLTOK) {
			tok = nexttok(strom,1);
		}
		else if(tok == SYMBOLTOK) {
			if(lookupsym(SymBuf,endptr) != sDELIM)
				n = aERROR;
		}
		else if(tok != RPARENTOK)
			n = aERROR;
		break;
	}
	if(tok == RPARENTOK) {
		*endptr = nullsym;
	}
	return(n);
}
/*--------------------------------------------------------------*/
PRIVATE int varsaux(tok,strom,typptr,endptr)
int tok;
truc *strom;
truc *typptr, *endptr;
{
	truc obj;
	int sflg;
	int n = 0;

	*typptr = zero;		/* default type integer */
	*endptr = voidsym;
	if(tok != SYMBOLTOK) {
		return(aERROR);
	}
	while(tok == SYMBOLTOK) {
		obj = mksym(SymBuf,&sflg);
		if(sflg >= sSYSTEM) {
			if(n == 0) {
				*endptr = obj;
				n = -1;
			}
			else
				n = aERROR;
			break;
		}
		PARSpush(obj);
		n++;
		tok = nexttok(strom,1);
		if(tok == COMMATOK) {
			tok = nexttok(strom,1);
			continue;
		}
		else if(tok == COLONTOK) {
			*typptr = typespec(strom);
			if(*typptr == parserrsym) {
				n = aERROR;
				break;
			}
			tok = skipeoltok(strom);
		}
		else if(tok == ASSIGNTOK) {
			nexttok(strom,0);
			if(n != 1) {
				n = aERROR;
				break;
			}
			obj = readexpr(strom,0);
			*typptr = obj;
			tok = skipeoltok(strom);
		}
		if(tok == SEMICOLTOK) {
			tok = nexttok(strom,1);
		}
		else if(tok == SYMBOLTOK) {
			if(lookupsym(SymBuf,endptr) != sDELIM)
				n = aERROR;
		}
		else
			n = aERROR;
		break;
	}
	return(n);
}
/*--------------------------------------------------------------*/
/*
** Liest eine Typ-Spezifikation und liefert als Ergebnis
** den Default-Anfangswert dieses Typs bzw. einen
** Funktions-Aufruf, der diesen Anfangswert erzeugt
** Im Fehlerfall wird parserrsym zurueckgegeben.
*/
PRIVATE truc typespec(strom)
truc *strom;
{
	truc typ, obj, sym;
	int tok, sflg, mode;
	int n;
	int weiter = 1;

	obj = parserrsym;
	tok = nexttok(strom,1);		/* ueberlese : oder = */
	if(tok != SYMBOLTOK)
		return(parserrsym);

	typ = mksym(SymBuf,&sflg);
	if(sflg == sTYPESPEC) {
	    if(typ == integsym || typ == boolsym || typ == charsym
           || typ == gf2nintsym) {
		    obj = SYMbind(typ);
	    }
	    else if(typ == realsym) {
		    obj = fltzero(deffltprec());
	    }
	    else if(typ == stringsym || typ == bstringsym) {
		tok = nexttok(strom,1);
		if(tok == LBRACKTOK) {
		    obj = readbrack(strom,&mode);
		    if(obj == parserrsym || mode == 1)
			    return(parserrsym);
		    PARSpush(obj);
		    sym = (typ == stringsym ? mkstrsym : mkbstrsym);
		    obj = mkunode(sym);		/* fBUILTIN1 */
		    PARSpop();
		}
		else {
		    obj = (typ == stringsym ? nullstring : nullbstring);
		}
		weiter = 0;
	    }
	    else if(typ == arraysym) {
		tok = nexttok(strom,1);
		if(tok == LBRACKTOK) {
			obj = readbrack(strom,&mode);
			if(obj == parserrsym || mode == 1)
				return(parserrsym);
		}
		else
			obj = zero;
		PARSpush(obj);
		n = 1;
		tok = skipeoltok(strom);
		if(tok == SYMBOLTOK) {
			sflg = lookupsym(SymBuf,&obj);
			if(obj == ofsym) {
				obj = typespec(strom);
				PARSpush(obj);
				n = 2;
			}
		}
		if(obj != parserrsym)
			obj = mkfunode(mkarrsym,n);	/* fBUILTINn */
		PARSnpop(n);
		weiter = 0;
	    }
	    else if(typ == recordsym) {
		obj = readrecdef(strom);
		weiter = 0;
		PARSpush(obj);
		if(obj != parserrsym)
			obj = mkunode(mkrecsym);	/* fBUILTIN1 */
		PARSpop();
	    }
	    else if(typ == pointrsym) {
		obj = pointertype(strom);
		weiter = 0;
		PARSpush(obj);
		PARSpush(nil);
		if(obj != parserrsym)
			obj = mkrecord(fPOINTER,argStkPtr-1,2);
		PARSnpop(2);
	    }
	    else if(typ == filesym || typ == stacksym || typ == symbsym) {
            obj = SYMbind(typ);
	    }
	}
	else if(sflg == sTYPEDEF) {	/* user defined type */
		obj = SYMbind(typ);
	}
	else if(sflg == sPARSAUX) {
		if(typ == funcsym || typ == procsym ||
		    (pstat(PQUERY,EXTERNDECL) && typ == constsym))
			obj = typ;
	}
	if(weiter)
		nexttok(strom,1);
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE truc globtypedef(strom)
truc *strom;
{
	truc *worksav, *ptr;
	truc delim;
	truc sym, obj;
	int i, n, tok, sflg;
	int errflg;

	if(pstat(PQUERY,COMPILING)) {
		rerror(typesym,err_synt,voidsym);
		return(parserrsym);
	}
	tok = nexttok(strom,1);		/* ueberspringe type */
	delim = voidsym;
	worksav = workStkPtr;
	n = 0;
	errflg = 1;

	pstat(PSET,POINTRECURS);
	while(tok == SYMBOLTOK) {
		sym = mksym(SymBuf,&sflg);
		if(sflg >= sSYSTEM) {
			delim = sym;
			if(n > 0)
				errflg = 0;
			break;
		}
		tok = nexttok(strom,1);
		if(tok != EQTOK) {
			errexpect(typesym,equalsym,voidsym);
			break;
		}
		WORKpush(sym);
		n++;
		obj = typespec(strom);
		if(obj == parserrsym) {
			rerror(typesym,err_btype,voidsym);
			break;
		}
		*SYMFLAGPTR(workStkPtr) = sTYPEDEF;
		*SYMBINDPTR(workStkPtr) = obj;
		tok = nexttok(strom,1);
		if(tok == SEMICOLTOK)
			tok = nexttok(strom,1);
	}
	pstat(PCLEAR,POINTRECURS);

	if(!decldelim(delim)) {
		errexpect(typesym,endsym,delim);
		errflg = 1;
	}
	else if(delim == endsym)
		nexttok(strom,0);
	if(!errflg) {
		obj = typesym;
		/******* fix recursive pointer types ********/
		for(i=1; i<=n; i++) {
		    sym = *SYMBINDPTR(worksav+i);
		    if(Tflag(sym) == fPOINTER) {
			ptr = Taddress(sym);
			if(*FLAGPTR(ptr+1) == fTUPLE) {
				/* nothing to do */
				continue;
			}
			sym = ptrtypeval(ptr[1]);
			if(sym == parserrsym) {
			    errflg = 1;
			    break;
			}
			else {
			    ptr[1] = sym;
			}
		    }
		} /* end for */
	}
	if(errflg) {
		while(--n >= 0) {
			unbindsym(workStkPtr);
			WORKpop();
		}
		obj = parserrsym;
	}
	workStkPtr = worksav;
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE truc globconstdef(strom)
truc *strom;
{
	truc *worksav;
	truc sym, obj, delim;
	int tok, sflg, errflg;
	int n;

	if(pstat(PQUERY,COMPILING)) {
		rerror(constsym,err_synt,voidsym);
		return(parserrsym);
	}
	worksav = workStkPtr;
	delim = voidsym;
	n = 0;
	errflg = 1;

	tok = nexttok(strom,1);		/* ueberspringe const */
	while(tok == SYMBOLTOK) {
		sym = mksym(SymBuf,&sflg);
		if(sflg >= sSYSTEM) {
			delim = sym;
			if(n > 0)
				errflg = 0;
			break;
		}
		tok = nexttok(strom,1);
		if(tok != EQTOK) {
			errexpect(constsym,equalsym,voidsym);
			errflg = 1;
			break;
		}
		WORKpush(sym);
		n++;
		tok = nexttok(strom,0);
		obj = readexpr(strom,0);
		if(obj == parserrsym || (obj = eval(&obj)) == breaksym) {
			errflg = 1;
			break;
		}
		*SYMFLAGPTR(workStkPtr) = sCONSTANT;
		*SYMBINDPTR(workStkPtr) = obj;
		tok = skipeoltok(strom);
		if(tok == SEMICOLTOK)
			tok = nexttok(strom,1);
	}
	if(!decldelim(delim)) {
		errexpect(constsym,endsym,delim);
		errflg = 1;
	}
	if(delim == endsym)
		nexttok(strom,0);
	if(errflg) {
		while(--n >= 0) {
			unbindsym(workStkPtr);
			WORKpop();
		}
		obj = parserrsym;
	}
	else
		obj = constsym;
	workStkPtr = worksav;
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE truc globvardef(strom)
truc *strom;
{
	truc obj, delim;

	if(pstat(PQUERY,COMPILING)) {
		rerror(varsym,err_synt,voidsym);
		return(parserrsym);
	}
	obj = varlist(strom,&delim);
	if(!decldelim(delim)) {
		errexpect(varsym,endsym,delim);
		return(parserrsym);
	}
	else if(delim == endsym) {
		nexttok(strom,0);
	}
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE int decldelim(delim)
truc delim;
{
	if(delim == endsym || delim == varsym || delim == constsym ||
	   delim == typesym || delim == procsym || delim == funcsym)
		return(1);
	else
		return(0);
}
/*--------------------------------------------------------------*/
/*
** Klammer-Ausdruck,
** Argumentliste fuer Funktionsaufruf,
** oder Komponenten eines Vektors.
** Liest eine Folge von Expressions, die durch Kommas
** getrennt sind und die durch endtok (RPARENTOK oder RBRACETOK)
** beendet wird.
** Die Expressions werden auf den PARSstack gelegt.
** Rueckgabewert ist bei Erfolg die Anzahl der Expressions,
** der PARSstack wird nicht aufgeraeumt!!
** Falls ein Fehler auftritt wird aERROR zurueckgegeben und der
** PARSstack wird aufgeraeumt
*/
PRIVATE int arglist(strom,endtok)
truc *strom;
int endtok;
{
	truc obj;
	int tok, ret;
	int fehler = 0;
	int n = 0;
    char *errmess;

	tok = nexttok(strom,1);		/* ueberlese linke Klammer */
	pstat(PSET,ARGLIST);
	while(tok != endtok) {
		obj = readexpr(strom,0);
		if(obj == parserrsym) {
			fehler = 1;
			break;
		}
		else {
			PARSpush(obj);
			n++;
		}
		tok = skipeoltok(strom);
		if(tok == COMMATOK)
			nexttok(strom,1);
		else if(tok != endtok) {
            errmess = (endtok == RBRACETOK ? err_0brace : err_0rparen);
			rerror(parserrsym,errmess,voidsym);
			fehler = 1;
			break;
		}
	}
	pstat(PCLEAR,ARGLIST);
	if(fehler) {
		PARSnpop(n);
		ret = aERROR;
	}
	else {
		nexttok(strom,0);
		ret = n;
	}
	return(ret);
}
/*--------------------------------------------------------------*/
/*
** Liefert ein fTUPLE der Laenge 2*n, das einen Record beschreibt
** oder ein Symbol, das moeglicherweise Typ-Bezeichnung
** eines Records ist.
** Im Fehlerfall wird parserrsym zurueckgegeben.
*/
PRIVATE truc pointertype(strom)
truc *strom;
{
	truc obj, typ;
	int sflg;

	nexttok(strom,0);
	sflg = obligsym(pointrsym,strom,tosym);
	if(sflg == aERROR || skipeoltok(strom) != SYMBOLTOK)
		return(parserrsym);

	typ = mksym(SymBuf,&sflg);
	if(typ == recordsym) {
		obj = readrecdef(strom);
	}
	else {
	/* typ must be a symbol designing a record type */
		if(!pstat(PQUERY,POINTRECURS)) {
		/* get record type (fTUPLE) given by typ */
			obj = ptrtypeval(typ);
		}
		else {
		/* to be fixed later */
			obj = typ;
		}
		nexttok(strom,1);
	}
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** typ ist ein Symbol, das fuer einen record-Typ steht.
** Funktion liefert ein 2*n-tupel aus n Feldbezeichnungen und n
** Anfangswerten bzw. Anfangswert-Prozeduren fuer den Record,
** auf den der Pointer zeigen soll.
** Im Fehlerfall wird parserrsym zurueckgegeben.
*/
PRIVATE truc ptrtypeval(typ)
truc typ;
{
	struct symbol *sptr;
	truc *ptr;
	truc obj;
	int flg;
	int depth = 0;

  nochmal:
	if(++depth > 64) {
		error(typesym,err_rec,mkfixnum(depth));
		goto errexit;
	}
	sptr = symptr(typ);
	if(*FLAGPTR(sptr) != sTYPEDEF)
		goto errexit;
	typ = sptr->bind.t;
	flg = Tflag(typ);
	if(flg == fSYMBOL)
		goto nochmal;
	else if(flg == fBUILTIN1) {
		ptr = Taddress(typ);
		if(ptr[0] != mkrecsym)
			goto errexit;
		obj = ptr[1];
		flg = Tflag(obj);
		if(flg == fSYMBOL) {
			typ = obj;
			goto nochmal;
		}
		else if(flg == fTUPLE)
			return(obj);
	}
  errexit:
	return(parserrsym);
}
/*--------------------------------------------------------------*/
/*
** Liest eine record-Definition und liefert ein 2*n-tupel
** aus den n Feldbezeichnern und den n Anfangsdaten
** Im Fehlerfall wird parserrsym zurueckgegeben.
*/
PRIVATE truc readrecdef(strom)
truc *strom;
{
	truc *worksav;
	truc *ptr, *ptr1, *ptr2;
	truc delim, obj;
	int i,n;

	obj = varlist(strom,&delim);
	if(obj == parserrsym)
		return(parserrsym);
	if(delim != endsym) {
		errexpect(recordsym,endsym,delim);
		return(parserrsym);
	}
	else
		nexttok(strom,1);
	worksav = workStkPtr;
	if(obj == voidsym)
		n = 0;
	else {
		ptr = Taddress(obj);
		ptr1 = Taddress(ptr[1]);		/* symbols */
		ptr2 = Taddress(ptr[2]);		/* initial values */
		n = *WORD2PTR(ptr1);
		if(WORKspace(2*n) == NULL)
			reset(err_wrkstk);
		for(i=1; i<=n; i++) {
			worksav[i] = ptr1[i];
			worksav[n+i] = ptr2[i];
		}
	}
	obj = mkntuple(fTUPLE,worksav+1,2*n);
	workStkPtr = worksav;
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE truc readwhile(strom)
truc *strom;
{
	truc *savptr;
	truc obj, delim;
	int m, n = 0;
	int sflg;

	savptr = argStkPtr;
	nexttok(strom,0);	/* ueberspringe while */
	obj = readexpr(strom,0);
	if(obj == parserrsym) {
		goto cleanup;
	}
	PARSpush(obj);
	n++;
	sflg = obligsym(whilesym,strom,dosym);
	if(sflg == aERROR) {
		obj = parserrsym;
		goto cleanup;
	}
	m = statements(strom,&delim);
	if(m == aERROR || delim != endsym) {
		errexpect(whilesym,endsym,voidsym);
		obj = parserrsym;
		goto cleanup;
	}
	n += m;
	obj = mkntuple(fWHILEXPR,argStkPtr-n+1,n);
  cleanup:
	argStkPtr = savptr;
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE truc readfor(strom)
truc *strom;
{
	truc *savptr;
	truc obj, delim, inc;
	int m, n = 0;
	int tok, sflg;

	savptr = argStkPtr;
	tok = nexttok(strom,1);		/* ueberspringe for */
	if(tok != SYMBOLTOK || (sflg=parsedsym(SymBuf,&obj)) == aERROR ||
				sflg > sVARIABLE) {
		/* obj ist Lauf-Variable */
		/* genauere Fehlermeldung wuenschenswert */
		rerror(forsym,err_sym,voidsym);
		return(parserrsym);
	}
	PARSpush(obj);
	n++;	
	tok = nexttok(strom,1);
	if(tok != ASSIGNTOK) {
		rerror(forsym,err_synt,voidsym);
		obj = parserrsym;
		goto cleanup;
	}
	nexttok(strom,1);
	obj = readexpr(strom,0);      /* Start-Wert */
	PARSpush(obj);
	n++;
	sflg = obligsym(forsym,strom,tosym);
	if(sflg == aERROR) {
		obj = parserrsym;
		goto cleanup;
	}
	obj = readexpr(strom,0);      /* End-Wert */
	PARSpush(obj);
	n++;
	tok = skipeoltok(strom);
	if(tok == SYMBOLTOK) {
		delim = mksym(SymBuf,&sflg);
		if(delim == dosym) {
			nexttok(strom,1);
			inc = constone;
		}
		else if(delim == bysym) {
			nexttok(strom,1);
			inc = readexpr(strom,0);
			sflg = obligsym(forsym,strom,dosym);
			if(sflg == aERROR) {
				obj = parserrsym;
				goto cleanup;
			}
		}
		else
			sflg = aERROR;
	}
	else
		sflg = aERROR;
	if(sflg == aERROR) {
		errexpect(forsym,dosym,voidsym);
		obj = parserrsym;
		goto cleanup;
	}
	PARSpush(inc);
	n++;
	m = statements(strom,&delim);
	if(m == aERROR || delim != endsym) {
		errexpect(forsym,endsym,voidsym);
		obj = parserrsym;
		goto cleanup;
	}
	n += m;
	obj = mkntuple(fFOREXPR,argStkPtr-n+1,n);
  cleanup:
	argStkPtr = savptr;
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** Stellt fest, ob das zuletzt aus strom gelesene Objekt
** das Symbol symb war und gibt ggf. Fehlermeldung aus.
** Es wird weitergelesen
*/
PRIVATE int obligsym(fun,strom,symb)
truc fun;
truc *strom;
truc symb;
{
	truc obj;
	int tok, ret;
	int sflg;

	tok = skipeoltok(strom);
	if(tok != SYMBOLTOK) {
/* genauere Fehlerbehandlung wuenschenswert */
		ret = aERROR;
	}
	else {
		sflg = lookupsym(SymBuf,&obj);
		ret = (obj == symb ? sflg : aERROR);
	}
	if(ret == aERROR) {
		errexpect(fun,symb,voidsym);
	}
	nexttok(strom,0);
	return(ret);
}
/*--------------------------------------------------------------*/
PRIVATE void errexpect(fun,symb,esym)
truc fun, symb, esym;
{
	char buf[80];

	strcopy(buf + strcopy(buf,SYMname(symb))," expected");
	rerror(fun,buf,esym);
}
/*--------------------------------------------------------------*/
/*
** Liest eine Folge von Expressions, die durch Semicolons
** getrennt sind und durch ein Trenn-Symbol beendet wird,
** das in *endptr abgelegt wird.
** Die Expressions werden auf den PARSstack gelegt.
** Rueckgabewert ist bei Erfolg die Anzahl der Expressions, 
** der PARSstack wird nicht aufgeraeumt.
** Falls ein Fehler auftritt, wird aERROR zurueckgegeben und der
** PARSstack aufgeraeumt
*/
PRIVATE int statements(strom,endptr)
truc *strom;
truc *endptr;
{
	truc obj;
	int tok;
	int n = 0;

	while(1) {
		tok = skipeoltok(strom);
		if(tok == SEMICOLTOK) {
			nexttok(strom,1);
			continue;
		}
		else if(tok == SYMBOLTOK) {
			if(lookupsym(SymBuf,endptr) == sDELIM) {
				nexttok(strom,0);
				break;
			}
		}
		obj = readexpr(strom,0);
		if(obj == parserrsym || obj == breaksym) {
			PARSnpop(n);
			return(aERROR);
		}
        else if(obj == enddotsym) {
            break;
        }
		else {
			PARSpush(obj);
			n++;
		}
	}
	return(n);
}
/*--------------------------------------------------------------*/
PRIVATE truc readif(strom)
truc *strom;
{
	truc *savptr;
	truc obj, delim;
	int sflg;
	int m, n = 0;

	savptr = argStkPtr;
	nexttok(strom,0);	/* ueberspringe if */
	while(1) {
		obj = readexpr(strom,0);
		if(obj == parserrsym) {
			goto cleanup;
		}
		PARSpush(obj);
		n++;
		sflg = obligsym(ifsym,strom,thensym);
		if(sflg == aERROR) {
			obj = parserrsym;
			goto cleanup;
		}
		m = statements(strom,&delim);
		if(m == aERROR) {
			errexpect(ifsym,endsym,voidsym);
			obj = parserrsym;
			goto cleanup;
		}
		obj = mkcompnode(fCOMPEXPR,m);
		PARSnpop(m);
		PARSpush(obj);
		n++;
		if(delim != elsifsym)
			break;
	}
	if(delim == elsesym) {
		m = statements(strom,&delim);
		if(m == aERROR || delim != endsym) {
			errexpect(ifsym,endsym,voidsym);
			obj = parserrsym;
			goto cleanup;
		}
		obj = mkcompnode(fCOMPEXPR,m);
		PARSnpop(m);
		PARSpush(obj);
		n++;
	}
	else if(delim == endsym) {
		PARSpush(voidsym);
		n++;
	}
	else {
		rerror(ifsym,err_synt,delim);
		obj = parserrsym;
		goto cleanup;
	}
	obj = mkntuple(fIFEXPR,argStkPtr-n+1,n);
  cleanup:
	argStkPtr = savptr;
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE truc negate(obj)
truc obj;
{
	PARSpush(zero);
	PARSpush(obj);
	obj = optbnode(uminsym);
	PARSnpop(2);
	return(obj);
}
/*--------------------------------------------------------------*/
PRIVATE void recoverr(strom)
truc *strom;
{
	int tok;
				/* vorlaeufig */
	tok = curtok(strom);
	while(tok != EOLTOK)
		tok = nexttok(strom,0);
	if(*strom == tstdin)
		dumpinput();
}
/*--------------------------------------------------------------*/
PRIVATE truc tmptoksym(tok)
int tok;
{
    char *name;
    switch(tok) {
    case EOFTOK:
        name = "end of input";
        break;
    case EOLTOK:
        name = "end-of-line token";
        break;
    case LPARENTOK:
        name = "left parenthesis (";
        break;
    case RPARENTOK:
        name = "right parentesis )";
        break;
    case LBRACKTOK:
        name = "left bracket [";
        break;
    case RBRACKTOK:
        name = "right bracket ]";
        break;
    case LBRACETOK:
        name = "left brace {";
        break;
    case RBRACETOK:
        name = "right brace }";
        break;
    case BEGCOMMTOK:
        name = "begin comment token";
        break;
    case ENDCOMMTOK:
        name = "end comment token";
        break;
    case COMMATOK:
        name = "comma token";
        break;
    case COLONTOK:
        name = "colon token ':'";
        break;
    case SEMICOLTOK:
        name = "semi colon token";
        break;
    case DOTTOK:
        name = "dot token";
        break;
    case DOTDOTTOK:
        name = "dot dot token '..'";
        break;
    case RECDOTTOK:
        name = "record field separator";
        break;
    case DEREFTOK:
        name = "dereferencing token '^'";
        break;
    case DOLLARTOK:
        name = "dollar token";
        break;
    case HISTORYTOK:
        name = "history token '!'";
        break;
    case QUESTIONTOK:
        name = "question mark";
        break;
    case ASSIGNTOK:
        name = "assignment token :=";
        break;
    case ORTOK:
        name = "or token";
        break;
    case ANDTOK:
        name = "and token";
        break;
    case NOTTOK:
        name = "not token";
        break;
    case EQTOK:
        name = "equal token";
        break;
    case NETOK:
        name = "not equal token";
        break;
    case LTTOK:
        name = "less than token";
        break;
    case LETOK:
        name = "token <=";
        break;
    case GTTOK:
        name = "greater than token";
        break;
    case GETOK:
        name = "token >=";
        break;
    case PLUSTOK:
        name = "plus token";
        break;
    case MINUSTOK:
        name = "minus token";
        break;
    case TIMESTOK:
        name = "times token '*'";
        break;
    case DIVIDETOK:
        name = "divide token '/'";
        break;
    case DIVTOK:
        name = "div token";
        break;
    case MODTOK:
        name = "mod token";
        break;
    case UMINUSTOK:
        name = "unitary minus token";
        break;
    case POWERTOK:
        name = "power token **";
        break;
    case BOOLTOK:
        name = "boolean value";
        break;
    case CHARTOK:
        name = "character token";
        break;
    case INUMTOK:
        name = "integer";
        break;
    case GF2NTOK:
        name = "gf2nint";
        break;
    case FLOATTOK:
        name = "floating point number";
        break;
    case STRINGTOK:
        name = "string token";
        break;
    case BSTRINGTOK:
        name = "byte_string token";
        break;
    case SYMBOLTOK:
        name = "symbol";
        break;
    case VECTORTOK:
        name = "array";
        break;
    default:
        name = "unidentified token";
    }
    return(scratch(name));
}
/********************************************************************/
