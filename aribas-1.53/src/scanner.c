/****************************************************************/
/* file scanner.c

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

Email   forster@mathematik.uni-muenchen.de
*/
/****************************************************************/

/*
** scanner.c
** scanning input
**
** date of last change
** 1995-03-21
** 1997-01-24     moved skiptobel() to file.c
** 1997-04-11     changed trimblanks()
** 1997-07-04     changed readln to handle multi-line integers
** 1998-04-05     changed readln to handle reading empty strings
** 1998-10-06     readln: corrected handling of EOF
** 1998-11-14     nexttok: handle DOS line endings under UNiX
** 1999-06-15     readln: #ifdef Win32GUI, changed peekchar
** 2004-02-09     numval: corrected value of count
*/

#include "common.h"

PUBLIC void iniscan _((void));
PUBLIC int nexttok  _((truc *strom, int skip));
PUBLIC int curtok   _((truc *strom));
PUBLIC int fltreadprec  _((void));
PUBLIC int skipeoltok   _((truc *strom));
PUBLIC int isalfa   _((int ch));
PUBLIC int isdigalfa    _((int ch));
PUBLIC int isdecdigit   _((int ch));
PUBLIC int ishexdigit   _((int ch));
PUBLIC int isoctdigit   _((int ch));
PUBLIC int isbindigit   _((int ch));
PUBLIC int toupcase _((int ch));
PUBLIC int tolowcase    _((int ch));
PUBLIC char *trimblanks _((char *str, int mode));
PUBLIC int rerror   _((truc sym1, char *mess, truc sym2));

PUBLIC char *StrBuf;        /* input buffer */
PUBLIC char *SymBuf;        /* buffer for symbol names */
PUBLIC numdata Curnum;      /* currently processed number */
PUBLIC truc Curop;      /* currently processed operator */

/*------------------------------------------------------------*/
PRIVATE ifun0 chread    _((truc *strom));
PRIVATE ifun0 readfrom  _((char *str));
PRIVATE ifun0 lnread    _((truc *strom, char *buf));
PRIVATE int nextchar    _((void));
PRIVATE int nextstrchar _((void));
PRIVATE int nextlnchar  _((void));
PRIVATE int tnextchar   _((void));
PRIVATE int peekchar    _((ifun0 nextch));
PRIVATE int isdelim0    _((int ch));
PRIVATE int isdelim1    _((int ch));
PRIVATE int skipblanks  _((int ch, ifun0 nextch));
PRIVATE int skipcomment  _((ifun0 nextch));
PRIVATE int histinp _((ifun0 nextch, char *buf));
PRIVATE int stringinp   _((ifun0 nextch, char *buf, ifun delimfun));
PRIVATE int charinp _((ifun0 nextch, char *buf));
PRIVATE int symbolchar  _((int ch));
PRIVATE int isbasemark  _((int ch, int mode));
PRIVATE int isexpmark   _((int ch));
PRIVATE int normdecstr  _((char *str));
PRIVATE int digsequence  _((ifun0 nextch, char *buf, int maxlen));
PRIVATE int bstrinp _((ifun0 nextch, char *str, int maxlen));
PRIVATE int signinp _((ifun0 nextch, char *str));
PRIVATE int intinp  _((ifun0 nextch, char *str, int maxlen));
PRIVATE int numinp  _((ifun0 nextch, char *str, int maxlen));
PRIVATE int accumint    _((numdata *nptr));
PRIVATE int accumbstr   _((numdata *nptr));
PRIVATE int accumfloat  _((numdata *nptr));
PRIVATE truc Satoi  _((void));
PRIVATE truc Satof  _((void));
PRIVATE truc Snumval    _((truc symb));
PRIVATE int numval  _((char *str, int *pcount));
PRIVATE int readlnitem  _((ifun0 nextch, truc *ptr));
PRIVATE truc Sreadln    _((void));

PRIVATE truc readlnsym, atoisym, atofsym;

PRIVATE int curbase;
PRIVATE int curfltprec;
PRIVATE char *numstring;
PRIVATE char *expstring;
PRIVATE int expsign;
PRIVATE int digsep = '_';

PRIVATE int strbufSize;

PRIVATE ifun checkdig;      /* current function for checking digits */

PRIVATE FILE *cFil;     /* current input file pointer */
PRIVATE struct stream *cStream; /* current input stream */
PRIVATE char *tBufptr = "\n";
        /* pointer to next character of terminal input */
PRIVATE char *cStrptr;
        /* pointer to next character in currently read string */

PRIVATE FILE *rFil;     /* readln input file pointer */
PRIVATE char rChar;     /* current character during readln */
PRIVATE char *rBuf;     /* buffer used during readln */
PRIVATE char *rBufptr;      /* pointer to char in rBuf */

PRIVATE int inpsource = TERMINALINP;

