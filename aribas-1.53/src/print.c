/****************************************************************/
/* file print.c

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
** print.c
** functions for output formatting
**
** date of last change
** 1995-01-03: integer return for write and writeln
** 1995-01-26: NEEDFLUSH
** 1995-03-11: changed s1form, s2form
** 1995-03-28: fRECORD, fPOINTER
** 1995-04-08: ASCII 0 in strings
** 1995-07-16: fixed bug in left alignment of strings
** 1996-08-04: fixed bug in base 8 formatting
** 1996-11-03: renamed protocol() to transcript()
** 1997-04-13: reorg (newintsym)
** 1997-07-05: changed setcols (no special role of tstdout)
**	     error return value of write() and writeln() set to -1
** 1997-12-20: small change in fprintline
** 1998-10-01: adjustments for Win32GUI
** 2001-01-01: multiprec floats
** 2001-03-30: Win32GUI replaces genWinGUI
** 2001-06-02: function flushlog
** 2002-04-01: allow interrupt while printing vectors
** 2002-08-03: baseprefix changed,
**      changed argument type of s1form, s2form, sformaux
** 2003-02-21: Gformat also for gf2nint's
*/

#include "common.h"

#ifdef DjGPP
#define NEEDFLUSH
#endif

PUBLIC void iniprint	_((int cols));
PUBLIC int logout	_((int ch));
PUBLIC void strlogout	_((char *str));
PUBLIC void closelog	_((void));
PUBLIC void flushlog	_((void));
PUBLIC void tprint	_((truc strom, truc obj));
PUBLIC int strcopy	_((char *tostr, char *fromstr));
PUBLIC int strncopy	_((char *tostr, char *fromstr, int maxlen));
PUBLIC int fprintstr	_((truc strom, char *str));
PUBLIC void fprintline	_((truc strom, char *str));
PUBLIC void fnewline	_((truc strom));
PUBLIC void ffreshline	_((truc strom));
PUBLIC void flinepos0	_((truc strom));
PUBLIC int s1form	_((char *buf, char *fmt, wtruc dat));
PUBLIC int s2form	_((char *buf, char *fmt, wtruc dat1, wtruc dat2));
PUBLIC int setprnprec _((int prec));
PUBLIC truc writesym, writlnsym, formatsym;
PUBLIC truc transcsym;

PUBLIC char OutBuf[IOBUFSIZE+4];
PUBLIC int Log_on = 0;

/*----------------------------------------------------*/
#define FORMPARAM	5
#define NOTSET	   -32768
#define DEFAULT		0

typedef struct {
	int	mode;
	int	param[FORMPARAM];
} forminfo;

PRIVATE ifun putcfun	_((truc strom));
PRIVATE int charout	_((int ch));
PRIVATE int log2out	_((int ch));
PRIVATE truc Ftranscript	_((int argn));
PRIVATE truc F1write	_((int argn));
PRIVATE truc Fwritln	_((int argn));
PRIVATE truc Fint2str	_((int argn));
PRIVATE truc Fflt2str	_((void));
PRIVATE truc Secvt	_((void));
PRIVATE truc Gformat	_((void));
PRIVATE int Gprint	_((int argn, int nl));
PRIVATE int setcols	_((truc strom, truc *ptr));
PRIVATE void getform	_((forminfo *fptr, truc obj, truc *arr, int n));
PRIVATE truc Fsetpbase	_((void));
PRIVATE truc Fgetpbase	_((void));
PRIVATE int admissbase	_((int base));
PRIVATE int printbase	_((int base));
PRIVATE char *baseprefix   _((int base, int mode));
PRIVATE int nibasci0	   _((word2 *x, int k));
PRIVATE void printfint	   _((truc strom, truc obj, forminfo *fptr));
PRIVATE void printfloat	   _((truc strom, truc obj, forminfo *fptr));
PRIVATE int printvector	   _((truc strom, truc obj));
PRIVATE int printrecord	   _((truc strom, truc obj));
PRIVATE int printvvrr	   _((truc strom, truc obj, int flg));
PRIVATE void printstring   _((truc strom, truc obj, forminfo *fptr));
PRIVATE void printbstring  _((truc strom, truc obj, forminfo *fptr));
PRIVATE int bytes2hex	_((char *str, byte *buf, int len));
PRIVATE int sym2str	_((truc obj, char *buf));
PRIVATE int float2str	_((truc obj, char *buf, forminfo *fptr, word2 *hilf));
PRIVATE int fixstring	_((numdata *nptr, int dec, char *buf));
PRIVATE int char2str	_((truc obj, char *buf, forminfo *fptr));
PRIVATE int bool2str	_((truc obj, char *buf));
PRIVATE word4 truc2msf	_((truc obj));
PRIVATE int ptr2str	_((truc obj, char *buf));
PRIVATE int obj2str	_((int flg, truc obj, char *buf));
PRIVATE int leftpad	_((char *buf, int width, int ch));
PRIVATE int fillspaces	_((char *buf, int n));
PRIVATE void strnfcopy	_((char *tostr, char *fromstr, unsigned len));
PRIVATE int fprintwrap	_((truc strom, char *str, int bound, int contmark));
PRIVATE int fprintmarg	_((truc strom, char *str, int len));
PRIVATE int fprintch	_((truc strom, int ch));
PRIVATE int long2alfa	_((char *buf, long u));
PRIVATE int long2s0alfa	 _((char *buf, long u, int len));
PRIVATE int word4xalfa	_((char *buf, word4 u));
PRIVATE int sformaux	_((forminfo *fmptr, char *buf, wtruc dat));
PRIVATE char *formscan	_((char *str, forminfo *fmptr));
PRIVATE int isformdir	_((int ch));

PRIVATE truc basesym, groupsym, columsym, digssym;
PRIVATE truc setpbsym, getpbsym;
PRIVATE truc itoasym, ftoasym, ecvtsym;


PRIVATE int quotemode = 1;

PRIVATE FILE *outfile;
PRIVATE FILE *logfile;

PRIVATE int MaxCols;
PRIVATE int PrintCols;
PRIVATE int PrintPrec;

