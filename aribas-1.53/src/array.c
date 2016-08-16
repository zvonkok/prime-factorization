/****************************************************************/
/* file array.c

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
** array.c
** array, string, byte_string, record and stack procedures
**
** date of last change
** 1995-03-05:	Findex changed
** 1995-03-20:	ARGV sSYSTEMVAR
** 1995-04-03:	records, pointer
** 1995-04-07:	alloc
** 1996-10-03:	max_arraysize
** 1997-01-29:	Fbstring
** 1997-02-11:	ARIBUFSIZE -> aribufSize in Fstrsplit
** 1997-03-25:	Fstrsplit now PRIVATE
** 1997-03-29:	changed function bytestraddr
** 1997-04-13:	reorg (newintsym)
** 1997-08-01:	removed create_array
** 2000-01-02:  fixed bug in byteswap()
** 2001-06-17:  corrected type in error message
** 2002-03-08:  changed direction of sort with user defined sort fcn
** 2002-03-28:  functions string_scan, stack2string, stack_arraypush, realloc
** 2002-04-14:  function binsearch
** 2002-08-05:  small change in GmemBB
** 2003-02-11:  mkcopy in Fbstring and Ftextstring
** 2003-06-06:  mkcopy in Srecfield, Farrele and Fstkpush
*/

#include "common.h"

#define QUICKSORT

PUBLIC truc arraysym, stringsym, charsym, stacksym;
PUBLIC truc vectorsym, pairsym;
PUBLIC truc mkarrsym, mkstrsym, mkbstrsym;
PUBLIC truc nullstring, nullbstring;
PUBLIC truc bstringsym, bstr_sym, str_sym;
PUBLIC truc ofsym;
PUBLIC truc arr_sym, subarrsym;
PUBLIC truc recordsym, mkrecsym, rec_sym, pointrsym, derefsym;

/**** Prototypen exportierter Funktionen ****/
PUBLIC void iniarray	_((void));
PUBLIC void iniargv	    _((int argc, char *argv[]));
PUBLIC int stringsplit	_((char *str, char *trenn, word2 *offsets));
PUBLIC int indrange	    _((truc *ptr, long len, long *pn0, long *pn1));
PUBLIC truc arrassign	_((truc *arr, truc obj));
PUBLIC truc subarrassign  _((truc *arr, truc obj));
PUBLIC void sortarr	    _((truc *arr, unsigned len, ifuntt cmpfun));
PUBLIC int bytestraddr	_((truc *ptr, truc **ppbstr, byte **ppch,
						unsigned *plen));
PUBLIC truc fullrecassign  _((truc *rptr, truc obj));
PUBLIC truc recfassign	_((truc *rptr, truc field, truc obj));
PUBLIC truc Pdispose	_((truc *ptr));

/******* Modulglobale Variablen **********/
PRIVATE truc argvsym;

PRIVATE truc lengthsym, maxarrsym;
PRIVATE truc concatsym, splitsym, toupsym, tolowsym, indexsym, sscansym;
PRIVATE truc pushsym, popsym, topsym, resetsym, emptysym;
PRIVATE truc stk2arrsym, stk2strsym, arrpushsym;
PRIVATE truc chrsym, ordsym;
PRIVATE truc sortsym, bsearchsym;
PRIVATE truc mbtestsym, mbsetsym, mbclrsym, mshiftsym;
PRIVATE truc mandsym, morsym, mxorsym, mnotsym, mbitswsym, mbyteswsym;
PRIVATE truc mknewsym, allocsym, reallocsym;


/******* Prototypen modul-interner Funktionen ******/
PRIVATE truc Flength	_((void));
PRIVATE truc Fchr	    _((void));
PRIVATE truc Ford	    _((void));
PRIVATE truc Fvector	_((int argn));
PRIVATE truc Fpair	    _((void));
PRIVATE truc Fmkarray	_((int argn));
PRIVATE truc Falloc	    _((int argn));
PRIVATE truc Srealloc   _((void));
PRIVATE truc Fmkstring	_((void));
PRIVATE truc mkstring	_((truc *ptr, int flg, int ch));
PRIVATE truc Fmkbstring _((void));
PRIVATE truc Fconcat	_((int argn));
PRIVATE truc Findex	    _((void));
PRIVATE long substrindex  _((byte *str, size_t len, byte *sub, size_t len1));
PRIVATE truc Fstrsplit	_((int argn));
PRIVATE truc Fstrscan   _((int argn));
PRIVATE truc Ftolower	_((void));
PRIVATE truc Ftoupper	_((void));
PRIVATE truc Gchangecase  _((truc symb));
PRIVATE truc Farrele	_((void));
PRIVATE truc Fsubarr	_((void));
PRIVATE int arrindex	_((truc *ptr, long *pindex));
PRIVATE char *stringele	 _((truc *ptr, long index));
PRIVATE truc *vectele	_((truc *ptr, long index));
PRIVATE int arrcompat	_((int flg1, int flg2));
PRIVATE long stacklength  _((truc *ptr));
PRIVATE truc Fstkpush	_((void));
PRIVATE truc Fstkarrpush  _((int argn));
PRIVATE truc Fstkpop	_((void));
PRIVATE truc Fstktop	_((void));
PRIVATE truc Gstkretr	_((truc symb));
PRIVATE truc Fstkreset	_((void));
PRIVATE truc Fstkempty	_((void));
PRIVATE truc Fstk2array	 _((void));
PRIVATE truc Fstk2string _((void));
PRIVATE truc Fmaxarray	 _((void));
PRIVATE int compfun	_((truc *ptr1, truc *ptr2));
PRIVATE int ucompfun	_((truc *ptr1, truc *ptr2));
PRIVATE truc Ssort	_((void));
PRIVATE truc Hsort1	_((truc *argptr));
PRIVATE truc Hsort2	_((truc *argptr, truc fun));
PRIVATE truc Fbsearch   _((int argn));
PRIVATE truc Hbsearch1  _((truc *ele, truc *vptr, int flg));
PRIVATE int vectaddr	_((truc *ptr, truc **ppvec, truc **parr,
					unsigned *plen));
PRIVATE truc Ftextstring  _((void));
PRIVATE truc Fbstring	_((int argn));
PRIVATE truc Smembtest	_((void));
PRIVATE truc Smembset	_((void));
PRIVATE truc Smembclear _((void));
PRIVATE truc Smemshift	_((void));
PRIVATE truc Smemand	_((void));
PRIVATE truc Smemor	_((void));
PRIVATE truc Smemxor	_((void));
PRIVATE truc Smemnot	_((void));
PRIVATE truc Smembitsw	_((void));
PRIVATE truc Smembytesw _((void));
PRIVATE truc GmemBi	_((truc symb));
PRIVATE truc GmemBB	_((truc symb));
PRIVATE void byteshift	_((byte *ptr, unsigned len, long sh));
PRIVATE void byteswap	_((byte *ptr, unsigned len, unsigned grp));

PRIVATE int Paddr	_((truc *ptr, trucptr *pvptr));
PRIVATE truc Fmkrec0	_((void));
PRIVATE truc Srecfield	_((void));
PRIVATE truc Sderef	_((void));
PRIVATE truc Snew	_((void));
PRIVATE truc *Ltrucf	_((int flg, truc *pptr));
PRIVATE truc *recfield	_((truc *rptr, truc field));
PRIVATE truc pnew10	_((truc *point, int mode));

