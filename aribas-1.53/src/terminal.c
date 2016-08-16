/****************************************************************/
/* file terminal.c

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
** terminal.c
** terminal input
**
** date of last change
** 1995-01-25	str0[] in initerm
** 1995-03-10	fixed bug in Fsaveinput
** 1995-03-22	treadline changed
** 1997-03-31	moved comprline inside #ifdef PAGEINPUT
** 1997-04-13	reorg (newintsym)
** 1998-10-28   adjustments for Win32GUI
** 1999-04-27	HISTBOX
** 1999-05-11   fixed problems with signed char in expandtabs
** 2002-02-16   testcomment
*/

#include "common.h"
#ifdef DOSorTOS
#ifndef LINEINPUT
#define PAGEINPUT
#define GETKEY
#include "console.inc"
#endif
#endif

#include "logscr.inc"

#ifdef genWinGUI
#define HISTBOX
#endif


PUBLIC void initerm	_((void));
PUBLIC void inputprompt	 _((void));
PUBLIC void dumpinput	_((void));
PUBLIC char *treadline	_((void));
PUBLIC void historyout	_((int flg));

PUBLIC truc historsym, savinsym, bufovflsym;

#ifdef genWinGUI
PUBLIC int testcomment(char *buf);
#endif
/*--------------------------------------------------------*/
PRIVATE void loadinput	_((char *str));
PRIVATE int tinput		_((void));
PRIVATE int expandtabs	_((char *dest, char *src, int tabwidth));
PRIVATE truc Fsaveinput	 _((int argn));
PRIVATE int filout		_((int ch));
PRIVATE int getinpbuf	_((int n));
PRIVATE int inploop		_((void));
PRIVATE int endtest		_((int curline));
PRIVATE void protocinput  _((byte *str));
PRIVATE void inputout	_((byte *str, ifun putfun));
PRIVATE void historydisp  _((void));
PRIVATE int previnput	_((int k));
PRIVATE void display	_((void));

PRIVATE truc nullinp;
PRIVATE trucptr *inpPtr;

#ifdef GETKEY
PRIVATE truc Fgetkey	_((void));
PRIVATE truc getkeysym;
#endif

#ifdef PAGEINPUT
PRIVATE truc Floadedit	_((void));
PRIVATE void getloadedit  _((void));
PRIVATE int liesein		_((char *buffer, FILE *fptr));
PRIVATE char *comprline	 _((char *cpt, char *buf));
PRIVATE int processkey	_((int key));
PRIVATE int printable	_((int ch));
PRIVATE void repaint	_((int startrow, int line0, int n));
PRIVATE void curdown	_((int curline));
PRIVATE void curup		_((int curline));
PRIVATE void clrzeilrest  _((int curline, int col));
PRIVATE void delzeile	_((int curline));
PRIVATE void backspace	_((int curline));
PRIVATE void mergelines	 _((int curline));
PRIVATE void tabright	_((int curline, int tabwidth));
PRIVATE void tableft	_((int curline, int tabwidth));
PRIVATE void startpage	_((void));
PRIVATE void endpage	_((void));
PRIVATE void retbreak	_((int curline));
PRIVATE void opennl		_((void));
PRIVATE void crnewline	_((int curline));

PRIVATE truc loadedsym;
PRIVATE int Load_edit = 0;

/* Row0 gibt an, wo auf dem Bildschirm Zeile 0 der Eingabe ist*/
PRIVATE int Row0;

#else	/* #ifndef PAGEINPUT */
PRIVATE int processline	 _((char *line));
#endif

PRIVATE char prompt[] = "==> ";
PRIVATE char quitstring[] = "exit";

PRIVATE int Hist_out = 0;
PRIVATE int newinput = 1;
PRIVATE int endinput = 1;

PRIVATE FILE *savfil;
PRIVATE byte *Input;
PRIVATE int inpcursor = 0;

PRIVATE char TinpBuf[LINELEN] = {EOL};
	/* line buffer for terminal input */

#define EXPANDMAX	255
#define ETABWIDTH	8	/* tabwidth of loaded text */

#ifdef PAGEINPUT
#define TABWIDTH	4
PRIVATE int tabwidth = TABWIDTH;
#endif

#define HISTIMAX        4

#ifdef HISTBOX
PRIVATE void HB_ini        _((void));
PRIVATE int HB_anz         _((void));
PRIVATE char *HB_retrieve  _((int k));
PRIVATE int HB_store       _((char *str));
PRIVATE int HB_export      _((int k));
PRIVATE int HB_import      _((int k));

