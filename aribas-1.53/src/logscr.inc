/****************************************************************/
/* file logscr.inc

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
** logscr.inc
** logical screen
**
** date of last change
** 1995-03-22: Bremse in L_strappend
** 1998-01-03: cosmetic changes
** 1998-05-31: #define-s for Win32GUI
** 2002-02-16: L_text2blatt, L_insidecomment
*/

#if defined(UNiXorGCC) || defined(Win32GUI)
#define LINELEN		162
#define BUFLINES	200
#else
#define LINELEN		82
#define BUFLINES	50
#endif


typedef char zeile[LINELEN];

#define LOCAL static
LOCAL  zeile *Blatt = NULL;

/*
** Zeilen-Nummern beginnen bei 0
** Die einzelnen Zeilen haben in Position 0 die Zeilenlaenge
** Die Strings werden durch ein Null-Byte abgeschlossen
** Es gilt also stets fuer ptr = Blatt[n], dass ptr[ptr[0]+1] = 0
*/

LOCAL int L_iniscr	_((zeile *Buf, char *prompt));
LOCAL int L_insert	_((int n));
LOCAL void L_strappend	_((int n, int i, char *str));
LOCAL int L_len		_((int n));
LOCAL int L_pagelen	_((void));
LOCAL int L_efflen	_((void));
LOCAL int L_efffirst	_((void));
LOCAL int L_indent	_((int n));
LOCAL int L_trimlen	_((int n));
LOCAL int L_compress	_((void));
LOCAL int L_expand	_((byte *buf));
LOCAL char *L_linerest	_((int n, int k));
LOCAL char *L_line	_((int n));

PRIVATE void L_nulline	_((int n));
PRIVATE void L_linecpy	_((char *z1, char *z2));

PRIVATE int L_makebstream(int lineno);
PRIVATE int L_bnextch(void);
PRIVATE int L_bpeekch(void);
PRIVATE int L_bskiptoeol(void);
PRIVATE int L_bskiptoeostr(void);
PRIVATE int L_bstream(int mode);
LOCAL int L_insidecomment(int lineno);

#ifdef genWinGUI
LOCAL int L_text2blatt(char *buf);
#endif

#ifdef PAGEINPUT
LOCAL void L_clreol	_((int n, int i));
LOCAL void L_delete	_((int n));
LOCAL int L_spaceins	_((int n, int i, int anz, int bound));
LOCAL int L_charins	_((int n, int i, int ch, int bound));
LOCAL int L_chardel	_((int n, int i));
LOCAL int L_charndel	_((int n, int i, int anz));
LOCAL int L_nextgroup	_((int n, int k));
LOCAL int L_prevgroup	_((int n, int k));
LOCAL int L_nextword	_((int n, int k));
LOCAL int L_merge	_((int n, int bound));
LOCAL int L_retbreak	_((int n, int k));
#endif

