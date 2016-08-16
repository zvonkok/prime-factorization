/****************************************************************/
/* file storage.c

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

/*
** storage.c
** storage of symbols
**
** date of last change
** 1995-04-08: fixed bug with hugefloat
** 1997-04-13: newreflsym(), neworg (newintsym)
** 2000-12-28: removed some debugging code
** 2002-04-19: mkgf2n, mk0gf2n
** 2004-11-29: changed mkcopy, mkcopy0
*/

#include "common.h"

PUBLIC void inistore	_((void));

PUBLIC truc *nextsymptr	 _((int i));
PUBLIC truc symbobj	_((truc *ptr));
PUBLIC int  lookupsym	_((char *name, truc *pobj));
PUBLIC truc mksym	_((char *name, int *sflgptr));
PUBLIC truc scratch	_((char *name));
PUBLIC truc newselfsym	_((char *name, int flg));
PUBLIC truc newreflsym	_((char *name, int flg));
PUBLIC truc newintsym	_((char *name, int flg, wtruc bind));
PUBLIC int  tokenvalue	_((truc op));
PUBLIC truc newsym	_((char *name, int flg, truc bind));
PUBLIC truc newsymsig	_((char *name, int flg, wtruc bind, int sig));
PUBLIC truc new0symsig	_((char *name, int flg, wtruc bind, int sig));
PUBLIC truc mkcopy	_((truc *x));
PUBLIC truc mkcopy0  _((truc *x));
PUBLIC truc mkarrcopy	_((truc *x));
PUBLIC truc mkinum	_((long n));
PUBLIC truc mkarr2	_((unsigned w0, unsigned w1));
PUBLIC truc mklocsym	_((int flg, unsigned u));
PUBLIC truc mkfixnum	_((unsigned n));
PUBLIC truc mksfixnum	_((int n));
PUBLIC truc mkint	_((int sign, word2 *arr, int len));
PUBLIC truc mkgf2n  _((word2 *arr, int len));
PUBLIC truc mk0gf2n _((word2 *arr, int len));
PUBLIC truc mkfloat	_((int prec, numdata *nptr));
PUBLIC truc fltzero	_((int prec));
PUBLIC truc mk0float	_((numdata *nptr));
PUBLIC truc mkchar	_((int ch));
PUBLIC truc mkbstr	_((byte *arr, unsigned len));
PUBLIC truc mkstr	_((char *str));
PUBLIC truc mkstr0	_((unsigned len));
PUBLIC truc mkbstr0	_((unsigned len));
PUBLIC truc mknullstr	_((void));
PUBLIC truc mknullbstr	_((void));
PUBLIC truc mkvect0	_((unsigned len));
PUBLIC truc mkrecord	_((int flg, truc *ptr, unsigned len));
PUBLIC truc mkstack	_((void));
PUBLIC truc mkstream	_((FILE *file, int mode));
PUBLIC truc mk0stream	_((FILE *file, int mode));
PUBLIC truc mk0fun	_((truc op));
PUBLIC truc mkpair	_((int flg, truc sym1, truc sym2));
PUBLIC truc mkunode	_((truc op));
PUBLIC truc mkbnode	_((truc op));
PUBLIC truc mkspecnode	_((truc fun, truc *argptr, int k));
PUBLIC truc mkfunode	_((truc fun, int n));
PUBLIC truc mkfundef	_((int argc, int argoptc, int varc));
PUBLIC truc mkntuple	_((int flg, truc *arr, int n));
PUBLIC truc mkcompnode	_((int flg, int n));

/*----------------------------------------------------------------*/
PRIVATE int hash	_((char *name));
PRIVATE truc *mksymaux	_((int flg, char *name, int mode));
PRIVATE truc *findsym	_((char *name, int mode));
PRIVATE truc mkstraux	_((int flg, char *str, unsigned len, int mode));
PRIVATE truc mkstraux0	_((int flg, unsigned len, int mode));
PRIVATE void streamaux	_((FILE *file, int mode, struct stream *ptr));