#define DECprec(prec)	((prec)*95 - 10)/20
/*------------------------------------------------------------------*/
PUBLIC void iniprint(cols)
int cols;
{
	truc write_sym, writln_sym;

	if(cols >= MAXCOLS/2 && cols <= 2*MAXCOLS)
		MaxCols = cols;
	else
		MaxCols = MAXCOLS;
	PrintCols = MaxCols-1;
	PrintPrec = deffltprec();

	write_sym = new0symsig("write",sFBINARY,(wtruc)F1write, s_1u);
	writesym  = newsym("write",   sPARSAUX, write_sym);
	writln_sym= new0symsig("writeln",sFBINARY,(wtruc)Fwritln,s_0u);
	writlnsym = newsym("writeln",	sPARSAUX, writln_sym);

	basesym	  = newsym("base",	sUNBOUND, nullsym);
	groupsym  = newsym("group",	sUNBOUND, nullsym);
	columsym  = newsym("columns",	sUNBOUND, nullsym);
	digssym	  = newsym("digits",	sUNBOUND, nullsym);

	formatsym = newintsym("", sFBINARY, (wtruc)Gformat);
	setpbsym  = newsymsig("set_printbase",sFBINARY,(wtruc)Fsetpbase,s_ii);
	getpbsym  = newsymsig("get_printbase",sFBINARY,(wtruc)Fgetpbase,s_0);
	transcsym = newsymsig("transcript", sFBINARY,(wtruc)Ftranscript, s_01);
	itoasym	  = newsymsig("itoa",	   sFBINARY, (wtruc)Fint2str, s_12);
	ftoasym	  = newsymsig("ftoa",	   sFBINARY, (wtruc)Fflt2str, s_1);
	ecvtsym	  = newsymsig("float_ecvt",sSBINARY,(wtruc)Secvt, s_4);
}
/*------------------------------------------------------------------*/
PRIVATE ifun putcfun(strom)
truc strom;
{
	outfile = STREAMfile(strom);
	if(Log_on && (strom == tstdout || strom == tstderr))
		return(log2out);
	else {
#ifdef genWinGUI
        if ((strom == tstdout || strom == tstderr))
            return(wincharout);
        else
#endif
		return(charout);
    }
}
/*------------------------------------------------------------------*/
#ifdef NEEDFLUSH
/*
** there were problems with one version the DOS-Port (D.J. Delorie)
** of the Gnu-Compiler (ver 2.5.7)
** which necessitated a fflush
*/
/*------------------------------------------------------------------*/
PRIVATE int charout(ch)
int ch;
{
	int ret = putc(ch,outfile);

	fflush(outfile);
	return ret;
}
/*------------------------------------------------------------------*/
PUBLIC int logout(ch)
int ch;
{
	int ret = putc(ch,logfile);

	fflush(logfile);
	return ret;
}
/*------------------------------------------------------------------*/
PRIVATE int log2out(ch)
int ch;
{
	int ret;

	putc(ch,logfile);
	fflush(logfile);
	ret = putc(ch,stdout);
	fflush(stdout);
	return ret;
}
/*-------------------------------------------------------------------*/
#else	/* #ifndef NEEDFLUSH */
/*------------------------------------------------------------------*/
PRIVATE int charout(ch)
int ch;
{
	return putc(ch,outfile);
}
/*------------------------------------------------------------------*/
PUBLIC int logout(ch)
int ch;
{
	return putc(ch,logfile);
}
/*------------------------------------------------------------------*/
PRIVATE int log2out(ch)
int ch;
{
	putc(ch,logfile);
#ifdef genWinGUI
    return wincharout(ch);
#else
	return putc(ch,stdout);
#endif
}
/*-------------------------------------------------------------------*/
#endif	/* #ifndef NEEDFLUSH */

