/****************************************************************/
/* file file.c

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
** file.c
** routines for file i/o
**
** date of last change
** 1995-01-25:	ariExtens
** 1995-03-11:	loadaux
** 1997-01-24:	moved skiptolabel() from scanner.c to file.c
** 1997-02-03:	findarifile
** 1997-04-13:	changed skiptolabel, reorg (newintsym)
** 1998-12-27:  issepdir()
** 2001-06-02:	flush(transcript)
*/

#include "common.h"

PUBLIC void inifile	_((void));
PUBLIC int fnextens	_((char *str, char *name, char *extens));
PUBLIC int findarifile	_((char *name, char *buf));
PUBLIC int findfile	_((char *paths, char *fnam, char *buf));
PUBLIC int issepdir     _((int ch));
PUBLIC int isoutfile	_((truc *strom, int mode));
PUBLIC int isinpfile	_((truc *strom, int mode));
PUBLIC int loadaux	_((char *str, int verb, char *skipto));
PUBLIC long filelen	_((truc *ptr));

PUBLIC truc filesym;
PUBLIC truc eofsym;
PUBLIC truc tstdin, tstdout, tstderr;

PUBLIC char *ariExtens = ".ari";
/*----------------------------------------------------------------------*/

PRIVATE truc stdinsym, stdoutsym, stderrsym;
PRIVATE truc loadsym, openrsym, openwsym, openasym, closesym, flushsym;
PRIVATE truc getcwdsym, setcwdsym;
PRIVATE truc binarysym;
PRIVATE truc rewindsym, setposym, getposym;
PRIVATE truc rdbytesym, wrbytesym, rdblksym, wrblksym;

PRIVATE int skiptolabel _((truc *strom, char *lab));
PRIVATE truc Fload	_((int argn));
PRIVATE truc Sdumstream	  _((void));
PRIVATE truc openaux	  _((char *name, int mode));
PRIVATE int closestream	  _((truc *strom));
PRIVATE truc F1flush	_((int argn));
PRIVATE truc Frewind	_((void));
PRIVATE truc Fgetpos	_((void));
PRIVATE truc Fsetpos	_((void));
PRIVATE truc Fgetcwd    _((void));
PRIVATE truc Fsetcwd    _((void));
PRIVATE truc Sclose	_((void));
PRIVATE int Sopen0	_((truc symb, int mode));
PRIVATE truc Sopenread	  _((void));
PRIVATE truc Sopenwrite	  _((void));
PRIVATE truc Sopenapp	_((void));
PRIVATE truc Frdbyte	_((void));
PRIVATE truc Fwrbyte	_((void));
PRIVATE truc Srdblock	_((void));
PRIVATE int Gblockaux	_((truc symb,FILE **pfil, size_t *anz, byte **pbuf));
PRIVATE truc Swrblock	_((void));
#ifdef genUNiX
PRIVATE void expandtilde    _((char *buf));
#endif