PRIVATE truc scratchsym;

/* #define STORLOG */
#ifdef STORLOG
FILE *logf;
static char *Logfile = "storlog.txt";
static char logbuf[82];
#endif
/*----------------------------------------------------------------*/
PUBLIC void inistore()
{
#ifdef STORLOG
logf = fopen(Logfile,"w");
#endif
	scratchsym = newselfsym("",sINTERNAL);
}
/*----------------------------------------------------------------*/
PRIVATE int hash(name)
char *name;
{
	register unsigned long h = 0;
	register int ch;
	int i = 10;

	while((ch = *name++)) {
		h += ch;
		if(--i >= 0) h <<= 3;
	}
	return(h % hashtabSize);
}
/* -----------------------------------------------------------*/
PRIVATE truc *mksymaux(flg,name,mode)
int flg;
char *name;
int mode;	/* mode != 0: allocate new space for name */
{
	truc *ptr;
	char *str;
	size_t wo;

	wo = new0(SIZEOFSYMBOL);
	ptr = Symbol + wo;

	*FLAGPTR(ptr) = flg;
	*SEGPTR(ptr) = 0;
	*OFFSPTR(ptr) = (word2)wo;

	if(mode) {
		str = stringalloc(strlen(name) + 1);
		((struct symbol *)ptr)->name = str;
		strcopy(str,name);
	}
	else
		((struct symbol *)ptr)->name = name;

	((struct symbol *)ptr)->bind.t = zero;	/* zur Sicherheit */
	((struct symbol *)ptr)->cc.xx = 0;

	return(ptr);
}
/*-------------------------------------------------------------------*/
/*
** Zum Durchlaufen der Symboltabelle.
** Anfang mit Argument 0
** used during garbage collection
*/
PUBLIC truc *nextsymptr(i)
int i;
{
	static truc *ptr = NULL;
	static int index = -1;
	truc *ptr1;

	if(i == 0) {
		ptr = NULL;
		index = -1;
	}
	while(ptr == NULL) {
		if(++index >= hashtabSize) {
			index = -1;
			return(NULL);
		}
		ptr = Symtab[index];
	}
	ptr1 = ptr;
	ptr = ((struct symbol *)ptr)->link;
	return(ptr1);
}
/*-------------------------------------------------------------------*/
/*
** constructs symbol truc associated to symbol at place ptr
*/
PUBLIC truc symbobj(ptr)
truc *ptr;
{
	variant v;

	v.xx = *ptr;
	v.pp.b0 = fSYMBOL;
	return(v.xx);
}
/*-------------------------------------------------------------------*/
/*
** find resp. make a symbol object
*/
PRIVATE truc *findsym(name,mode)
char *name;
int mode;	/* mode == 0: use string space of name */
{
	trucptr *pptr;
	truc *ptr, *ptr1;
	int cmp;

	pptr = Symtab + hash(name);
	while((ptr = *pptr) != NULL) {
		cmp = strcmp(name,((struct symbol *)ptr)->name);
		if(cmp == 0)		/* found */
			return(ptr);
		if(cmp < 0)		/* not present */
			break;
		pptr = (trucptr *)&(((struct symbol *)ptr)->link);
	}
	*pptr = ptr1 = mksymaux(sUNBOUND,name,mode);	 /* insert */
	((struct symbol *)ptr1)->link = ptr;
	return(ptr1);
}
/*---------------------------------------------------------------------*/
PUBLIC int lookupsym(name,pobj)
char *name;
truc *pobj;
{
	truc *ptr;
	int cmp, sflg;

	ptr = Symtab[hash(name)];
	while(ptr != NULL) {
		cmp = strcmp(name,((struct symbol *)ptr)->name);
		if(cmp == 0) {		/* found */
			sflg = *FLAGPTR(ptr);
			*pobj = symbobj(ptr);
			return(sflg);
		}
		if(cmp < 0)		/* not present */
			break;
		ptr = ((struct symbol *)ptr)->link;
	}
	return(aERROR);			 /* not found */
}
/*---------------------------------------------------------------------*/
PUBLIC truc mksym(name,sflgptr)
char *name;
int *sflgptr;
{
	variant v;

	v.xx = *findsym(name,1);
	*sflgptr = v.pp.b0;
	v.pp.b0 = fSYMBOL;

	return(v.xx);
}
/*---------------------------------------------------------------------*/
/*
** make a temporary symbol with given name
*/
PUBLIC truc scratch(name)
char *name;
{
	SYMname(scratchsym) = name;
	return(scratchsym);
}
/*----------------------------------------------------------------------*/
/*
** make internal symbol, not in hash table,
** bound to itself
*/
PUBLIC truc newselfsym(name,flg)
char *name;
int flg;
{
	truc obj;

	obj = newintsym(name,flg,(wtruc)0);
	SYMbind(obj) = obj;

	return(obj);
}
/*----------------------------------------------------------------------*/
/*
** make internal symbol, not in hash table,
** with given name, flag, binding
*/
PUBLIC truc newintsym(name,flg,bind)
char *name;
int flg;
wtruc bind;
{
	truc *ptr;
	variant v;
	size_t wo;

	wo = new0(SIZEOFINTSYMBOL);
	ptr = Symbol + wo;

	*FLAGPTR(ptr) = flg;
	*SEGPTR(ptr) = 0;
	*OFFSPTR(ptr) = (word2)wo;

	((struct intsymbol *)ptr)->name = name;
	((struct intsymbol *)ptr)->bind.w = bind;

	v.xx = *ptr;
	v.pp.b0 = fSYMBOL;

	return(v.xx);
}
/*----------------------------------------------------------------------*/
/*
** make internal symbol, not in hash table,
** with given name, flag, binding and signature
*/
PUBLIC truc new0symsig(name,flg,bind,sig)
char *name;
int flg;
wtruc bind;
int sig;
{
	truc *ptr;
	variant v;

	ptr = mksymaux(flg,name,0);
	v.xx = *ptr;
	v.pp.b0 = fSYMBOL;
	((struct symbol *)ptr)-> bind.w = bind;
	((struct symbol *)ptr)->cc.yy.ww = sig;

	return(v.xx);
}
/*---------------------------------------------------------------------*/
/*
** returns token associated to symbols representing infix operators
*/
PUBLIC int tokenvalue(op)
truc op;
{
	variant v;

	v.xx = SYMcc(op);
	return(v.pp.ww);
}
/*---------------------------------------------------------------------*/
/*
** make a new symbol object (in hash table)
** with given name, flag and binding
*/
PUBLIC truc newsym(name,flg,bind)
char *name;
int flg;
truc bind;
{
	variant v;
	truc *ptr;

	ptr = findsym(name,0);
	*FLAGPTR(ptr) = flg;
	((struct symbol *)ptr)->bind.t = bind;
	v.xx = *ptr;
	v.pp.b0 = fSYMBOL;

	return(v.xx);
}
/*---------------------------------------------------------------------*/
/*
** make a new symbol object (in hash table)
** with given name and flag, bound to itself
*/
PUBLIC truc newreflsym(name,flg)
char *name;
int flg;
{
	variant v;
	truc *ptr;

	ptr = findsym(name,0);
	*FLAGPTR(ptr) = flg;
	v.xx = *ptr;
	v.pp.b0 = fSYMBOL;
	((struct symbol *)ptr)->bind.t = v.xx;

	return(v.xx);
}
/*---------------------------------------------------------------------*/
/*
** make a new symbol object with given name, flag, binding and signature
*/
PUBLIC truc newsymsig(name,flg,bind,sig)
char *name;
int flg;
wtruc bind;
int sig;
{
	variant v;
	truc *ptr;

	ptr = findsym(name,0);
	*FLAGPTR(ptr) = flg;
	((struct symbol *)ptr)->bind.w = bind;
	((struct symbol *)ptr)->cc.yy.ww = sig;
	v.xx = *ptr;
	v.pp.b0 = fSYMBOL;

	return(v.xx);
}
/*--------------------------------------------------------*/
/*
** Stellt Kopie eines Objekts *x her (top-level)
** Es wird vorausgesetzt, dass x bei gc geschuetzt ist
*/
PUBLIC truc mkcopy0(x)
truc *x;
{
	truc *ptr;
	truc obj;
	unsigned int len;
	int flg = *FLAGPTR(x);

	if((flg & FIXMASK) || (!*SEGPTR(x)))
		return(*x);
	len = obj4size(flg,TAddress(x));
	obj = newobj(flg,len,&ptr);
	cpy4arr(TAddress(x),len,ptr);
	return(obj);
}
/*--------------------------------------------------------*/
/*
** Stellt Kopie eines Objekts *x her (top-level)
** Es wird vorausgesetzt, dass x bei gc geschuetzt ist
*/
PUBLIC truc mkcopy(x)
truc *x;
{
	truc *ptr;
	truc obj;
	unsigned int len;
	int flg = *FLAGPTR(x);

	if((flg & FIXMASK))
		return(*x);
	len = obj4size(flg,TAddress(x));
#ifdef STORLOG
fprintf(logf,"mknumcopy: flg = %d; obj4size = %d\n",flg,len);
#endif
	obj = newobj(flg,len,&ptr);
	cpy4arr(TAddress(x),len,ptr);
	return(obj);
}
/*--------------------------------------------------------*/
/*
** Stellt Kopie eines Objekts *x her (Array beliebigen Ranges oder record)
** Es wird vorausgesetzt, dass x bei gc geschuetzt ist
*/
PUBLIC truc mkarrcopy(x)
truc *x;
{
	truc *ptr;
	truc obj;
	unsigned int i, len;
	int flg = *FLAGPTR(x);

	if(flg & FIXMASK)
		return(*x);
	len = obj4size(flg,TAddress(x));
	obj = newobj(flg,len,&ptr);
	cpy4arr(TAddress(x),len,ptr);
	if(flg < fRECORD || flg > fVECTOR || len < 2)
		return(obj);
	/* now recursively copy components of vector or record */
	WORKpush(obj);
	ARGpush(zero);
	ptr++;
	for(i=1; i<len; i++) {
		flg = *FLAGPTR(ptr);
		if(flg >= fRECORD && flg <= fVECTLIKE1) {
			*argStkPtr = *ptr;
			obj = mkarrcopy(argStkPtr);
			ptr = TAddress(workStkPtr)+i;
			*ptr++ = obj;
		}
		else
			ptr++;
	}
	ARGpop();
	return(WORKretr());
}
/*--------------------------------------------------------*/
/*
** make an intobj from long
*/
PUBLIC truc mkinum(n)
long n;
{
	struct bigcell *ptr;
	truc obj;
	variant v;
	int sign;

	sign = (n < 0 ? MINUSBYTE : 0);
	if(sign)
		n = -n;
	if(n < 0x10000) {
		v.pp.b0 = fFIXNUM;
		v.pp.b1 = sign;
		v.pp.ww = n;
		return(v.xx);
	}
	else {
		obj = newobj(fBIGNUM,SIZEOFBIG(2),(trucptr *)&ptr);
		ptr->flag = fBIGNUM;
		ptr->signum = sign;
		ptr->len = 2;
		ptr->digi0 = n & 0xFFFF;
		ptr->digi1 = n >> 16;
		return(obj);
	}
}
/*--------------------------------------------------------------*/
PUBLIC truc mkarr2(w0,w1)
unsigned w0, w1;
{
	variant v;

	v.yy.w0 = w0;
	v.yy.ww = w1;
	return(v.xx);
}
/*--------------------------------------------------------------*/
PUBLIC truc mklocsym(flg,u)
int flg;
unsigned u;
{
	variant v;

	v.pp.b0 = flg;
	v.pp.b1 = 0;
	v.pp.ww = u;
	return(v.xx);
}
/*--------------------------------------------------------------*/
/*
** stellt aus der Zahl n >= 0 ein fixnum her
*/
PUBLIC truc mkfixnum(n)
unsigned n;
{
	variant v;

	v.pp.b0 = fFIXNUM;
	v.pp.b1 = 0;
	v.pp.ww = n;
	return(v.xx);
}
/*--------------------------------------------------------------*/
/*
** stellt aus der Zahl n ein fixnum her;
** Vorzeichen von n wird beruecksichtigt
*/
PUBLIC truc mksfixnum(n)
int n;
{
	variant v;

	v.pp.b0 = fFIXNUM;
	if(n < 0) {
		v.pp.b1 = MINUSBYTE;
		n = -n;
	}
	else
		v.pp.b1 = 0;
	v.pp.ww = n;
	return(v.xx);
}
/*--------------------------------------------------------------*/
/*
** make intobj from big-array
*/
PUBLIC truc mkint(sign,arr,len)
int sign;
word2 *arr;
int len;
{
	struct bigcell *big;
	variant v;
	truc  obj;

	if(len <= 1) {
		v.pp.b0 = fFIXNUM;
		if(!len) {
			v.pp.b1 = 0;
			v.pp.ww = 0;
		}
		else {
			v.pp.b1 = sign;
			v.pp.ww = *arr;
		}
		return(v.xx);
	}
	/* else if(len >= 2) */
	obj = newobj(fBIGNUM,SIZEOFBIG(len),(trucptr *)&big);
	big->flag = fBIGNUM;
	big->signum = sign;
	big->len = len;
	cpyarr(arr,len,&(big->digi0));
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** make gf2nint from (arr,len)
*/
PUBLIC truc mkgf2n(arr,len)
word2 *arr;
int len;
{
	struct bigcell *big;
	truc  obj;

	if(len <= 1) {
        if (!len)
            return gf2nzero;
        else if (arr[0] == 1)
            return gf2none;
	/* else fall through */
    }
	obj = newobj(fGF2NINT,SIZEOFBIG(len),(trucptr *)&big);
	big->flag = fGF2NINT;
	big->signum = 0;
	big->len = len;
	cpyarr(arr,len,&(big->digi0));
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** make gf2nint which is not moved during garbage collection
*/
PUBLIC truc mk0gf2n(arr,len)
word2 *arr;
int len;
{
	struct bigcell *big;
	truc obj;

	obj = new0obj(fGF2NINT,SIZEOFBIG(len),(trucptr *)&big);

	big->flag = fGF2NINT;
	big->signum = 0;
	big->len = len;
	cpyarr(arr,len,&(big->digi0));

	return(obj);
}
/*--------------------------------------------------------------*/
PUBLIC truc mkfloat(prec,nptr)
int prec;		/* must be one of FltPrec[k] */
numdata *nptr;
{
	struct floatcell *fl;
	truc obj;
	long ex;
	unsigned hugelow;
	int hugeflg = 0;
	int n, flg, pcode;

	n = normfloat(prec,nptr);
	if(n == 0)
		return(fltzero(prec));
	ex = nptr->expo;
	pcode = fltpreccode(prec);
	flg = fFLTOBJ + (pcode<<1);
	if(ex >= 0x8000 || -ex > 0x8000) {
		hugeflg = 1;
		flg |= HUGEFLTBIT;
		hugelow = ex & 0x7F;
		ex >>= 7;
	}
	obj = newobj(flg,SIZEOFFLOAT(prec),(trucptr *)&fl);
	fl->flag = flg;
	fl->signum = (nptr->sign ? FSIGNBIT : 0);
	if(hugeflg)
		fl->signum |= hugelow;
	fl->expo = ex;
	cpyarr(nptr->digits,prec,&(fl->digi0));
	return(obj);
}
/*--------------------------------------------------------------*/
PUBLIC truc fltzero(prec)
int prec;	/* must be one of FltPrec[k] */
{
	variant v;
	int pcode;

	pcode = fltpreccode(prec);
	v.pp.b0 = fFLTOBJ + (pcode<<1) + FLTZEROBIT;
	v.pp.b1 = 0;
	v.pp.ww = 0;
	return(v.xx);
}
/*--------------------------------------------------------------*/
/*
** make a float which is not moved during garbage collection
*/
PUBLIC truc mk0float(nptr)
numdata *nptr;
/* nptr wird als normalisiert und nicht huge vorausgesetzt */
{
	struct floatcell *fl;
	truc obj;
	int prec, flg, pcode;

	prec = nptr->len;
	pcode = fltpreccode(prec);
	flg = fFLTOBJ + (pcode<<1);
	obj = new0obj(flg,SIZEOFFLOAT(prec),(trucptr *)&fl);
	fl->flag = flg;
	fl->signum = (nptr->sign ? FSIGNBIT : 0);
	fl->expo = nptr->expo;

	cpyarr(nptr->digits,prec,&(fl->digi0));
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** make a character object
*/
PUBLIC truc mkchar(n)
int n;
{
	variant v;

	v.pp.b0 = fCHARACTER;
	v.pp.b1 = 0;
	v.pp.ww = (n & 0x00FF);
	return(v.xx);
}
/*--------------------------------------------------------------*/
/*
** make a byte_string object for byte array (arr,len)
*/
PUBLIC truc mkbstr(arr,len)
byte *arr;
unsigned int len;
{
	return(mkstraux(fBYTESTRING,(char *)arr,len,1));
}
/*--------------------------------------------------------------*/
/*
** make a string object for string str
*/
PUBLIC truc mkstr(str)
char *str;
{
	unsigned len = strlen(str);

	return(mkstraux(fSTRING,str,len,1));
}
/*---------------------------------------------------------*/
/*
** make a string object for unknown string of length len
*/
PUBLIC truc mkstr0(len)
unsigned len;
{
	return(mkstraux0(fSTRING,len,1));
}
/*---------------------------------------------------------*/
/*
** make a bytestring object for unknown string of length len
*/
PUBLIC truc mkbstr0(len)
unsigned len;
{
	return(mkstraux0(fBYTESTRING,len,1));
}
/*---------------------------------------------------------*/
/*
** make a nullstring, not moved during gc
*/
PUBLIC truc mknullstr()
{
	return(mkstraux0(fSTRING,0,0));
}
/*---------------------------------------------------------*/
/*
** make a nullbytestring, not moved during gc
*/
PUBLIC truc mknullbstr()
{
	return(mkstraux0(fBYTESTRING,0,0));
}
/*---------------------------------------------------------*/
PRIVATE truc mkstraux(flg,str,len,mode)
int flg;	/* fSTRING or fBYTESTRING */
char *str;
unsigned len;
int mode;	/* mode = 0: string not moved during gc */
{
	unsigned k;
	struct strcell *ptr;
	truc  obj;
	char *cpt;

	if(mode)
		obj = newobj(flg,SIZEOFSTRING(len),(trucptr *)&ptr);
	else
		obj = new0obj(flg,SIZEOFSTRING(len),(trucptr *)&ptr);

	ptr->flag = fSTRING;
	ptr->flg2 = 0;
	ptr->len = len;
	cpt = (char *)&(ptr->ch0);
	for(k=0; k<len; k++)
		*cpt++ = *str++;
	*cpt = 0;
	return(obj);
}
/*---------------------------------------------------------*/
PRIVATE truc mkstraux0(flg,len,mode)
int flg;	/* fSTRING or fBYTESTRING */
unsigned len;
int mode;	/* mode = 0: string not moved during gc */
{
	unsigned k;
	struct strcell *ptr;
	truc  obj;
	char *cpt;

	if(mode)
		obj = newobj(flg,SIZEOFSTRING(len),(trucptr *)&ptr);
	else
		obj = new0obj(flg,SIZEOFSTRING(len),(trucptr *)&ptr);

	ptr->flag = fSTRING;
	ptr->flg2 = 0;
	ptr->len = len;
	cpt = (char *)&(ptr->ch0);
	for(k=0; k<=len; k++)
		*cpt++ = 0;
	return(obj);
}
/*---------------------------------------------------------*/
/*
** make a vector object for vector of length len
** initialized with zeroes
*/
PUBLIC truc mkvect0(len)
unsigned int len;
{
	struct vector *ptr;
	truc *vec;
	truc obj;
	unsigned int k;

	k = SIZEOFVECTOR(len);	/* k is positive */
	obj = newobj(fVECTOR,k,(trucptr *)&ptr);

	ptr->flag = fVECTOR;
	ptr->flg2 = 0;
	ptr->len = len;
	vec = (truc *)&(ptr->ele0);
	while(--k)
		*vec++ = zero;
	return(obj);
}
/*-------------------------------------------------------------*/
PUBLIC truc mkrecord(flg,ptr,len)
int flg;	/* fRECORD or fPOINTER */
truc *ptr;
unsigned len;
{
	struct record *rptr;
	truc obj;
	unsigned k;

	k = SIZEOFRECORD(len);
	obj = newobj(flg,k,(trucptr *)&rptr);
	rptr->flag = flg;
	rptr->flg2 = 0;
	rptr->len = len;
	cpy4arr(ptr,len+1,(truc *)&(rptr->recdef));
	return(obj);
}
/*-------------------------------------------------------------*/
PUBLIC truc mkstack()
{
	struct stack *ptr;
	truc obj;

	obj = newobj(fSTACK,SIZEOFSTACK,(trucptr *)&ptr);
	ptr->flag = fSTACK;
	ptr->line = 0;
	ptr->pageno = 0;
	ptr->type = zero;
	ptr->page = nullsym;
	return(obj);
}
/*-------------------------------------------------------------*/
PUBLIC truc mkstream(file,mode)
FILE *file;
int mode;
{
	struct stream *ptr;
	truc strm = newobj(fSTREAM,SIZEOFSTREAM,(trucptr *)&ptr);

	streamaux(file,mode,ptr);
	return(strm);
}
/* ---------------------------------------------------------- */
PRIVATE void streamaux(file,mode,ptr)
FILE *file;
int mode;
struct stream *ptr;
{
	ptr->flag   = fSTREAM;
	ptr->mode   = mode;
	ptr->pos    = 0;
	ptr->lineno = 1;
	ptr->ch	    = EOL;
	ptr->tok    = EOLTOK;
	ptr->file   = file;
}
/* ---------------------------------------------------------- */
/*
** make a stream which is not moved during garbage collection
*/
PUBLIC truc mk0stream(file,mode)
FILE *file;
int mode;
{
	struct stream *ptr;
	truc strm = new0obj(fSTREAM,SIZEOFSTREAM,(trucptr *)&ptr);

	streamaux(file,mode,ptr);
	return(strm);
}
/*--------------------------------------------------------------*/
/*
** make function object without arguments
*/
PUBLIC truc mk0fun(op)
truc op;
{
	variant v;
	int sflg;

	sflg = Symflag(op);
	if(sflg == sFBINARY || sflg == sSBINARY) {
		v.xx = op;
		v.pp.b0 = fSPECIAL0;
		return(v.xx);
	}
	else
		return(mkfunode(op,0));
}
/*--------------------------------------------------------------*/
PUBLIC truc mkpair(flg,sym1,sym2)
int flg;
truc sym1, sym2;
{
	struct opnode *node;
	truc obj;

	obj = newobj(flg,SIZEOFOPNODE(1),(trucptr *)&node);
	node->op = sym1;
	node->arg0 = sym2;
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** make unary opnode with arg from ParseStack
*/
PUBLIC truc mkunode(op)
truc op;
{
	struct opnode *node;
	truc obj;
	int flg, sflg;

	sflg = Symflag(op);
	if(sflg == sSBINARY)
		flg = fSPECIAL1;
	else if(sflg == sFBINARY)
		flg = fBUILTIN1;
	else
		return(mkfunode(op,1));

	obj = newobj(flg,SIZEOFOPNODE(1),(trucptr *)&node);
	node->op = op;
	node->arg0 = *argStkPtr;
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** make binary opnode with arg0 and arg1 from ParseStack
*/
PUBLIC truc mkbnode(op)
truc op;
{
	struct opnode *node;
	truc obj;
	int flg, sflg;

	sflg = Symflag(op);

	if(sflg == sSBINARY)
		flg = fSPECIAL2;
	else if(sflg == sFBINARY)
		flg = fBUILTIN2;
	else
		return(mkfunode(op,2));

	obj = newobj(flg,SIZEOFOPNODE(2),(trucptr *)&node);
	node->op = op;
	node->arg0 = argStkPtr[-1];
	node->arg1 = argStkPtr[0];
	return(obj);
}
/*--------------------------------------------------------------*/
PUBLIC truc mkspecnode(fun,argptr,k)
truc fun;
truc *argptr;
int k;		/* k == 1 or k == 2 */
{
	struct opnode *node;
	int flg;
	truc obj;

	flg = (k == 1 ? fSPECIAL1 : fSPECIAL2);
	obj = newobj(flg,SIZEOFOPNODE(k),(trucptr *)&node);
	node->op = fun;
	node->arg0 = argptr[0];
	if(k == 2)
		node->arg1 = argptr[1];
	return(obj);
}
/*--------------------------------------------------------------*/
PUBLIC truc mkfunode(fun,n)
truc fun;
int n;
{
	struct funode *node;
	truc obj;
	truc *ptr;
	int flg, sflg;

	sflg = Symflag(fun);
	if(sflg == sFBINARY) {
		flg = fBUILTINn;
	}
	else if(sflg == sSBINARY) {
		flg = fSPECIALn;
	}
	else
		flg = fFUNCALL;

	obj = newobj(flg,SIZEOFFUNODE(n),(trucptr *)&node);
	node->op = fun;
	node->argno = mkfixnum(n);
	ptr = (truc *)&(node->arg1);
	while(--n >= 0) {
		*ptr++ = argStkPtr[-n];
	}
	return(obj);
}
/*--------------------------------------------------------------*/
PUBLIC truc mkfundef(argc,argoptc,varc)
int argc, argoptc, varc;
{
	struct fundef *node;
	truc obj;

	obj = newobj(fFUNDEF,SIZEOFFUNDEF,(trucptr *)&node);

	node->flag = fFUNDEF;
	node->flg2 = argoptc;
	node->argc = argc;
	node->varno = mkfixnum(varc);
	node->body = argStkPtr[0];
	node->parms = argStkPtr[-2];
	node->vars = argStkPtr[-1];

	return(obj);
}
/*--------------------------------------------------------------*/
/*
** make node with n expressions from array arr
*/
PUBLIC truc mkntuple(flg,arr,n)
int flg, n;
truc *arr;
{
	truc *node;
	truc obj;
	variant v;

	obj = newobj(flg,SIZEOFTUPLE(n),&node);

	v.pp.b0 = flg;
	v.pp.b1 = 0;
	v.pp.ww = n;
	*node++ = v.xx;
	cpy4arr(arr,n,node);
	return(obj);
}
/*--------------------------------------------------------------*/
/*
** make node with n statements from ParseStack in reverse order
*/
PUBLIC truc mkcompnode(flg,n)
int flg, n;
{
	truc *node;
	truc obj;
	variant v;
	int i;

	obj = newobj(flg,SIZEOFCOMP(n),&node);

	v.pp.b0 = flg;
	v.pp.b1 = 0;
	v.pp.ww = n;
	*node++ = v.xx;
	for(i=0; i<n; i++)
		*node++ = argStkPtr[-i];
	return(obj);
}
/*********************************************************************/