#define HB_MAXITEM    36
#define HB_SIZE     8000

PRIVATE struct {
    char Buffer[HB_SIZE];
    int Entry[HB_MAXITEM];
    int bot;
    int ceil;
    int last;
} HB_Box;
#endif /* HISTBOX */
/*------------------------------------------------------------*/
PUBLIC void initerm()
{
	static char text[24];
	static trucptr ptrarr[4+HISTIMAX];
	static char inam[] = "$_";
	static char str0[] = " 0.";
	truc tempsym;
	int k;

	inpPtr = ptrarr + 3;
	str0[0] = strlen(prompt) + 1;
	nullinp = mkstr(str0);

	for(k=-3; k<=HISTIMAX; k++) {
            inam[1] = 'd'+k;
            tempsym = newsym(inam, sINTERNVAR, nullinp);
            inpPtr[k] = SYMBINDPTR(&tempsym);
	}
#ifdef HISTBOX
	HB_ini();
#endif
	historsym = newintsym("!",sINTERNAL, (wtruc)0);
    SYMbind(historsym) = constone;
#ifndef genWinGUI
	savinsym  = newsymsig("save_input",sFBINARY,(wtruc)Fsaveinput,s_12ii);
#endif /* genWinGUI */

#ifdef PAGEINPUT
	loadedsym = newsymsig("load_edit", sFBINARY,(wtruc)Floadedit, s_bs);
#endif

#ifdef GETKEY
	getkeysym = newsymsig("get_key",   sFBINARY,(wtruc)Fgetkey, s_0);
#endif
    s1form(text,"more than ~D lines",(wtruc)BUFLINES);
	bufovflsym = newselfsym(text,sINTERNAL);
}
/*------------------------------------------------------------*/
#ifdef HISTBOX
PRIVATE void HB_ini()
{
    int k;

    for(k=0; k<HB_MAXITEM; k++)
        HB_Box.Entry[k] = 0;
    HB_Box.bot = 0;
    HB_Box.ceil = HB_SIZE;
    HB_Box.last = -1;
}
/*-----------------------------------------------------------*/
PRIVATE int HB_anz()
{
    return(HB_Box.last + 1);
}
/*-----------------------------------------------------------*/
PRIVATE char *HB_retrieve(k)
int k;
{
    int pos;

    if(k < 0 || k > HB_Box.last)
        return NULL;
    else {
        pos = HB_Box.Entry[k];
        return (HB_Box.Buffer + pos);
    }
}
/*-----------------------------------------------------------*/
PRIVATE int HB_store(str)
char *str;
{
    char *Buffer = HB_Box.Buffer;
    int *Entry = HB_Box.Entry;
    int bot = HB_Box.bot;
    int ceil = HB_Box.ceil;
    int last = HB_Box.last;
    int len = strlen(str)+1;
    int len1, ceil1, diff, k;
    char *ptr1, *ptr2;

    if(len > HB_SIZE/2)
        return(-1);
    while((len > ceil - bot) || last >= HB_MAXITEM-1) {
        last--;
        ceil1 = Entry[last];
        if(ceil1 < bot) {
            diff = HB_SIZE - bot;
            len1 = bot - ceil1;
            ptr1 = Buffer + ceil1;
            ptr2 = ptr1 + diff;
            while(--len1 >= 0)
                *ptr2++ = *ptr1++;
            for(k=0; k<=last; k++)
                Entry[k] += diff;
            ceil = ceil1 + diff;
            bot = 0;
        }
        else
            ceil = ceil1;
    }
    last++;
    for(k=last; k>0; k--)
        Entry[k] = Entry[k-1];
    strcpy(Buffer+bot,str);
    Entry[0] = bot;
    HB_Box.bot = bot + len;
    HB_Box.ceil = ceil;
    HB_Box.last = last;
    return(len);
}
/*-----------------------------------------------------------*/
PRIVATE int HB_export(k)
int k;
{
    char *str;

    str = (char *)STRINGPTR(inpPtr[k]);
    return HB_store(str);
}
/*-----------------------------------------------------------*/
PRIVATE int HB_import(k)
int k;
{
    int anz;
    char *str;

    anz = HB_anz();
    k -= HISTIMAX + 1;
    if(k < 0 || anz <= 0) {
        *inpPtr[0] = nullinp;
        return 0;
    }
    if(k >= anz)
        k = anz-1;
    str = HB_retrieve(k);
    *inpPtr[0] = mkstr(str);
    return (k + HISTIMAX + 1);
}
/*------------------------------------------------------------*/
#endif /* HISTBOX */
/*------------------------------------------------------------*/
PUBLIC void inputprompt()
{
	if(endinput)
		newinput = 1;
}
/*------------------------------------------------------------*/
PUBLIC void dumpinput()
{
	newinput = 1;
}
/*------------------------------------------------------------*/
/*
** read line from terminal
*/
PUBLIC char *treadline()
{
	char *str;
	int ch;
	int ret = 1;

	if(newinput) {
#ifndef genWinGUI
		if(!Hist_out)
			fnewline(tstdout);
#endif
		STRMlineno(tstdin) = 0;
		ret = inploop();
		getinpbuf(1);
		inpcursor = 0;
		newinput = 0;
		endinput = 0;
	}
	str = TinpBuf;
	if(ret >= 0) {
		ch = tinput();
		while(ch >= ' ') {
			*str++ = ch;
			ch = tinput();
		}
	}
	else {
		*str++ = ZESC;
		*str++ = '\001';
	}
	*str = EOL;
	return(TinpBuf);
}
/*------------------------------------------------------------*/
PRIVATE void loadinput(str)
char *str;
{
	if(!Hist_out) {
#ifdef HISTBOX
		HB_export(4);
#endif
		*inpPtr[4] = *inpPtr[3];
		*inpPtr[3] = *inpPtr[2];
		*inpPtr[2] = *inpPtr[1];
	}
	else
		Hist_out = 0;
	*inpPtr[1] = nullinp;	/* release memory */
	*inpPtr[1] = mkstr(str);
}
/*--------------------------------------------------------------*/
PRIVATE int tinput()
{
	int ch;

	ch = Input[inpcursor++];
	if(ch >= ' ') {
		return(ch);
	}
    else if(ch == 0) {
		endinput = 1;
		return(FORMFEED);
	}
	else {
		if(ch == TABESC)
			inpcursor++;
		return(EOL);
	}
}
/*-------------------------------------------------------------------*/
/*
** Ersetzt die TABs des Strings src durch die entsprechende
** Zahl von Leerzeichen. Die Umwandlung wird beendet, sobald
** ein EOL oder ein Nullbyte angetroffen wird.
** Sonstige Zeichen mit Ascii-Code < ' ' werden durch Leerzeichen
** ersetzt.
** Es wird vorausgesetzt, dass der Puffer dest genuegend lang ist.
** Spaetestens nach EXPANDMAX Zeichen wird der expandierte
** String abgeschnitten.
** Leer- und Steuerzeichen am Ende des Strings werden
** ebenfalls abgeschnitten.
** Rueckgabewert ist die Laenge des expandierten Strings.
*/
PRIVATE int expandtabs(dest,src,tabwidth)
char *dest, *src;
int tabwidth;
{
	char *ptr;
	int k, len = 0;
	int ch;

	ptr = dest;
	while(len < EXPANDMAX) {
		ch = *src++;
		if(ch == EOL || ch == 0)
			break;
		else if(ch == '\t') {
			k = ((len+tabwidth)/tabwidth) * tabwidth;
			if(k > EXPANDMAX)
				k = EXPANDMAX;
			while(len < k) {
				*ptr++ = ' ';
				len++;
			}
		}
		else {
			*ptr++ = ((byte)ch >= ' ' ? ch : ' ');
			len++;
		}
	}
	while(len > 0 && (byte)dest[len-1] <= ' ')
		len--;
	dest[len] = '\0';
	return(len);
}
/*-------------------------------------------------------------------*/
#ifndef genWinGUI
PRIVATE truc Fsaveinput(argn)
int argn;
{
	truc *argptr;
	int k, k0;
	int flg;
	char *str;
	char name[MAXPFADLEN+2];

	argptr = argStkPtr - argn + 1;
	if(argn == 2) {
	    flg = *FLAGPTR(argStkPtr);
	    if(flg == fFIXNUM || flg == fCHARACTER) {
		k = *WORD2PTR(argStkPtr);
		if(k >= 'a' && k <= 'c')
		    k0 = 'a' - k - 1;
		else if(k >= 1 && k <= 3) {
		    k0 = k + 1;
		}
		else
		    goto errex1;
	    }
	    else {
  errex1:
		error(savinsym,
		"integer 1,2,3 or character 'a', 'b' or 'c' expected",
		*argStkPtr);
		goto errexit;
	    }
	}
	else
		k0 = 2;
	flg = *FLAGPTR(argptr);
	if(flg == fCHARACTER) {
		k = *WORD2PTR(argptr);
		if(k < 'a' || k > 'c') {
			error(savinsym,
			"character 'a', 'b' or 'c' expected",*argptr);
			goto errexit;
		}
		*inpPtr['a' - k - 1] = mkcopy(inpPtr[k0]);
	}
	else if(flg == fSTRING) {
		str = STRINGPTR(argptr);
		if(!str[0])
			goto errexit;
		fnextens(str,name,ariExtens);
		savfil = fopen(name,"w");
		if(savfil == NULL) {
			error(savinsym,err_open,scratch(name));
			goto errexit;
		}
		str = STRINGPTR(inpPtr[k0]);
		inputout((byte *)str,filout);
		fclose(savfil);
	}
	else {
		error(savinsym,
		"filename or character 'a', 'b' or 'c' expected",
		*argStkPtr);
		goto errexit;
	}
	return(*argptr);
  errexit:
	return(false);
}
#endif /* genWinGUI */
/*-------------------------------------------------------------------*/
PRIVATE int filout(ch)
int ch;
{
	return(fputc(ch,savfil));
}
/*-------------------------------------------------------------------*/
PRIVATE int getinpbuf(n)
int n;
{
	int ret = n;
#ifdef HISTBOX
	if(n > HISTIMAX) {
		ret = HB_import(n);
		n = 0;
	}
#endif
	Input = (byte *)STRINGPTR(inpPtr[n]);
	return(ret);
}
/*-------------------------------------------------------------------*/
PRIVATE int endtest(curline)
int curline;
{
	char *str;
	int ch;
	int k, k1;
	int ret = 0;

	k = L_trimlen(curline);

	ch = L_linerest(curline,k)[0];
#ifdef PAGEINPUT
	if(Col <= k)
		ch = 0;
#endif
	if(ch == '.' || ch == '?') {
		ret = 1;
	}
	else if(curline == L_efffirst()) {
		k1 = L_indent(curline);
		str = L_linerest(curline,k1+1);
		if(str[0] == '!' || strcmp(str,quitstring) == 0) {
			ret = 1;
		}
	}
	if(ret == 1) {
		if(L_insidecomment(curline) == 1)
			ret = 0;
	}
	return(ret);
}
/*-------------------------------------------------------------------*/
PRIVATE void protocinput(str)
byte *str;
{
	int indent = strlen(prompt);

	if(indent >= str[0])
		indent = 0;
	str[0] -= indent;
	inputout(str,logout);
	str[0] += indent;
}
/*-------------------------------------------------------------------*/
PRIVATE void inputout(str,putfun)
byte *str;
ifun putfun;
{
	int ch, k;

	ch = *str++;
	while(ch) {
		if(ch < TABESC-1)
			k = ch - 1;
		else if(ch == TABESC)
			k = *str++;
		else
			k = 0;
		while(--k >= 0)
			putfun(' ');
		while((ch = *str++) > TABESC)
			putfun(ch);
		putfun('\n');
	}
}
/*-------------------------------------------------------------------*/
PUBLIC void historyout(flg)
int flg;
{
	Hist_out = flg;
}
/*-------------------------------------------------------------------*/
PRIVATE void historydisp()
{
	int k;

	k = SYMbind2(historsym);
	if(k >= 1 && k <= 3)
		k++;
	else if(k >= 'a' || k <= 'c')
		k = 'a' - k - 1;
	else
		k = 2;
	previnput(k);
}
/*-------------------------------------------------------------------*/
PRIVATE int previnput(k)
int k;
{
	k = getinpbuf(k);
	L_expand(Input);
	display();
	return k;
}
/*-------------------------------------------------------------------*/
#ifdef genWinGUI
PUBLIC int cpyprevinput(buf,k)
char *buf;
int k;
{
	char InpBuf[BUFLINES][LINELEN];
	int i, n;
	char *str;

	Col0 = L_iniscr(InpBuf,prompt);
	k = getinpbuf(k);
	L_expand(Input);
	n = L_efflen() - 1;
	for(i=0; i<=n; i++) {
		str = L_line(i) + (i==0 ? Col0-1 : 0);
		buf += strcopy(buf,str);
		if(i < n) {
#ifdef Win32GUI
			buf += strcopy(buf,"\r\n");
#else
			buf += strcopy(buf,"\n");
#endif
		}
	}
	*buf = '\0';
	return k;
}
/*-------------------------------------------------------------------*/
PUBLIC int testcomment(buf)
char *buf;
{
	char InpBuf[BUFLINES][LINELEN];
	int curline;
	int ret;

	Col0 = L_iniscr(InpBuf,prompt);
	curline = L_text2blatt(buf)-1;
	if(curline < 0)
		ret = -1;
	else
		ret = L_insidecomment(curline);
	return ret;
}
/*-------------------------------------------------------------------*/
#endif
/*-------------------------------------------------------------------*/
#ifndef PAGEINPUT
/*-------------------------------------------------------------------*/
PRIVATE int inploop()
{
	char InpBuf[BUFLINES][LINELEN];
	char linebuf[LINELEN];
	int ret = 1;
	char *str;
#ifdef genWinGUI
	char *winbuf;
	int k, ch;
#endif

	Col0 = L_iniscr(InpBuf,prompt);
#ifndef genWinGUI
	if(Hist_out) {
		historydisp();
	}
	else {
		fprintstr(tstdout,prompt);
		while(1) {
			str = fgets(linebuf,LINELEN-3,stdin);
			if(str == NULL) {
			    strcopy(linebuf,quitstring);
			}
			ret = processline(linebuf);
			if(ret <= 0)
				break;
		}
	}
#else   /* ifdef genWinGUI */
	winbuf = getWinscrBuf();
	while(1) {
		str = linebuf;
		for(ch=*winbuf++,k=1; (ch && ch!='\n' && k<=LINELEN-3); k++) {
			*str++ = ch;
			ch = *winbuf++;
		}
		*str = '\0';
		ret = processline(linebuf);
        if(ret <= 0 || ch == 0)
			break;
	}
#endif  /* ?genWinGUI */
	L_compress();
	str = InpBuf[0];
	if(Log_on) {
		protocinput((byte *)str);
	}
	loadinput(str);
	return(ret);
}
/*-------------------------------------------------------------------*/
PRIVATE int processline(line)
char *line;
{
	char expandbuf[EXPANDMAX+1];
	int lineno, len, n;
	int ret, fertig;

	lineno = L_pagelen() - 1;
	len = L_len(lineno);
	n = expandtabs(expandbuf,line,ETABWIDTH);
	L_strappend(lineno,len+1,expandbuf);
	ret = L_insert(lineno+1);
    fertig = endtest(lineno);
	if(ret == 0 && fertig == 0)
		return(-1);
	else if(fertig)
		return(0);
	else
		return(ret);
}
/*-------------------------------------------------------------------*/
PRIVATE void display()
{
	char *str;
	int i, n;

	n = L_efflen();
	for(i=0; i<n; i++) {
		str = L_line(i);
		fprintline(tstdout,str);
	}
}
/*-------------------------------------------------------------------*/
#else /* ifdef PAGEINPUT */
/*-------------------------------------------------------------------*/
PRIVATE int inploop()
{
	char InpBuf[BUFLINES][LINELEN];
	int weiter = 1;
	int key;
	char *str;

	flushkeyb();
	getwininfo();
	Row0 = Row;
	Col0 = L_iniscr(InpBuf,prompt);
	cursorto(Row0,1);
	clineout(prompt);
	if(Log_on)
		strlogout(prompt);
	cursorto(Row0,Col0);
	cleareos();
	if(Load_edit)
		getloadedit();
	else if(Hist_out)
		historydisp();
	while(weiter) {
		key = keyin();
		weiter = processkey(key);
	}
	L_compress();
	str = InpBuf[0];
	if(Log_on)
		protocinput((byte *)str);
	loadinput(str);
	return(1);
}
/*-------------------------------------------------------------------*/
PRIVATE void display()
{
	int k, n;
	int startrow, line0;

	n = L_efflen();
	if(n >= MaxRow) {
		startrow = 1;
		line0 = n - MaxRow;
		n = MaxRow;
	}
	else if(Row0 + n - 1 > MaxRow) {
		k = Row0 + n - 1 - MaxRow;
		while(--k >= 0)
			scrollup();
		startrow = MaxRow - n + 1;
		line0 = 0;
	}
	else if(Row0 <= 0) {
		startrow = 1;
		line0 = 0;
	}
	else {
		startrow = Row0;
		line0 = 0;
	}
	Row0 = startrow - line0;
	repaint(startrow,line0,n);
}
/*-------------------------------------------------------------------*/
PRIVATE int liesein(buffer,fptr)
char *buffer;
FILE *fptr;
{
	char expandbuf[EXPANDMAX+1];
	char linbuf[LINELEN];
	char *cpt;
	int count, len;

	cpt = buffer;
	for(count=0; count<BUFLINES; count++) {
	    if(fgets(linbuf,LINELEN-2,fptr) == NULL)
		break;
	    else {
		len = expandtabs(expandbuf,linbuf,ETABWIDTH);
		if(count == 0 && len >= (LINELEN-2) - Col0 - 1) {
			*cpt++ = '\001';
			count = 1;
		}
		cpt = comprline(cpt,expandbuf);
	    }
	}
	if(!count)
		*cpt++ = '\001';
	*cpt = 0;
	fclose(fptr);
	return(count);
}
/*-------------------------------------------------------------------*/
/*
** Uebersetzt die Zeile buf in das interne
** Compress-Format (siehe L_compress in LOGSCR.INC)
** Es wird vorausgesetzt, dass buff keine TABs mehr enthaelt.
*/
PRIVATE char *comprline(cpt,buf)
char *cpt, *buf;
{
	int k = 0;
	int ch;

	while((ch = *buf) == ' ') {
		buf++;
		k++;
	}
	if(k <= TABESC - 2) {
		*cpt++ = k+1;
	}
	else {
		*cpt++ = TABESC;
		*cpt++ = k;
	}
	while((ch = *buf++))
		*cpt++ = ch;
	return(cpt);
}
/*-------------------------------------------------------------------*/
PRIVATE truc Floadedit()
{
	char Buffer[BUFLINES * LINELEN];
	char name[84];
	char *str;
	int errflg = 0;
	int ret;
	FILE *fptr;

	if(*FLAGPTR(argStkPtr) == fSTRING) {
		str = STRINGPTR(argStkPtr);
		errflg = (str[0] ? 0 : 1);
	}
	else
		errflg = 1;
	if(errflg) {
		error(loadedsym,err_str,*argStkPtr);
		return(brkerr());
	}
	fnextens(str,name,ariExtens);

	fptr = fopen(name,"r");
	if(fptr == NULL) {
		error(loadedsym,err_open,scratch(name));
		return(false);
	}
	ret = liesein(Buffer,fptr);
	if(ret < 0) {
		if(ret == -1)
			error(loadedsym,"input file too long",voidsym);
		else
			error(loadedsym,"inadmissible input",voidsym);
		return(false);
	}
	Load_edit = 1;
	*inpPtr[0] = mkstr(Buffer);
	return(true);
}
/*-------------------------------------------------------------------*/
PRIVATE void getloadedit()
{
	previnput(0);
	Load_edit = 0;
	*inpPtr[0] = nullinp;   /* release memory */
}
/*-------------------------------------------------------------------*/
PRIVATE int processkey(key)
int key;
{
	char *str;
	int curline;
	int k, n;
	int ret = 1;

  nochmal:
	curline = Row - Row0;
	switch(key) {
	case CURDOWN:
		curdown(curline);
		break;
	case CURUP:
		curup(curline);
		break;
	case CURRIGHT:
		if(Col < MaxCol) {
			Col++;
			cursorto(Row,Col);
		}
		break;
	case CURLEFT:
		if(Col > 1) {
			if(curline > 0 || Col > Col0)
				Col--;
			cursorto(Row,Col);
		}
		break;
	case DELETE:
		n = L_chardel(curline,Col);
		if(n) {
			str = L_linerest(curline,Col);
			clineout(str);
		}
		break;
	case DELLINE:
		if(curline == 0)
			clrzeilrest(0,Col0);
		else
			delzeile(curline);
		break;
	case DELWORD:
		k = L_nextword(curline,Col);
		if(k > Col) {
			L_charndel(curline,Col,k-Col);
			str = L_linerest(curline,Col);
			clineout(str);
		}
		break;
	case BACKSPACE:
		if((curline > 0 && Col > 1) || Col > Col0)
			backspace(curline);
		else if(Col == 1 && curline > 0 && Row > 1)
			mergelines(curline);
		break;
	case STARTLINE:
		Col = (curline > 0 ? 1 : Col0);
		cursorto(Row,Col);
		break;
	case ENDLINE:
		n = L_len(curline);
		cursorto(Row,n+1);
		break;
	case TABRIGHT:
		tabright(curline,tabwidth);
		break;
	case TABLEFT:
		tableft(curline,tabwidth);
		break;
	case CTRLRIGHT:
		n = L_nextgroup(curline,Col);
		cursorto(Row,n);
		break;
	case CTRLLEFT:
		n = L_prevgroup(curline,Col);
		cursorto(Row,n);
		break;
	case STARTPAGE:
		startpage();
		break;
	case ENDPAGE:
		endpage();
		break;
	case TOPSCREEN:
		if(Row == 1)
			curup(curline);
		else {
			Row = (Row0 > 1 ? Row0 : 1);
			if(Row0 >= 1 && Col < Col0)
				Col = Col0;
			cursorto(Row,Col);
		}
		break;
	case BOTSCREEN:
		if(Row == MaxRow)
			curdown(curline);
		else {
			n = Row0 + L_pagelen() - 1;
			if(n >= MaxRow)
				Row = MaxRow;
			else if(n > 1)
				Row = n;
			else
				Row = 1;
			cursorto(Row,Col);
		}
		break;
	case RETURN:
		if(curline == L_efflen()-1 && endtest(curline)) {
			if(Row == MaxRow)
				scrollup();
			else
				Row++;
			cursorto(Row,1);
			ret = 0;
		}
		else if(Col > L_len(curline)
			&& curline < L_pagelen()-1
			&& L_len(curline+1) == 0) {
			crnewline(curline);
		}
		else
			retbreak(curline);
		break;
	case CTRLRET:
		if(curline == L_efflen()-1 && endtest(curline)) {
			if(Row == MaxRow)
				scrollup();
			else
				Row++;
			cursorto(Row,1);
			ret = 0;
		}
		else if(curline < L_pagelen()-1 && L_len(curline+1) == 0) {
			crnewline(curline);
		}
		else if(L_insert(curline+1))
			opennl();
		break;
	case PREVINP1:
		previnput(1);
		break;
	case PREVINP2:
		previnput(2);
		break;
	case PREVINP3:
		previnput(3);
		break;
	case PREVINP4:
		previnput(4);
		break;
	case PREVINPA1:
		previnput(-1);
		break;
	case PREVINPA2:
		previnput(-2);
		break;
	case PREVINPA3:
		previnput(-3);
		break;
	case ESCKEY:
		key = keyin();
		if(key == ESCKEY) {
			endpage();
			key = '.';
			ret = 0;
		}
		goto nochmal;
	default:
		if(printable(key)) {
			n = L_charins(curline,Col,key,MaxCol-1);
			if(n) {
				str = L_linerest(curline,Col);
				clineout(str);
				Col = Col++;
				cursorto(Row,Col);
			}
		}
		break;
	}
	return(ret);
}
/*-------------------------------------------------------------------*/
PRIVATE int printable(code)
int code;
{
	if(code & 0xFF00)
		return(0);
	else if(' ' <= code && code != '\177')
		return(1);
	else
		return(0);
}
/*-------------------------------------------------------------------*/
/*
** Schreibt ab Zeile startrow insgesamt n Zeilen des Dokuments,
** beginnend mit Zeile line0, auf den Bildschirm und loescht den
** Rest des Bildschirms.
** Es wird vorausgesetzt, dass (startrow + n - 1) <= MaxRow
** und line0 + n <= Laenge des Dokuments.
*/
PRIVATE void repaint(startrow,line0,n)
int startrow, line0, n;
{
	int i;
	char *str;

	for(i=0; i<n; i++) {
		str = L_line(line0+i);
		cursorto(startrow+i,1);
		clineout(str);
	}
	Col = L_len(line0+n-1) + 1;
	Row = startrow + n - 1;
	cursorto(Row,Col);
	cleareos();
}
/*-------------------------------------------------------------------*/
PRIVATE void curdown(curline)
int curline;
{
	int temp;
	char *str;

	if(curline >= L_pagelen() - 1)
		return;
	if(Row < MaxRow) {
		Row++;
	}
	else {
		scrollup();
		Row0--;
		temp = Col;
		str = L_line(curline+1);
		cursorto(MaxRow,1);
		clineout(str);
		Col = temp;
	}
	cursorto(Row,Col);
}
/*-------------------------------------------------------------------*/
PRIVATE void curup(curline)
int curline;
{
	int temp;
	char *str;

	if(curline == 1 && Col < Col0)
		cursorto(Row,Col0);
	if(curline == 0)
		return;
	if(Row > 1) {
		Row--;
	}
	else {
		scrolldown();
		Row0++;
		temp = Col;
		str = L_line(curline-1);
		cursorto(1,1);
		clineout(str);
		Col = temp;
	}
	cursorto(Row,Col);
}
/*-------------------------------------------------------------------*/
PRIVATE void clrzeilrest(curline,col)
int curline, col;
{
	L_clreol(curline,col);
	cursorto(Row,col);
	cleareol();
}
/*-------------------------------------------------------------------*/
PRIVATE void delzeile(curline)
int curline;
{
	int n, temp;
	char *str;

	n = L_pagelen();
	if(n > 1) {
		L_delete(curline);
		if(curline == n-1 && Row == 1) {
			cursorto(1,1);
			str = L_line(curline-1);
			clineout(str);
			Row0++;
		}
		else {
			deleteline();
			if(MaxRow - Row0 < n-1) {
				temp = Row;
				str = L_line(MaxRow - Row0);
				cursorto(MaxRow,1);
				clineout(str);
				Row = temp;
			}
			else if(curline == n-1)
				Row--;
		}
		Col = (Row == Row0 ? Col0 : 1);
		cursorto(Row,Col);
	}
}
/*-------------------------------------------------------------------*/
PRIVATE void backspace(curline)
int curline;
{
	int n;
	char *str;

	Col--;
	n = L_chardel(curline,Col);
	cursorto(Row,Col);
	if(n) {
		str = L_linerest(curline,Col);
		clineout(str);
	}
}
/*-------------------------------------------------------------------*/
PRIVATE void mergelines(curline)
int curline;
{
	int n, col0, temp;
	char *str;

	col0 = L_merge(curline-1,MaxCol-1);
	if(col0) {
		deleteline();
		n = MaxRow - Row0;
		if(n < L_pagelen()-1) {
			temp = Row;
			str = L_line(n);
			cursorto(MaxRow,1);
			clineout(str);
			Row = temp;
		}
		str = L_linerest(curline-1,col0);
		cursorto(Row-1,col0);
		clineout(str);
	}
}
/*-------------------------------------------------------------------*/
PRIVATE void tabright(curline,tabwidth)
int curline, tabwidth;
{
	int n, k;
	char *str;

	n = L_len(curline);
	k = tabwidth - (Col - 1) % tabwidth;
	if(Col <= n) {
		k = L_spaceins(curline,Col,k,MaxCol-1);
		if(n) {
			str = L_linerest(curline,Col);
			clineout(str);
			cursorto(Row,Col+k);
		}
	}
	else if(Col+k <= MaxCol)
		cursorto(Row,Col+k);
}
/*-------------------------------------------------------------------*/
PRIVATE void tableft(curline,tabwidth)
int curline, tabwidth;
{
	int k;
	char *str;

	k = (Col - 1) % tabwidth;
	if(k == 0 && Col > tabwidth)
		k = tabwidth;
	if(curline == 0 && Col-k < Col0) {
		k = Col - Col0;
		Col = Col0;
	}
	else
		Col = Col - k;
	L_charndel(curline,Col,k);
	cursorto(Row,Col);
	str = L_linerest(curline,Col);
	clineout(str);
}
/*-------------------------------------------------------------------*/
PRIVATE void startpage()
{
	int i, k;
	char *str;

	if(Row0 >= 1) {
		cursorto(Row0,Col0);
	}
	else {
		k = 1 - Row0;
		Row0 = 1;
		if(k > MaxRow)
			k = MaxRow;
		cursorto(1,1);
		for(i=k-1; i>=0; i--) {
			scrolldown();
			str = L_line(i);
			clineout(str);
		}
		cursorto(1,Col0);
	}
}
/*-------------------------------------------------------------------*/
PRIVATE void endpage()
{
	char *str;
	int i, k, n;

	n = L_efflen() - 1;
	k = Row0 + n;
	if(k <= MaxRow) {
		cursorto(k,L_len(n)+1);
	}
	else {
		k = Row0 + n - MaxRow;
		Row0 -= k;
		if(k > MaxRow)
			k = MaxRow;
		cursorto(MaxRow,1);
		for(i=k-1; i>=0; i--) {
			scrollup();
			str = L_line(n-i);
			clineout(str);
		}
		cursorto(MaxRow,L_len(n)+1);
	}
}
/*-------------------------------------------------------------------*/
PRIVATE void retbreak(curline)
int curline;
{
	char *str;

	if(L_retbreak(curline,Col)) {
		cleareol();
		opennl();
		str = L_line(curline+1);
		clineout(str);
	}
}
/*-------------------------------------------------------------------*/
PRIVATE void opennl()
{
	if(Row < MaxRow) {
		Row++;
		cursorto(Row,1);
		if(Row < MaxRow)
			insertline();
		else
			cleareol();
	}
	else {
		scrollup();
		Row0--;
		cursorto(MaxRow,1);
	}
}
/*-------------------------------------------------------------------*/
PRIVATE void crnewline(curline)
{
	curdown(curline);
	cursorto(Row,1);
}
/*------------------------------------------------------------*/
#endif /* ?PAGEINPUT */
/*------------------------------------------------------------*/
#ifdef GETKEY
/*------------------------------------------------------------*/
PRIVATE truc Fgetkey()
{
	unsigned key = keyin();

	return(mkfixnum(key));
}
/*------------------------------------------------------------*/
#endif
/*********************************************************************/