/*----------------------------------------------------------------------*/
PUBLIC void inifile()
{
	truc temp;

	temp	  = newintsym("",  sSBINARY, (wtruc)Sdumstream);
	filesym	  = newsym("file", sTYPESPEC, mk0fun(temp));
	eofsym	  = newselfsym("eof",sINTERNAL);
	binarysym = newreflsym("binary", sSYSSYMBOL);

	tstdin	  = mk0stream(stdin, INSTREAM | DEVICE);
	tstdout	  = mk0stream(stdout,OUTSTREAM | DEVICE);
	tstderr	  = mk0stream(stderr,OUTSTREAM | DEVICE);

	stdinsym  = newsym("stdin",  sSCONSTANT, tstdin);
	stdoutsym = newsym("stdout", sSCONSTANT, tstdout);
	stderrsym = newsym("stderr", sSCONSTANT, tstderr);

	loadsym	  = newsymsig("load",	    sFBINARY,(wtruc)Fload, s_12);
	openrsym  = newsymsig("open_read",  sSBINARY,(wtruc)Sopenread, s_23);
	openwsym  = newsymsig("open_write", sSBINARY,(wtruc)Sopenwrite, s_23);
	openasym  = newsymsig("open_append",sSBINARY,(wtruc)Sopenapp, s_23);
	closesym  = newsymsig("close",	    sSBINARY,(wtruc)Sclose, s_bV);
	flushsym  = newsymsig("flush",	    sFBINARY,(wtruc)F1flush, s_01);
	rewindsym = newsymsig("rewind",	    sFBINARY,(wtruc)Frewind, s_bV);
	setposym  = newsymsig("set_filepos",sFBINARY,(wtruc)Fsetpos, s_2);
	getposym  = newsymsig("get_filepos",sFBINARY,(wtruc)Fgetpos, s_1);
    getcwdsym = newsymsig("get_workdir",sFBINARY,(wtruc)Fgetcwd, s_0);
    setcwdsym = newsymsig("set_workdir",sFBINARY,(wtruc)Fsetcwd, s_1);

	rdbytesym = newsymsig("read_byte",  sFBINARY,(wtruc)Frdbyte, s_1);
	wrbytesym = newsymsig("write_byte", sFBINARY,(wtruc)Fwrbyte, s_2);
	rdblksym  = newsymsig("read_block", sSBINARY,(wtruc)Srdblock, s_3);
	wrblksym  = newsymsig("write_block",sSBINARY,(wtruc)Swrblock, s_3);
}
/*----------------------------------------------------------------------*/
/*
** load(FileName)
*/
PRIVATE truc Fload(argn)
int argn;		/* argn = 1 or 2 */
{
	truc *argptr;
	char *argv[ARGCMAX];
	char name[MAXPFADLEN+4];
	word2 *offsets;
	char *str, *str0;
	int ret, strerr, verbose;
	int i, count;

	argptr = argStkPtr-argn+1;
	if(*FLAGPTR(argptr) == fSTRING) {
		str = STRINGPTR(argptr);
		strerr = (str[0] ? 0 : 1);
	}
	else
		strerr = 1;
	if(strerr) {
		error(loadsym,err_str,*argptr);
		return(brkerr());
	}
	if(argn == 2 && *argStkPtr == zero) {
		verbose = 0;
	}
	else
		verbose = 1;

	str0 = (char *)AriBuf;
	offsets = AriScratch;
	strncopy(str0,str,IOBUFSIZE);
	count = stringsplit(str0,NULL,offsets);
	if(count > ARGCMAX)
		count = ARGCMAX;
	for(i=0; i<count; i++)
		argv[i] = str0 + offsets[i];
	iniargv(count,argv);

	findarifile(argv[0],name);

	if(verbose) {
		s1form(OutBuf,"(** loading file ~A **)",(wtruc)name);
		fprintline(tstdout,OutBuf);
	}

	ret = loadaux(name,verbose,0);
	if(ret == EXITREQ)
		return(Sexit());
	else
		return(ret != aERROR ? true : false);
}
/*----------------------------------------------------------------------*/
PUBLIC int issepdir(ch)
int ch;
{
    int i;

    for(i=strlen(SEP_DIR)-1; i>=0; --i) {
        if(ch == SEP_DIR[i])
            return 1;
   }
   return 0;
}
/*----------------------------------------------------------------------*/
/*
** Sucht ein File namens name mit ARIBAS source code
** Zunaechst wird im aktuellen Directory gesucht, dann in
** den durch apathsym gegebenen Pfaden.
** Falls name nicht die Endung .ari hat, wird auch nach einem
** File mit dem durch die Endung .ari erweiterten Namen gesucht.
** Bei Erfolg ist der Rueckgabewert = 1, der Name, unter dem
** die Datei gefunden wurde, wird nach buf kopiert.
** Bei Misserfolg wird 0 zurueckgegeben.
*/
PUBLIC int findarifile(name,buf)
char *name, *buf;
{
	static char path0[2] = {SEPPATH,0};
	char *fnam[2];
	char *paths;
	char nam1[MAXPFADLEN+4];
	int ext,k;

	ext = fnextens(name,nam1,ariExtens);
	fnam[0] = nam1;
	fnam[1] = name;
	paths = SYMname(apathsym);
	for(k=0; k<=ext; k++) {
		if(findfile(path0,fnam[k],buf)) 
			return(1);
		if(findfile(paths,fnam[k],buf))
			return(1);
	}
	strcopy(buf,name);
	return(0);
}
/*-----------------------------------------------------------*/
/*
** sucht eine Datei namens fnam in den Directories,
** die durch den String paths gegeben werden.
** Falls gefunden, wird der vollstaendige Pfad in buf abgelegt
** und 1 zurueckgegeben; bei Misserfolg ist der Rueckgabewert 0.
*/
PUBLIC int findfile(paths,fnam,buf)
char *paths, *fnam, *buf;
{
	int ch;
	char *ptr, *ptr2;
	FILE *fil;

	ptr = paths;
	ch = *ptr;
	while(ch) {
		ptr2 = buf;
		if(*ptr == SEPPATH)
			ptr++;
		while((ch = *ptr++) && (ch != SEPPATH))
			*ptr2++ = ch;
		if(!issepdir(ch) && (ptr2 != buf))
			*ptr2++ = SEP_DIR[0];
		strcopy(ptr2,fnam);
#ifdef genUNiX
		if(strncmp(buf,"~/",2) == 0) {
            expandtilde(buf);
		}
#endif
		fil = fopen(buf,"r");
		if(fil != NULL) {
			fclose(fil);
			return(1);
		}
	}
	return(0);
}
/*-------------------------------------------------------------*/
#ifdef genUNiX
PRIVATE void expandtilde(buf)
char *buf;
{
	char hbuf[MAXPFADLEN+2];
	char *home;
	int n;

    home = getenv("HOME");
    if(home != NULL) {
		n = strncopy(hbuf,home,MAXPFADLEN);
   		if(issepdir(hbuf[n-1]))
    		n--;
	    strncopy(hbuf+n,buf+1,MAXPFADLEN-n);
		strncopy(buf,hbuf,MAXPFADLEN);
    }
    return;
}
#endif
/*-------------------------------------------------------------*/
PUBLIC int fnextens(str,name,extens)
char *str, *name, *extens;
{
	int k,n,n1,m,ch;

	n = strncopy(name,str,MAXPFADLEN);
	m = strlen(extens);
	if((n >= m) && (strcmp(extens,name+(n-m)) == 0))
		return(0);
	n1 = (n > m ? n-m: 0);
	for(k=n-1; k>=n1; k--) {
		ch = name[k];
		if(ch == '.')
			return(0);
		else if(!isdigalfa(ch))
			break;
	}
	strcopy(name+n,extens);
	return(1);
}
/*-------------------------------------------------------------*/
PUBLIC int isoutfile(strom,mode)
truc *strom;
int mode;	/* aTEXT or BINARY */
{
	int fmode = *STREAMMODEPTR(strom);

	return((fmode & OUTSTREAM) && ((fmode & BINARY) == mode));
}
/*-------------------------------------------------------------*/
PUBLIC int isinpfile(strom,mode)
truc *strom;
int mode;	/* aTEXT or BINARY */
{
	int fmode = *STREAMMODEPTR(strom);

	return((fmode & INSTREAM) && ((fmode & BINARY) == mode));
}
/*-------------------------------------------------------------*/
PRIVATE truc Sdumstream()
{
	return(mkstream(NULL,NOSTREAM));
}
/*-------------------------------------------------------------*/
PRIVATE truc openaux(name,mode)
char *name;
int mode;
{
	FILE *file;
	char access[4];

	if(mode & INSTREAM)
		strcopy(access,"r");
	else if(mode & OUTSTREAM) {
		if(mode & APPEND)
			strcopy(access,"a");
		else
			strcopy(access,"w");
	}
	else
		return(brkerr());
	if(mode & BINARY)
		strcopy(access+1,"b");
	file = fopen(name,access);
	if(file == NULL) {
		return(breaksym);
	}
	return(mkstream(file,mode));
}
/*----------------------------------------------------------*/
/*
** returns 0 if successfully closed, otherwise EOF
*/
PRIVATE int closestream(strom)
truc *strom;
{
	struct stream *fluss;
	int ret;

	fluss = STREAMPTR(strom);
	if(fluss->mode == NOSTREAM)
		return(EOF);
	else
		fluss->mode = NOSTREAM; /* arg is no longer a stream */
	ret = fclose(fluss->file);
	fluss->file = NULL;
	return(ret);
}
/*------------------------------------------------------------*/
/*
** Liest aus strom soviel Zeilen, bis
** label am Anfang einer Zeile (evtl. nach blanks) entdeckt wird
*/
PRIVATE int skiptolabel(strom,lab)
truc *strom;
char *lab;
{
	struct stream *strmptr;
	FILE *fil;
	char *str;
	size_t len = strlen(lab);
	int count = 0;

	strmptr = STREAMPTR(strom);
	fil = strmptr->file;
	while(fgets(StrBuf,IOBUFSIZE,fil)) {
		count++;
		str = trimblanks(StrBuf,0);
		if(strncmp(lab,str,len) == 0)
			break;
	}
	strmptr->lineno += count;
	return(count);
}
/*----------------------------------------------------------*/
/*
** Rueckgabewert: 1 bei Erfolg, 
** EXITREQ (=-1) bei exit-Wunsch
** 0 bei err_open
** aERROR bei Fehler
**
** if skipto != NULL, all lines inclusive the line with skipto are skipped
*/
#define SKIPLABEL	"-init"