LOCAL int Col0;	       /* in der 0-ten Zeile nach Prompt */
PRIVATE int Actlen = 0;
/*--------------------------------------------------------------------*/
LOCAL int L_iniscr(Buf,prompt)
zeile *Buf;
char *prompt;
{
	int n;
	int ch;
	char *ptr;

	Blatt = Buf;
	Actlen = 1;
	ptr = Blatt[0];
	n = 0;
	while((ch = *prompt++)) {
		n++;
		ptr[n] = ch;
	}
	ptr[n+1] = 0;
	ptr[0] = n;
	Col0 = n+1;
	for(n=1; n<BUFLINES; n++)
		L_nulline(n);
	return(Col0);
}
/*-------------------------------------------------------------------*/
PRIVATE void L_nulline(n)
int n;
{
	char *ptr;

	ptr = Blatt[n];
	*ptr++ = 0;
	*ptr = 0;
}
/*-------------------------------------------------------------------*/
/*
** Fuegt in Blatt eine neue leere n-te Zeile ein;
** alle folgenden Zeilen werden um eins nach unten verschoben
** Es wird vorausgesetzt, dass n <= Actlen < BUFLINES
*/
LOCAL int L_insert(n)
int n;
{
	int i;

	if(Actlen >= BUFLINES)
		return(0);
	for(i=Actlen; i>n; i--)
		L_linecpy(Blatt[i],Blatt[i-1]);
	L_nulline(n);
	Actlen++;
	return(1);
}
/*-------------------------------------------------------------------*/
PRIVATE void L_linecpy(z1,z2)
char *z1, *z2;
{
	*z1++ = *z2++;
	strcpy(z1,z2);
}
/*-------------------------------------------------------------------*/
/*
** Schreibt in Zeile n ab Position i den String str;
** Es wird vorausgesetzt, dass n < Actlen und 1 <= i < LINELEN-1
** Falls str zu lang ist, wird abgeschnitten
*/
LOCAL void L_strappend(n,i,str)
int n,i;
char *str;
{
	char *ptr, *ptr0;

	ptr0 = Blatt[n];
	ptr = ptr0 + i;
	while((*ptr++ = *str++) && (i < LINELEN-1))
		i++;
	ptr0[i] = 0;
	ptr0[0] = i - 1;
}
/*-------------------------------------------------------------------*/
/*
** Laenge der n-ten Zeile
*/
LOCAL int L_len(n)
int n;
{
	return(Blatt[n][0]);
}
/*-------------------------------------------------------------------*/
LOCAL int L_pagelen()
{
	return(Actlen);
}
/*-------------------------------------------------------------------*/
/*
** effektive Laenge; leere Zeilen am Ende zaehlen nicht
*/
LOCAL int L_efflen()
{
	int n;

	n = Actlen;
	while(n > 1 && Blatt[n-1][0] == 0)
		n--;
	return(n);
}
/*-------------------------------------------------------------------*/
/*
** effektive erste Zeile
*/
LOCAL int L_efffirst()
{
	int i;

	if(Blatt[0][0] > Col0)
		return(0);
	for(i=1; i<Actlen; i++) {
		if(Blatt[i][0] != 0)
			return(i);
	}
	return(0);
}
/*-------------------------------------------------------------------*/
/*
** Anzahl der Spaces am Anfang von Zeile n
*/
LOCAL int L_indent(n)
int n;
{
	char *ptr;
	int k;

	if(n >= Actlen)
		return(0);
	ptr = Blatt[n];
	k = (n > 0 ? 1 : Col0);
	while(ptr[k] == ' ')
		k++;
	return(k-1);
}
/*-------------------------------------------------------------------*/
/*
** Laenge der Zeile n ohne Leerzeichen am Ende
*/
LOCAL int L_trimlen(n)
int n;
{
	int k, k0;
	char *ptr;

	if(n >= Actlen)
		return(0);
	ptr = Blatt[n];
	k = ptr[0];
	k0 = (n > 0 ? 1 : Col0);
	while(k >= k0 && ptr[k] == ' ')
		k--;
	return(k);
}
/*-------------------------------------------------------------------*/
/*
** Komprimiert den Text in Blatt durch Ersetzen von Leerzeichen
** am Anfang der Zeile durch verallgemeinerte TAB's.
** Der entstehende String wird in den Puffer Blatt geschrieben.
** Rueckgabewert: Laenge des entstehenden Strings
*/
LOCAL int L_compress()
{
	char *ptr1, *str;
	int i, k, len, n;

	n = L_efflen();
	ptr1 = Blatt[0];
	for(i=0; i<n; i++) {
		k = L_indent(i);
		len = L_trimlen(i) - k;
		str = L_linerest(i,k+1);
		if(k < TABESC-1) {
			*ptr1++ = k+1;
		}
		else {
			*ptr1++ = TABESC;
			*ptr1++ = k;
		}
		while(--len >= 0)
			*ptr1++ = *str++;
	}
	*ptr1 = 0;
	return(ptr1 - Blatt[0]);
}
/*-------------------------------------------------------------------*/
/*
** Umkehrung von L_compress
** Der komprimierte String steht in buf; das Ergebnis in Blatt
** Rueckgabewert: Laenge in Zeilen;
*/
LOCAL int L_expand(buf)
byte *buf;
{
	int i, k, n;
	int ch;
	char *ptr;

	for(n=0, ch=*buf++; ch && n<BUFLINES; n++) {
		if(ch < TABESC)
			k = ch - 1;
		else
			k = *buf++;
		ptr = Blatt[n];
		i = (n > 0 ? 1 : Col0);
		while(i <= k)
			ptr[i++] = ' ';
		while((ch = *buf++) > TABESC)
			ptr[i++] = ch;
		ptr[i] = 0;
		ptr[0] = i-1;
	}
	Actlen = n;
	return(n);
}
/*-------------------------------------------------------------------*/
LOCAL char *L_linerest(n,k)
int n, k;
{
	char *ptr;

	ptr = Blatt[n];
	if(k > ptr[0])
		k = ptr[0] + 1;
	return(ptr + k);
}
/*-------------------------------------------------------------------*/
LOCAL char *L_line(n)
int n;
{
	char *ptr;

	ptr = Blatt[n];
	return(ptr + 1);
}
/*-------------------------------------------------------------------*/
#ifdef genWinGUI
/*-------------------------------------------------------------------*/
/*
** copies text from buffer buf to Blatt
** return value: actual length of Blatt
**               or -1 if text is too long
*/
LOCAL int L_text2blatt(buf)
char *buf;
{
	int i, n;
	int ch;
	char *ptr;

	for(n=0, ch=*buf++; ch && n<BUFLINES; n++) {
		ptr = Blatt[n];
		i = (n > 0 ? 1 : Col0);
		while(ch && ch != '\n' && i<LINELEN-2) {
			ptr[i++] = ch;
			ch = *buf++;
		}
		while(ch == '\n' || ch == '\r') {
			ch = *buf++;
		}
		ptr[i] = 0;
		ptr[0] = i-1;
	}
	Actlen = n;
	if(ch)
		return(-1);
	else 
		return(n);
}
/*-------------------------------------------------------------------*/
#endif
/*-------------------------------------------------------------------*/
/*
** Tests whether line with number lineno in Blatt
** is inside a comment
*/
LOCAL int L_insidecomment(lineno)
int lineno;
{
    int insidecomm = 0;
    int ch;

    if(L_makebstream(lineno) < 0)
        return(-1);
    ch = 'A';
    while(ch != EOF) {
        ch = L_bnextch();
        switch(ch) {
        case '#':
            if(insidecomm)
                break;
            L_bskiptoeol();
            if(L_bpeekch() == EOF)
                return(1);
            else
                insidecomm = 0;
            break;
        case '(':
            if(insidecomm)
                break;
            ch = L_bnextch();
            if(ch == '*')
                insidecomm = 1;
            break;
        case '*':
            if(!insidecomm)
                break;
            ch = L_bpeekch();
            if(ch == ')') {
                insidecomm = 0;
				L_bnextch();
            }
            break;
		case '"':
			if(!insidecomm)
				L_bskiptoeostr();
			break;
        default:
            break;
        }
    }
    return(insidecomm);
}
/*-------------------------------------------------------------------*/
PRIVATE int L_makebstream(lineno)
int lineno;
{
    return(L_bstream(lineno));
}
/*-------------------------------------------------------------------*/
PRIVATE int L_bnextch()
{
    return L_bstream(-1);
}
/*-------------------------------------------------------------------*/
PRIVATE int L_bpeekch()
{
    return L_bstream(-2);
}
/*-------------------------------------------------------------------*/
PRIVATE int L_bskiptoeol()
{
    return L_bstream(-3);
}
/*-------------------------------------------------------------------*/
PRIVATE int L_bskiptoeostr()
{
    return L_bstream(-4);
}
/*-------------------------------------------------------------------*/
PRIVATE int L_bstream(mode)
int mode;
{
    static int curline = 0;
    static int curlen = 0;
    static int lastline = 0;
    static int col = 0;
    int ch;

    if(mode >= 0) {		/* makebstream */
        if(Blatt == NULL)
            return(-1);
        lastline = mode;
        if(lastline >= BUFLINES)
            lastline = BUFLINES-1;
        curline = 0;
        curlen = Blatt[curline][0];
        col = 1;
        return(lastline);
    }
  nochmal:
    switch(mode) {
    case -1:	/* bnextch */
        if(curline > lastline)
            return(EOF);
        else if(col > curlen) {
            curline++;
            col = 1;
            if(curline <= lastline)
                curlen = Blatt[curline][0];
            else
                curlen = 0;
            return(EOL);
        }
        else {
            ch = Blatt[curline][col];
            col++;
            return(ch);
        }
        break;
    case -2:	/* bpeekch */
        if(curline > lastline)
            return(EOF);
        else if(col > curlen)
            return(EOL);
        else
            return Blatt[curline][col];
		break;
    case -3:	/* skiptoeol */
        curline++;
        col = 1;
        if(curline <= lastline)
            curlen = Blatt[curline][0];
        else 
            curlen = 0;
        return(EOL);
		break;
	case -4:	/* skip to end of string */
		if(col > curlen || curline > lastline) {
			mode = -1;
			goto nochmal;
		}
        while(col <= curlen) {
			ch = Blatt[curline][col];
            col++;
            if(ch == '"')
                break;
        }
		break;
	default:
		break;
	}
    return(-1);
}
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
#ifdef PAGEINPUT
/*-------------------------------------------------------------------*/
LOCAL void L_clreol(n,i)
int n, i;
{
	char *ptr;

	ptr = Blatt[n];
	ptr[i] = 0;
	ptr[0] = i - 1;
}
/*-------------------------------------------------------------------*/
/*
** Loescht in Blatt die n-te Zeile;
** alle folgenden Zeilen werden um eins nach oben verschoben
*/
LOCAL void L_delete(n)
int n;
{
	int i;

	if(n >= Actlen)
		return;
	for(i=n+1; i<Actlen; i++)
		L_linecpy(Blatt[i-1],Blatt[i]);
	Actlen--;
	L_nulline(Actlen);
}
/*-------------------------------------------------------------------*/
/*
** Fuegt in Zeile n ab Position i insgesamt n Spaces ein, falls
** dadurch Zeilenlaenge nicht groesser als bound wird.
** Sonst (oder falls  i > Zeilenlaenge), keine Aktion, Rueckgabewert 0
** Andernfalls Rueckgabewert anz
*/
LOCAL int L_spaceins(n,i,anz,bound)
int n,i,anz,bound;
{
	char *ptr;
	int k, len;

	ptr = Blatt[n];
	len = ptr[0];
	if(n >= Actlen || i > len || len+anz > bound)
		return(0);
	ptr[0] = len + anz;
	for(k=len+1; k>=i; k--)
		ptr[k+anz] = ptr[k];
	for(k=0; k<anz; k++)
		ptr[i+k] = ' ';
	return(anz);
}
/*-------------------------------------------------------------------*/
/*
** Fuegt in Zeile n an der Stelle i den Buchstaben ch ein, falls
** dadurch die Zeile nicht laenger als bound wird.
** Falls bound ueberschritten wuerde, keine Aktion und Rueckgabewert 0.
** Sonst: Rueckgebewert neue Zeilenlaenge.
** Falls i > Zeilenlaenge+1, werden vor ch Leerzeichen eingefuellt
*/
LOCAL int L_charins(n,i,ch,bound)
int n,i,ch,bound;
{
	char *ptr;
	int k, len;

	ptr = Blatt[n];
	len = ptr[0];
	if(len >= bound)
		return(0);
	if(i <= len) {
		len++;
		for(k=len; k>=i; k--)
			ptr[k+1] = ptr[k];
		ptr[i] = ch;
	}
	else {
		for(k=len+1; k<i; k++)
			ptr[k] = ' ';
		ptr[i] = ch;
		ptr[i+1] = 0;
		len = i;
	}
	ptr[0] = len;
	return(len);
}
/*-------------------------------------------------------------------*/
/*
** Loescht in Zeile n den i-ten Buchstaben und zieht alle
** folgenden um eins nach vorne
*/
LOCAL int L_chardel(n,i)
int n,i;
{
	return(L_charndel(n,i,1));
}
/*-------------------------------------------------------------------*/
/*
** Loescht in Zeile n ab Position i insgesamt anz Buchstaben
** und zieht die folgenden um anz nach vorne
** Es wird vorausgesetzt, dass i >= anz ist
** Falls i > Zeilenlaenge, werden entsprechend weniger
** Buchstaben gestrichen.
** Rueckgabewert: Anzahl der tatsaechlich entfernten Zeichen
*/
LOCAL int L_charndel(n,i,anz)
{
	char *ptr;
	int k, len;

	ptr = Blatt[n];
	len = ptr[0];
	if(i > len)
		return(0);
	else if(i >= len - anz + 1) {
		ptr[i] = 0;
		ptr[0] = i - 1;
		return(len - i + 1);
	}
	else {
		ptr[0] = len - anz;
		for(k=i; k<=len+1-anz; k++)
			ptr[k] = ptr[k+anz];
		return(anz);
	}
}
/*-------------------------------------------------------------------*/
/*
** Fuer Cursor-Bewegung mit CTRL-Pfeilrechts:
** Gibt Position der naechsten Gruppe von Zeichen ohne Spaces
*/
LOCAL int L_nextgroup(n,k)
int n,k;
{
	char *ptr;
	int ch;

	if(n >= Actlen)
		return(k);
	ptr = Blatt[n];
	if(k > ptr[0])
		return(k);
	while((ch = ptr[k]) && ch != ' ')
		k++;
	while((ch = ptr[k]) && ch == ' ')
		k++;
	return(k);
}
/*-------------------------------------------------------------------*/
/*
** Fuer Cursor-Bewegung mit CTRL-Pfeillinks:
** Gibt Anfang der vorherigen Gruppe von Zeichen ohne Spaces
*/
LOCAL int L_prevgroup(n,k)
int n,k;
{
	char *ptr;
	int len;
	int k0;

	if(n >= Actlen)
		return(k);
	k0 = (n > 0 ? Col0 : 1);

	ptr = Blatt[n];
	len = ptr[0];
	if(len == 0)
		return(1);
	else if(k > len)
		k = len;
	else if(k > k0)
		k--;
	while(k > k0 && ptr[k] == ' ')
		k--;
	while(k > k0 && ptr[k-1] != ' ')
		k--;
	return(k);
}
/*-------------------------------------------------------------------*/
/*
** gibt Postion des Anfangs des naechsten Wortes fuer das
** Kommando Wort loeschen.
** Es wird bis zu dieser Position ausschliesslich geloescht
**
*/
LOCAL int L_nextword(n,k)
int n,k;
{
	char *ptr;
	int ch;

	if(n >= Actlen)
		return(k);
	ptr = Blatt[n];
	if(k > ptr[0])
		return(k);
	ch = ptr[k];
	if(isdigalfa(ch)) {
		k++;
		while((ch = ptr[k]) && isdigalfa(ch))
			k++;
	}
	else if(ch != ' ') {
		k++;
		while((ch = ptr[k]) && ch != ' ' && !isdigalfa(ch))
			k++;
	}
	else {
		k++;
		while(ptr[k] == ' ')
			k++;
	}
	return(k);
}
/*-------------------------------------------------------------------*/
/*
** Die Zeile n wird mit der Zeile n+1 zusammengefuegt und der
** Rest des Blattes nach oben gezogen.
** Voraussetzung ist, dass die Summe der Zeilenlaengen
** <= bound ist. In diesem Fall ist der Rueckgabewert die
** Position des Anfangs der angehaengten Zeile in Zeile n.
** Falls die Summe der Zeilenlaengen > bound ist, wird
** das Blatt unveraendert gelassen und 0 zurueckgegeben.
** Ausserdem wird vorausgesetzt, dass die Zeile n+1 existiert;
** andernfalls keine Aktion und Rueckgabewert 0.
*/
LOCAL int L_merge(n, bound)
int n, bound;
{
	int k;
	char *str;

	if(n >= L_pagelen() - 1)
		return(0);
	k = L_len(n);
	if(k + L_len(n+1) > bound)
		return(0);
	else {
		str = L_linerest(n+1,1);
		L_strappend(n,k+1,str);
		L_delete(n+1);
		return(k+1);
	}
}
/*-------------------------------------------------------------------*/
/*
** Fuehrt einen Umbruch (wie bei Betaetigung der Return-Taste)
** von Zeile n ab Position k in eine neuzuschaffende naechste
** Zeile durch.
** Falls Blatt zu lang wuerde, wird keine Aktion durchgefuehrt
** und 0 zurueckgegeben; sonst Rueckgabewert 1.
*/
LOCAL int L_retbreak(n,k)
int n,k;
{
	char *str;
	int len;

	len = L_pagelen();
	if(len >= BUFLINES || n >= len)
		return(0);
	L_insert(n+1);
	if(k <= L_len(n)) {
		str = L_linerest(n,k);
		L_strappend(n+1,1,str);
		L_clreol(n,k);
	}
	return(1);
}
/*-------------------------------------------------------------------*/
#endif /* PAGEINPUT */
#undef LOCAL
/*********************************************************************/