PRIVATE byte BitSwap[256] = {
0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

/*--------------------------------------------------------------------*/
PUBLIC void iniarray()
{
	truc temp;

	arr_sym	  = newintsym("array[ ]", sFBINARY, (wtruc)Farrele);
	subarrsym = newintsym("array[..]",sFBINARY, (wtruc)Fsubarr);

	lengthsym = newsymsig("length", sFBINARY, (wtruc)Flength, s_1);
	chrsym	  = newsymsig("chr",	sFBINARY, (wtruc)Fchr, s_1);
	ordsym	  = newsymsig("ord",	sFBINARY, (wtruc)Ford, s_1);
	sortsym	  = newsymsig("sort",	sSBINARY, (wtruc)Ssort, s_12);
    bsearchsym= newsymsig("binsearch", sFBINARY, (wtruc)Fbsearch, s_23);
	concatsym = newsymsig("concat", sFBINARY, (wtruc)Fconcat, s_1u);
	splitsym  = newsymsig("string_split",sFBINARY,(wtruc)Fstrsplit,s_12);
    sscansym  = newsymsig("string_scan", sFBINARY,(wtruc)Fstrscan, s_23);
	toupsym	  = newsymsig("toupper",sFBINARY, (wtruc)Ftoupper,s_1);
	tolowsym  = newsymsig("tolower",sFBINARY, (wtruc)Ftolower,s_1);
	indexsym  = newsymsig("substr_index",sFBINARY,(wtruc)Findex,s_2);
	arraysym  = newsym("array",	sTYPESPEC, nullsym);
	ofsym	  = newsym("of",	sPARSAUX,  nullsym);

	recordsym = newsym("record",	  sTYPESPEC, nullsym);
	mkrecsym  = newintsym("record",	  sFBINARY, (wtruc)Fmkrec0);
	rec_sym	  = newintsym("rec.field",sSBINARY,(wtruc)Srecfield);
	pointrsym = newsym("pointer",	  sTYPESPEC, nil);
	derefsym  = newintsym("^",	  sSBINARY,(wtruc)Sderef);

	nullstring= mknullstr();
	stringsym = newsym("string",   sTYPESPEC, nullstring);
	str_sym	  = new0symsig("string",sFBINARY,(wtruc)Ftextstring, s_1);
	mkstrsym  = newintsym("string[]",sFBINARY, (wtruc)Fmkstring);

	nullbstring= mknullbstr();
	bstringsym= newsym("byte_string",   sTYPESPEC, nullbstring);
	bstr_sym  = new0symsig("byte_string",sFBINARY,(wtruc)Fbstring, s_12);
	mkbstrsym = newintsym("$",  sFBINARY, (wtruc)Fmkbstring);

	temp	  = newintsym("",   sSBINARY, (wtruc)mkstack);
	stacksym  = newsym("stack", sTYPESPEC, mk0fun(temp));

	charsym	  = newsym("char",  sTYPESPEC, mkchar(' '));

	vectorsym = newintsym("vector", sFBINARY, (wtruc)Fvector);
	pairsym	  = newintsym("pair",	sFBINARY, (wtruc)Fpair);
	mkarrsym  = newintsym("mkarray",sFBINARY,(wtruc)Fmkarray);
	mknewsym  = newsymsig("new",	sSBINARY,(wtruc)Snew,s_1);
	allocsym  = newsymsig("alloc",	sFBINARY,(wtruc)Falloc,s_23);
    reallocsym= newsymsig("realloc",sSBINARY,(wtruc)Srealloc,s_23);

	pushsym	  = newsymsig("stack_push", sFBINARY,(wtruc)Fstkpush,s_2);
	arrpushsym = newsymsig("stack_arraypush", sFBINARY,(wtruc)Fstkarrpush,s_23);
	popsym	  = newsymsig("stack_pop",  sFBINARY,(wtruc)Fstkpop, s_1);
	topsym	  = newsymsig("stack_top",  sFBINARY,(wtruc)Fstktop, s_1);
	resetsym  = newsymsig("stack_reset",sFBINARY,(wtruc)Fstkreset, s_1);
	emptysym  = newsymsig("stack_empty",sFBINARY,(wtruc)Fstkempty, s_1);
	stk2arrsym= newsymsig("stack2array",sFBINARY,(wtruc)Fstk2array, s_1);
	stk2strsym= newsymsig("stack2string",sFBINARY,(wtruc)Fstk2string, s_1);
	maxarrsym = newsymsig("max_arraysize",sFBINARY,(wtruc)Fmaxarray, s_0);
	mbtestsym = newsymsig("mem_btest",  sSBINARY,(wtruc)Smembtest, s_2);
	mbsetsym  = newsymsig("mem_bset",   sSBINARY,(wtruc)Smembset, s_2);
	mbclrsym  = newsymsig("mem_bclear", sSBINARY,(wtruc)Smembclear, s_2);
	mshiftsym = newsymsig("mem_shift",  sSBINARY,(wtruc)Smemshift, s_2);
	mandsym	  = newsymsig("mem_and",    sSBINARY,(wtruc)Smemand, s_2);
	morsym	  = newsymsig("mem_or",	    sSBINARY,(wtruc)Smemor, s_2);
	mxorsym	  = newsymsig("mem_xor",    sSBINARY,(wtruc)Smemxor, s_2);
	mnotsym	  = newsymsig("mem_not",    sSBINARY,(wtruc)Smemnot, s_1);
	mbitswsym = newsymsig("mem_bitswap",sSBINARY,(wtruc)Smembitsw, s_1);
	mbyteswsym= newsymsig("mem_byteswap",sSBINARY,(wtruc)Smembytesw,s_2);

	argvsym	  = newsym("ARGV", sSYSTEMVAR, nullsym);
}
/*--------------------------------------------------------------------*/
PUBLIC void iniargv(argc,argv)
int argc;
char *argv[];
{
	truc obj;
	int i;

	if(argc > ARGCMAX)
		argc = ARGCMAX;
	for(i=0; i<argc; i++) {
		obj = mkstr(argv[i]);
		ARGpush(obj);
	}
	SYMbind(argvsym) = Fvector(argc);
	ARGnpop(argc);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Flength()
{
	long nn;
	unsigned len;

	switch(*FLAGPTR(argStkPtr)) {
	case fVECTOR:
	case fSTRING:
	case fBYTESTRING:
	case fRECORD:
		len = *VECLENPTR(argStkPtr);
		return(mkfixnum(len));
	case fSTACK:
		nn = stacklength(argStkPtr);
		return(mkinum(nn));
	case fSTREAM:
		nn = filelen(argStkPtr);
		if(nn < 0)
			return(brkerr());
		return(mkinum(nn));
	default:
		return(constone);
	}
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fchr()
{
	int flg;
	int ch;

	flg = *FLAGPTR(argStkPtr);
	if(flg != fCHARACTER && flg != fFIXNUM) {
		error(chrsym,err_chr,*argStkPtr);
		return(brkerr());
	}
	ch = *WORD2PTR(argStkPtr);
	if(*SIGNPTR(argStkPtr))
		ch = -ch;
	return(mkchar(ch));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Ford()
{
	int flg;

	flg = *FLAGPTR(argStkPtr);
	if(flg == fFIXNUM || flg == fBIGNUM)
		return(*argStkPtr);
	else if(flg != fCHARACTER) {
		error(ordsym,err_char,*argStkPtr);
		return(brkerr());
	}
	return(mkfixnum(*WORD2PTR(argStkPtr)));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fvector(argn)
int argn;
{
	truc *argptr, *ptr;
	truc obj;

	obj = mkvect0(argn);
	argptr = argStkPtr - argn + 1;
	ptr = VECTOR(obj);
	while(--argn >= 0)
		*ptr++ = *argptr++;
	return(obj);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fpair()
{
	return(mkntuple(fTUPLE,argStkPtr-1,2));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Falloc(argn)
int argn;	/* argn = 2 or 3 */
{
	truc *argptr;
	int flg, flg1;
	int ch;

	argptr = argStkPtr - argn + 1;
	if(*argptr == arraysym)
		return(Fmkarray(argn-1));
	else if(*argptr == stringsym) {
		flg = fSTRING;
		ch = ' ';
	}
	else if(*argptr == bstringsym) {
		flg = fBYTESTRING;
		ch = 0;
	}
	else {
		error(allocsym,err_syarr,*argptr);
		return(brkerr());
	}
	if(argn == 3) {
		flg1 = *FLAGPTR(argStkPtr);
		if(flg1 == fCHARACTER || flg1 == fFIXNUM) {
			ch = *WORD2PTR(argStkPtr);
			if(*SIGNPTR(argStkPtr))
				ch = -ch;
		}
	}
	return(mkstring(argptr+1,flg,ch));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fmkarray(argn)
int argn;	/* argn = 1 or 2 */
{
	truc *argptr, *ptr;
	truc vector, ele;
	unsigned i, len;
	int flg;

	argptr = argStkPtr - argn + 1;
	flg = *FLAGPTR(argptr);
	if(flg != fFIXNUM || *SIGNPTR(argptr)) {
		error(allocsym,err_pfix,*argptr); /* vorlaeufig */
		return(brkerr());
	}
	len = *WORD2PTR(argptr);
	vector = mkvect0(len);
	if(argn == 1) {
	    return(vector);
	}
	WORKpush(vector);
	if(!len)
	    len = 1;
	if(*FLAGPTR(argStkPtr) >= fBOOL) {
	    ele = *argStkPtr;
	    ptr = VECTORPTR(workStkPtr);
	    while(len--)
			*ptr++ = ele;
	}
	else {
	    for(i=0; i<len; i++) {
			ele = mkarrcopy(argStkPtr);
			ptr = VECTORPTR(workStkPtr);
			ptr[i] = ele;
	    }
	}
	return(WORKretr());
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fmkstring()
{
	return(mkstring(argStkPtr,fSTRING,' '));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fmkbstring()
{
	return(mkstring(argStkPtr,fBYTESTRING,0));
}
/*--------------------------------------------------------------------*/
PRIVATE truc mkstring(ptr,flg,ch)
truc *ptr;
int flg;	/* fSTRING or fBYTESTRING */
int ch;
{
	truc string;
	truc symb;
	char *cptr;
	unsigned len;

	if(*FLAGPTR(ptr) != fFIXNUM || *SIGNPTR(ptr)) {
		symb = (flg == fSTRING ? mkstrsym : mkbstrsym);
		error(symb,err_pfix,*ptr); /* vorlaeufig */
		return(brkerr());
	}
	len = *WORD2PTR(ptr);
	string = (flg == fSTRING ? mkstr0(len) : mkbstr0(len));
	cptr = STRINGPTR(&string);
	while(len) {
		*cptr++ = ch;
		len--;
	}
	*cptr = 0;
	return(string);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Srealloc()
{
    truc *argptr, *ptr1, *ptr2;
	truc res, ele;
    truc string, vector;
    char *cpt1, *cpt2;
    unsigned len, len1;
	int argn, k;
	int flg, flg1;
    int ch;

	argn = *ARGCOUNTPTR(evalStkPtr);
    argptr = ARG1PTR(evalStkPtr);
    flg = is_lval(argptr);
    switch(flg) {
    case aERROR:
        error(reallocsym,err_lval,eval(argptr));
        return brkerr();
    case vSUBARRAY:
        error(reallocsym,"subarray cannot be realloc'd",eval(argptr));
        return brkerr();
    case vCONST:
        error(reallocsym,"constants cannot be realloc'd",*argptr);
        return brkerr();
    default:
        break;
    }
	res = eval(argptr);
	ARGpush(res);
    res = eval(ARGNPTR(evalStkPtr,2));
    ARGpush(res);
    if(*FLAGPTR(argStkPtr) != fFIXNUM || *SIGNPTR(argStkPtr)) {
        error(reallocsym,err_pfix,*argStkPtr);
        ARGnpop(2);
        return(brkerr());
    }
    len1 = *WORD2PTR(argStkPtr);
    ARGpop();
	flg = *FLAGPTR(argStkPtr);
    if(flg == fVECTOR || flg == fSTRING || flg == fBYTESTRING) {
        len = *VECLENPTR(argStkPtr);
    }
    else {
        error(reallocsym,err_arr,*argStkPtr);
        ARGpop();
        return(brkerr());
    }
    if(len1 <= len) {
        *VECLENPTR(argStkPtr) = len1;
        res = ARGretr();
        return Lvalassign(ARG1PTR(evalStkPtr),res);
    }
    /* now len1 > len, i.e. proper realloc */
    if(argn == 3) {
        res = eval(ARGNPTR(evalStkPtr,3));
        ARGpush(res);
    }
    else if(flg == fVECTOR)
        ARGpush(zero);
    if(flg != fVECTOR) {    /* flg == fSTRING || flg == fBYTESTRING */
        if(argn == 3) {
            flg1 = *FLAGPTR(argStkPtr);
            if(flg1 == fCHARACTER || flg1 == fFIXNUM) {
                ch = *WORD2PTR(argStkPtr);
			    if(*SIGNPTR(argStkPtr))
				    ch = -ch;
            }
            else {
                ch = 0;
            }
            ARGpop();
        }
        else {
            ch = 0;
        }
       	string = (flg == fSTRING ? mkstr0(len1) : mkbstr0(len1));
    	cpt1 = STRINGPTR(&string);
        cpt2 = STRINGPTR(argStkPtr);
        for(k=0; k<len; k++)
            *cpt1++ = *cpt2++;
        for(k=len; k<len1; k++)
            *cpt1++ = ch;
        *cpt1 = 0;
        ARGpop();
        res = Lvalassign(ARG1PTR(evalStkPtr),string);
        return res;
    }
    /* else flg == fVECTOR */
    vector = mkvect0(len1);
    WORKpush(vector);
    ptr1 = VECTORPTR(workStkPtr);
    ptr2 = VECTORPTR(argStkPtr-1);
    for(k=0; k<len; k++)
        *ptr1++ = *ptr2++;
    if(*FLAGPTR(argStkPtr) >= fBOOL) {
        ele = *argStkPtr;
        for(k=len; k<len1; k++)
            *ptr1++ = ele;
    }
    else {
        for(k=len; k<len1; k++) {
            ele = mkarrcopy(argStkPtr);
			ptr1 = VECTORPTR(workStkPtr);
			ptr1[k] = ele;
        }
    }
    ARGnpop(2);
    res = Lvalassign(ARG1PTR(evalStkPtr),*workStkPtr);
    WORKpop();
    return res;
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fconcat(argn)
int argn;
{
	truc *argptr, *ptr;
	truc strobj;
	char *str, *str1;
	long len;
	unsigned n;
	int i, flg;

	argptr = argStkPtr - argn + 1;
	len = 0;
	for(i=0; i<argn; i++) {
		ptr = argptr + i;
		flg = *FLAGPTR(ptr);
		if(flg == fSTRING)
			len += *STRLENPTR(ptr);
		else if(flg == fCHARACTER || flg == fFIXNUM)
			len++;
		else {
			error(concatsym,err_str,*ptr);
			return(brkerr());
		}
	}
	if(len > 0xFFFF) {
		error(concatsym,err_2long,voidsym);
		return(brkerr());
	}
	else {
		n = len;
	}
	strobj = mkstr0(n);
	str = STRING(strobj);
	for(i=0; i<argn; i++,argptr++) {
		if(*FLAGPTR(argptr) == fSTRING) {
			str1 = STRINGPTR(argptr);
			n = *STRLENPTR(argptr);
			while(n--)
				*str++ = *str1++;
		}
		else
			*str++ = *WORD2PTR(argptr);
	}
	*str = 0;
	return(strobj);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Findex()
{
	byte *str, *str1;
	long res;
	size_t len, len1;
	int i, flg;

	for(i=-1; i<=0; i++) {
		flg = *FLAGPTR(argStkPtr+i);
		if(flg != fSTRING && flg != fBYTESTRING) {
		    error(indexsym,err_str,argStkPtr[i]);
		    res = -1;
		    goto ausgang;
		}
	}
	len = *STRLENPTR(argStkPtr-1);
	len1 = *STRLENPTR(argStkPtr);
	str = BYTEPTR(argStkPtr-1);
	str1 = BYTEPTR(argStkPtr);
	res = substrindex(str,len,str1,len1);
  ausgang:
	return(mksfixnum((int)res));
}
/*--------------------------------------------------------------------*/
PRIVATE long substrindex(str,len,sub,len1)
byte *str, *sub;
size_t len, len1;
{
	byte *ptr, *ptr1;
	size_t i,k,diff;
	long res;
	int ch;

	if(len1 == 0 || len1 > len)
		return(-1);
	diff = len - len1;
	ch = *sub++;
	for(i=0; i<=diff; i++) {
		if(*str++ != ch)
			continue;
		ptr = str; ptr1 = sub;
		res = i;
		for(k=1; k<len1; k++) {
			if(*ptr++ != *ptr1++) {
				res = -1;
				break;
			}
		}
		if(res >= 0)
			return(res);
	}
	return(-1);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fstrsplit(argn)
int argn;
{
	truc *vptr, *basptr;
	truc *argptr;
	truc obj;
	word2 *offsets;
	char *trenn;
	char *str, *str0;
	unsigned len;
	int k, count, flg;

	if(argn == 2) {
		flg = *FLAGPTR(argStkPtr);
		if(flg != fSTRING) {
			error(splitsym,err_str,*argStkPtr);
			return(brkerr());
		}
		trenn = STRINGPTR(argStkPtr);
	}
	else {
		trenn = NULL;
	}
	argptr = argStkPtr - argn + 1;
	flg = *FLAGPTR(argptr);
	if(flg != fSTRING) {
		error(splitsym,err_str,*argptr);
		return(brkerr());
	}
	str = STRINGPTR(argptr);
	len = *STRLENPTR(argptr);
	if(len > sizeof(word2) * (aribufSize/2-1)) {
		error(splitsym,err_2long,mkfixnum(len));
		return(brkerr());
	}
	str0 = (char *)AriBuf;
	strncopy(str0,str,len);
	offsets = AriBuf + aribufSize/2;
	count = stringsplit(str0,trenn,offsets);

	basptr = workStkPtr + 1;
	for(k=0; k<count; k++) {
		obj = mkstr(str0 + offsets[k]);
		WORKpush(obj);
	}
	*argptr = mkvect0(count);
	vptr = VECTORPTR(argptr);
	if(!count)
		vptr[0] = nullstring;
	else for(k=0; k<count; k++)
		vptr[k] = basptr[k];
	workStkPtr = basptr - 1;
	return(*argptr);
}
/*--------------------------------------------------------------------*/
/*
** Zerlegt den String str destruktiv (durch Einfuegen von
** Nullbytes) in Teilstrings; dabei werden alle Characters aus trenn
** als Trenn-Elemente aufgefasst. Fuer trenn == NULL werden SPACE,
** TAB und '\n' als Trenner verwendet.
** In offsets werden die Offsets der Teilstrings von str abgelegt;
** Rueckgabewert ist die Anzahl der Teilstrings.
*/
PUBLIC int stringsplit(str,trenn,offsets)
char *str, *trenn;
word2 *offsets;
{
	static char trenn0[5] = {' ','\t','\n','\r',0};
	word4 sep4[64];		/* space for 256 bytes */
	word4 *pt4;
	byte *sep;
	int k, count, ch;

	k = 64; pt4 = sep4;
	while(--k >= 0)
		*pt4++ = 0;
	sep = (byte *)sep4;
	if(trenn == NULL)
		trenn = trenn0;
	while((ch = *(byte *)trenn++))
		sep[ch] = 1;
	k = count = 0;
	while(1) {
		while((ch = *(byte *)str++) && sep[ch])
			k++;
		if(!ch) break;
		offsets[count++] = k++;
		while((ch = *(byte *)str++) && !sep[ch])
			k++;
		if(!ch) break;
		str[-1] = 0;
		k++;
	}
	return(count);
}
/*--------------------------------------------------------------------*/
/*
** string_scan(s,bag: string): integer;
**   returns the position in s of the first character that belongs to bag;
**   if there is no such character, -1 is returned
** string_scan(s,bag: string; false): integer;
**   returns the position in s of the first character that does not
**   belong to bag; if all characters of s also belog to bag,
**   -1 is returned
*/
PRIVATE truc Fstrscan(argn)
int argn;
{
    truc *argptr;
    byte *str, *bag, *sep;
    int flg, tst, ch;
    int k, len, len2, pos;
	word4 sep4[64];		/* space for 256 bytes */
	word4 *pt4;
    word4 fill;

    if(argn == 3 && (*argStkPtr == zero || *argStkPtr == false)) {
        fill = 0xFFFFFFFF; tst = 0;
    }
    else {
        fill = 0; tst = 0xFF;
    }
    argptr = argStkPtr - argn + 1;
	for(k=0; k<2; k++) {
		flg = *FLAGPTR(argptr+k);
		if(flg != fSTRING && flg != fBYTESTRING) {
			error(sscansym,err_str,argptr[k]);
			return(brkerr());
		}
	}
    str = BYTEPTR(argptr);
    len = *STRLENPTR(argptr);
	bag = BYTEPTR(argptr+1);
    len2 = *STRLENPTR(argptr+1);

	k = 64; pt4 = sep4;
	while(--k >= 0)
		*pt4++ = fill;

	sep = (byte *)sep4;
	for(k=0; k<len2; k++) {
        ch = bag[k];
		sep[ch] = tst;
    }
    pos = -1;
    for(k=0; k<len; k++) {
        ch = *str++;
        if(sep[ch]) {
            pos = k;
            break;
        }
    }
    return(mksfixnum(pos));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Ftolower()
{
	return(Gchangecase(tolowsym));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Ftoupper()
{
	return(Gchangecase(toupsym));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Gchangecase(symb)
truc symb;
{
	ifun chfun;
	char *str;
	unsigned k, len;
	int flg, ch;

	flg = *FLAGPTR(argStkPtr);

	chfun = (symb == toupsym ? toupcase : tolowcase);
	if(flg == fCHARACTER || flg == fFIXNUM) {
		ch = *WORD2PTR(argStkPtr);
		return(mkchar(chfun(ch)));
	}
	else if(flg == fSTRING) {
		*argStkPtr = mkcopy(argStkPtr);
		str = STRINGPTR(argStkPtr);
		len = *STRLENPTR(argStkPtr);
		for(k=0; k<len; k++)
			*str++ = chfun(*str);
		return(*argStkPtr);
	}
	else {
		error(symb,err_str,*argStkPtr);
		return(brkerr());
	}
}
/*--------------------------------------------------------------------*/
PRIVATE truc Farrele()
{
	truc *ptr;
	truc obj;
	byte *cptr;
	long index;
	int flg;

	flg = arrindex(argStkPtr-1,&index);
	switch(flg) {
	case fVECTOR:
		ptr = vectele(argStkPtr-1,index);
		if(ptr) 
			return(*ptr);
		else
			return(brkerr());
	case fSTRING:
	case fBYTESTRING:
		cptr = (byte *)stringele(argStkPtr-1,index);
		if(cptr) {
			if(flg == fSTRING)
				obj = mkchar(*cptr);
			else
				obj = mkfixnum(*cptr);
			return(obj);
		}
		else
			return(brkerr());
	default:
		return(brkerr());
	}
}
/*--------------------------------------------------------------------*/
/*
** Auswertung von arr[start .. end]
** In argStkPtr[-1] steht das Array, in argStkPtr[0] ein
** 2-tupel mit start und end.
*/
PRIVATE truc Fsubarr()
{
	truc *ptr, *ptr1;
	truc obj;
	char *cptr, *cptr1;
	long len, len0, n0, n1;
	int flg;

	flg = *FLAGPTR(argStkPtr-1);
	if(flg < fVECTLIKE0 && flg > fVECTLIKE1) {
		error(subarrsym,err_arr,argStkPtr[-1]);
		return(brkerr());
	}
	len = *VECLENPTR(argStkPtr-1);

	if(indrange(argStkPtr,len,&n0,&n1) == aERROR) {
		return(brkerr());
	}
	len0 = n1 - n0 + 1;
	if(flg == fVECTOR) {
		obj = mkvect0((unsigned)len0);
		ptr1 = VECTORPTR(argStkPtr-1) + n0;
		ptr = VECTOR(obj);
		while(--len0 >= 0)
			*ptr++ = *ptr1++;
		return(obj);
	}
	if(flg == fSTRING)
		obj = mkstr0((unsigned)len0);
	else if(flg == fBYTESTRING)
		obj = mkbstr0((unsigned)len0);
	else			/* this case should not happen */
		return(brkerr());

	/* case fSTRING or fBYTESTRING */
	cptr1 = STRINGPTR(argStkPtr-1) + n0;
	cptr = STRING(obj);
	while(--len0 >= 0)
		*cptr++ = *cptr1++;
	*cptr = 0;
	return(obj);
}
/*--------------------------------------------------------------------*/
PUBLIC int indrange(ptr,len,pn0,pn1)
truc *ptr;
long len;
long *pn0, *pn1;
{
	long n0, n1;
	int ret = 0;

	ptr = VECTORPTR(ptr);
	if(*FLAGPTR(ptr) == fFIXNUM) {
		n0 = *WORD2PTR(ptr);
		if(*SIGNPTR(ptr))
			n0 = 0;
	}
	else
		ret = aERROR;
	ptr++;
	if(*FLAGPTR(ptr) == fFIXNUM) {
		n1 = *WORD2PTR(ptr);
		if(*SIGNPTR(ptr))
			n1 = -1;
	}
	else if(*ptr == endsym) {
		n1 = len-1;
	}
	else
		ret = aERROR;
	if(ret == aERROR) {
		error(subarrsym,err_sarr,voidsym);
		return(ret);
	}
	if(n1 > len-1)
		n1 = len-1;
	if(n0 > n1)
		n0 = n1+1;
	*pn0 = n0;
	*pn1 = n1;
	return(ret);
}
/*--------------------------------------------------------------------*/
PRIVATE int arrindex(ptr,pindex)
truc *ptr;
long *pindex;
{
	long index;
	int flg, flg1;

	flg = *FLAGPTR(ptr);
	if(flg < fVECTLIKE0 && flg > fVECTLIKE1) {
		error(arr_sym,err_arr,*ptr);
		return(aERROR);
	}
	ptr++;
	flg1 = *FLAGPTR(ptr);
	if(flg1 == fFIXNUM) {
		index = *WORD2PTR(ptr);
		if(*SIGNPTR(ptr))
			index = -index;
		*pindex = index;
	}
	else {		/* vorlaeufig */
		error(arr_sym,err_fix,*ptr);
		return(aERROR);
	}
	return(flg);
}
/*--------------------------------------------------------------------*/
PRIVATE char *stringele(ptr,index)
truc *ptr;
long index;
{
	struct strcell *str;
	char *cptr;
	unsigned len;

	str = (struct strcell *)TAddress(ptr);
	len = str->len;
	if(index < 0 || index >= len) {
		error(arr_sym,err_irange,mkinum(index));
		return(NULL);
	}
	cptr = &(str->ch0);
	return(cptr + index);
}
/*--------------------------------------------------------------------*/
PRIVATE truc *vectele(ptr,index)
truc *ptr;
long index;
{
	struct vector *vec;
	truc *ptr1;
	unsigned len;

	vec = (struct vector *)TAddress(ptr);
	len = vec->len;
	if(index < 0 || index >= len) {
		error(arr_sym,err_irange,mkinum(index));
		return(NULL);
	}
	ptr1 = &(vec->ele0);
	return(ptr1 + index);
}
/*--------------------------------------------------------------------*/
/*
** arr[0] enthaelt Array, arr[1] den Index
** In die entsprechende Komponente des Arrays wird obj eingetragen
*/
PUBLIC truc arrassign(arr,obj)
truc *arr;
truc obj;
{
	truc *ptr;
	char *cptr;
	variant v;
	long index;
	int flg;
	int ch;

	flg = arrindex(arr,&index);
	switch(flg) {
	case fVECTOR:
		ptr = vectele(arr,index);
		if(ptr)
			*ptr = obj;
		else
			return(brkerr());
		break;
	case fSTRING:
	case fBYTESTRING:
		cptr = stringele(arr,index);
		if(cptr) {
			v.xx = obj;
			flg = v.pp.b0;
			if(flg == fCHARACTER)
				ch = v.pp.ww;
			else if(flg == fFIXNUM) {
				ch = v.pp.ww;
				if(v.pp.b1)	/* signum */
					ch = -ch;
				arr[2] = mkchar(ch);
			}
			else {
				error(arr_sym,err_char,obj);
				return(brkerr());
			}
			*cptr = ch;
		}
		else
			return(brkerr());
		break;
	default:
		return(brkerr());
	}
	return(obj);
}
/*--------------------------------------------------------------------*/
PRIVATE int arrcompat(flg1,flg2)
int flg1, flg2;
{
	if(flg1 < fVECTLIKE0 && flg1 > fVECTLIKE1)
		return(-1);
	else if(flg1 == flg2)
		return(0);
	else if(flg2 < fVECTLIKE0 && flg2 > fVECTLIKE1)
		return(-2);
	else if((flg1 == fVECTOR && flg2 != fVECTOR) ||
		(flg2 == fVECTOR && flg1 != fVECTOR))
		return(-3);
	else
		return(0);
}
/*--------------------------------------------------------------------*/
/*
** arr[0] enthaelt Array, arr[1] ein Indexpaar (als fTUPLE der Laenge 2)
*/
PUBLIC truc subarrassign(arr,obj)
truc *arr;
truc obj;
{
	truc *ptr, *ptr1;
	char *cptr, *cptr1;
	long len, len0, len1, n0, n1;
	int flg, err;

	flg = *FLAGPTR(arr);
	err = arrcompat(flg,Tflag(obj));
	if(err < 0) {
		if(err >= -2) {
			if(err == -1)
				obj = arr[0];
			error(subarrsym,err_arr,obj);
		}
		return(brkerr());
	}
	len = *VECLENPTR(arr);

	if(indrange(arr+1,len,&n0,&n1) == aERROR) {
		return(brkerr());
	}
	len0 = n1 - n0 + 1;
	len1 = VEClen(obj);
	if(len0 > len1)
		len0 = len1;
	switch(flg) {
	case fVECTOR:
		ptr1 = VECTORPTR(arr) + n0;
		ptr = VECTOR(obj);
		while(--len0 >= 0)
			*ptr1++ = *ptr++;
		break;
	case fSTRING:
	case fBYTESTRING:
		cptr1 = STRINGPTR(arr) + n0;
		cptr = STRING(obj);
		while(--len0 >= 0)
			*cptr1++ = *cptr++;
		break;
	default:
		return(brkerr());
	}
	return(obj);
}
/*--------------------------------------------------------------------*/
PRIVATE long stacklength(ptr)
truc *ptr;
{
	struct stack *sptr;
	long len;

	sptr = (struct stack *)TAddress(ptr);
	len = sptr->pageno;
	len <<= PAGELENBITS;	   /* times PAGELEN */
	len += sptr->line;
	return(len);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fstkpush()
{
	struct stack *sptr;
	struct stackpage *spage;
	truc currpage, sheet;
	int line;

	if(*FLAGPTR(argStkPtr-1) != fSTACK) {
		error(pushsym,err_stkv,voidsym);
		return(brkerr());
	}
	if(*argStkPtr == breaksym)
		return(brkerr());
	sptr = (struct stack *)TAddress(argStkPtr-1);
	line = sptr->line;
	if(line == 0) {		/* create new page */
		sheet = mkvect0(PAGELEN+1);
		sptr = (struct stack *)TAddress(argStkPtr-1);
		currpage = sptr->page;
		sptr->page = sheet;
		spage = (struct stackpage *)Taddress(sheet);
		spage->prevpage = currpage;
	}
	else
		spage = (struct stackpage *)Taddress(sptr->page);
	spage->data[line] = mkcopy(argStkPtr); /* 20030606 */
	line++;
	if(line >= PAGELEN) {
		line = 0;
		sptr->pageno++;
	}
	sptr->line = line;
	return *argStkPtr;
}
/*--------------------------------------------------------------------*/
/*
** stack_arraypush(st: stack; vec: array [; direction: integer]): integer;
*/
PRIVATE truc Fstkarrpush(argn)
int argn;
{
	struct stack *sptr;
	struct stackpage *spage;
	struct vector *vecstruct;
	truc *argptr, *vec;
	truc currpage, sheet;
	int line, incr;
	unsigned len, pos, k;

	argptr = argStkPtr-argn+1;
	if(*FLAGPTR(argptr) != fSTACK) {
		error(arrpushsym,err_stkv,voidsym);
		return(brkerr());
	}
	if(*FLAGPTR(argptr+1) != fVECTOR) {
		error(arrpushsym,err_vect,argptr[1]);	
		return(brkerr());
	}
	if(argn == 3) {
		if(*FLAGPTR(argStkPtr) != fFIXNUM) {
			error(arrpushsym,"integer +1 or -1 expected",*argStkPtr);
			return(brkerr());
		}
		incr = (*SIGNPTR(argStkPtr) ? -1 : 1);
	}
	else
		incr = 1;
    vecstruct = VECSTRUCTPTR(argptr+1);
	vec = &(vecstruct->ele0);
	len = vecstruct->len;
	if(len == 0)
		return zero;

	pos = (incr > 0 ? 0 : len-1);

	sptr = (struct stack *)TAddress(argptr);
	spage = (struct stackpage *)Taddress(sptr->page);
	line = sptr->line;
	for(k=0; k<len; k++, pos += incr) {
		if(line == 0) {		/* create new page */
			sheet = mkvect0(PAGELEN+1);
			sptr = (struct stack *)TAddress(argptr);
			currpage = sptr->page;
			sptr->page = sheet;
			spage = (struct stackpage *)Taddress(sheet);
			spage->prevpage = currpage;
			vec = VECTORPTR(argptr+1); /* may have changed during gc */
		}
		spage->data[line] = vec[pos];
		line++;
		if(line >= PAGELEN) {
			line = 0;
			sptr->pageno++;
		}
	}
	sptr->line = line;

	return(mkfixnum(len));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fstkpop()
{
	return(Gstkretr(popsym));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fstktop()
{
	return(Gstkretr(topsym));
}
/*--------------------------------------------------------------------*/
PRIVATE truc Gstkretr(symb)
truc symb;
{
	struct stack *sptr;
	struct stackpage *spage;
	truc currpage, obj;
	int line;

	if(*FLAGPTR(argStkPtr) != fSTACK) {
		error(symb,err_stkv,voidsym);
		return(brkerr());
	}
	sptr = (struct stack *)TAddress(argStkPtr);
	line = sptr->line;
	currpage = sptr->page;
	if(currpage == nullsym) {
		error(symb,err_stke,voidsym);
		return(brkerr());
	}
	spage = (struct stackpage *)Taddress(currpage);
	line = (line > 0 ? line-1 : PAGELEN-1);
	obj = spage->data[line];
	if(symb == popsym) {
		sptr->line = line;
		if(line == 0)		/* delete current page */
			sptr->page = spage->prevpage;
		else if(line == PAGELEN-1)
			sptr->pageno--;
	}
	return(obj);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fstk2array()
{
	struct stack *sptr;
	struct stackpage *spage;
	truc currpage;
	truc *vec;
	truc arr;
	long llen;
	unsigned len;
	int lineno, pageno;

	if(*FLAGPTR(argStkPtr) != fSTACK) {
		error(stk2arrsym,err_stkv,voidsym);
		return(brkerr());
	}
	sptr = (struct stack *)TAddress(argStkPtr);
	pageno = sptr->pageno;
	lineno = sptr->line;
	llen = pageno;
	llen <<= PAGELENBITS;	   /* times PAGELEN */
	llen += lineno;
	if(llen > (long)getblocksize()) {
		error(stk2arrsym,err_stkbig,voidsym);
		return(brkerr());
	}
	len = llen;
	arr = mkvect0(len);
	vec = VECTORPTR(&arr);
	if(len == 0)
		goto ausgang;

	sptr = (struct stack *)TAddress(argStkPtr);
	/* may have changed after garbage collection */
	currpage = sptr->page;
	spage = (struct stackpage *)Taddress(currpage);
	lineno = (lineno > 0 ? lineno-1 : PAGELEN-1);
	while(len > 0) {
	    len--;
	    vec[len] = spage->data[lineno];
	    lineno--;
	    if(lineno < 0) {
			lineno += PAGELEN;
			if(--pageno < 0)
				break;
			currpage = spage->prevpage;
			spage = (struct stackpage *)Taddress(currpage);
	    }
	}
	sptr->line = 0;
	sptr->pageno = 0;
	sptr->page = nullsym;
  ausgang:
	return(arr);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fstk2string()
{
	struct stack *sptr;
	struct stackpage *spage;
	truc currpage;
	truc *ptr;
    char *str, *str1, *str2;
	long llen;
	unsigned len, mlen, slen, pos, k;
	int lineno, pageno;
	int flg;

	if(*FLAGPTR(argStkPtr) != fSTACK) {
		error(stk2arrsym,err_stkv,voidsym);
		return(brkerr());
	}
	sptr = (struct stack *)TAddress(argStkPtr);
	pageno = sptr->pageno;
	lineno = sptr->line;
	llen = pageno;
	llen <<= PAGELENBITS;	   /* times PAGELEN */
	llen += lineno;
	mlen = (getblocksize()-1) & 0xFFFC;
	if(llen > (long)mlen) 
		goto errexit;
	len = llen;
	pos = aribufSize*sizeof(word2) & 0xFFFC;
	if(pos/4 > mlen)
		pos = 4*mlen;
    str = (char*)AriBuf;
	str[pos] = '\0';
	currpage = sptr->page;
	spage = (struct stackpage *)Taddress(currpage);
	lineno = (lineno > 0 ? lineno-1 : PAGELEN-1);
	while(len > 0) {
	    len--;
	    ptr = spage->data + lineno;
		flg = *FLAGPTR(ptr);
		if(flg == fCHARACTER) {
			if(pos > 0) {
				pos--;
				str[pos] = *WORD2PTR(ptr);
			}
			else
				goto errexit;
		}
		else if(flg == fSTRING) {
			str2 = STRINGPTR(ptr);
			slen = *STRLENPTR(ptr);
			if(pos >= slen) {
				pos -= slen;
				for(str1=str+pos, k=0; k<slen; k++)
					*str1++ = *str2++;
			}
			else
				goto errexit;
		}
	    lineno--;
	    if(lineno < 0) {
			lineno += PAGELEN;
			if(--pageno < 0)
				break;
			currpage = spage->prevpage;
			spage = (struct stackpage *)Taddress(currpage);
	    }
	}
	sptr->line = 0;
	sptr->pageno = 0;
	sptr->page = nullsym;
	return(mkstr(str+pos));
  errexit:
	error(stk2strsym,err_stkbig,voidsym);
	return(brkerr());
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fstkreset()
{
	struct stack *sptr;

	if(*FLAGPTR(argStkPtr) != fSTACK) {
		error(resetsym,err_stkv,voidsym);
		return(brkerr());
	}
	sptr = (struct stack *)TAddress(argStkPtr);
	sptr->line = 0;
	sptr->pageno = 0;
	sptr->page = nullsym;

	return(zero);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fstkempty()
{
	struct stack *sptr;

	if(*FLAGPTR(argStkPtr) != fSTACK) {
		error(emptysym,err_stkv,voidsym);
		return(brkerr());
	}
	sptr = (struct stack *)TAddress(argStkPtr);
	return(sptr->page == nullsym ? true : false);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Fmaxarray()
{
	unsigned len = getblocksize();

	return(mkfixnum((len-1) & 0xFFFC));
}
/*--------------------------------------------------------------------*/
#ifdef QUICKSORT
typedef int (*ifunvv)(const void *, const void *);
PUBLIC void sortarr(arr,len,cmpfn)
truc *arr;
unsigned len;
ifuntt cmpfn;
{
    qsort(arr,(size_t)len,sizeof(truc),(ifunvv)cmpfn);
}
#else
/*---------------------------------------------------------*/
/*
** destructively shellsorts array arr[0],...,arr[len-1]
** with ordering given by
**		int cmpfn(truc *ptr1, truc *ptr2);
** arr should be safe w.r.t garbage collection in case cmpfn
** allocates new memory
*/
PUBLIC void sortarr(arr,len,cmpfn)
truc *arr;
unsigned len;
ifuntt cmpfn;
{
#define DDLEN	14
	static unsigned dd[DDLEN] =
	{1,3,7,17,37,83,191,421,929,2053,4517,9941,21871,0xFFFF};

	unsigned i, k, d;
	int n = 0;

	ARGpush(zero);
	while(n < DDLEN && dd[n] < len)
		n++;
	while(--n >= 0) {
		d = dd[n];
		i = len - d;
		while(i) {
			*argStkPtr = arr[--i];
			k = i + d;
			while(k<len && cmpfn(argStkPtr,arr+k) > 0) {
				arr[k-d] = arr[k];
				k += d;
			}
			arr[k-d] = *argStkPtr;
		}
	}
	ARGpop();
}
#endif  /* QUICKSORT */
/*---------------------------------------------------------*/
PRIVATE int tttype;
PRIVATE int compfun(ptr1,ptr2)
truc *ptr1, *ptr2;
{
	char *str1, *str2;

	if(tttype >= fFIXNUM)
		return(cmpnums(ptr1,ptr2,tttype));
	else if(tttype == fSTRING) {
		str1 = STRINGPTR(ptr1);
		str2 = STRINGPTR(ptr2);
		return(strcmp(str1,str2));
	}
	else
		return(0);	 /* this case should not happen */
}
/*---------------------------------------------------------*/
PRIVATE truc *usercmpfun;
PRIVATE int ucompfun(ptr1,ptr2)
truc *ptr1, *ptr2;
{
	truc argv[2];
	truc res;
	int flg;

	argv[0] = *ptr1;
	argv[1] = *ptr2;
	res = ufunapply(usercmpfun,argv,2);
	if(res == zero)
		return(0);
	else if((flg = *FLAGPTR(&res)) == fFIXNUM)
		return(*SIGNPTR(&res) ? -1 : 1);
	else if(flg == fBIGNUM)
		return(*SIGNUMPTR(&res) ? -1 : 1);
	else {
		error(sortsym,err_case,mkfixnum(flg));
		return(aERROR);
	}
}
/*---------------------------------------------------------*/
PRIVATE truc Ssort()
{
	struct fundef *fundefptr;
	struct symbol *sptr;
	truc *argptr;
	truc *ptr;
	truc obj, fun;
	int argn;

	argn = *ARGCOUNTPTR(evalStkPtr);
	if(argn == 1) {
		argptr = ARG1PTR(evalStkPtr);
		return(Hsort1(argptr));
	}
	else {	/* argn == 2, second argument is compare function */
		ptr = ARGNPTR(evalStkPtr,2);
		obj = eval(ptr);
		if(Tflag(obj) != fSYMBOL)
			goto errexit;
		sptr = symptr(obj);
		if(*FLAGPTR(sptr) != sFUNCTION)
			goto errexit;
		fun = sptr->bind.t;
		fundefptr = (struct fundef *)Taddress(fun);
		if(fundefptr->argc != 2)
			goto errexit;

		argptr = ARG1PTR(evalStkPtr);
		WORKpush(*argptr);
		obj = Hsort2(workStkPtr,fun);
		WORKpop();
		return(obj);
  errexit:
		error(sortsym,"bad compare function",obj);
		return(brkerr());
	}
}
/*---------------------------------------------------------*/
PRIVATE truc Hsort1(argptr)
truc *argptr;
{
	truc *arr;
	truc *vptr;
	unsigned k, len;
	int flg0, flg;

	flg0 = vectaddr(argptr,&vptr,&arr,&len);
	if(flg0 == aERROR) {
		error(sortsym,err_vect,*argptr);
		return(brkerr());
	}
	if(!len)
		return(eval(argptr));
	flg = *FLAGPTR(arr);
	if(flg >= fFIXNUM)
		flg = chknums(sortsym,arr,len);
	else if(flg == fSTRING) {
		for(k=1; k<len; k++) {
			if(*FLAGPTR(arr+k) != fSTRING) {
				flg = aERROR;
				break;
			}
		}
	}
	else
		flg = aERROR;
	if(flg == aERROR) {
		return(brkerr());
	}
	tttype = flg;		/* global variable */
	sortarr(arr,len,(ifuntt)compfun);
	return(eval(argptr));
}
/*---------------------------------------------------------*/
PRIVATE truc Hsort2(argptr,fun)
truc *argptr;
truc fun;
{
	truc *arr, *workarr, *ptr;
	truc *vptr;
	unsigned k, len;
	int flg;

	flg = vectaddr(argptr,&vptr,&arr,&len);
	if(flg == aERROR) {
		error(sortsym,err_vect,*argptr);
		return(brkerr());
	}
	if(!len) {
		goto ausgang;
	}
	workarr = workStkPtr + 1;
	if(WORKspace(len) == NULL) {
		error(sortsym,err_memev,voidsym);
		goto ausgang;
	}
	ptr = workarr;
	for(k=0; k<len; k++)
		*ptr++ = *arr++;
	WORKpush(fun);
	usercmpfun = workStkPtr;
	sortarr(workarr,(int)len,(ifuntt)ucompfun);
	WORKpop();
	vectaddr(argptr,&vptr,&arr,&len);
	for(k=0; k<len; k++)
		*arr++ = *workarr++;
	WORKnpop(len);
  ausgang:
	return(eval(argptr));
}
/*---------------------------------------------------------*/
/*
** binsearch(ele,vec,[compfun]): integer;
**   Searches for ele in the array vec; 
**   returns position; if not found, -1 is returned.
**   The array must be sorted.
**   The optional argument compfun is a comparing function
**   as in sort
*/
PRIVATE truc Fbsearch(argn)
int argn;
{
   	struct symbol *sptr;
	truc *argptr, *vptr, *ele;
	int flg, vergl;
	unsigned n1, n2, m;

    argptr = argStkPtr - argn + 1;
	vptr = argptr + 1;
	flg = *FLAGPTR(vptr);
	if(flg != fVECTOR) {
		error(bsearchsym,err_vect,*vptr);
		return(brkerr());
	}

    if(argn == 2) {     /* default compare function for numbers or strings */
        flg = *FLAGPTR(argptr);
        if(flg >= fFIXNUM || flg == fSTRING) {
			return Hbsearch1(argptr,vptr,flg);
        }
        else {
            error(bsearchsym,"lacking compare function",voidsym);
            return(brkerr());
        }
    }
    else {      /* argn=3, user supplied compare function */
		if(*FLAGPTR(argStkPtr) != fSYMBOL)
			goto badcompare;
		sptr = SYMPTR(argStkPtr);
		if(*FLAGPTR(sptr) != sFUNCTION)
			goto badcompare;
		*argStkPtr = sptr->bind.t;
		if(*FUNARGCPTR(argStkPtr) != 2)
			goto badcompare;
	    usercmpfun = argStkPtr;		/* global variable */

		n1 = 0;
		n2 = *VECLENPTR(vptr);
		while(n2 > n1) {
			m = (n1 + n2)/2;
			ele = VECTORPTR(vptr) + m;
			vergl = ucompfun(argptr,ele);
			if(vergl < 0)
				n2 = m;
			else if(vergl > 0)
				n1 = m+1;
			else
				return(mkfixnum(m));
		}
		return(mksfixnum(-1));
	}
  badcompare:
    error(bsearchsym,"bad compare function",voidsym);
    return(brkerr());
}
/*---------------------------------------------------------*/
/*
** binary search in array of numbers or strings
*/
PRIVATE truc Hbsearch1(ele,vptr,flg)
truc *ele, *vptr;
int flg;
{
	int flg1, vergl;
	truc *arr;
	unsigned n1, n2, m;

	arr = VECTORPTR(vptr);
	n1 = 0;
	n2 = *VECLENPTR(vptr);
	if(flg == fSTRING) {
		tttype = flg;
		while(n2 > n1) {
			m = (n1 + n2)/2;
			flg1 = *FLAGPTR(arr+m);
			if(flg1 != flg) {
				error(bsearchsym,err_str,arr[m]);
				return(brkerr());
			}
			vergl = compfun(ele,arr+m);
			if(vergl < 0)
				n2 = m;
			else if(vergl > 0)
				n1 = m+1;
			else
				return(mkfixnum(m));
		}
		return(mksfixnum(-1));
	}
	else if(flg >= fFIXNUM) {
		while(n2 > n1) {
			m = (n1 + n2)/2;
			flg1 = *FLAGPTR(arr+m);
			if(flg1 < fFIXNUM) {
				error(bsearchsym,err_num,arr[m]);
				return(brkerr());
			}
			tttype = (flg1 > flg ? flg1 : flg);
			vergl = compfun(ele,arr+m);
			if(vergl < 0)
				n2 = m;
			else if(vergl > 0)
				n1 = m+1;
			else
				return(mkfixnum(m));
		}
		return(mksfixnum(-1));
	}
	else {
		error(bsearchsym,err_case,voidsym);
		return(brkerr());
	}
}
/*---------------------------------------------------------*/
PRIVATE int vectaddr(ptr,ppvec,parr,plen)
truc *ptr;
truc **ppvec;
truc **parr;
unsigned *plen;
{
	truc *vecptr;
	truc *arr;
	long len, n0, n1;
	int ret;

	ret = Lvaladdr(ptr,&vecptr);
	switch(ret) {
	case vARRELE:
	case vRECFIELD:
		vecptr = Ltrucf(ret,vecptr);
		if(vecptr == NULL)
			return(aERROR);
		/* else fall through */
	case vBOUND:
		if(*FLAGPTR(vecptr) != fVECTOR) {
			return(aERROR);
		}
		len = *VECLENPTR(vecptr);
		arr = VECTORPTR(vecptr);
		break;
	case vSUBARRAY:
		ARGpush(vecptr[1]);
		ARGpush(vecptr[2]);
		argStkPtr[-1] = eval(argStkPtr-1);
		argStkPtr[0] = eval(argStkPtr);
		vecptr = argStkPtr-1;
		if(*FLAGPTR(vecptr) != fVECTOR) {
			ARGnpop(2);
			return(aERROR);
		}
		len = *VECLENPTR(vecptr);
		ret = indrange(argStkPtr,len,&n0,&n1);
		ARGnpop(2);
		if(ret == aERROR) {
			return(aERROR);
		}
		len = n1 - n0 + 1;
		arr = VECTORPTR(vecptr) + (size_t)n0;
		break;
	default:
		return(aERROR);
	}
	*ppvec = vecptr;
	*parr = arr;
	*plen = (unsigned)len;
	return(ret);
}
/*---------------------------------------------------------*/
PUBLIC int bytestraddr(ptr,ppbstr,ppch,plen)
truc *ptr;
truc **ppbstr;
byte **ppch;
unsigned *plen;
{
	struct strcell *string;
	truc *bstrptr;
	byte *cpt;
	long len, n0, n1;
	int flg, ret;

	ret = Lvaladdr(ptr,&bstrptr);
	switch(ret) {
	case vARRELE:
	case vRECFIELD:
		bstrptr = Ltrucf(ret,bstrptr);
		if(bstrptr == NULL)
			return(aERROR);
		/* else fall through */
	case vBOUND:
		flg = *FLAGPTR(bstrptr);
		if(flg != fBYTESTRING && flg != fSTRING) {
			return(aERROR);
		}
		string = STRCELLPTR(bstrptr);
		len = string->len;
		cpt = (byte *)&(string->ch0);
		break;
	case vSUBARRAY:
		ARGpush(bstrptr[1]);
		ARGpush(bstrptr[2]);
		argStkPtr[-1] = eval(argStkPtr-1);
		argStkPtr[0] = eval(argStkPtr);
		bstrptr = argStkPtr-1;
		flg = *FLAGPTR(bstrptr);
		if(flg != fBYTESTRING && flg != fSTRING) {
			ARGnpop(2);
			return(aERROR);
		}
		string = STRCELLPTR(bstrptr);
		ret = indrange(argStkPtr,(long)string->len,&n0,&n1);
		ARGnpop(2);
		if(ret == aERROR) {
			return(aERROR);
		}
		len = n1 - n0 + 1;
		cpt = (byte *)&(string->ch0) + (size_t)n0;
		break;
	default:
		return(aERROR);
	}
	*ppbstr = bstrptr;
	*ppch = cpt;
	*plen = (unsigned)len;
	return(flg);
}
/*---------------------------------------------------------*/
PRIVATE truc *Ltrucf(flg,pptr)
int flg;
truc *pptr;
{
	truc *ptr;
	long n0;

	switch(flg) {
	case vARRELE:
		ARGpush(pptr[1]);
		ARGpush(pptr[2]);
		argStkPtr[-1] = eval(argStkPtr-1);
		argStkPtr[0] = eval(argStkPtr);
		flg = arrindex(argStkPtr-1,&n0);
		if(flg == fVECTOR) {
			ptr = vectele(argStkPtr-1,n0);
		}
		else {
			ptr = NULL;
		}
		ARGnpop(2);
		break;
	case vRECFIELD:
		ARGpush(pptr[1]);
		*argStkPtr = eval(argStkPtr);
		ptr = recfield(argStkPtr,pptr[2]);
		ARGpop();
		break;
	default:
		ptr = NULL;
	}
	return(ptr);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Ftextstring()
{
	variant v;
	int flg;

	flg = *FLAGPTR(argStkPtr);
    if(flg == fSTRING) {
        return(*argStkPtr);
    }
	else if(flg != fBYTESTRING) {
		error(str_sym,err_bystr,*argStkPtr);
		return(brkerr());
	}
    else { /* flg == fBYTESTRING */
        *argStkPtr = mkcopy(argStkPtr);
    }
	v.xx = *argStkPtr;
	v.pp.b0 = fSTRING;
	return(v.xx);
}
/*---------------------------------------------------------*/
/*
** byte_string(x,n: integer): byte_string;
**   Interpretiert integer x als byte_string der Laenge n;
**   negative Zahlen in Zweier-Komplement-Darstellung
** byte_string(s: string): byte_string;
**   verwandelt string in byte_string;
*/
PRIVATE truc Fbstring(argn)
int argn;
{
	truc *argptr;
	truc bstr;
	word2 *x;
	byte *ptr;
	unsigned len, i, m;
	unsigned u, v;
	int flg, n, sign, pad;
	variant vv;

	argptr = argStkPtr - argn + 1;
	flg = *FLAGPTR(argptr);
	if(argn == 1) {
        if(flg == fSTRING) {
            *argStkPtr = mkcopy(argStkPtr);
            vv.xx = *argStkPtr;
            vv.pp.b0 = fBYTESTRING;
            return(vv.xx);
        }
        else if(flg == fBYTESTRING) {
            return *argStkPtr;
        }
	}
	if(flg < fINTTYPE0 || flg > fINTTYPE1) {
        error(bstringsym,err_intt,*argptr);
		return(brkerr());
	}
	if(argn == 2) {
		if((*FLAGPTR(argStkPtr) != fFIXNUM) || *SIGNPTR(argStkPtr)) {
		    error(bstringsym,err_pfix,*argStkPtr);
		    return(brkerr());
		}
		len = *WORD2PTR(argStkPtr);
	}
	else {
		n = bigref(argStkPtr,&x,&sign);
		len = (bit_length(x,n) + 7) >> 3;
	}
	bstr = mkbstr0(len);
	ptr = (byte *)STRING(bstr);
	x = AriBuf;
    if(flg == fGF2NINT) {
        n = bigretr(argptr,x,&sign);
        pad = 0;
    }
	n = twocretr(argptr,x);
	pad = (n >= (len+1)/2 ? 0 : 1);
	m = (pad ? n : len/2);
	if(pad)
		v = x[n];	/* 0x00 or 0xFF */
	for(i=0; i<m; i++) {
		u = *x++;
		*ptr++ = u;
		*ptr++ = (u >> 8);
	}
	if(!pad && (len & 1)) {
		*ptr = *x;
	}
	else if(pad) {
		len -= 2*m;
		for(i=0; i<len; i++)
			*ptr++ = v;
	}
	return(bstr);
}
/*---------------------------------------------------------*/
PRIVATE truc Smembtest()
{
	return(GmemBi(mbtestsym));
}
/*---------------------------------------------------------*/
PRIVATE truc Smembset()
{
	return(GmemBi(mbsetsym));
}
/*---------------------------------------------------------*/
PRIVATE truc Smembclear()
{
	return(GmemBi(mbclrsym));
}
/*---------------------------------------------------------*/
PRIVATE truc Smemshift()
{
	return(GmemBi(mshiftsym));
}
/*---------------------------------------------------------*/
PRIVATE truc Smemand()
{
	return(GmemBB(mandsym));
}
/*---------------------------------------------------------*/
PRIVATE truc Smemor()
{
	return(GmemBB(morsym));
}
/*---------------------------------------------------------*/
PRIVATE truc Smemxor()
{
	return(GmemBB(mxorsym));
}
/*---------------------------------------------------------*/
PRIVATE truc Smemnot()
{
	return(GmemBB(mnotsym));
}
/*---------------------------------------------------------*/
PRIVATE truc Smembitsw()
{
	return(GmemBB(mbitswsym));
}
/*---------------------------------------------------------*/
PRIVATE truc Smembytesw()
{
	return(GmemBi(mbyteswsym));
}
/*---------------------------------------------------------*/
PRIVATE truc GmemBi(symb)
truc symb;
{
	truc *argptr, *bstrptr;
	truc obj;
	byte *cpt;
	long pos;
	unsigned len, n, k;
	unsigned mask = 1;
	int ret;

	obj = eval(ARG1PTR(evalStkPtr));
	pos = intretr(&obj);
	argptr = ARG0PTR(evalStkPtr);
	if(pos == LONGERROR) {
		error(symb,err_p4int,obj);
		goto errexit;
	}
	ret = bytestraddr(argptr,&bstrptr,&cpt,&len);
	if(ret != fBYTESTRING)
		goto errexit;

	if(symb == mshiftsym) {
		byteshift(cpt,len,pos);
		return(*bstrptr);
	}
	if(symb == mbyteswsym) {
		byteswap(cpt,len,(unsigned)pos);
		return(*bstrptr);
	}
	n = pos >> 3;
	k = pos & 0x7;
	mask <<= k;
	if(symb == mbtestsym) {
		if(pos < 0 || n >= len)
			return(zero);
		else if(cpt[n] & mask)
			return(constone);
		else
			return(zero);

	}
	if(pos >= 0 && n < len) {
		if(symb == mbsetsym)
			cpt[n] |= mask;
		else if(symb == mbclrsym)
			cpt[n] &= ~mask;
	}
	return(*bstrptr);
  errexit:
	error(symb,err_vbystr,*argptr);
	return(brkerr());
}
/*---------------------------------------------------------*/
PRIVATE truc GmemBB(symb)
truc symb;
{
	truc *argptr, *bstrptr, *bstrptr2;
	byte *cpt, *cpt2;
	unsigned len, len2, u;
	int ret;

	argptr = ARG0PTR(evalStkPtr);
	ret = bytestraddr(argptr,&bstrptr,&cpt,&len);
	if(ret != fBYTESTRING)
		goto errexit;

	if(symb == mnotsym) {
		while(len--) {
            u = *cpt;
		    *cpt++ = ~u;
        }
		return(*bstrptr);
	}
	else if(symb == mbitswsym) {
		while(len--) {
		    u = *cpt;
		    *cpt++ = BitSwap[u];
		}
		return(*bstrptr);
	}
	WORKpush(*bstrptr);

	argptr = ARG1PTR(evalStkPtr);
	ret = bytestraddr(argptr,&bstrptr2,&cpt2,&len2);
	if(ret != fBYTESTRING)
		goto errexit2;
	if(len2 < len)
		len = len2;
	if(symb == mxorsym) {
		while(len--)
		    *cpt++ ^= *cpt2++;
	}
	else if(symb == mandsym) {
		while(len--)
		    *cpt++ &= *cpt2++;
	}
	else if(symb == morsym) {
		while(len--)
		    *cpt++ |= *cpt2++;
	}
	return(WORKretr());
  errexit2:
	WORKpop();
  errexit:
	error(symb,err_vbystr,*argptr);
	return(brkerr());
}
/*---------------------------------------------------------*/
PRIVATE void byteshift(ptr,len,sh)
byte *ptr;
unsigned len;
long sh;
{
	word4 k;
	unsigned m, sh0, sh1;

	k = (sh > 0 ? sh >> 3 : (-sh) >> 3);
	if(k >= len) {
		for(m=0; m<len; m++)
			*ptr++ = 0;
		return;
	}
	sh0 = (sh > 0 ? sh & 0x7 : (-sh) & 0x7);
	if(sh > 0) {
		if(k) {
			for(m=len; m>k; --m)
				ptr[m-1] = ptr[m-k-1];
			for(m=0; m<k; m++)
				ptr[m] = 0;
		}
		if(sh0) {
			sh1 = 8 - sh0;
			for(m=len-1; m>k; m--)
				ptr[m] = (ptr[m] << sh0) | (ptr[m-1] >> sh1);
			ptr[k] <<= sh0;
		}
	}
	else if(sh < 0) {
		if(k) {
			for(m=k; m<len; m++)
				ptr[m-k] = ptr[m];
			for(m=len-k; m<len; m++)
				ptr[m] = 0;
		}
		if(sh0) {
			sh1 = 8 - sh0;
			for(m=1; m<len-k; m++)
				ptr[m-1] = (ptr[m-1]>>sh0) | (ptr[m]<<sh1);
			ptr[len-k-1] >>= sh0;
		}
	}
}
/*---------------------------------------------------------*/
PRIVATE void byteswap(ptr,len,grp)
byte *ptr;
unsigned len, grp;
{
	byte *ptr1, *ptr2;
	unsigned x,k;

	if(len < grp || !grp)
	    return;
	len -= grp;
	for(k=0; k<=len; k+=grp,ptr+=grp) {
	    ptr1 = ptr;
	    ptr2 = ptr1 + grp - 1;
	    while(ptr2 > ptr1) {
		x = *ptr1;
		*ptr1++ = *ptr2;
		*ptr2-- = x;
	    }
	}
}
/*--------------------------------------------------------------------*/
/*
** Beschafft die Addresse einer Pointer-Variablen
*/
PRIVATE int Paddr(ptr,pvptr)
truc *ptr;
trucptr *pvptr;
{
	truc *vptr;
	int ret;

	ret = Lvaladdr(ptr,&vptr);
	switch(ret) {
	case vBOUND:
		break;
	case vARRELE:
	case vRECFIELD:
		vptr = Ltrucf(ret,vptr);
		if(vptr == NULL)
			return(aERROR);
		break;
	default:
		return(aERROR);
	}
	if(*FLAGPTR(vptr) != fPOINTER)
		ret = aERROR;
	else
		*pvptr = vptr;
	return(ret);
}
/*--------------------------------------------------------------------*/
/*
** Argument in *argStkPtr ist ein 2n-tupel mit
** n Feldbezeichnungen und n Anfangswerten bzw. Prozeduren,
** die Anfangswerte erzeugen.
** Resultat ein initialisierter Record
*/
PRIVATE truc Fmkrec0()
{
	truc *ptr;
	truc obj;
	unsigned i, n;

	n = *VECLENPTR(argStkPtr);
	n /= 2;
	ptr = VECTORPTR(argStkPtr) + n;
	for(i=0; i<n; i++)
		ARGpush(ptr[i]);
	ptr = argStkPtr - n + 1;
	for(i=0; i<n; i++,ptr++)
		*ptr = eval(ptr);
	obj = mkrecord(fRECORD,argStkPtr-n,n);
	ARGnpop(n);
	return(obj);
}
/*--------------------------------------------------------------------*/
PRIVATE truc Srecfield()
{
	truc *ptr;
	truc field;

	field = *ARG1PTR(evalStkPtr);
	ARGpush(*ARG0PTR(evalStkPtr));
	*argStkPtr = eval(argStkPtr);
	ptr = recfield(argStkPtr,field);
	ARGpop();
	if(ptr == NULL) {
		return(brkerr());
	}
	else {
		return(*ptr);
	}
}
/*--------------------------------------------------------------------*/
PRIVATE truc Sderef()
{
	truc *ptr;
	truc obj;

	obj = eval(ARG0PTR(evalStkPtr));
	ARGpush(obj);
	if(*FLAGPTR(argStkPtr) != fPOINTER) {
		error(derefsym,err_vpoint,voidsym);
		obj = brkerr();
	}
	else {
		ptr = TAddress(argStkPtr);
		obj = ptr[2];
		if(obj == nil) {
		    error(derefsym,err_nil,voidsym);
		    obj = brkerr();
		}
	}
	ARGpop();
	return(obj);
}
/*--------------------------------------------------------------------*/
/*
** Argument ist muss eine Pointer-Variable in *evalStkPtr sein
*/
PRIVATE truc Snew()
{
	return(pnew10(ARG0PTR(evalStkPtr),1));
}
/*--------------------------------------------------------------------*/
PUBLIC truc Pdispose(ptr)
truc *ptr;
{
	return(pnew10(ptr,0));
}
/*--------------------------------------------------------------------*/
PRIVATE truc pnew10(ptr,mode)
truc *ptr;
int mode;	/* mode = 1: new, mode = 0: dispose */
{
	truc *vptr;
	truc obj, symb;

	if(Paddr(ptr,&vptr) == aERROR) {
		symb = (mode ? mknewsym : nil);
		error(symb,err_vpoint,voidsym);
		return(brkerr());
	}
	if(mode) {
		ptr = TAddress(vptr);
		ARGpush(ptr[1]);
		*argStkPtr = Fmkrec0();
		*vptr = mkcopy(vptr);
		obj = ARGretr();
	}
	else
		obj = nil;
	*PTARGETPTR(vptr) = obj;
	return(obj);
}
/*--------------------------------------------------------------------*/
/*
** Schreibt in das Feld field des Records *rptr den Eintrag obj
*/
PUBLIC truc recfassign(rptr,field,obj)
truc *rptr;
truc field, obj;
{
	truc *ptr;

	ptr = recfield(rptr,field);
	if(ptr == NULL) {
		return(brkerr());
	}
	*ptr = obj;
	return(obj);
}
/*--------------------------------------------------------------------*/
/*
** Zuweisung von obj an den Record *rptr
** Es wird vorausgesetzt, dass bereits gecheckt ist, dass *rptr
** tatsaechlich ein Record ist
** und dass obj eine Kopie des urspruenglichen Objects ist.
** Die Vertraeglichkeit der Feldtypen wird nicht geprueft
*/
PUBLIC truc fullrecassign(rptr,obj)
truc *rptr;
truc obj;
{
	struct record *ptr1, *ptr2;

	ptr1 = RECORDPTR(rptr);
	ptr2 = recordptr(obj);

	if(ptr2->flag != fRECORD || ptr2->len != ptr1->len) {
		error(assignsym,err_mism,voidsym);
		return(brkerr());
	}
	ptr2->recdef = ptr1->recdef;
	return(*rptr = obj);
}
/*--------------------------------------------------------------------*/
PRIVATE truc *recfield(rptr,field)
truc *rptr;
truc field;
{
	struct record *sptr;
	truc *ptr, *fptr;
	unsigned n, i;
	int flg;

	flg = *FLAGPTR(rptr);
	if(flg == fPOINTER) {
		sptr = RECORDPTR(rptr);
		if(sptr->field1 != nil) {
			rptr = &(sptr->field1);
			flg = *FLAGPTR(rptr);
		}
	}
	if(flg != fRECORD) {
		error(recordsym,"record variable expected",*rptr);
		return(NULL);
	}
	ptr = TAddress(rptr);
	fptr = TAddress(ptr+1);
	n = *WORD2PTR(fptr) / 2;
	fptr++;
	for(i=0; i<n; i++) {
		if(*fptr++ == field)
			return(ptr + i + 2);
	}
	error(recordsym,err_field,field);
	return(NULL);
}
/**********************************************************************/