PUBLIC int loadaux(fnam,verb,skipto)
char *fnam;
int verb;
char *skipto;
{
	truc *pstrom;
	truc strom;
	truc obj, res;
	int ret = 0;

	strom = openaux(fnam,INSTREAM);
	if(strom == breaksym) {
		error(loadsym,err_open,scratch(fnam));
		return(0);
	}
	WORKpush(strom);
	pstrom = workStkPtr;
	if(skipto) {
		skiptolabel(pstrom,skipto);
	}
	while((obj = tread(pstrom,FILEINPUT)) != eofsym) {
		res = eval(&obj);
		if(res == breaksym) {
			if(*brkmodePtr == exitsym)
				ret = EXITREQ;
			else
				ret = aERROR;
			break;
		}
		if(verb) {
			tprint(tstdout,res);
			fnewline(tstdout);
		}
	}
	closestream(pstrom);
	workStkPtr = pstrom - 1;
	return(ret);
}
/*----------------------------------------------------------*/
PRIVATE truc F1flush(argn)
int argn;
{
	struct stream *strm;
	FILE *fptr;

	if(argn == 0) {
		fptr = stdout;
	}      
	else if(*FLAGPTR(argStkPtr) == fSTREAM) {
		strm = STREAMPTR(argStkPtr);
		if(strm->mode & OUTSTREAM)
			fptr = strm->file;
		else
			goto ausgang;
	}
	else {
		if(*argStkPtr == transcsym)
			flushlog();
		goto ausgang;
      }
	fflush(fptr);
  ausgang:
	return(voidsym);
}
/*----------------------------------------------------------*/
PRIVATE truc Frewind()
{
	struct stream *strm;
	int mode, flg;

	flg = *FLAGPTR(argStkPtr);
	if(flg == fSTREAM)
		strm = STREAMPTR(argStkPtr);
	if(flg != fSTREAM || !((mode=strm->mode) & INSTREAM)
			  || mode & DEVICE) {
		/* Fehlermeldung fehlt! */
		return(false);
	}
	rewind(strm->file);
	return(true);
}
/*----------------------------------------------------------*/
PUBLIC long filelen(ptr)
truc *ptr;
{
	FILE *fil;
	struct stream *strm;
	long pos, len;
	int mode;

	strm = STREAMPTR(ptr);
	mode = strm->mode;
	if(mode == NOSTREAM || (mode & DEVICE))
		return(-1);
	fil = strm->file;
	pos = ftell(fil);
	if(mode & OUTSTREAM)
		return(pos);
	else {
		fseek(fil,0L,2);	/* 2 = from end */
		len = ftell(fil);
		fseek(fil,pos,0);	/* 0 = from start */
		return(len);
	}
}
/*----------------------------------------------------------*/
PRIVATE truc Fgetpos()
{
	struct stream *strm;
	long pos;
	int flg;

	flg = *FLAGPTR(argStkPtr);
	if(flg == fSTREAM)
		strm = STREAMPTR(argStkPtr);
	if(flg != fSTREAM || !(strm->mode & BINARY)) {
		/* Fehlermeldung fehlt */
		return(brkerr());
	}
	pos = ftell(strm->file);
	return(mkinum(pos));
}
/*----------------------------------------------------------*/
PRIVATE truc Fsetpos()
{
	struct stream *strm;
	FILE *fil;
	word2 *x;
	long pos0, pos, len;
	int flg, n, sign;

	flg = *FLAGPTR(argStkPtr-1);
	if(flg == fSTREAM)
		strm = STREAMPTR(argStkPtr-1);
	if(flg != fSTREAM || strm->mode != (INSTREAM | BINARY)) {
		error(setposym,err_binp,voidsym);
		return(brkerr());
	}
	if(chkints(setposym,argStkPtr,1) == aERROR)
		return(brkerr());
	n = bigref(argStkPtr,&x,&sign);
	if(n > 2)
		pos = 0x7F000000;
	else if(sign)
		pos = -1;
	else
		pos = big2long(x,n);
	fil = strm->file;
	pos0 = ftell(fil);
	fseek(fil,0L,2);		/* 2 = from end */
	len = ftell(fil);
	if(pos < 0 || pos > len)
		pos = pos0;
	fseek(fil,pos,0);		/* 0 = from start */
	return(mkinum(pos));
}
/*----------------------------------------------------------*/
PRIVATE truc Sclose()
{
	truc *ptr, *fptr;
	int ret;

	ptr = ARG0PTR(evalStkPtr);
	if(Lvaladdr(ptr,&fptr) != vBOUND ||
		*FLAGPTR(fptr) != fSTREAM) {
		error(closesym,err_filv,*ptr);
		return(false);
	}
	ret = closestream(fptr);
	return(ret == 0 ? true : false);
}
/*----------------------------------------------------------*/
PRIVATE int Sopen0(symb,mode)
truc symb;
int mode;
{
	truc *ptr, *fptr;
	truc strobj, fobj;
	char *str;
	int argn, ret;

	argn = *ARGCOUNTPTR(evalStkPtr);

	/* erstes Argument: File Variable */
	ptr = ARG1PTR(evalStkPtr);
	ret = Lvaladdr(ptr,&fptr);
	if(ret != vBOUND && ret != vUNBOUND) {
		/* fehlt type check */
		error(symb,err_sym,*ptr);
		return(aERROR);
	}

	/* zweites Argument: File-Name */
	ptr = ARGNPTR(evalStkPtr,2);
	strobj = eval(ptr);
	ptr = &strobj;
	if(*FLAGPTR(ptr) == fSTRING)
		str = STRINGPTR(ptr);
	else {
		error(symb,err_str,*ptr);
		return(aERROR);
	}
	if(argn == 3) {
		ptr = ARGNPTR(evalStkPtr,3);
		if(*ptr == binarysym)
			mode |= BINARY;
		else {
			error(symb,err_pars,*ptr);
			return(aERROR);
		}
	}
	fobj = openaux(str,mode);
	if(fobj == breaksym)
		return(aERROR);
	else {
		*fptr = fobj;
		return(0);
	}
}
/* ---------------------------------------------------------*/
PRIVATE truc Sopenread()
{
	int ret = Sopen0(openrsym,INSTREAM);

	return(ret == aERROR ? false : true);
}
/* ---------------------------------------------------------*/
PRIVATE truc Sopenwrite()
{
	int ret = Sopen0(openwsym,OUTSTREAM);

	return(ret == aERROR ? false : true);
}
/*----------------------------------------------------------*/
PRIVATE truc Sopenapp()
{
	int ret = Sopen0(openasym,(OUTSTREAM | APPEND));

	return(ret == aERROR ? false : true);
}
/*----------------------------------------------------------*/
PRIVATE truc Frdbyte()
{
	struct stream *strm;
	int ch;
	int flg;

	flg = *FLAGPTR(argStkPtr);
	if(flg != fSTREAM || !isinpfile(argStkPtr,BINARY)) {
		error(rdbytesym,err_binp,voidsym);
		return(brkerr());
	}
	strm = STREAMPTR(argStkPtr);
	ch = fgetc(strm->file);
	return(mksfixnum(ch));
}
/*----------------------------------------------------------*/
PRIVATE truc Fwrbyte()
{
	struct stream *strm;
	int ch;
	int flg;

	flg = *FLAGPTR(argStkPtr-1);
	if(flg != fSTREAM || !isoutfile(argStkPtr-1,BINARY)) {
		error(wrbytesym,err_bout,voidsym);
		return(brkerr());
	}
	flg = *FLAGPTR(argStkPtr);
	if(flg == fFIXNUM) {
		ch = *WORD2PTR(argStkPtr);
		if(*SIGNPTR(argStkPtr))
			ch = -ch;
	}
	else if(flg == fCHARACTER) {
		ch = *WORD2PTR(argStkPtr);
	}
	else
		return(mksfixnum(-1));
	strm = STREAMPTR(argStkPtr-1);
	ch = fputc(ch,strm->file);
#ifdef ATARIST
	ch &= 0x00FF;	/* Fehler in ATARI-Turbo-C-Compiler, v. 1.0 */
#endif
	return(mksfixnum(ch));
}
/*----------------------------------------------------------*/
PRIVATE truc Srdblock()
{
	FILE *fil;
	byte *bpt;
	size_t anz;
	unsigned n;
	int ret;

	ret = Gblockaux(rdblksym,&fil,&anz,&bpt);
	if(ret == aERROR)
		return(brkerr());
	n = fread(bpt,1,anz,fil);
	return(mkfixnum(n));
}
/*----------------------------------------------------------*/
PRIVATE int Gblockaux(symb,pfil,panz,pbuf)
truc symb;
FILE **pfil;
size_t *panz;
byte **pbuf;
{
	struct stream *strm;
	truc *optr, *argptr, *bufptr;
	truc obj;
	size_t anz;
	unsigned len;
	int ret, flg;

	optr = &obj;
	/* 1. Argument: file */
	obj = eval(ARG1PTR(evalStkPtr));
	flg = *FLAGPTR(optr);
	if(symb == rdblksym)
		if(flg != fSTREAM || !isinpfile(optr,BINARY)) {
			error(symb,err_binp,voidsym);
			return(aERROR);
		}
	else if(symb == wrblksym)
		if(flg != fSTREAM || !isoutfile(optr,BINARY)) {
			error(symb,err_bout,voidsym);
			return(aERROR);
		}
	strm = STREAMPTR(optr);
	*pfil = strm->file;

	/* 3. Argument: Anzahl */
	obj = eval(ARGNPTR(evalStkPtr,3));
	if(*FLAGPTR(optr) != fFIXNUM) {
		error(symb,err_pfix,obj);
		return(aERROR);
	}
	anz = *WORD2PTR(optr);

	/* 2. Argument: Puffer als byte_string */
	argptr = ARGNPTR(evalStkPtr,2);
	ret = bytestraddr(argptr,&bufptr,pbuf,&len);
	if(ret == aERROR) {
		error(symb,err_vbystr,*argptr);
	}
	else if(len < anz) {
		error(symb,err_buf,voidsym);
		ret = aERROR;
	}
	else
		*panz = anz;
	return(ret);
}
/*----------------------------------------------------------*/
PRIVATE truc Swrblock()
{
	FILE *fil;
	byte *bpt;
	size_t anz;
	unsigned n;
	int ret;

	ret = Gblockaux(wrblksym,&fil,&anz,&bpt);
	if(ret == aERROR)
		return(brkerr());
	n = fwrite(bpt,1,anz,fil);
	return(mkfixnum(n));
}
/*----------------------------------------------------------*/
PRIVATE truc Fgetcwd()
{
    char *pfad;

    pfad = getworkdir();

    return mkstr(pfad);
}
/*----------------------------------------------------------*/
PRIVATE truc Fsetcwd()
{
    int res, len;
    char pfad[MAXPFADLEN];
    char *pf1;

    if(*FLAGPTR(argStkPtr) != fSTRING) {
        error(setcwdsym,err_str,*argStkPtr);
        return brkerr();
    }
    len = *STRLENPTR(argStkPtr);
    if(len < MAXPFADLEN) {
        strncopy(pfad,STRINGPTR(argStkPtr),MAXPFADLEN);
#ifdef genUNiX
		if(strncmp(pfad,"~/",2) == 0) {
            expandtilde(pfad);
        }
#endif
        res = setworkdir(pfad);
    }
    else
        res = 0;
    if(res == 0)
        pf1 = "";
    else
        pf1 = getworkdir();
    return mkstr(pf1);
}
/**************************************************************/