/*-------------------------------------------------------------------*/
PUBLIC void strlogout(str)
char *str;
{
	int ch;

	while((ch = *str++))
		logout(ch);
}
/*------------------------------------------------------------------*/
PRIVATE truc Ftranscript(argn)
int argn;
{
	char name[84];
	char *logname = "aribas";
	char *extens = ".log";
	char *str;
	int strerr;

	if(argn == 1 && *argStkPtr == zero) {
		closelog();
		return(zero);
	}
	if(argn == 1) {
		if(*FLAGPTR(argStkPtr) == fSTRING) {
			str = STRINGPTR(argStkPtr);
			strerr = (str[0] ? 0 : 1);
		}
		else
			strerr = 1;
		if(strerr) {
			error(transcsym,err_str,*argStkPtr);
			return(brkerr());
		}
	}
	else
		str = logname;
	fnextens(str,name,extens);
	logfile = fopen(name,"w");
	if(logfile == NULL) {
		error(transcsym,err_open,scratch(name));
		return(false);
	}
	else {
		Log_on = 1;
		return(true);
	}
}
/*-------------------------------------------------------------------*/
PUBLIC void flushlog()
{
	if(Log_on)
		fflush(logfile);
}
/*------------------------------------------------------------------*/
PUBLIC void closelog()
{
	if(Log_on) {
		Log_on = 0;
		fclose(logfile);
	}
}
/*------------------------------------------------------------------*/
PRIVATE truc F1write(argn)  /* to avoid name clash with system function */
int argn;
{
	int n = Gprint(argn,0);
	return(mksfixnum(n));
}
/*------------------------------------------------------------------*/
PRIVATE truc Fwritln(argn)
int argn;
{
	int n = Gprint(argn,1);
	return(mksfixnum(n));
}
/*------------------------------------------------------------------*/
#define MAXINT2STRLEN	4000
PRIVATE truc Fint2str(argn)
int argn;
{
	truc *argptr;
	word2 *x, *y;
	truc strobj;
	char *cpt;
	long nn;
	int flg, len, len1, sign, base;
	int bpd, dig;
	int errflg = 0;

	argptr = argStkPtr - argn + 1;

	flg = *FLAGPTR(argptr);
	if(flg != fFIXNUM && flg != fBIGNUM) {
		error(itoasym,err_int,*argptr);
		return(brkerr());
	}
	if(argn == 2) {
		if(*FLAGPTR(argStkPtr) != fFIXNUM)
			errflg = 1;
		base = *WORD2PTR(argStkPtr);
		if(base != 10 && base != 16 && base != 2 && base != 8)
			errflg = 1;
		if(errflg) {
			error(itoasym,err_pbase,*argStkPtr);
			return(brkerr());
		}
	}
	else
		base = 10;
	x = AriBuf;
	y = (base == 10 ? AriScratch : x);
	len = bigretr(argptr,y,&sign);
	if(len == 0)
		return(mkstr("0"));
	if(len > MAXINT2STRLEN) {
		error(itoasym,err_2big,voidsym);
		return(brkerr());
	}
	if(base == 10) {
		len = big2bcd(y,len,x);
		len = (len + 3) >> 2;
	}
	if(base >= 10)		/* base == 16 || base == 10 */
		bpd = 4;
	else if(base == 8) {
		x[len] = 0;	/* !!! */
		bpd = 3;
	}
	else			/* base == 2 */
		bpd = 1;
	nn = bit_length(x,len);
	nn = (nn + bpd - 1)/bpd;	/* Anzahl der Ziffern */
	len1 = (sign ? nn+1 : nn);
	strobj = mkstr0(len1);
	cpt = STRING(strobj);
	if(sign)
		*cpt++ = '-';
	while(--nn >= 0) {
		dig = nibndigit(bpd,x,nn);
		*cpt++ = hexascii(dig);
	}
	return(strobj);
}
#undef MAXINT2STRLEN
/*------------------------------------------------------------------*/
PRIVATE truc Fflt2str()
{
	forminfo fmt;
	int prec, flg;
	char *out;

	flg = chknum(ftoasym,argStkPtr);
	if(flg == aERROR)
		return(brkerr());
	prec = fltprec(flg);
	fmt.mode = 'G';
	fmt.param[0] = 0;
	fmt.param[1] = DECprec(prec);
	out = (char *)AriBuf;
	float2str(*argStkPtr,out,&fmt,AriScratch);
	return(mkstr(out));
}
/*------------------------------------------------------------------*/
PRIVATE truc Secvt()
{
	numdata acc;
	truc res;
	word2 *x;
	char *cpt ;
	int k, len, digs, digsmax, decpos, sign, flg;
	int errflg = 0;

	acc.digits = x = AriBuf;

	res = eval(ARGNPTR(evalStkPtr,1));
	ARGpush(res);
	res = eval(ARGNPTR(evalStkPtr,2));
	ARGpush(res);
	flg = *FLAGPTR(argStkPtr-1);
	if(flg < fFIXNUM) {
		error(ecvtsym,err_num,argStkPtr[-1]);
		errflg = 1;
		goto cleanup;
	}
	flg = *FLAGPTR(argStkPtr);
	if(flg != fFIXNUM) {
		error(ecvtsym,err_int,*argStkPtr);
		errflg = 1;
		goto cleanup;
	}
	digs = *WORD2PTR(argStkPtr);
	if(digs < 2)
		digs = 2;
	else if(digs > (digsmax = FltPrec[MaxFltLevel] * 5))
		digs = digsmax;
	len = float2bcd(digs,argStkPtr-1,&acc,AriScratch);
	sign = (acc.sign ? -1 : 0);
	decpos = (len ? len + acc.expo : 0);
	res = mkstr0(digs);
	cpt = STRING(res);
	for(k=len-1; k>=0; k--)
		*cpt++ = nibascii(x,k);
	for(k=digs-len; k>0; k--)
		*cpt++ = '0';
	Lvalassign(ARGNPTR(evalStkPtr,3),mksfixnum(decpos));
	Lvalassign(ARGNPTR(evalStkPtr,4),mksfixnum(sign));
  cleanup:
	if(errflg)
		res = brkerr();
	ARGnpop(2);
	return(res);
}
/*------------------------------------------------------------------*/
/*
** Auswertung der Format-Anweisung bei write oder writeln
** argStkPtr[-1]: zu schreibendes Objekt,
** argStkPtr[0]: Tupel mit Formatangaben
** Zurueckgegeben wird ein Tupel, dessen 0-te Komponente
** das Objekt und die weiteren Komponenten die ausgewerteten
** Formatangaben sind. Falls die Formatangaben nicht
** sinnvoll sind, wird nur das Objekt zurueckgegeben.
*/
PRIVATE truc Gformat()
{
	truc *arr, *ptr;
	truc obj, grp, bas, wid, dig;
	int i, m, n;
	int flg;

	flg = *FLAGPTR(argStkPtr-1);
	arr = workStkPtr + 1;
	WORKpush(argStkPtr[-1]);
	ptr = VECTORPTR(argStkPtr);
	m = *VECLENPTR(argStkPtr);
	for(i=0; i<m; i++)
		WORKpush(ptr[i]);
	if(flg >= fFLTOBJ) {
		n = (m > 1 ? 3 : 2);
		for(i=1; i<n; i++) {
			arr[i] = eval(arr+i);
			if(*FLAGPTR(arr+i) != fFIXNUM) {
				n = 0;
				break;
			}
		}
	}
	else if((flg >= fINTTYPE0 && flg <= fINTTYPE1) || flg == fBYTESTRING) {
	    if(m < 4)
		    for(i=m; i<4; i++)
			    WORKpush(zero);
	    bas = wid = dig = zero;
	    grp = nullsym;
	    n = 2;
	    for(i=1; i<=m; i++) {
		if(*FLAGPTR(arr+i) == fFUNCALL) {
		    ptr = TAddress(arr+i);
		    if(*ptr == basesym) {
				if(n < 3) n = 3;
				if(ptr[1] == constone) {
					obj = eval(ptr+2);
					if(Tflag(obj) == fFIXNUM)
						bas = obj;
				}
				continue;
		    }
		    else if(*ptr == groupsym) {
				if(n < 4) n = 4;
				if(ptr[1] == constone) {
					obj = eval(ptr+2);
					if(Tflag(obj) == fFIXNUM)
						grp = obj;
				}
				continue;
		    }
		    else if(*ptr == digssym) {
				if(n < 5) n = 5;
				if(ptr[1] == constone) {
					obj = eval(ptr+2);
					if(Tflag(obj) == fFIXNUM)
						dig = obj;
				}
				continue;
		    }
		}
		obj = eval(arr+i);
		if(Tflag(obj) == fFIXNUM)
			wid = obj;
	    }
	    arr[1] = wid;
	    arr[2] = bas;
	    arr[3] = grp;
	    arr[4] = dig;
	}
	else if(flg == fSTRING || flg == fCHARACTER) {
	/* fehlt Analyse von escape Anweisungen */
		arr[1] = eval(arr+1);
		if(*FLAGPTR(arr+1) != fFIXNUM)
		    n = 0;
		else
		    n = 2;
	}
	else if(flg == fSTREAM) {
	    n = 0;
		if(*FLAGPTR(arr+1) == fFUNCALL) {
			ptr = TAddress(arr+1);
			if(*ptr == columsym) {
		    	if(ptr[1] == constone) {
					obj = eval(ptr+2);
					if(Tflag(obj) == fFIXNUM) {
						arr[1] = obj;
						n = 2;
					}
		    	}
			}
	    }
	}
	else
		n = 0;
	obj = (n ? mkntuple(fTUPLE,arr,n) : arr[0]);
	workStkPtr = arr - 1;
	return(obj);
}
/*------------------------------------------------------------------*/
PRIVATE int Gprint(argn,nl)
int argn;
int nl;
{
	truc *ptr;
	truc strom;
	truc obj;
	int changecols = 0;
	int savemode, flg;
	int i;

	strom = tstdout;
	if(argn > 0) {
		ptr = argStkPtr - argn + 1;
		flg = *FLAGPTR(ptr);
		if(flg == fTUPLE) {
		    obj = *ptr;
		    if(*VECLENPTR(ptr) >= 2) {
				ptr = VECTOR(obj);
				flg = *FLAGPTR(ptr);
				changecols = 1;
		    }
		}
		if(flg == fSTREAM) {
			if(!isoutfile(ptr,aTEXT)) {
				error(writesym,err_tout,voidsym);
				return(-1);
			}
			else {
				argn--;
				strom = *ptr;
			}
			if(changecols) {
				PrintCols = setcols(strom,ptr+1);
			}
		}
	}
	savemode = quotemode;
	quotemode = 0;
	for(i=-argn+1; i<=0; i++) {
		tprint(strom,argStkPtr[i]);
	}
	if(nl)
		fnewline(strom);
	quotemode = savemode;
	PrintCols = MaxCols-1;
	return(argn);
}
/*------------------------------------------------------------------*/
PRIVATE int setcols(strom,ptr)
truc strom;
truc *ptr;
{
	unsigned k;


	if(*FLAGPTR(ptr) != fFIXNUM)
		return(MaxCols-1);
	k = *WORD2PTR(ptr);
	if(k < MAXCOLS/4)
		k = MAXCOLS/4;
	else if(k > IOBUFSIZE)
		k = IOBUFSIZE;
	return(k);
}
/*------------------------------------------------------------------*/
PUBLIC void tprint(strom,obj)
truc strom, obj;
{
	forminfo fmt;
	truc *ptr;
	int m, flg, len;

	flg = Tflag(obj);
	if(flg == fTUPLE) {		/* Format-Angaben */
		ptr = VECTOR(obj);
		m = VEClen(obj);
		flg = *FLAGPTR(ptr);
		obj = ptr[0];
		getform(&fmt,obj,ptr+1,m-1);
	}
	else {				/* default format */
		getform(&fmt,obj,NULL,0);
	}

	if(flg == fSYMBOL)
		len = sym2str(obj,OutBuf);
	else if(flg == fBIGNUM || flg == fFIXNUM) {
		printfint(strom,obj,&fmt);
		return;
	}
	else if(flg == fGF2NINT) {
		printfint(strom,obj,&fmt);
		return;
	}
	else if(flg >= fFLTOBJ) {
		printfloat(strom,obj,&fmt);
		return;
	}
	else if(flg == fCHARACTER) {
		len = char2str(obj,OutBuf,&fmt);
	}
	else if(flg == fBOOL) {
		len = bool2str(obj,OutBuf);
	}
	else if(flg == fSTRING) {
		printstring(strom,obj,&fmt);
		return;
	}
	else if(flg == fBYTESTRING) {
		printbstring(strom,obj,&fmt);
		return;
	}
	else if(flg == fVECTOR) {
		printvector(strom,obj);
		return;
	}
	else if(flg == fRECORD) {
		printrecord(strom,obj);
		return;
	}
	else if(flg == fPOINTER) {
		len = ptr2str(obj,OutBuf);
	}
	else
		len = obj2str(flg,obj,OutBuf);

	fprintmarg(strom,OutBuf,len);
}
/*--------------------------------------------------------------*/
PRIVATE void getform(fptr,obj,arr,n)
forminfo *fptr;
truc obj;
truc *arr;
int n;
{
	int x;
	int prec, len, base, group, digs;
	int flg = Tflag(obj);

	if(n >= 1) {
		x = *WORD2PTR(arr);
		if(x > PrintCols-1)
			x = PrintCols - 1;
		if(*SIGNPTR(arr))
			x = -x;
		fptr->param[0] = x;	/* Gesamt-Breite */
	}
	else {
		fptr->param[0] = 0;	/* default */
	}

	if(flg >= fFLTOBJ) {
		prec = fltprec(flg);
		if(fptr->param[0] == 0 && PrintPrec && prec > PrintPrec)
			prec = PrintPrec;
		if(n <= 1) {
			fptr->mode = (n==0 ? 'G' : 'E');
			fptr->param[1] = DECprec(prec);
		}
		else {
			fptr->mode = 'F';
			fptr->param[2] = DECprec(prec);
			x = *WORD2PTR(arr+1);
			if(x < 1)
				x = 1;
			else if(x > FltPrec[MaxFltLevel]*5)
				x = FltPrec[MaxFltLevel]*5;
			fptr->param[1] = x;
			/* Stellen nach dem Dezimalpunkt */
		}
	}
	else if(flg == fFIXNUM || flg == fBIGNUM || flg == fGF2NINT) {
		if(n <= 1)
			base = printbase(0);
		else if(n >= 2) {
			base = *WORD2PTR(arr+1);
			base = admissbase(base);
		}
        if(flg == fGF2NINT) {
            if(base == 10)
                base = 16;
        }
		if(n < 3 || arr[2] == nullsym) {
            if(flg == fBIGNUM || flg == fGF2NINT)
                len = VEClen(obj);
            else
		        len = 1;
		    switch(base) {
		    case 16:
			    group = (len > 2 ? 4 : 8);
			    break;
		    case 8:
			    group = 5;
			    break;
		    case 2:
			    group = 8;
			    break;
		    default:	/* base == 10 */
			    group = (len > 2 ? 5 : 10);
			    break;
		    }
		}
		else
		    group = *WORD2PTR(arr+2);

		if(n >= 4)
			digs = *WORD2PTR(arr+3) & 0x7FFF;
		else
			digs = 0;
		fptr->param[1] = base;
		fptr->param[2] = group;
		fptr->param[3] = digs;
	}
	else if(flg == fBYTESTRING) {
		if(n < 3 || arr[2] == nullsym)
			group = 4;
		else
			group = *WORD2PTR(arr+2);
		fptr->param[2] = group;
	}
	else {
		fptr->mode = flg;
		if(n > 1)		/* vorlaeufig */
			n = 1;
		fptr->param[n] = DEFAULT;
	}
}
/*--------------------------------------------------------------*/
PRIVATE truc Fsetpbase()
{
	int flg;
	int base;
	int errflg = 0;

	flg = *FLAGPTR(argStkPtr);
	if(flg != fFIXNUM)
		errflg = 1;
	else {
		base = *WORD2PTR(argStkPtr);
		if(base == 16 || base == 10 || base == 8 || base == 2)
			printbase(base);
		else {
			errflg = 1;
		}
	}
	if(errflg) {
		base = printbase(0);
		error(setpbsym,err_pbase,*argStkPtr);
	}
	return(mkfixnum(base));
}
/*--------------------------------------------------------------*/
PRIVATE truc Fgetpbase()
{
	return(mkfixnum(printbase(0)));
}
/*--------------------------------------------------------------*/
PRIVATE int admissbase(base)
int base;
{
	switch(base) {
	case 2:
	case 8:
	case 10:
	case 16:
		return(base);
	default:
		return(printbase(0));
	}
}
/*--------------------------------------------------------------*/
PRIVATE int printbase(base)
int base;
{
	static int pbase = 10;

	switch(base) {
	case 2:
	case 8:
	case 10:
	case 16:
		pbase = base;
		return(base);
	default:
		return(pbase);
	}
}
/*--------------------------------------------------------------*/
PUBLIC int setprnprec(prec)
int prec;
{
	int prec1;

	if(prec < 0)
		return(PrintPrec);
	/* else */
	prec1 = maxfltprec();
	if(prec > prec1)
		prec = prec1;
	return (PrintPrec = prec);
}
/*--------------------------------------------------------------*/
PRIVATE char *baseprefix(base, mode)
int base, mode;
{
    static char pref[4];

	switch(base) {
	case 16:
		strcopy(pref,"0x");
        break;
	case 8:
		strcopy(pref,"0o");
        break;
	case 2:
		strcopy(pref,"0y");
        break;
	default:
		pref[0] = '\0';
	}
    if(mode == 2 && strlen(pref) > 1)
        pref[0] = '2';
    return pref;
}
/*--------------------------------------------------------------*/
/*
** Formatierte Ausgabe von Integern
*/
PRIVATE void printfint(strom,obj,fptr)
truc strom, obj;
forminfo *fptr;
{
	truc big;
	word2 *x;
	char *cpt;
	long nn, NN, nn1, nn2, pp, m;
	int sign, len, base, width, grp, noofdigs;
	int k, n, diff, bpd;
	int dig;
	int rightpad;
    int mode;

	big = obj;
	len = bigretr(&big,AriBuf,&sign);
	base = fptr->param[1];
	if(base == 10) {
		len = big2bcd(AriBuf,len,AriScratch);
		len = (len + 3)>>2;
		x = AriScratch;
	}
	else {
		x = AriBuf;
	}
	width = fptr->param[0];
	grp = fptr->param[2];
	if(grp == 0)
		grp = 1;
	else if(grp == 1)
		grp = 2;
	else if(grp > PrintCols-1)
		grp = PrintCols-1;

	noofdigs = fptr->param[3];

	if(len == 0 && noofdigs == 0)
		noofdigs = 1;

	if(base >= 10) {
		bpd = 4;
    }
	else if(base == 8) {
		x[len] = 0;	/* !!! */
		bpd = 3;
	}
	else
		bpd = 1;

	nn = bit_length(x,len);
	nn = (nn + bpd - 1)/bpd;  /* Anzahl der Ziffern ohne leading 0 */
	NN = (noofdigs > nn ? noofdigs : nn);
	pp = (NN + grp - 1)/grp;  /* Zahl der Bloecke */
	k = NN - (pp-1)*grp;	  /* Zahl der Ziffern im 1.Block */
	n = (sign ? strcopy(OutBuf,"-") : 0);
	if(quotemode) {
        mode = (FLAG(big) == fGF2NINT ? 2 : 0);
		n += strcopy(OutBuf+n,baseprefix(base,mode));
    }
	m = NN + n + (grp==1 ? 0 : (pp-1));

	rightpad = 0;
	if(width > 0 && m < width) {
		diff = width - m;
		OutBuf[n] = 0;
		n = leftpad(OutBuf,n+diff,' ');
	}
	else if(width < 0 && m < -width) {
		diff = -width - m;
		rightpad = 1;
	}
	while(--pp >= 0) {
		cpt = OutBuf + n;
		nn2 = pp * grp;
		for(nn1 = nn2+k-1; nn1 >= nn2; nn1--) {
			if(nn1 >= nn)
				dig = 0;
			else
				dig = nibndigit(bpd,x,nn1);
			*cpt++ = hexascii(dig);
		}
		if(pp > 0 && grp > 1)
			*cpt++ = '_';
		*cpt = 0;
		n = cpt - OutBuf;
		fprintmarg(strom,OutBuf,n);
		n = 0; k = grp;
	}
	if(rightpad) {
		fillspaces(OutBuf,diff);
		fprintstr(strom,OutBuf);
	}
}
/*--------------------------------------------------------------*/
PRIVATE int nibasci0(x,k)
word2 *x;
int k;
{
	if(k >= 0)
		return nibascii(x,k);
	else
		return '0';
}
/*--------------------------------------------------------------*/
/*
** prints float obj
** uses AriBuf and ScratchBuf
*/
PRIVATE void printfloat(strom,obj,fptr)
truc strom, obj;
forminfo *fptr;
{
	numdata acc;
	truc fltnum;
	word2 *x;
	char *cpt;
	long decpos, expo;
	int k, len, digs, dec, width, sign, mode;
	int eflg;
	int grp=5;

	mode = fptr->mode;
	width = fptr->param[0];
	if(mode == 'G' || mode == 'E') {
		digs = fptr->param[1];
	}
	else {		/* mode == 'F' */
		dec = fptr->param[1];
		digs = fptr->param[2];
	}
	if(width > 0 && width <= PrintCols-1) {	/* vorlaeufig */
		len = float2str(obj,OutBuf,fptr,AriScratch);
		fprintmarg(strom,OutBuf,len);
		return;
	}	
	/* else */
	acc.digits = x = AriBuf;
	fltnum = obj;
	len = float2bcd(digs,&fltnum,&acc,AriScratch);
	sign = (acc.sign ? -1 : 0);
	decpos = (len ? len + acc.expo : 0);
	if(decpos <= grp && decpos > -grp) {
		eflg = 0;
	}
	else {
		eflg = 1;
		expo = decpos - 1;
		decpos = 1;
	}

	if(digs < 20)	/* vorlaeufig */
		grp = 20;
	/* print head */
	cpt = OutBuf; 
	if(sign)
		*cpt++ = '-';
	if(decpos <= 0) {
		cpt += strcopy(cpt,"0.");
		for(k=0; k>=decpos+1; k--)
			*cpt++ = '0';
		for(k=decpos; (k>-grp) && (--digs>=0); k--)
			*cpt++ = nibasci0(x,--len);
	}
	else {	/* decpos > 0 */
		for(k=decpos; (k>0) && (--digs>=0); k--)
			*cpt++ = nibasci0(x,--len);
		*cpt++ = '.';
		for(k=0; (k<grp) && (--digs>=0); k++)
			*cpt++ = nibasci0(x,--len);
	}
	if(digs > 0)
		*cpt++ = '_';
	*cpt = 0;
	fprintmarg(strom,OutBuf,strlen(OutBuf));
	/* print next groups */
	while(digs > 0) {
		cpt = OutBuf;
		for(k=0; (k<grp) && (--digs>=0); k++)
			*cpt++ = nibasci0(x,--len);
		if(digs > 0)
			*cpt++ = '_'; 
		*cpt = 0;
		fprintmarg(strom,OutBuf,strlen(OutBuf));
	}
	if(eflg) {
		k = s1form(OutBuf,"e~D",(wtruc)expo);
		fprintmarg(strom,OutBuf,k);
	}
}
/*--------------------------------------------------------------*/
PRIVATE int printvector(strom,obj)
truc strom, obj;
{
	return(printvvrr(strom,obj,fVECTOR));
}
/*--------------------------------------------------------------*/
PRIVATE int printrecord(strom,obj)
truc strom, obj;
{
	return(printvvrr(strom,obj,fRECORD));
}
/*--------------------------------------------------------------*/
/*
** Ausgabe von Arrays und Records
*/
PRIVATE int printvvrr(strom,obj,flg)
truc strom, obj;
int flg;
{
	static char *brace[] = {"(","{","&("};
	static char *closebrace[] = {")","}",")"};
	truc *ptr;
	int braceno;
	int savemode;
	int n, len;
	int pos;

	savemode = quotemode;
	quotemode = 1;
	WORKpush(obj);
	n = len = *VECLENPTR(workStkPtr);
	ptr = VECTORPTR(workStkPtr);
	if(flg == fRECORD) {
		ptr++;
		braceno = 2;
	}
	else if(len == 1)
		braceno = 1;
	else
		braceno = 0;
	fprintmarg(strom,brace[braceno],5);
	while(--n >= 0) {
		tprint(strom,*ptr);
		if(n) {
			fprintch(strom,',');
			pos = STREAMpos(strom);
			if(pos >= 1 && pos <= PrintCols - 1)
				fprintch(strom,' ');
		}
        if((n & 0xF) == 0 && INTERRUPT) {
            setinterrupt(0);
            len = strcopy(OutBuf,"...user interrupt... ");
            fprintmarg(strom,OutBuf,len);
            break;
        }
		ptr++;
	}
	WORKpop();
	fprintmarg(strom,closebrace[braceno],2);
	quotemode = savemode;
	return(len);
}
/*--------------------------------------------------------------*/
PRIVATE void printstring(strom,obj,fptr)
truc strom, obj;
forminfo *fptr;
{
	struct strcell *strpt;
	char *s;
	unsigned len;
	int width, fill;
	int align;

	strpt = stringptr(obj);
	len = strpt->len;
	s = (char *)AriScratch;
	strnfcopy(s,&(strpt->ch0),len);
/**** fehlt Behandlung nichtdruckbarer Zeichen *****/

	width = fptr->param[0];
	if(width >= 0) {
		align = 1;
	}
	else {
		align = -1;
		width = -width;
	}
	if(width > len)
		fill = width - len;
	else
		align = 0;
	if(fill >= PrintCols)
		align = 0;
	if(align == 0) {
		fill = 0;
		width = len;
	}
	if(quotemode)		/* only in unformatted mode */
		width += 2;
	if(STREAMpos(strom) >= PrintCols-width)
		fnewline(strom);
	if(quotemode) {
		fprintch(strom,'"');
		fprintwrap(strom,s,PrintCols-1,'\\');
		fprintch(strom,'"');
	}
	else {
		if(align <= 0)
			fprintwrap(strom,s,PrintCols,0);
		if(align) {
			fillspaces(OutBuf,fill);
			fprintstr(strom,OutBuf);
		}
		if(align > 0)
			fprintwrap(strom,s,PrintCols,0);
	}
}
/*--------------------------------------------------------------*/
/*
** print byte string in hex format
*/
PRIVATE void printbstring(strom,obj,fptr)
truc strom,obj;
forminfo *fptr;
{
	struct strcell *strpt;
	byte *buf;
	size_t k = 0, len;
	int n = 0, m, grp;
	int weiter = 1;

	strpt = stringptr(obj);
	buf = (byte *)&(strpt->ch0);
	len = strpt->len;
	grp = fptr->param[2];
	if(grp == 1)
		grp = 2;
	else if(grp > PrintCols-1)
		grp = PrintCols-1;
	if(quotemode)
		n = strcopy(OutBuf,"$");
	while(weiter) {
		m = (grp ? grp/2 : 2);
		if(k+m > len)
			m = (k <= len ? len-k : 0);
		n += bytes2hex(OutBuf+n,buf+k,m);
		if(k+m < len) {
			if(grp)
			    n += strcopy(OutBuf+n,"_");
			k += m;
		}
		else
			weiter = 0;
		fprintmarg(strom,OutBuf,n);
		n = 0;
	}
}
/*--------------------------------------------------------------*/
PRIVATE int bytes2hex(str,buf,len)
char *str;
byte *buf;
int len;
{
	unsigned u;
	int i;

	for(i=0; i<len; i++) {
		u = buf[i];
		*str++ = hexascii((u >> 4) & 0x0F);
		*str++ = hexascii(u & 0x0F);
	}
	*str = 0;
	return(2*len);
}
/*--------------------------------------------------------------*/
/*
** kopiert fromstr nach tostr
** Rueckgabewert: Laenge des Strings
*/
PUBLIC int strcopy(tostr,fromstr)
char *tostr, *fromstr;
{
	int i=0;

	while((*tostr++ = *fromstr++))
		i++;
	return(i);
}
/*--------------------------------------------------------------*/
/*
** kopiert hoechstens maxlen characters von fromstr nach tostr
** und setzt in diesen den Endmarkierer '\0'
** Rueckgabewert: Laenge des neuen Strings
*/
PUBLIC int strncopy(tostr,fromstr,maxlen)
char *tostr, *fromstr;
int maxlen;
{
	int i=0;

	while((*tostr++ = *fromstr++)) {
		if(++i >= maxlen) {
			*tostr = 0;
			break;
		}
	}
	return(i);
}
/*--------------------------------------------------------------*/
/*
** kopiert len Zeichen von fromstr nach tostr, wobei jedes Nullbyte
** durch SPACE ersetzt wird. tostr wird durch Nullbyte abgeschlossen
*/
PRIVATE void strnfcopy(tostr,fromstr,len)
char *tostr, *fromstr;
unsigned len;
{
	int ch;

	while(len--) {
		ch = *fromstr++;
		*tostr++ = (ch ? ch : ' ');
	}
	*tostr = 0;
}
/*--------------------------------------------------------------*/
PRIVATE int sym2str(obj,buf)
truc obj;
char *buf;
{
	return(strncopy(buf,SYMname(obj),PrintCols-1));
}
/*--------------------------------------------------------------*/
PRIVATE int float2str(obj,buf,fptr,hilf)
truc obj;
char *buf;
forminfo *fptr;
word2 *hilf;
{
	numdata acc;
	truc fltnum;
	long  expo;
	word2 *scratch;
	int len,len1,n,fill;
	int mode, width, wd1, prec, prec1, dec;

	acc.digits = hilf;

	mode = fptr->mode;
	width = fptr->param[0];
	if(mode == 'G' || mode == 'E') {
		prec = fptr->param[1];
	}
	else {		/* mode == 'F' */
		dec = fptr->param[1];
		prec = fptr->param[2];
	}

	fltnum = obj;
	scratch = hilf + 4 + (prec/4);
	len = float2bcd(prec,&fltnum,&acc,scratch);
	if(mode == 'F') {
		len1 = len + acc.expo + dec + 1;
		if(len1 <= width || len1 <= prec-1) {
			prec1 = prec + dec + acc.expo;
			if(prec1 < prec)
				roundbcd(prec1,&acc);
			wd1 = fixstring(&acc,dec,buf);
			width = leftpad(buf,width,' ');
			return(width);
		}
		else {
			mode = 'G';
		}
	}
	if(mode == 'E') {
		if(width-8 >= prec) {
			dec = prec - 1;
		}
		else if(width >= 10) {
			dec = width - 9;
		}
		else {
			dec = 1;
			width = 10;
		}
		len = roundbcd(dec+1,&acc);
		expo = (len ? acc.expo + dec : 0);
		acc.expo = -dec;
		fill = width - dec - 9;
		if(acc.sign == 0)
			fill++;
		buf += fillspaces(buf,fill);
		wd1 = fixstring(&acc,dec,buf);
		buf[wd1] = 'e';
		n = long2s0alfa(buf+wd1+1,expo,5);
		return(fill+wd1+1+n);
	}
	if(len > 0) {	  /* mode == 'G' */
		if(acc.expo <= -1 && acc.expo >= -prec - 2) {
			dec = -acc.expo;
			n = fixstring(&acc,dec,buf);
		}
		else {
			expo = acc.expo + prec - 1;
			acc.expo = -prec + 1;
			n = fixstring(&acc,prec-1,buf);
			n += s1form(buf+n,"e~D",(wtruc)expo);
		}
	}
	else {
		n = strcopy(buf,"0.0");
	}
	return(n);
}
/*--------------------------------------------------------------*/
/*
** Schreibt die in *nptr gegebene Float-Zahl als fixed-point-string
** mit dec Dezimalstellen hinter dem Komma in buf.
** Rueckgabewert Laenge des Strings.
*/
PRIVATE int fixstring(nptr,dec,buf)
numdata *nptr;
int dec;
char *buf;
{
	word2 *x;
	char *cpt;
	int len, k, sh;

	cpt = buf;
	if(nptr->sign)
		*cpt++ = '-';
	len = nptr->len;

	x = nptr->digits;
	if(len > 0) {
		sh = dec + nptr->expo;
		len = shiftbcd(x,len,sh);
	}
	if(len <= dec) {	    /* '0' vor dem Dezimalpunkt */
		*cpt++ = '0';
		*cpt++ = '.';
		for(k=dec-1; k>=len; k--)
			*cpt++ = '0';
		for(k=len-1; k>=0; k--)
			*cpt++ = nibascii(x,k);
	}
	else {		/* len-dec Stellen vor dem Dezimalpunkt */
		for(k=len-1; k>=dec; k--)
			*cpt++ = nibascii(x,k);
		*cpt++ = '.';
		for(k=dec-1; k>=0; k--)
			*cpt++ = nibascii(x,k);
	}
	*cpt = 0;
	return(cpt-buf);
}
/*--------------------------------------------------------------*/
PRIVATE int char2str(obj,buf,fptr)
truc obj;
char *buf;
forminfo *fptr;
{
	variant v;
	int k, ch, len, width;

	v.xx = obj;
	ch = v.pp.ww;
	if(quotemode) {
		if(ch < ' ' || ch == 127) {
			len = s1form(buf,"chr(~D)",(wtruc)ch);
		}
		else {
			len = s1form(buf,"'~C'",(wtruc)ch);
		}
		return(len);
	}
	if(!ch)
		ch = ' ';
	width = fptr->param[0];
	if(width == 0)
		len = 1;
	else
		len = (width > 0 ? width : -width);
	fillspaces(buf,len);
	k = (width >= 0 ? len-1 : 0);
	buf[k] = ch;
	return(len);
}
/*--------------------------------------------------------------*/
PRIVATE int bool2str(obj,buf)
truc obj;
char *buf;
{
	obj = (obj == true ? truesym : falsesym);
	return(strcopy(buf,SYMname(obj)));
}
/*--------------------------------------------------------------*/
/*
** Verwandelt ein truc obj = (b0,b1: byte; ww: word2) in ein word4,
** so dass b0 das most significant byte wird
*/
PRIVATE word4 truc2msf(obj)
truc obj;
{
	variant v;
	word4 u;

	v.xx = (word4)obj;
	u = v.pp.b0;
	u = (u << 8)|v.pp.b1;
	u = (u << 16)|v.pp.ww;
	return(u);
}
/*--------------------------------------------------------------*/
PRIVATE int ptr2str(obj,buf)
truc obj;
char *buf;
{
	obj = PTRtarget(obj);
	if(obj == nil)
		return(strcopy(buf,SYMname(nil)));
	return(s1form(buf,"<PTR^~X>",(wtruc)truc2msf(obj)));
}
/*--------------------------------------------------------------*/
PRIVATE int obj2str(flg,obj,buf)
int flg;
truc obj;
char *buf;
{
	word4 u;
	char *str;

	u = truc2msf(obj);
	switch(flg) {
	case fSTACK:
		str = "STACK";
		break;
	case fFUNDEF:
		str = "FUNCTION";
		break;
	case fSTREAM:
		u = (word4)STREAMfile(obj);
		str = "STREAM";
		break;
	default:
		if(flg >= fSPECIAL1 && flg <= fBUILTINn) {
			str = "PROC";
		}
		else
			str = "OBJECT";
	}
	return(s2form(buf,"<~A:~X>",(wtruc)str,(wtruc)u));
}
/*--------------------------------------------------------------*/
PRIVATE int leftpad(buf,width,ch)
char *buf;
int width;
int ch;
{
	int i, diff;
	int len = strlen(buf);

	diff = width - len;
	if(diff <= 0)
		return(len);
	for(i=len; i>=0; i--)
		buf[diff+i] = buf[i];
	for(i=0; i<diff; i++)
		buf[i] = ch;
	return(width);
}
/*--------------------------------------------------------------*/
PRIVATE int fillspaces(buf,n)
char *buf;
int n;
{
	int i;

	for(i=0; i<n; i++)
		*buf++ = ' ';
	*buf = 0;
	return(n);
}
/*--------------------------------------------------------------*/
PUBLIC int fprintstr(strom,str)
truc strom;
char *str;
{
	return fprintwrap(strom,str,PrintCols,0);
}
/*--------------------------------------------------------------*/
PRIVATE int fprintwrap(strom,str,bound,contmark)
truc strom;
char *str;
int bound, contmark;
{
	ifun writech;
	int ch;
	int i, n;

	writech = putcfun(strom);
	i = STREAMpos(strom);
	n = 0;
	while((ch = *str++)) {
		writech(ch);
		n++;
		if(ch >= ' ') {
	    	i++;
		}
		else {
		    if((ch == EOL) || (ch == '\r'))
        		i = 0;
    		else if(ch == '\b')
        		i = (i > 0 ? i-1 : 0);
    		else if(ch == '\t')
        		i = (i | 0x3) + 1;
    		else if(ch)
        		i++;
		}
		if(i >= bound) {
			i = 0;
			if(contmark)
				writech(contmark);
			writech(EOL);
		}
	}
	STREAMpos(strom) = i;
	return(n);
}
/*--------------------------------------------------------------*/
PRIVATE int fprintmarg(strom,str,len)
truc strom;
char *str;
int len;
{
	if(STREAMpos(strom) >= PrintCols-len)
		fnewline(strom);
	return fprintwrap(strom,str,PrintCols,0);
}
/*--------------------------------------------------------------*/
PUBLIC void fprintline(strom,str)
truc strom;
char *str;
{
	fprintwrap(strom,str,PrintCols,0);
	fnewline(strom);
}
/*--------------------------------------------------------------*/
PUBLIC void fnewline(strom)
truc strom;
{
	ifun writech;

	writech = putcfun(strom);
	writech(EOL);
	STREAMpos(strom) = 0;
}
/*--------------------------------------------------------------*/
PUBLIC void ffreshline(strom)
truc strom;
{
	if(STREAMpos(strom) != 0)
		fnewline(strom);
}
/*--------------------------------------------------------------*/
PUBLIC void flinepos0(strom)
truc strom;
{
	STREAMpos(strom) = 0;
}
/*--------------------------------------------------------------*/
PRIVATE int fprintch(strom,ch)
truc strom;
int ch;
{
	ifun writech;

	writech = putcfun(strom);
	if(STREAMpos(strom) >= PrintCols || ch == EOL) {
		writech(EOL);
		STREAMpos(strom) = 0;
	}
	if(ch != EOL) {
		writech(ch);
		STREAMpos(strom) += 1;
	}
	return(STREAMpos(strom));
}
/*--------------------------------------------------------------*/
PRIVATE int long2alfa(buf,u)
char *buf;
long u;
{
	word2 x[2], y[3];
	int n, s = 0;

	if(u < 0) {
		u = - u;
		*buf++ = '-';
		s = 1;
	}
	n = long2big(u,x);
	n = big2bcd(x,n,y);
	return(bcd2str(y,n,buf) + s);
}
/*--------------------------------------------------------------*/
PRIVATE int long2s0alfa(buf,u,len)
char *buf;
long u;
int len;
{
	word2 x[2], y[3];
	int i,n;
	int m = 0, s = 0;

	if(u < 0) {
		u = - u;
		s = 1;
	}
	n = long2big(u,x);
	n = big2bcd(x,n,y);
	if(s || n < len) {
		*buf++ = (s ? '-' : '+');
		m++;
	}
	for(i=n+m; i<len; i++) {
		*buf++ = '0';
		m++;
	}
	if(n > 0)
		bcd2str(y,n,buf);
	else
		*buf = 0;
	return(n + m);
}
/*--------------------------------------------------------------*/
PRIVATE int word4xalfa(buf,u)
char *buf;
word4 u;
{
	word2 x[2];
	int n;

	n = long2big(u,x);
	return(big2xstr(x,n,buf));
}
/*--------------------------------------------------------------*/
PUBLIC int s1form(buf,fmt,dat)
char *buf;
char *fmt;
wtruc dat;
{
	return(s2form(buf,fmt,dat,(wtruc)0));
}
/*--------------------------------------------------------------*/
PUBLIC int s2form(buf,fmt,dat1,dat2)
char *buf;
char *fmt;
wtruc dat1, dat2;
{
	forminfo finf;
	char *fmt1;
	wtruc dat;
	int mode;
	int n = 0;
	int count = 0;

	while((fmt1 = formscan(fmt,&finf))) {
		mode = finf.mode;
		if(mode == 0) {
			n += strncopy(buf+n,fmt,finf.param[0]);
		}
		else if(mode == '%') {
			n += strcopy(buf+n,"\n");
		}
		else if(mode == aERROR) {
			buf[n] = 0;
			break;
		}
		else {
			if(++count > 2)
				break;
			dat = (count == 1 ? dat1 : dat2);
			n += sformaux(&finf,buf+n,dat);
		}
		fmt = fmt1;
	}
	return(n);
}
/*--------------------------------------------------------------*/
/*
** Unterstuetzte Optionen:
** ~A string, ~D long int, ~X long hex, ~C character
**
*/
PRIVATE int sformaux(fmptr,buf,dat)
forminfo *fmptr;
char *buf;
wtruc dat;
{
	char *ptr1, *ptr2;
	int i, len, width, mode;
    char fill;

    mode = fmptr->mode;
	switch(mode) {
		case 'A':
			ptr1 = (char *)dat;
			len = strcopy(buf,ptr1);
			break;
		case 'D':
			len = long2alfa(buf,(long)dat);
			break;
		case 'X':
			len = word4xalfa(buf,(word4)dat);
			break;
		case 'C':
			buf[0] = (char)dat;
			buf[1] = 0;
			len = 1;
			break;
		default:
			buf[0] = 0;
			return(0);
	}
	width = fmptr->param[0];
	if(width > len) {
        fill = fmptr->param[1];
		ptr2 = buf + width;
		ptr1 = buf + len;
		for(i=0; i<=len; i++, ptr1--, ptr2--)
			*ptr2 = *ptr1;
		for(i=len; i<width; i++, ptr2--)
			*ptr2 = fill;
		len = width;
	}
	return(len);
}
/*--------------------------------------------------------------*/
PRIVATE char *formscan(str,fmptr)
char *str;
forminfo *fmptr;
{
	int n, x;
	int sign, ch;

	if(*str == 0)
		return(NULL);
	for(n=0; n<FORMPARAM; n++)
		fmptr->param[n] = NOTSET;

	if(*str != '~') {
		fmptr->mode = 0;
		for(n=1; (ch=*++str); n++)
			if(ch == '~') break;
		fmptr->param[0] = n;
		return(str);
	}

	str++;
	ch = *str;
	if(ch == '0') {
        fmptr->param[1] = '0';
		str++;
    }
    else {
        fmptr->param[1] = ' ';
    }
	x = str2int(str,&n);
	fmptr->param[0] = x;
	str += n;

	if((ch = isformdir(*str))) {
		fmptr->mode = ch;
		return(str + 1);
	}
	else {
		fmptr->mode = aERROR;
		return(NULL);
	}
}
/*--------------------------------------------------------------*/
PRIVATE int isformdir(ch)
int ch;
{
	if(ch == 'A' || ch == 'C' || ch == 'D' || ch == 'X' || ch == '%')
		return(ch);
	else
		return(0);
}
/****************************************************************/