/*--------------------------------------------------------------------*/
PUBLIC void iniscan()
{
    StrBuf = (char *)AriBuf;
    strbufSize = sizeof(word2) * aribufSize - (IOBUFSIZE+4);

    SymBuf = StrBuf + strbufSize;

    readlnsym = newsymsig("readln", sSBINARY,(wtruc)Sreadln, s_0u);
    atoisym   = newsymsig("atoi",   sSBINARY,(wtruc)Satoi,   s_12);
    atofsym   = newsymsig("atof",   sSBINARY,(wtruc)Satof,   s_12);

    Curop = nullsym;
}
/*------------------------------------------------------------*/
PRIVATE ifun0 chread(strom)
truc *strom;
{
    cStream = STREAMPTR(strom);
    cFil = cStream->file;
    if(cFil == stdin) {
        inpsource = TERMINALINP;
        return(tnextchar);
    }
    else {
        inpsource = FILEINPUT;
        return(nextchar);
    }
}
/*------------------------------------------------------------*/
PRIVATE ifun0 lnread(strom,buf)
truc *strom;
char *buf;
{
    struct stream *strmptr;

    strmptr = STREAMPTR(strom);
    rFil = strmptr->file;
    rBuf = rBufptr = buf;
    rBuf[0] = rChar = 0;
    inpsource = READLNINPUT;
    return(nextlnchar);
}
/*------------------------------------------------------------*/
PRIVATE ifun0 readfrom(str)
char *str;
{
    cStrptr = str;
    inpsource = STRINGINPUT;
    return(nextstrchar);
}
/*------------------------------------------------------------*/
/*
** next character from current input file
*/
PRIVATE int nextchar()
{
    int ch = fgetc(cFil);

    if(ch == EOL)
        cStream->lineno++;
    return(cStream->ch = ch);
}
/*------------------------------------------------------------*/
/*
** next character during readln
*/
PRIVATE int nextlnchar()
{
    int ch;

  nochmal:
    ch = *rBufptr;
    if(ch == 0) {
#ifdef Win32GUI
        if(rFil == stdin) {
            if(!wingets(rBuf,IOBUFSIZE))
                return(rChar = EOF);
        }
        else
#endif
        if(!fgets(rBuf,IOBUFSIZE,rFil))
            return(rChar = EOF);
        if(Log_on && rFil == stdin)
            strlogout(rBuf);
        rBufptr = rBuf;
        goto nochmal;
    }
    rBufptr++;
    return(rChar = ch);
}
/*------------------------------------------------------------*/
/*
** next character from current string stream
*/
PRIVATE int nextstrchar()
{
    int ch;

    ch = *cStrptr;
    if(ch)
        cStrptr++;
    else
        ch = EOF;
    return(ch);
}
/*------------------------------------------------------------*/
/*
** next character from terminal input
*/
PRIVATE int tnextchar()
{
    int ch = cStream->ch;

    if(ch == EOL || ch == EOF) {
        cStream->lineno++;
        tBufptr = treadline();
        ch = *tBufptr;
        if(ch != EOF && ch != EOL)
            tBufptr++;
    }
    else
        ch = *tBufptr++;
    return(cStream->ch = ch);
}
/*--------------------------------------------------------------*/
/*
** returns next character from current input stream
** without advancing file position
*/
PRIVATE int peekchar(nextch)
ifun0 nextch;
{
    int ch;

    if(nextch == (ifun0)tnextchar) {
        ch = *tBufptr;
    }
    else if(nextch == (ifun0)nextchar) {
        ch = getc(cFil);
        ungetc(ch,cFil);
    }
    else if(nextch == (ifun0)nextlnchar) {
        ch = *rBufptr;
        if(!ch)
            ch = EOF;
    }
    else if(nextch == (ifun0)nextstrchar) {
        ch = *cStrptr;
        if(!ch)
            ch = EOF;
    }
    else
        ch = aERROR;
    return(ch);
}
/*--------------------------------------------------------------------*/
/*
** fetch and return the next token from strom
** If an operator is encountered, the corresponding symbol
** is stored in the global variable Curop
** Returns integer aERROR in case of error
*/
PUBLIC int nexttok(strom,skip)
truc *strom;
int skip;   /* if skip != 0, skips EOL */
{
    char *buf;
    int ch, res;
    ifun0 nextch;

    nextch = chread(strom);
  nochmal:
    ch = skipblanks(cStream->ch,nextch);

    if(isalfa(ch) || ch == '_') {   /* symbol input */
        buf = SymBuf;
        *buf++ = ch;
        while(symbolchar(ch=nextch()))
            *buf++ = ch;
        *buf = 0;
        res = SYMBOLTOK;
    }
    else if(isdecdigit(ch)) {
        StrBuf[0] = ch;
        res = numinp(nextch,StrBuf,strbufSize);
    }
    else switch(ch) {
    case '+':
        nextch();
        Curop = plussym;
        res = PLUSTOK;
        break;
    case '-':
        nextch();
        Curop = minussym;
        res = MINUSTOK;
        break;
    case '*':
        if(nextch() == '*') {
            nextch();
            Curop = powersym;
            res = POWERTOK;
        }
        else {
            Curop = timessym;
            res = TIMESTOK;
        }
        break;
    case '/':
        if(nextch() == '=') {
            nextch();
            Curop = nequalsym;
            res = NETOK;
        }
        else {
            Curop = divfsym;
            res = DIVIDETOK;
        }
        break;
    case '(':
        if(nextch() != '*')
            res = LPARENTOK;
        else {
            nextch();
            skipcomment(nextch);
            goto nochmal;
        }
        break;
    case ')':
        nextch();
        res = RPARENTOK;
        break;
    case '[':
        nextch();
        res = LBRACKTOK;
        break;
    case ']':
        nextch();
        res = RBRACKTOK;
        break;
    case '{':
        nextch();
        res = LBRACETOK;
        break;
    case '}':
        nextch();
        res = RBRACETOK;
        break;
    case '#':   /* ueberlese Kommentar bis zum Ende der Zeile */
        while(!isdelim0(nextch()))
            ;
        nextch();
        goto nochmal;
    case '=':
        nextch();
        Curop = equalsym;
        res = EQTOK;
        break;
    case '<':
        ch = nextch();
        if(ch == '=') {
            nextch();
            Curop = arilesym;
            res = LETOK;
        }
        else if(ch == '>') {
            nextch();
            Curop = nequalsym;
            res = NETOK;
        }
        else {
            Curop = ariltsym;
            res = LTTOK;
        }
        break;
    case '>':
        if(nextch() == '=') {
            Curop = arigesym;
            nextch();
            res = GETOK;
        }
        else {
            Curop = arigtsym;
            res = GTTOK;
        }
        break;
    case '!':
        res = histinp(nextch,SymBuf);
        break;
    case '"':       /* string */
        nextch();
        res = stringinp(nextch,StrBuf,isdelim1);
        if(res == aERROR)
            rerror(parserrsym,err_brstr,scratch(StrBuf));
        break;
    case '\'':      /* character */
        nextch();
        res = charinp(nextch,StrBuf);
                /* unvollstaendig */
        if(cStream->ch == '\'')
            nextch();
        if(res == aERROR) {
            rerror(parserrsym,err_bchar,scratch(StrBuf));
        }
        break;
    case ':':
        if(nextch() == '=') {
            nextch();
            Curop = assignsym;
            res = ASSIGNTOK;
        }
        else
            res = COLONTOK;
        break;
    case ';':
        nextch();
        res = SEMICOLTOK;
        break;
    case ',':
        nextch();
        res = COMMATOK;
        break;
    case '^':
        nextch();
        res = DEREFTOK;
        break;
    case '?':
        ch = nextch();
        if(inpsource != TERMINALINP)
            goto nochmal;
        else if(ch == '.')
            nextch();
        res = QUESTIONTOK;
        break;
    case '.':
        ch = nextch();
        if(ch == '.') {
            nextch();
            res = DOTDOTTOK;
            break;
        }
        ch = skipblanks(ch,nextch);
        if(isalfa(ch) || ch == '_')
            res = RECDOTTOK;
        else
            res = DOTTOK;
        break;
    case '$':
        buf = (char *)AriScratch;
        buf[0] = nextch();
        res = bstrinp(nextch,buf,2*strbufSize);
        break;
    case EOL:
    case FORMFEED:
        if(skip) {
            nextch();
            goto nochmal;
        }
        else
            res = EOLTOK;
        break;
#ifdef genUNiX
    case '\015':    /* handle DOS line endings under UNiX */
        nextch();
        goto nochmal;
        break;
#endif
	case ZESC:
		ch = nextch();
		if(ch == '\001') {
			res = Z1TOK;
			break;
		}
		else
			goto nochmal;
    case EOF:
        res = EOFTOK;
        break;
    default:
        nextch();
        res = aERROR;
        break;
    }
    cStream->tok = res;
    return(res);
}
/*------------------------------------------------------------*/
PUBLIC int curtok(strom)
truc *strom;
{
    return(*STREAMTOKPTR(strom));
}
/*------------------------------------------------------------*/
PUBLIC int fltreadprec()    /* used by parser */
{
    return(curfltprec);
}
/*-----------------------------------------------------------------*/
PRIVATE int isdelim0(ch)
int ch;
{
    if(ch == EOL || ch == FORMFEED || ch == EOF)
        return(1);
    else
        return(0);
}
/*-----------------------------------------------------------------*/
PRIVATE int isdelim1(ch)
int ch;
{
    if(ch == '"' || ch == EOL || ch == EOF)
        return(1);
    else
        return(0);
}
/*-----------------------------------------------------------------*/
PRIVATE int skipblanks(ch,nextch)
int ch;
ifun0 nextch;
{
    while(ch == ' ' || ch == '\t')
        ch = nextch();
    return(ch);
}
/*-----------------------------------------------------------------*/
/*
** mode = 0: Loescht blanks vom Anfang des Strings str
** mode != 0: Loescht blanks vom Anfang des Strings und gibt
**  dann den String, der aus der naechsten fortlaufenden Serie 
**  von Nicht-Blanks besteht, zurueck.
** Arbeitet destruktiv auf str !!
*/
PUBLIC char *trimblanks(str,mode)
char *str;
int mode;   
{
    char *str1;

    while(*str == ' ' || *str == '\t')
        str++;
    if(mode) {
        str1 = str;
        while(*str1 > ' ')
            str1++;
        *str1 = 0;
    }
    return(str);
}
/*-----------------------------------------------------------------*/
/*
** skip eol token and return the next token
*/
PUBLIC int skipeoltok(strom)
truc *strom;
{
    ifun0 nextch;
    int tok;

    nextch = chread(strom);
    tok = cStream->tok;
    if(tok == EOLTOK) {
        nextch();
        tok = nexttok(strom,1);
    }
    return(tok);
}
/*-----------------------------------------------------------------*/
/*
** sucht im aktuellen Eingabe-Strom nach dem String "*)"
** Resultat: Erstes Zeichen nach "*)" oder EOF
*/
PRIVATE int skipcomment(nextch)
ifun0 nextch;
{
    int ch = cStream->ch;
    int ch1;

    while(ch != EOF) {
        ch1 = nextch();
        if(ch == '*' && ch1 == ')') {
            ch = nextch();
            break;
        }
        else {
            ch = ch1;
        }
    }
    return(ch);
}
/*-----------------------------------------------------------------*/
/*
** Liest die Symbole !, !!, !!!, !a, !b, !c und schreibt sie in buf
** Das erste ! ist bereits gelesen, aber noch nicht geschrieben.
*/
PRIVATE int histinp(nextch,buf)
ifun0 nextch;
char *buf;
{
    int ch;

    *buf++ = '!';
    ch = nextch();
    if(ch == '!') {
        *buf++ = '!';
        if(nextch() == '!') {
            *buf++ = '!';
            ch = nextch();
        }
    }
    else if(ch >= 'a' && ch <= 'c') {
        *buf++ = ch;
        ch = nextch();
    }
    if(ch == '.')
        nextch();
    *buf = 0;
    return(HISTORYTOK);
}
/*-----------------------------------------------------------------*/
/*
** Speichert String aus strom im Puffer buf
*/
PRIVATE int stringinp(nextch,buf,delimfun)
ifun0 nextch;
char *buf;
ifun delimfun;
{
    int ch;

    for(ch=cStream->ch; !delimfun(ch); ch=nextch()) {
        *buf++ = ch;
    }
    *buf = 0;
    if(ch == '"') {
        nextch();
        return(STRINGTOK);
    }
    else
        return(aERROR);
}
/*--------------------------------------------------------------------*/
PRIVATE int charinp(nextch,buf)
ifun0 nextch;
char *buf;
{
    int len = 0;
    int ch = cStream->ch;

    if(ch != EOL && ch != EOF) {
        *buf++ = ch;
        len++;
        ch = nextch();
    }
    while(ch != '\'' && ch != EOL && ch != EOF) {
        *buf++ = ch;
        len++;
        ch = nextch();
    }
    *buf = 0;
    if(len == 1)
        return(CHARTOK);
    else
        return(aERROR);
}
/*--------------------------------------------------------------------*/
PUBLIC int isalfa(ch)
int ch;
{
    if(ch >= 'a' && ch <= 'z')
        return(1);
    if(ch >= 'A' && ch <= 'Z')
        return(1);
    return(0);
}
/*------------------------------------------------------------------*/
PUBLIC int isdigalfa(ch)
int ch;
{
    return(isalfa(ch) || isdecdigit(ch));
}
/*--------------------------------------------------------------------*/
PUBLIC int isdecdigit(ch)
int ch;
{
    return(ch >= '0' && ch <= '9');
}
/*------------------------------------------------------------------*/
PUBLIC int ishexdigit(ch)
int ch;
{
    if(ch >= '0' && ch <= '9')
        return(1);
    else if(ch >= 'A' && ch <= 'F')
        return(1);
    else if(ch >= 'a' && ch <= 'f')
        return(1);
    else
        return(0);
}
/*------------------------------------------------------------------*/
PUBLIC int isoctdigit(ch)
int ch;
{
    return(ch >= '0' && ch < '8');
}
/*------------------------------------------------------------------*/
PUBLIC int isbindigit(ch)
int ch;
{
    return(ch == '0' || ch == '1');
}
/*-----------------------------------------------------------------*/
PRIVATE int symbolchar(ch)
int ch;
{
    if(isalfa(ch) || isdecdigit(ch) || ch == '_')
        return(1);
    else
        return(0);
}
/*-----------------------------------------------------------------*/
PUBLIC int toupcase(ch)
int ch;
{
    if(ch >= 'a' && ch <= 'z')
        return(ch + ('A' - 'a'));
    else
        return(ch);
}
/*-----------------------------------------------------------------*/
PUBLIC int tolowcase(ch)
int ch;
{
    if(ch >= 'A' && ch <= 'Z')
        return(ch + ('a' - 'A'));
    else
        return(ch);
}
/*-----------------------------------------------------------------*/
/*
** sets global variable curbase and global function checkdig
** to appropriate value
** mode = 0 or mode = 2
*/
PRIVATE int isbasemark(ch,mode)
int ch, mode;
{
    switch(ch) {
        case 'x':
        case 'X':
            checkdig = ishexdigit;
            return(curbase = 16);
        case 'y':
        case 'Y':
            checkdig = isbindigit;
            return(curbase = 2);
        case 'o':
        case 'O':
            checkdig = isoctdigit;
            return(curbase = 8);
        default:
            return(0);
    }
}
/*-----------------------------------------------------------------*/
/*
** Stellt fest, ob ch ein exponent marker ist und gibt gegebenenfalls
** zugehoerige float-Laenge zurueck, sonst 0
*/
PRIVATE int isexpmark(ch)
int ch;
{
    switch(ch) {
    case 'e':
    case 'E':
        return(deffltprec());
    case 'f':
    case 'F':
        return(FltPrec[0]);
    case 'd':
    case 'D':
        return(FltPrec[1]);
    case 'l':
    case 'L':
        return(FltPrec[2]);
    case 'x':
    case 'X':
        return(FltPrec[3]);
    default:
        return(0);
    }
}
/*-----------------------------------------------------------------*/
/*
** entfernt aus Dezimal-String den Dezimalpunkt und Nullen am Ende
** und gibt Dezimalexponenten zurueck
** !!! arbeitet destruktiv auf str !!!
*/
PRIVATE int normdecstr(str)
char *str;
{
    int decexp = 0;
    char *ptr, *ptr1;

    ptr = str;
    while(isdecdigit(*ptr))
        ptr++;
    ptr1 = ptr;
    if(*ptr == '.') {
        ptr++;
        while(isdecdigit(*ptr)) {
            *ptr1++ = *ptr++;
            decexp--;
        }
    }
    *ptr1 = 0;
    while(*--ptr1 == '0' && ptr1 > str) {
        *ptr1 = 0;
        decexp++;
    }
    return(decexp);
}
/*------------------------------------------------------------------*/
/*
** Liest aus dem aktuellen Eingabestrom in den Puffer buf 
** maximale fortlaufende Sequenz von Ziffern, wobei
** Zifferntrenner (globale Variable digsep) uebergangen werden.
** In buf[0] muss sich das aktuelle Zeichen befinden.
** Es wird die globale Funktion checkdig verwendet.
** Rueckgabewert Anzahl der Ziffern
*/
PRIVATE int digsequence(nextch,buf,maxlen)
ifun0 nextch;
char *buf;
int maxlen;
{
    int digs = 0;
    int ch = buf[0];

    while(digs < maxlen) {
        if(checkdig(ch)) {
            *buf++ = ch;
            digs++;
            ch = nextch();
        }
        else if(ch == digsep) {
            ch = peekchar(nextch);
            if(checkdig(ch))
                ch = nextch();
            else if(ch == EOL) {
                nextch();
                ch = nextch();
                ch = skipblanks(ch,nextch);
                if(!checkdig(ch))
                    break;
            }
            else
                break;
        }
        else
            break;
    }
    *buf = ch;
    return(digs >= maxlen ? -1 : digs);
}
/*------------------------------------------------------------------*/
/*
** Liest byte_string ein
** In str[0] muss sich das aktuelle Zeichen befinden
*/
PRIVATE int bstrinp(nextch,str,maxlen)
ifun0 nextch;
char *str;
int maxlen;
{
    int n;

    checkdig = ishexdigit;
    numstring = str;
    n = digsequence(nextch,str,maxlen);
    if(n < 0) {
        return(rerror(bstringsym,err_iovfl,voidsym));
    }
    if(n & 1)
        str[n++] = '0';
    str[n] = 0;
    return(accumbstr(&Curnum));
}
/*------------------------------------------------------------------*/
/*
** Liest Integer ein.
** Die erste Ziffer muss sich bereits in str[0] befinden.
*/
PRIVATE int intinp(nextch,str,maxlen)
ifun0 nextch;
char *str;
int maxlen;
{
    int n;

    numstring = str;
    n = digsequence(nextch,str,maxlen);
    if(n <= 0) {
        if(n < 0)
            rerror(voidsym,err_iovfl,voidsym);
        return(aERROR);
    }
    str[n] = 0;
    accumint(&Curnum);
    return(INUMTOK);
}
/*------------------------------------------------------------------*/
PRIVATE int signinp(nextch,str)
ifun0 nextch;
char *str;
{
    int ch;
    int sign = 0;

    ch = str[0];
    ch = skipblanks(ch,nextch);
    if(ch == '+' || ch == '-') {
        sign = (ch == '+' ? 0 : MINUSBYTE);
        ch = nextch();
    }
    str[0] = ch = skipblanks(ch,nextch);
    return(sign);
}
/*------------------------------------------------------------------*/
/*
** Liest Zahl (integer oder float) ein.
** Die erste Ziffer muss sich bereits in str[0] befinden.
** (Das Vorzeichen muss schon vorher mit eingelesen werden)
*/
PRIVATE int numinp(nextch,str,maxlen)
ifun0 nextch;
char *str;
int maxlen;
{
    char *str0;
    int len, len1, prec, ch, ch1;
    int phase, exdigs;
    int tok;

    str0 = str;
    ch = str[0];
    ch1 = peekchar(nextch);
    if(ch == '0' && isbasemark(ch1,0)) {
        nextch();
        *str = nextch();
        return(intinp(nextch,str,maxlen));
    }
/* #ifdef GF2NINTEGER */
    else if(ch == '2' && isbasemark(ch1,2)) {
        nextch();
        *str = nextch();
        tok = intinp(nextch,str,maxlen);
        if(tok == INUMTOK)
            tok = GF2NTOK;
        return(tok);
    }
/* #endif  GF2NINTEGER */
    else if(!isdecdigit(ch))
        return(aERROR);
    curbase = 10;
    checkdig = isdecdigit;
                /* before decimal point */
    phase = 0;
    tok = aERROR;
    numstring = str;
    len1 = maxlen - (str - str0);
    len = digsequence(nextch,str,len1);
    if(len < 0) {
        goto ovflexit;
    }
    str += len;
    ch = *str;
    if(ch == '.') {
        ch1 = peekchar(nextch);
        if(checkdig(ch1)){
            str++;
            *str = nextch();
            phase = '.';
        }
        else {
            *str = 0;
            tok = INUMTOK;
        }
    }
    /* else if(!isalfa(ch)) { */
    else {
        *str = 0;
        tok = INUMTOK;
    }
    if(phase == '.') {  /* before exponential sign 'e', 'E' */
        len1 = maxlen - (str - str0);
        len = digsequence(nextch,str,len1);
        if(len < 0) {
            goto ovflexit;
        }
        str += len;
        ch = *str;
        if((prec = isexpmark(ch))) {
            *str++ = 0;
            ch = nextch();
            phase = 'E';
        }
        else if(!isalfa(ch)) {
            *str = 0;
            prec = deffltprec();
            expstring = NULL;
            tok = FLOATTOK;
        }
    }
    if(phase == 'E') {   /* inside exponent */
        if(ch == '+') {
            expsign = 0;
            ch = nextch();
        }
        else if(ch == '-') {
            expsign = MINUSBYTE;
            ch = nextch();
        }
        else
            expsign = 0;
        expstring = str;
        exdigs = 0;
        while(isdecdigit(ch)) {
            *str++ = ch;
            exdigs++;
            ch = nextch();
        }
        if(exdigs) {
            *str = 0;
            tok = FLOATTOK;
        }
    }
    if(tok == INUMTOK)
        tok = accumint(&Curnum);
    else if(tok == FLOATTOK) {
        curfltprec = prec;
        tok = accumfloat(&Curnum);
    }
    return(tok);
  ovflexit:
    return(rerror(voidsym,err_iovfl,voidsym));
}
/*-----------------------------------------------------------------*/
/*
** Benuetzt die globalen Variablen numstring, curbase, um den 
** dadurch gegebenen (vorzeichenlosen) Integer in *numptr abzulegen
** Fuer numptr->digits wird AriBuf benuetzt,
** numptr->expo wird gleich 0 gesetzt
** (so dass numptr auch als float interpretiert werden kann)
*/
PRIVATE int accumint(numptr)
numdata *numptr;
{
    int n;
    word2 *x, *hilf;

    x = AriScratch;
    hilf = x + aribufSize;
    
    switch(curbase) {
    case 10:
        n = str2big(numstring,x,hilf);
        break;
    case 16:
        n = xstr2big(numstring,x);
        break;
    case 8:
        n = ostr2big(numstring,x);
        break;
    case 2:
        n = bstr2big(numstring,x);
        break;
    default:
        n = 0;
    }
    cpyarr(x,n,AriBuf);
    numptr->len = n;
    numptr->digits = AriBuf;
    numptr->sign = 0;
    numptr->expo = 0;

    return(INUMTOK);
}
/*-----------------------------------------------------------------*/
PRIVATE int accumbstr(numptr)
numdata *numptr;
{
    byte *ptr;
    int ch;
    unsigned u, v;
    unsigned len = 0;

    ptr = (byte *)AriBuf;
    while((ch = *numstring++)) {
        u = digval(ch);
        v = digval(*numstring++);
        *ptr++ = (u << 4) + v;
        len++;
    }
    numptr->digits = AriBuf;
    numptr->len = len;
    return(BSTRINGTOK);
}
/*-----------------------------------------------------------------*/
/*
** Benuetzt die globalen Variablen 
** numstring, expstring, expsign
** um die dadurch gegebene Float-Zahl in *numptr abzulegen
** Fuer numptr->digits wird AriBuf benuetzt.
*/
PRIVATE int accumfloat(numptr)
numdata *numptr;
{
    long decexp;
    word4 u;
    int prec = curfltprec + 2;
    int len; 
    word2 *x, *hilf;

    x = AriScratch;
    hilf = x + aribufSize;

    len = (expstring ? str2big(expstring,x,hilf) : 0);
    if(len > 2 || (u = big2long(x,len)) > maxdecex)
        return(rerror(parserrsym,err_ovfl,voidsym));
    decexp = u;
    if(expsign)
        decexp = -decexp;
    decexp += normdecstr(numstring);
    len = str2big(numstring,x,hilf);
    cpyarr(x,len,AriBuf);
    numptr->len = len;
    numptr->digits = AriBuf;
    numptr->expo = decexp;
    numptr->sign = 0;
    flodec2bin(prec,numptr,hilf);
    return(FLOATTOK);
}
/*----------------------------------------------------------*/
PRIVATE truc Satoi()
{
    return(Snumval(atoisym));
}
/*----------------------------------------------------------*/
PRIVATE truc Satof()
{
    return(Snumval(atofsym));
}
/*----------------------------------------------------------*/
PRIVATE truc Snumval(symb)
truc symb;
{
    truc obj;
    char *str;
    unsigned len;
    int argn, count, tok;

    obj = eval(ARG1PTR(evalStkPtr));
    if(Tflag(obj) != fSTRING) {
        error(symb,err_str,obj);
        return(brkerr());
    }
    len = STRlen(obj);
    if(len >= strbufSize-3) {
        error(symb,err_2long,mkfixnum(len));
        return(brkerr());
    }
    str = STRING(obj);
    tok = numval(str,&count);
    if(symb == atoisym) {
        if(tok == INUMTOK)
            obj = mkint(Curnum.sign,Curnum.digits,Curnum.len);
        else {
            count = 0;
            obj = zero;
        }
    }
    else {  /* symb == atofsym */
        if(tok != FLOATTOK)
            curfltprec = deffltprec();
        if(tok != FLOATTOK && tok != INUMTOK) {
            count = 0;
            obj = fltzero(curfltprec);
        }
        else {
            obj = mkfloat(curfltprec,&Curnum);
        }
    }
    argn = *ARGCOUNTPTR(evalStkPtr);
    if(argn == 2) {
        Lvalassign(ARGNPTR(evalStkPtr,2),mkfixnum(count));
    }
    return(obj);
}
/*----------------------------------------------------------*/
PRIVATE int numval(str,pcount)
char *str;
int *pcount;
{
    char *buf;
    ifun0 nextch;
    int sign;
    int n, count, tok;

    buf = StrBuf;   /* global variable, == AriBuf */
    n = strncopy(buf,str,strbufSize-3);
    buf[n] = EOL;
    buf[n+1] = EOL;
    buf[n+2] = 0;
    nextch = readfrom(buf);
    buf[0] = nextch();
    sign = signinp(nextch,buf);
    tok = numinp(nextch,buf,strbufSize);
    Curnum.sign = sign;
    if(tok == aERROR)
        count = 0;
    else {
        count = cStrptr - buf;
        count--;
    }
    *pcount = count;

    return(tok);
}
/*----------------------------------------------------------*/
/*
** reading one item (of data type given by *ptr) in function readln
*/
PRIVATE int readlnitem(nextch,ptr)
ifun0 nextch;
truc *ptr;
{
    int ch, typ, flg, tok, tok1, sign, maxlen;
    char *cptr;
    truc obj;
    truc *vptr, *varptr;

    ch = rChar;
    WORKpush(*ptr);
    typ = Lvaladdr(workStkPtr,&varptr);
    if(typ == vUNBOUND) {
        if(isdecdigit(ch))
            tok = INUMTOK;
        else
            tok = STRINGTOK;
    }
    else if(typ == vBOUND) {
        flg = *FLAGPTR(varptr);
        if(flg == fCHARACTER)
            tok = CHARTOK;
        else if(flg == fSTRING)
            tok = STRINGTOK;
        else if(flg == fFIXNUM || flg == fBIGNUM)
            tok = INUMTOK;
        else if(flg >= fFLTOBJ)
            tok = FLOATTOK;
        else
            tok = aERROR;
    }
    else if(typ == vARRELE) {
        ARGpush(varptr[1]);
        *argStkPtr = eval(argStkPtr);
        flg = *FLAGPTR(argStkPtr);
        if(flg == fSTRING)
            tok = CHARTOK;
        else if(flg == fVECTOR) {
            vptr = VECTORPTR(argStkPtr);
            flg = *FLAGPTR(vptr);
            if(flg == fCHARACTER)
                tok = CHARTOK;
            else if(flg == fSTRING)
                tok = STRINGTOK;
            else if(flg == fFIXNUM || flg == fBIGNUM)
                tok = INUMTOK;
            else if(flg >= fFLTOBJ)
                tok = FLOATTOK;
            else
                tok = aERROR;
        }
        else {
            tok = aERROR;
        }
        ARGpop();
    }
    else if(typ == vRECFIELD) {
        error(rec_sym,err_imp,voidsym);
        tok = aERROR;
    }
    else {
        tok = aERROR;
    }
        if(ch == EOL && tok != STRINGTOK) {
        tok = aERROR;
        }
    if(tok == aERROR) {
        goto ausgang;
    }
    if(tok == CHARTOK) {
        obj = mkchar(ch);
        nextch();
    }
    else if(tok == STRINGTOK) {
        maxlen = strbufSize;
        cptr = StrBuf;
        while(ch != EOL && ch != EOF && --maxlen > 0) {
            *cptr++ = ch;
            ch = nextch();
        }
        *cptr = 0;
        obj = mkstr(StrBuf);
    }
    else {
        StrBuf[0] = ch;
        sign = signinp(nextch,StrBuf);
        tok1 = numinp(nextch,StrBuf,strbufSize);
        Curnum.sign = sign;
        if(tok1 == aERROR || tok1 > tok) {
            tok = aERROR;
            goto ausgang;
        }
        if(tok == FLOATTOK)
            obj = mkfloat(curfltprec,&Curnum);
        else
            obj = mkint(Curnum.sign,Curnum.digits,Curnum.len);
    }
    Lvalassign(workStkPtr,obj);
  ausgang:
    WORKpop();
    return(tok);
}
/*----------------------------------------------------------*/
PRIVATE truc Sreadln()
{
    char buf[IOBUFSIZE+2];
    int ch, i, k, count, argn, typ, ret;
    truc *ptr;
    truc strom;
    ifun0 nextch;

    k = 1;
    strom = tstdin;
    argn = *ARGCOUNTPTR(evalStkPtr);
    if(argn >= 1) {
        typ = Lvaladdr(ARG1PTR(evalStkPtr),&ptr);
        if(typ == vBOUND && *FLAGPTR(ptr) == fSTREAM) {
            if(!isinpfile(ptr,aTEXT)) {
                error(readlnsym,err_tinp,voidsym);
                return(brkerr());
            }
            else {
                strom = *ptr;
                k = 2;
            }
        }
    }
    nextch = lnread(&strom,buf);
    ch = nextch();
    for(count=0,i=k; (i<=argn)&&(ch != EOF); i++,count++) {
        ret = readlnitem(nextch,ARGNPTR(evalStkPtr,i));
        if(ret == aERROR) {
            break;
        }
        ch = rChar;
    }
    if ((ch == EOF) && (count == 0))
        count = -1;
    return(mksfixnum(count));
}
/*----------------------------------------------------------*/
PUBLIC int rerror(sym1,mess,sym2)
truc sym1, sym2;
char *mess;
{
    char *st = "terminal input";
    char *sf = "loaded file";
    wtruc src;

    if(inpsource == STRINGINPUT)
        strcopy(OutBuf,"error while reading from string");
    else if(inpsource == READLNINPUT)
        strcopy(OutBuf,"error in function readln");
    else {
        if(inpsource == TERMINALINP)
            src = (wtruc)st;
        else    /* FILEINPUT */
            src = (wtruc)sf;
        s2form(OutBuf,
        "error in line <= ~D of ~A",(wtruc)(cStream->lineno),src);
    }
    fprintline(tstderr,OutBuf);
    return(error(sym1,mess,sym2));
}
/********************************************************************/


