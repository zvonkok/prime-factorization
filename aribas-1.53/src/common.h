/****************************************************************/
/* file common.h

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
WWW	http://www.mathematik.uni-muenchen.de/~forster

The latest version of ARIBAS can be obtained by anonymous ftp from

    ftp.mathematik.uni-muenchen.de

directory

    pub/forster/aribas
*/
/****************************************************************/
/*
** common.h
** header definitions and macros which are used
** by more than one C-file
**
** date of last change
** 1997-02-11	moved defn of ARIBUFSIZE to alloc.c
** 1997-04-13	reorg (newintsym)
** 1997-07-04	new #define READLNINPUT
** 1997-11-08	some defines for DjGPP changed
** 2001-03-30	Win32GUI, genWinGUI
** 2002-03-27   WORKnpush, VECSTRUCTPTR
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
** to compile ARIBAS, one of the following symbols
** must be defined
*/
/************
#define ATARIST

#define MsDOS

#define Dos386

#define Dos286

#define Win32GUI

#define Win32CON

#define DjGPP

#define UNiX

#define UNiX64

#define SCOUNiX

#define LiNUX
#define GtK	
************/



/*-----------------------------------------------------------------*/

#ifdef UNiX64
#define ARCHITEC	"UNiX64"
#define genUNiX
#define ALIGN8
#endif

#ifdef UNiX
#define ARCHITEC	"UNIX"
#define genUNiX
#endif

#ifdef SCOUNiX
#define ARCHITEC	"SCO-UNIX"
#define genUNiX
#define M_3264
#endif

#ifdef LiNUX
#ifdef GtK
#define genWinGUI
#define ARCHITEC	"LINUX-GTK"
#else
#define ARCHITEC	"LINUX386"
#endif
#define genUNiX
#define M_3264
#endif

#ifdef genUNiX
#define DOSorUNiX
#define UNiXorGCC
#define M_LARGE
#endif

#ifdef Win32GUI
#define ARCHITEC    "Win32GUI"
#define MsWIN32
#define genWinGUI
#endif

#ifdef Win32CON
#define ARCHITEC    "Win32Console"
#define MsWIN32
#define LINEINPUT
#endif

#ifdef MsWIN32
#ifndef NO_ASSEMB
#define M_3264
#endif
#define M_LARGE
#endif

#ifdef Dos386
#define ARCHITEC	"MS-DOS 386"
#define MsDOS
#define M_3264
#endif

#ifdef Dos286
#define ARCHITEC	"MS-DOS 286"
#define MsDOS
#endif

#ifdef MsDOS
#ifndef ARCHITEC
#define ARCHITEC	"MS-DOS 086"
#endif
#define DOSorUNiX
#define DOSorTOS
#define M_SMALL
#endif

#ifdef DjGPP
#define ARCHITEC	"DJGPP386"
#define DOSorUNiX
#define UNiXorGCC
#define M_3264
#define M_LARGE
#endif

#ifdef ATARIST
#define ARCHITEC	"ATARI-ST"
#define DOSorTOS
#define M_SMALL
#endif

#ifdef M_LARGE
#define FPREC_HIGH
#endif

/*-----------------------------------------------------------------*/
#ifdef PROTO
#define _(x)	x
#else
#define _(x)	()
#endif


#define PRIVATE		static
#define PUBLIC

/*-----------------------------------------------------------------*/
#ifdef MsDOS
#define SHIFTSTAT	(*(unsigned char *)0x00000417)
#endif
#ifdef ATARIST
#include <tos.h>
#define SHIFTSTAT	Kbshift(-1)
#endif
#ifdef DOSorTOS
#define SHIFT		2	/* linke Shift-Taste */
#define CONTROL		4
#define SHCTRL		(CONTROL | SHIFT)
#ifndef INTERRUPT
#define INTERRUPT	((SHIFTSTAT & SHCTRL) == SHCTRL)
#endif
#endif

#ifdef UNiXorGCC
#define INTERRUPT	Unterbrech
#endif
#ifdef MsWIN32
#define INTERRUPT	Unterbrech
#endif
/*-----------------------------------------------------------------*/

#define PRIMTABSIZE	2048	/* size of prime bitvector (word2's) */
#define MAXCOLS		  80	/* max no. of columns on the screen */
#define ARGCMAX		  64	/* maximal length of ARGV */

#ifdef M_LARGE
#define IOBUFSIZE  1024	/* size of output buffer */
#define MAXPFADLEN	256	/* avoid name collision with MAXPATHLEN */
#else
#define IOBUFSIZE	256	/* size of output buffer */
#define MAXPFADLEN	128
#endif

#ifdef genUNiX
#define SEPPATH		':'
#else
#define SEPPATH		';'
#endif
#ifdef UNiXorGCC
#define SEPDIR		'/'
#else
#define SEPDIR		'\\'
#endif
#ifdef genUNiX
#define SEP_DIR		"/"
#endif
#ifdef DOSorTOS
#define SEP_DIR		"\\"
#endif
#ifdef DjGPP
#define SEP_DIR		"/\\"
#endif
#ifndef SEP_DIR
#define SEP_DIR		"\\/"
#endif
/*-----------------------------------------------------------------*/
/* values for flag in symbol structure */
	/* all values are even, odd values reserved */
#define sUNBOUND     0x00	/* unbound symbol */
#define sVARIABLE    0x02	/* bound variable */
#define sCONSTANT    0x04	/* user defined constant */
#define sFUNCTION    0x06	/* user defined function */
#define sVFUNCTION   0x08	/* user defined function with var args */

#define sTYPEDEF     0x0C	/* user defined type */

#define sGCMOVEBIND  0x0E	/* mask used during garbage collection */

#define sSYSTEM	     0x10	/* all following are system symbols */
#define sFBINARY     0x10	/* builtin function */
#define sSBINARY     0x20	/* builtin special form */
#define sINFIX	     0x30	/* infix operator */
#define sSCONSTANT   0x40	/* system constant */
#define sSYMBCONST   0x50	/* symbolic constant */
#define sSYSSYMBOL   0x60	/* system symbol */
#define sPARSAUX     0x70	/* special treatment during parsing */
#define sTYPESPEC    0x80	/* type specifier */
#define sDELIM	     0xA0	/* delimiter do, then, else, .., end */
#define sINTERNAL    0xE0	/* internal symbol */
#define sINTERNVAR   0xE2	/* internal var, moved during gc */
#define sSYSTEMVAR   0x12	/* system var, moved during gc */

#define sEXTFUNCTION	(0x100 | sFUNCTION)	/* used during parsing */

#define mGLOBAL		0x8000	/* to mark external variable */
#define mLOCCONST	0x7000	/* to mark local constants */

/* values for flag of trucs */
	/* odd values are fixed during garbage collection */
#define fSYMBOL		  1
#define fLSYMBOL	  3	/* local symbol */
#define fRSYMBOL	  5	/* reference to symbol */
#define fLRSYMBOL	  7	/* reference to local symbol */
#define fTMPCONST	  9	/* temporary reference to local const */

#define fFUNEXPR	 10	/* until fSELFEVAL are kind of functions */
#define fSPECIAL0	 11	/* special form, no argument */
#define fSPECIAL1	 10	/* special form, 1 argument */
#define fSPECIAL2	 12	/* special form, 2 arguments */
#define fSPECIALn	 14	/* special form, n arguments */
#define fBUILTIN1	 16	/* built-in function, 1 argument */
#define fBUILTIN2	 18	/* built-in function, 2 arguments */
#define fBUILTINn	 20	/* built-in function, n arguments */
#define fFUNCALL	 22	/* call of user defined function */
#define fCOMPEXPR	 24	/* compound expression */
#define fIFEXPR		 26	/* if statement */
#define fWHILEXPR	 28	/* while statement */
#define fFOREXPR	 30	/* for statement */

#define fSELFEVAL	 32	/* all following are self evaluating */
#define fFUNDEF		 32	/* user function definition */

#define fPOINTER	 34
#define fTUPLE		 36
#define fSTACK		 38
#define fSTREAM		 40

#define fRECORD		 48
#define fVECTLIKE0	 50
#define fVECTOR		 50
#define fCONSTLIT	 52	/* all following are literal objects */
#define fSTRING		 52
#define fBYTESTRING	 54
#define fVECTLIKE1	 54

#define fBOOL		 57
#define fCHARACTER	 59
#define fINTTYPE0    60
#define fGF2NINT     60
#define fFIXNUM		 61
#define fBIGNUM		 62
#define fINTTYPE1    62
#define fFLTOBJ		128
#define fHUGEFLOAT  (fFLTOBJ + HUGEFLTBIT)

#define FIXMASK	      0x01	/* mask for checking fixed objects */
#define PRECMASK      0x3E	/* mask for retrieving float precision */
#define FLTZEROBIT    0x01	/* for floats = 0 */
#define HUGEFLTBIT    0x40	/* huge floats */
#define HUGEMASK      0x7F
#define FSIGNBIT      0x80	/* sign bit in signum of floats */
#define GCMARK	      0xFF	/* used during garbage collection */
#define MINUSBYTE     0xFF	/* sign of negative numbers */

/* streams */
#define INSTREAM	1	/* input stream bit */
#define OUTSTREAM	2	/* output stream bit */
#define IOMASK		3
#define APPEND		8
#define BINARY	   16
#define aTEXT		0	/* binary bit not set */
            /* avoid nameclash with TEXT in windows header */
#define DEVICE		32	/* console, printer */
#define NOSTREAM	0	/* unconnected stream */


/* values used for reading and printing */
#define EOL			'\n'
#define FORMFEED	'\014'
#define TABESC		'\036'	/* escape char for compression */
#define ZESC		'\177'

/* tokens for parser */

#define EOFTOK	   -1      /* end-of-file token */
#define EOLTOK		0      /* end-of-line token */

#define Z1TOK	-101

#define LPARENTOK	10	/* ( */
#define RPARENTOK	11	/* ) */
#define LBRACKTOK	12	/* [ */
#define RBRACKTOK	13	/* ] */
#define LBRACETOK	14	/* { */
#define RBRACETOK	15	/* } */
#define BEGCOMMTOK	18	/* (* */
#define ENDCOMMTOK	19	/* *) */
#define COMMATOK	20
#define COLONTOK	21
#define SEMICOLTOK	22
#define DOTTOK		30
#define DOTDOTTOK	31
#define RECDOTTOK	32	/* dot als record-field Trenner */
#define DEREFTOK	40	/* ^ for pointer dereferencing */
#define DOLLARTOK	50
#define HISTORYTOK	60	/* !,!!,!!!,!a,!b,!c */
#define QUESTIONTOK	70	/* ? */

#define ASSIGNTOK	101	/* odd value means right associative */

#define ORTOK		201
#define ANDTOK		211
#define NOTTOK		221

#define EQTOK		300
#define NETOK		310
#define LTTOK		320
#define LETOK		330
#define GTTOK		340
#define GETOK		350

#define PLUSTOK		400
#define MINUSTOK	410

#define TIMESTOK	500
#define DIVIDETOK	510
#define DIVTOK		520
#define MODTOK		540

#define UMINUSTOK	601

#define POWERTOK	701

#define BOOLTOK	       2010
#define CHARTOK	       2020	/* character token */
#define INUMTOK	       2030	/* integer number token */
#define FLOATTOK       2040
#define GF2NTOK        2045 /* gf2n_int token */
#define STRINGTOK      2050	/* string token */
#define BSTRINGTOK     2052	/* byte_string token */
#define SYMBOLTOK      2060	/* symbol token */
#define VECTORTOK      2070	/* vector token */

/* Lvals */
#define vUNBOUND	 0
#define vBOUND		 1
#define vCONST		 2
#define vVECTOR	    10
#define vARRELE		11
#define vSUBARRAY	12
#define vRECFIELD	20
#define vPOINTREF	30

/* defines for diverse return values */
#define EXITREQ		-1	/* possible return value of loadaux */
#define aERROR	    -32768	/* error return value for int functions */
            /* avoid nameclash with ERROR in windows header */
#define LONGERROR  -2147483647	/* error return value for int4 functions */
#define RESET	     0x1111	/* value handed by longjmp if reset */
#define HALTRET	     0x2222	/* value handed by longjmp if halt */


#define MAXFLTLIM  0x3FFF80
#define MOSTNEGEX -0x400000	/* exponent for float number zero */

/** used by scanner and parser **/
#define TERMINALINP	1
#define FILEINPUT	2
#define STRINGINPUT	3
#define READLNINPUT	4

/*-----------------------------------------------------------------*/
#ifdef M_LARGE
typedef int int4;		/* 4-byte integer */
typedef unsigned int word4;
#else
typedef long int4;		/* 4-byte integer */
typedef unsigned long word4;
#endif
typedef short int2;		/* 2-byte integer */
typedef unsigned short word2;
typedef unsigned char byte;

typedef word4	truc;
typedef void    *wtruc;

typedef truc	*trucptr;

typedef truc	(* funptr)  _((void));
typedef truc	(* funptr1) _((int k));
typedef int	    (* ifun0)   _((void));
typedef int	    (* ifun)    _((int x));
typedef int	    (* ifunaa)  _((word2 *arr1, int n1, word2 *arr2, int n2));
typedef int	    (* ifuntt)  _((truc *ptr1, truc *ptr2));

typedef struct {
	byte	b0;
	byte	b1;
	word2	ww;
} packet;

typedef struct {
	word2	w0;
	word2	ww;
} arr2;

typedef union {
	word4	xx;
	arr2	yy;
	packet	pp;
} variant;

typedef union {
    truc t;
    wtruc w;
} wvariant;

struct symbol {		/* symbol structure */
	truc		ident;
	variant 	cc;		/* information for syntax checking */
	wvariant	bind;		/* symbol binding */
	char		*name;		/* symbol name */
	truc		*link;		/* link to next symbol */
};
#define OFFSETcc        4
#define OFFSETcc1       6
#define OFFSETbind	    8
#define OFFSETname	   (OFFSETbind + sizeof(wtruc))
#define OFFSETlink     (OFFSETbind + 2*sizeof(wtruc))
#define SIZEOFSYMBOL	(sizeof(struct symbol)/sizeof(truc))

struct intsymbol {	/* internal symbol structure */
	truc		ident;
	variant 	cc;
	wvariant	bind;		/* symbol binding */
	char		*name;		/* symbol name */
};
#define SIZEOFINTSYMBOL	   (sizeof(struct intsymbol)/sizeof(truc))

struct floatcell {	/* float */
	byte	flag;
	byte	signum;		/* same position as in bigcell */
	int2	expo;
	word2	digi0;
	word2	digi1;
};
#define OFFSETexpo		2
#define OFFSETflodig	4
#define SIZEOFFLOAT(prec)	(unsigned)(1 + (prec>>1))

struct bigcell {	/* for big integers or gf2nint's */
	byte	flag;		/* = fBIGNUM or fGF2NINT */
	byte	signum;		/* same position as in floatcell */
	word2	len;		/* same position as in vector */
	word2	digi0;
	word2	digi1;
};
/*
** signum = 0 for nonnegative numbers,
** signum = MINUSBYTE for negative numbers
*/
#define OFFSETsignum	1
#define OFFSETbiglen	2
#define OFFSETbigdig	4
#define SIZEOFBIG(len)	(1 + (((unsigned)(len)+1)>>1))

typedef struct {
	long	expo;
	int	    sign;
	int	    len;
	word2	*digits;
} numdata;

struct strcell {	/* string */
	byte	flag;		/* = fSTRING */
	byte	flg2;
	word2	len;		/* same position as in struct vector */
	char	ch0;
	char	ch1;
	char	ch2;
	char	ch3;
};
#define OFFSETstrlen	2
#define OFFSETstring	4
#define SIZEOFSTRING(len)   (2 + ((unsigned)(len)>>2))	/* includes '\0' */

struct vector {
	byte	flag;		/* = fVECTOR or = fTUPLE */
	byte	flg2;
	word2	len;		/* same position as in bigcell */
	truc	ele0;
};
#define OFFSETveclen	2
#define OFFSETvector	4
#define SIZEOFTUPLE(len)	(1 + (unsigned)(len))
#define SIZEOFVECTOR(len)	(unsigned)(len ? (1 + (len)) : 2)
/* for arrays of length 0, ele0 contains type */

struct record {		/* also used for pointers */
	byte	flag;		/* fRECORD or fPOINTER */
	byte	flg2;
	word2	len;		/* same position as in vector */
	truc	recdef;		/* fTUPLE with field names and types */
	truc	field1;
	truc	field2;
};
#define SIZEOFRECORD(len)	 (2 + (unsigned)(len))
#define OFFSETfield1	8
/* for pointers, len = 1, and field1 contains truc
** designating the record pointed to, or nil
*/

struct stream {	   /* I/O stream structure */
	byte flag;
	byte mode;	/* one of INSTREAM,OUTSTREAM,NOSTREAM */
	int2 pos;	/* current position in line */
	int4 lineno;	/* current line number */
	int4 ch;	/* current character */
	int4 tok;	/* current token */
	FILE *file;	/* the file associated with stream */
};
#define OFFSETmode		1
#define OFFSETpos		2
#define OFFSETlineno	4
#define OFFSETch		8
#define OFFSETtok      12
#define OFFSETfile     16
#define SIZEOFSTREAM	(sizeof(struct stream)/sizeof(truc))

struct stack {
	byte flag;
	byte line;
	word2 pageno;
	truc type;	       /* = zero in this implementation */
	truc page;
};

#define OFFSETpage	8
#define SIZEOFSTACK	3	/* unit is sizeof(truc) */

#define PAGELENBITS	5
#define PAGELEN		32	/* 2**PAGELENBITS */
struct stackpage {
	byte flag;		/* fVECTOR */
	byte flg2;
	word2 len;		/* = PAGELEN + 1 */
	truc data[PAGELEN];
	truc prevpage;		/* for tail rec elimination during gc */
};

struct opnode {
	truc	op;
	truc	arg0;
	truc	arg1;
};
#define OFFSETarg0	4
#define OFFSETarg1	8
#define SIZEOFOPNODE(n) (1+(unsigned)(n))	/* unit is sizeof(truc) */

struct funode {
	truc	op;
	truc	argno;		/* number of args as FIXNUM */
	truc	arg1;		/* same position as in opnode */
};
#define OFFSETargcount	6
#define OFFSETargn(n)	(4 + ((n)<<2))
#define SIZEOFFUNODE(n) (2+(unsigned)(n))	  /* unit is sizeof(truc) */

struct fundef {
	byte	flag;		/* = fFUNDEF */
	byte	flg2;		/* number of optional arguments */
	word2	argc;		/* number of formal arguments */
	truc	varno;		/* number of local vars as FIXNUM */
	truc	body;
	truc	parms;		/* default initializations of formal args */
	truc	vars;		/* list of initializations of local vars */
};
#define OFFSETfargc		2
#define OFFSETvarcount	6
#define OFFSETbody		8
#define OFFSETparms    12
#define OFFSETvars     16
#define OFFS4body		2
#define SIZEOFFUNDEF	5		/* unit is sizeof(truc) */

struct compnode {	/* compound statement */
	byte	flag;
	byte	flg2;
	word2	len;
	truc	expr0;
	truc	expr1;
};
#define SIZEOFCOMP(len) (1+(unsigned)(len))	  /* unit is sizeof(truc) */
#define OFFSETcomplen	2

struct fornode {
	byte	flag;
	byte	flg2;
	word2	len;	/* len = 4+bodylen, same position as in compnode */
	truc	runvar;
	truc	start;
	truc	end;
	truc	inc;
	truc	body0;
	truc	body1;
};

/*----------------------------------------------------------------*/
/*
** MACROS
*/

/* TAddress, SAddress, Taddress, Saddress defined in mem0.c */
#define bTAddress(p)	((byte *)TAddress(p))				
#define bSAddress(p)	((byte *)SAddress(p))				
#define bTaddress(x)	((byte *)Taddress(x))				
#define bSaddress(x)	((byte *)Saddress(x))				


#define symptr(x)	((struct symbol *)Saddress(x))			  
#define streamptr(x)	((struct stream *)Taddress(x))			  
#define stringptr(x)	((struct strcell *)Taddress(x))			  
#define recordptr(x)	((struct record *)Taddress(x))
								  
#define FLAG(x)		*(byte *)&(x)					  
#define STRING(x)	(char *)(bTaddress(x) + OFFSETstring)		  
#define STRlen(x)	*(word2 *)(bTaddress(x) + OFFSETstrlen)		  
#define STRMlineno(x)	*(int4 *)(bTaddress(x) + OFFSETlineno)		  
#define VECTOR(x)	((truc *)(bTaddress(x) + OFFSETvector))		  
#define VEClen(x)	*(word2 *)(bTaddress(x) + OFFSETveclen)		  
#define PTRtarget(x)	*((truc *)(bTaddress(x) + OFFSETfield1))	  
#define NODEarg0(x)	*((truc *)(bTaddress(x) + OFFSETarg0))		  
								  
#define FLAGPTR(p)	(byte *)(p)
#define SEGPTR(p)	((byte *)(p) + 1)				  
#define SIGNPTR(p)	((byte *)(p) + 1)				  
#define FLG2PTR(p)	((byte *)(p) + 1)
#define OFFSPTR(p)	((word2 *)(p) + 1)				  
#define WORD2PTR(p)	((word2 *)(p) + 1)				  
#define INT2PTR(p)	((int2 *)(p) + 1)				  
#define ARGCPTR(p)	(word2 *)((byte *)(p) + OFFSETargcount)		  
#define VARCPTR(p)	(word2 *)((byte *)(p) + OFFSETvarcount)		  
#define PARMSPTR(p)	(truc *)((byte *)(p) + OFFSETparms)		  
#define VARSPTR(p)	(truc *)((byte *)(p) + OFFSETvars)

#define STREAMPTR(p)	((struct stream *)TAddress(p))
#define STRCELLPTR(p)	((struct strcell *)TAddress(p))
#define RECORDPTR(p)	((struct record *)TAddress(p))
#define VECSTRUCTPTR(p) ((struct vector *)TAddress(p))
#define STREAMTOKPTR(p)	 ((int2 *)(bTAddress(p) + OFFSETtok))
#define STREAMMODEPTR(p) ((byte *)(bTAddress(p) + OFFSETmode))
#define STREAMLINOPTR(p) ((int4 *)(bTAddress(p) + OFFSETlineno))
#define STRLENPTR(p)	((word2 *)(bTAddress(p) + OFFSETstrlen))
#define STRINGPTR(p)	((char *)(bTAddress(p) + OFFSETstring))
#define BYTEPTR(p)		((byte *)(bTAddress(p) + OFFSETstring))
#define VECLENPTR(p)	((word2 *)(bTAddress(p) + OFFSETveclen))
#define VECTORPTR(p)	((truc *)(bTAddress(p) + OFFSETvector))
#define PTARGETPTR(p)	((truc *)(bTAddress(p) + OFFSETfield1))
#define SIGNUMPTR(p)	(bTAddress(p) + OFFSETsignum)
#define BIGLENPTR(p)	((word2 *)(bTAddress(p) + OFFSETbiglen))
#define BIGNUMPTR(p)	((word2 *)(bTAddress(p) + OFFSETbigdig))
#define FLTEXPOPTR(p)	((int2 *)(bTAddress(p) + OFFSETexpo))

#define ARGCOUNTPTR(p)	((word2 *)(bTAddress(p) + OFFSETargcount))
#define OPNODEPTR(p)    (truc *)bTAddress(p)
#define ARG0PTR(p)		((truc *)(bTAddress(p) + OFFSETarg0))
#define ARG1PTR(p)		((truc *)(bTAddress(p) + OFFSETarg1))
#define ARGNPTR(p,n)	((truc *)(bTAddress(p) + OFFSETargn(n)))

#define COMPLENPTR(p)	((word2 *)(bTAddress(p) + OFFSETcomplen))
#define FUNARGCPTR(p)	((word2 *)(bTAddress(p) + OFFSETfargc))
#define FUNVARCPTR(p)	((word2 *)(bTAddress(p) + OFFSETvarcount))
#define FUNVARSPTR(p)	((truc *)(bTAddress(p) + OFFSETvars))

#define SYMPTR(p)		((struct symbol *)SAddress(p))
#define SYMFLAGPTR(p)	bSAddress(p)
#define SYMBINDPTR(p)	((truc *)(bSAddress(p) + OFFSETbind))
#define SYMWBINDPTR(p)	((wtruc *)(bSAddress(p) + OFFSETbind))
#define SYMNAMEPTR(p)	(*(char **)(bSAddress(p) + OFFSETname))
#define SYMCCPTR(p)		((word4 *)(bSAddress(p) + OFFSETcc))
#define SYMCC0PTR(p)	((word2 *)(bSAddress(p) + OFFSETcc))
#define SYMCC1PTR(p)	((word2 *)(bSAddress(p) + OFFSETcc1))
#define LSYMBOLPTR(p)	(basePtr + *WORD2PTR(p))
#define LRSYMBOLPTR(p)	(ArgStack + *WORD2PTR(p))
#define LSYMFLAGPTR(p)	(byte *)(basePtr + *WORD2PTR(p))

#define STREAMmode(x)	*(byte *)(bTaddress(x) + OFFSETmode)
#define STREAMch(x)		*(int2 *)(bTaddress(x) + OFFSETch)
#define STREAMtok(x)	*(int2 *)(bTaddress(x) + OFFSETtok)		 
#define STREAMpos(x)	*(int2 *)(bTaddress(x) + OFFSETpos)
#define STREAMfile(x)	*(FILE **)(bTaddress(x) + OFFSETfile)		 
								 
#define SYMflag(x)	*bSaddress(x)					 
#define SYMname(x)	*(char **)(bSaddress(x) + OFFSETname)		 
#define SYMbind(x)	*(truc *)(bSaddress(x) + OFFSETbind)		 
#define SYMbind2(x)	*(word2 *)(bSaddress(x) + OFFSETbind + 2)	 
#define SYMlink(x)	*(truc **)(bSaddress(x) + OFFSETlink)
#define SYMcc(x)	*(word4 *)(bSaddress(x) + OFFSETcc)		 
#define SYMcc0(x)	*(word2 *)(bSaddress(x) + OFFSETcc)		 
#define SYMcc1(x)	*(word2 *)(bSaddress(x) + OFFSETcc1)		 

/*----- pushes and pops ------------------------------------------*/

#define EVALpush(obj) \
	if(--evalStkPtr > workStkPtr) *evalStkPtr = (obj); \
	else reset(err_evstk)
#define EVALpop()	evalStkPtr++

#define WORKpush(obj) \
	if(++workStkPtr < evalStkPtr) *workStkPtr = (obj); \
	else reset(err_wrkstk)
#define WORKpop()	workStkPtr--
#define WORKnpop(n)	workStkPtr -= (n)
#define WORKretr()	*workStkPtr--
#define WORKnpush(n) \
 	if(!((workStkPtr += (n)) < evalStkPtr)) reset(err_wrkstk)
#define WORKspace(n) \
	(workStkPtr < evalStkPtr-(n)-32 ? workStkPtr += (n) : NULL)

#define ARGpush(obj) \
	if(++argStkPtr < saveStkPtr) *argStkPtr = (obj); \
	else reset(err_astk)
#define ARGretr()	*argStkPtr--
#define ARGpop()	argStkPtr--
#define ARGnpop(n)	argStkPtr -= (n)

#define SAVEpush(obj) \
	if(--saveStkPtr > argStkPtr) *saveStkPtr = (truc)(obj); \
	else reset(err_savstk)
#define SAVEretr()	(trucptr)(*saveStkPtr++)
#define SAVEpop()	saveStkPtr++
#define SAVEtop()	(trucptr)(*saveStkPtr)
#define SAVEnpop(n)	saveStkPtr += (n)
#define SAVEspace(n) \
	(saveStkPtr > argStkPtr+(n) ? saveStkPtr -= (n) : NULL)

#define PARSpush(obj) \
	if(++argStkPtr < saveStkPtr) *argStkPtr = (obj); \
	else reset(err_pstk)
#define PARSpop()	argStkPtr--
#define PARSnpop(n)	argStkPtr -= (n)
#define PARSretr()	*argStkPtr--

/*--------------------- external declarations -----------------------*/
#ifdef Win32GUI
#include "ariwin.h"
#endif
#ifdef GtK
#include "gnariwin.h"
#endif

/* errtext.c */
extern char	*err_funest, *err_funame, *err_2ident,
		*err_type, *err_btype, *err_mism,
		*err_synt, *err_args, *err_pars, *err_parl,
		*err_varl, *err_unvar,
		*err_memory, *err_2large, *err_memev, *err_garb,
		*err_evstk, *err_wrkstk, *err_astk, *err_savstk, *err_pstk,
		*err_imp, *err_case, *err_rec, *err_intr,
		*err_rparen, *err_0rparen, *err_0lparen, 
		*err_0brace, *err_0rbrack,
		*err_brstr, *err_bchar, *err_inadm,
		*err_stkv, *err_stke, *err_stkbig, *err_nil, *err_vpoint,
		*err_filv, *err_outf, *err_tout, *err_bout,
		*err_inpf, *err_tinp, *err_binp,
		*err_then, *err_end,
		*err_ovfl, *err_div, *err_2big, *err_float, *err_bool, 
		*err_int, *err_intt, *err_fix, *err_pfix, *err_p4int,
        *err_odd, *err_oddprim,
		*err_char, *err_chr, *err_2long, *err_iovfl,
		*err_num, *err_pnum, *err_p0num, *err_intvar,
		*err_pbase, *err_range, *err_irange, *err_var, *err_lval,
		*err_vsym, *err_vasym, *err_sym, *err_gsym, *err_sym2,
		*err_buf, *err_str, *err_bystr, *err_vbystr,
		*err_arr, *err_syarr, *err_sarr, *err_vect, *err_field, *err_open,
		*err_bltin, *err_ubound, *err_ufunc;

/* alloc.c */
extern void inialloc	_((void));
extern int memalloc	_((int mem));
extern void dealloc	_((void));
extern void resetarr	_((void));
extern int initend	_((void));
extern int tempfree	_((int flg));
extern int inpack	_((truc obj, truc pack));
extern char *stringalloc  _((unsigned int size));
extern unsigned getblocksize  _((void));
extern size_t new0	_((unsigned int size));
extern truc newobj	_((int flg, unsigned int size, trucptr *ptraddr));
extern truc new0obj	_((int flg, unsigned int size, trucptr *ptraddr));
extern unsigned obj4size   _((int type, truc *ptr));
extern void cpy4arr	_((truc *ptr1, unsigned len, truc *ptr2));

extern size_t	hashtabSize, aribufSize, auxbufSize, scrbufSize;

extern truc	*Symbol;
extern truc	*Memory[];
extern trucptr *Symtab;
extern truc	*WorkStack, *evalStkPtr, *workStkPtr;
extern truc	*ArgStack, *argStkPtr, *saveStkPtr;
extern truc	*basePtr;
extern word2 *AriBuf, *AriScratch, *AuxBuf, *PrimTab;

/* array.c */
extern void iniarray	_((void));
extern void iniargv		_((int argc, char *argv[]));
extern int stringsplit	_((char *str, char *trenn, word2 *offsets));
extern int indrange		_((truc *ptr, long len, long *pn0, long *pn1));
extern truc arrassign	_((truc *arr, truc obj));
extern truc subarrassign  _((truc *arr, truc obj));
extern void sortarr		_((truc *arr, unsigned len, ifuntt cmpfun));
extern int bytestraddr	_((truc *ptr, truc **ppbstr, byte **ppch,
						unsigned *plen));
extern truc recfassign	_((truc *rptr, truc field, truc obj));
extern truc fullrecassign  _((truc *rptr, truc obj));
extern truc Pdispose	_((truc *ptr));

extern truc	arr_sym, subarrsym, arraysym;
extern truc	stringsym, charsym, bitvecsym, stacksym;
extern truc	bstringsym, bstr_sym, str_sym;
extern truc	mkstrsym, mkbstrsym, mkarrsym, vectorsym, pairsym;
extern truc	recordsym, mkrecsym, rec_sym, pointrsym, derefsym;
extern truc	nullstring, nullbstring;
extern truc	ofsym;

/* arith.c: */
extern void iniarith	_((void));
extern truc  addints	_((truc *ptr, int minflg));
extern unsigned random2 _((unsigned u));
extern unsigned random4 _((unsigned u));
extern int cmpnums		_((truc *ptr1, truc *ptr2, int type));
extern truc scalintvec 	_((truc *ptr1, truc *ptr2));
extern truc Gvecmod     _((int flg));

extern truc	integsym, int_sym, realsym;
extern truc	zero, constone, flt0zero;
extern truc	shfloatsym, sfloatsym, dfloatsym, lfloatsym, xfloatsym;
extern truc	plussym, minussym, uminsym,
		divsym, modsym, divfsym, timessym, powersym;
extern truc	ariltsym, arigtsym, arilesym, arigesym, arieqsym, arinesym;
extern long	maxfltex, maxdecex, exprange;

/* aritx.c */
extern void	iniaritx	_((void));
extern int	prime16	  	_((unsigned u));
extern int	prime32	  	_((word4 u));
extern unsigned fact16    _((word4 u));
extern unsigned trialdiv  _((word2 *x, int n, unsigned u0, unsigned u1));
extern int	jac	  		_((unsigned x, unsigned y));
extern int	jacobi	  	_((int sign, word2 *x, int n, word2 *y, int m,
			     word2 *hilf));
extern int	rabtest	  	_((word2 *x, int n, word2 *aux));
extern int nextprime32 _((word4 u, word2 *x));
extern int pemult	_((word2 *x, int n, word2 *ex, int exlen,
			   word2 *aa, int alen,
			   word2 *mm, int modlen, word2 *z, word2 *hilf));
extern int	modinverse	_((word2 *x, int n, word2 *y, int m, word2 *zz,
				word2 *hilf));
extern int modinv   _((int x, int mm));
extern int	modpower	_((word2 *x, int n, word2 *ex, int exlen,
				word2 *mm, int modlen, word2 *p, word2 *hilf));
extern unsigned modpow    _((unsigned x, unsigned n, unsigned mm));
extern truc	modpowsym;

/* arity.c */
extern void	iniarity  	_((void));
extern void workmess   _((void));
extern void tick   	_((int c));

/* aritz.c */
extern void	iniaritz	_((void));
extern truc gf2nzero, gf2none, gf2nintsym, gf2n_sym;
extern truc polmultsym, polNmultsym, polmodsym, polNmodsym,
        poldivsym, polNdivsym;
extern truc addgf2ns    _((truc *ptr));
extern truc multgf2ns   _((truc *ptr));
extern truc divgf2ns    _((truc *ptr));
extern truc exptgf2n    _((truc *ptr));
extern int fpSqrt   _((word2 *pp, int plen, word2 *aa, int alen,
                    word2 *zz, word2 *hilf));
extern int fp2Sqrt     _((word2 *pp, int plen, word2 *aa, int alen,
                word2 *zz, word2 *hilf));
extern unsigned fp_sqrt    _((unsigned p, unsigned a));

/* aritaux.c */
extern int FltPrec[];
extern int MaxFltLevel;
extern int setfltprec	_((int prec));
extern int deffltprec	_((void));
extern int maxfltprec	_((void));
extern int fltprec		_((int type));
extern int fltpreccode	_((int prec));
extern int refnumtrunc	_((int prec, truc *ptr, numdata *nptr));
extern int getnumtrunc	_((int prec, truc *ptr, numdata *nptr));
extern int getnumalign	_((int prec, truc *ptr, numdata *nptr));
extern int alignfloat	_((int prec, numdata *nptr));
extern int alignfix		_((int prec, numdata *nptr));
extern void adjustoffs	_((numdata *npt1, numdata *npt2));
extern int normfloat	_((int prec, numdata *nptr));
extern int multtrunc	_((int prec, numdata *npt1, numdata *npt2,
			   word2 *hilf));
extern int divtrunc		_((int prec, numdata *npt1, numdata *npt2,
			   word2 *hilf));
extern int pwrtrunc		_((int prec, unsigned base, unsigned a,
			   numdata *nptr, word2 *hilf));
extern int float2bcd	_((int places, truc *p, numdata *nptr,
			   word2 *hilf));
extern int roundbcd		_((int prec, numdata *nptr));
extern int flodec2bin	_((int prec, numdata *nptr, word2 *hilf));
extern void int2numdat	_((int x, numdata *nptr));
extern void cpynumdat	_((numdata *npt1, numdata *npt2));
extern int numposneg	_((truc *ptr));
extern truc wipesign	_((truc *ptr));
extern truc changesign	_((truc *ptr));
extern long intretr		_((truc *ptr));
extern int bigref		_((truc *ptr, word2 **xp, int *sp));
extern int bigretr	_((truc *ptr, word2 *x, int *sp));
extern int twocretr	_((truc *ptr, word2 *x));
extern int and2arr	_((word2 *x, int n, word2 *y, int m));
extern int or2arr	_((word2 *x, int n, word2 *y, int m));
extern int xor2arr	_((word2 *x, int n, word2 *y, int m));
extern int xorbitvec	_((word2 *x, int n, word2 *y, int m));
extern long bit_length	_((word2 *x, int n));
extern int chkintnz _((truc sym, truc *ptr));
extern int chkints	_((truc sym, truc *argptr, int n));
extern int chkint   _((truc sym, truc *ptr));
extern int chkintt  _((truc sym, truc *ptr));
extern int chknums	_((truc sym, truc *argptr, int n));
extern int chknum	_((truc sym, truc *ptr));
extern int chkintvec	_((truc sym, truc *vptr));
extern int chknumvec	_((truc sym, truc *vptr));

/* arito386.asm */
#ifdef M_3264
extern int mult4arr	_((word2 *x, int n, word4 a, word2 *y));
extern int div4arr	_((word2 *x, int n, word4 a, word4 *restptr));
extern word4 mod4arr	_((word2 *x, int n, word4 a));
#endif

/* aritool0.c */
extern int multarr	_((word2 *x, int n, unsigned a, word2 *y));
extern int divarr	_((word2 *x, int n, unsigned a, word2 *restptr));
extern unsigned modarr	_((word2 *x, int n, unsigned a));

extern int sumarr	_((word2 *x, int n, word2 *y));
extern int diffarr	_((word2 *x, int n, word2 *y));
extern int diff1arr	_((word2 *x, int n, word2 *y));
extern int incarr	_((word2 *x, int n, unsigned a));
extern int decarr	_((word2 *x, int n, unsigned a));
extern void cpyarr	_((word2 *x, int n, word2 *y));
extern void cpyarr1	_((word2 *x, int n, word2 *y));
extern int cmparr	_((word2 *x, int n, word2 *y, int m));
extern int shrarr	_((word2 *x, int n, int k));
extern int shlarr	_((word2 *x, int n, int k));
extern void setarr	_((word2 *x, int n, unsigned a));
extern void notarr	_((word2 *x, int n));
extern void andarr	_((word2 *x, int n, word2 *y));
extern void orarr	_((word2 *x, int n, word2 *y));
extern void xorarr	_((word2 *x, int n, word2 *y));
extern unsigned int2bcd _((unsigned x));
extern unsigned bcd2int _((unsigned x));
extern int big2bcd	_((word2 *x, int n, word2 *y));
extern int long2big	_((word4 u, word2 *x));
extern word4 big2long	_((word2 *x, int n));
extern word4 intsqrt	_((word4 u));
extern int bitlen	_((unsigned x));
extern int niblen	_((unsigned x));
extern int bitcount     _((unsigned u));

/* aritools.c */
extern int shiftarr	_((word2 *x, int n, int sh));
extern int lshiftarr	_((word2 *x, int n, long sh));
extern int addarr	_((word2 *x, int n, word2 *y, int m));
extern int subarr	_((word2 *x, int n, word2 *y, int m));
extern int sub1arr	_((word2 *x, int n, word2 *y, int m));
extern int addsarr  _((word2 *x, int n, int sign1,
                    word2 *y, int m, int sing2, int *psign));
extern int multbig	_((word2 *x, int n, word2 *y, int m, word2 *z,
                    word2 *hilf));
extern int divbig	_((word2 *x, int n, word2 *y, int m, word2 *quot,
                    int *rlenptr, word2 *hilf));
extern int modbig	_((word2 *x, int n, word2 *y, int m, word2 *hilf));
extern int modnegbig	_((word2 *x, int n, word2 *y, int m, word2 *hilf));
extern int modmultbig   _((word2 *xx, int xlen, word2 *yy, int ylen,
                word2 *mm, int mlen, word2 *zz, word2 *hilf));
extern int multfix	_((int prec, word2 *x, int n, word2 *y, int m,
                    word2 *z, word2 *hilf));
extern int divfix	_((int prec, word2 *x, int n, word2 *y, int m,
                    word2 *z, word2 *hilf));
extern unsigned shortgcd _((unsigned x, unsigned y));
extern int biggcd	_((word2 *x, int n, word2 *y, int m, word2 *hilf));
extern int power	_((word2 *x, int n, unsigned a, word2 *p,
			   word2 *temp, word2 *hilf));
extern int bigsqrt	_((word2 *x, int n, word2 *z, int *rlenptr,
			   word2 *temp));
extern int lbitlen	_((word4 x));
extern int bcd2big	_((word2 *x, int n, word2 *y));
extern int str2int	_((char *str, int *panz));
extern int str2big	_((char *str, word2 *arr, word2 *hilf));
extern int bcd2str	_((word2 *arr, int n, char *str));
extern int big2xstr	_((word2 *arr, int n, char *str));
extern int digval	_((int ch));
extern int xstr2big	_((char *str, word2 *arr));
extern int ostr2big	_((char *str, word2 *arr));
extern int bstr2big	_((char *str, word2 *arr));
extern int nibdigit	_((word2 *arr, int k));
extern int nibndigit	_((int n, word2 *arr, long k));
extern int nibascii	_((word2 *arr, int k));
extern int hexascii	_((int n));
extern int shiftbcd	_((word2 *arr, int n, int k));
extern int incbcd	_((word2 *x, int n, unsigned a));

/* analysis.c */
extern void inianalys	_((void));
extern int lognum	_((int prec, numdata *nptr, word2 *hilf));
extern int expnum	_((int prec, numdata *nptr, word2 *hilf));

/* eval.c: */
extern void inieval	_((void));
extern truc eval	_((truc *ptr));
extern truc ufunapply	_((truc *fun, truc *arr, int n));
extern truc arreval	_((truc *arr, int n));

/* file.c */
extern void inifile	_((void));
extern int fnextens	_((char *str, char *name, char *extens));
extern int issepdir     _((int ch));
extern int isoutfile	_((truc *strom, int mode));
extern int isinpfile	_((truc *strom, int mode));
extern int loadaux	_((char *str, int verb, char *skipto));
extern long filelen	_((truc *ptr));
extern char	*ariExtens;
extern truc	filesym, eofsym;
extern truc	tstdout, tstdin, tstderr;

/* control.c: */
extern void inicont	_((void));
extern int is_lval  _((truc *ptr));
extern int Lvaladdr	_((truc *ptr, trucptr *pvptr));
extern truc Lvalassign	_((truc *ptr, truc obj));
extern truc Swhile	_((void));
extern truc Sfor	_((void));
extern void Sifaux	_((void));
extern truc Sexit	_((void));
extern truc brkerr	_((void));
extern truc Lconsteval	_((truc *ptr));
extern int Lconstini	_((truc consts));
extern truc unbindsym	_((truc *ptr));
extern truc unbinduser  _((void));

extern truc	exitsym, exitfun, ret_sym, retsym;
extern truc lpbrksym, lpbrkfun, lpcontsym, lpcontfun;
extern truc	equalsym, nequalsym;
extern truc	funcsym, procsym, beginsym, endsym;
extern truc	extrnsym, constsym, typesym;
extern truc	varsym, var_sym, inivarsym;
extern truc	whilesym, dosym, ifsym, thensym, elsifsym, elsesym;
extern truc	forsym, tosym, bysym;

extern truc	not_sym, notsym;
extern truc	*brkbindPtr, *brkmodePtr;
extern truc	breaksym, errsym, nullsym, voidsym;
extern truc	contsym, contnsym;
extern truc	assignsym;
extern truc	boolsym, truesym, falsesym, true, false, nil;
extern truc	usersym, arisym, symbsym;

/* mainloop.c */
#ifdef DTRACE
extern FILE *DTraceF;
extern int DTraceWrite(char *mess);
extern char DTraceZeile[80];
#endif
extern truc	helpsym;
extern truc	apathsym;
extern truc	*res1Ptr, *res2Ptr, *res3Ptr;
extern int	Unterbrech;

extern int error	_((truc source, char *message, truc obj));
extern void setinterrupt  _((int flg));
extern void reset	_((char *message));
extern void faterr	_((char *mess));
extern int findfile	_((char *paths, char *fnam, char *buf));
extern int findarifile	_((char *name, char *buf));

#ifdef DOSorUNiX
extern void ctrlcreset	_((int sig));
#endif

#ifdef MYFUN
extern void inimyfun	_((void));
#endif

/* mem0.c */
extern truc *Taddress	_((truc x));
extern truc *TAddress	_((truc *p));
extern truc *Saddress	_((truc x));
extern truc *SAddress	_((truc *p));
extern int Tflag	_((truc x));
extern int Symflag	_((truc x));

/* parser.c */
extern void iniparse	_((void));
extern truc tread	_((truc *strom, int mode));
extern void clearcompile  _((void));

extern truc	parserrsym;

/* scanner.c */
extern void iniscan	_((void));
extern int nexttok	_((truc *strom, int skip));
extern int curtok	_((truc *strom));
extern int fltreadprec	_((void));
extern int skipeoltok	_((truc *strom));
extern int isalfa	_((int ch));
extern int isdigalfa	_((int ch));
extern int isdecdigit	_((int ch));
extern int ishexdigit	_((int ch));
extern int isoctdigit	_((int ch));
extern int isbindigit	_((int ch));
extern int toupcase	_((int ch));
extern int tolowcase	_((int ch));
extern char *trimblanks _((char *str, int mode));
extern int rerror	_((truc sym1, char *mess, truc sym2));

extern	numdata Curnum;
extern	char *StrBuf;		/* string buffer */
extern	char *SymBuf;		/* buffer for symbol names */
extern	truc Curop;		/* currently processed operator */

/* print.c */
extern void iniprint	_((int cols));
extern int logout	_((int ch));
extern void strlogout	_((char *str));
extern void closelog	_((void));
extern void flushlog	_((void));
extern int setprnprec	_((int prec));
extern void tprint	_((truc strom, truc obj));
extern int strcopy	_((char *tostr, char *fromstr));
extern int strncopy	_((char *tostr, char *fromstr, int maxlen));
extern int fprintstr	_((truc strom, char *str));
extern void fprintline	_((truc strom, char *str));
extern void fnewline	_((truc strom));
extern void ffreshline	_((truc strom));
extern void flinepos0	_((truc strom));
extern int s1form	_((char *buf, char *fmt, wtruc dat));
extern int s2form	_((char *buf, char *fmt, wtruc dat1, wtruc dat2));

extern truc	writesym, writlnsym, formatsym;
extern truc transcsym;
extern char	OutBuf[];
extern int	Log_on;

/* storage.c */
extern void inistore	_((void));
extern truc *nextsymptr	 _((int i));
extern truc symbobj	_((truc *ptr));
extern int  lookupsym	_((char *name, truc *pobj));
extern truc mksym	_((char *name, int *sflgptr));
extern truc scratch	_((char *name));
extern truc newselfsym	_((char *name, int flg));
extern truc newreflsym	_((char *name, int flg));
extern truc newintsym	_((char *name, int flg, wtruc bind));
extern int  tokenvalue	_((truc op));
extern truc newsym	_((char *name, int flg, truc bind));
extern truc newsymsig	_((char *name, int flg, wtruc bind, int sig));
extern truc new0symsig	_((char *name, int flg, wtruc bind, int sig));
extern truc mkcopy	_((truc *x));
extern truc mkcopy0  _((truc *x));
extern truc mkarrcopy	_((truc *x));
extern truc mkinum	_((long n));
extern truc mkarr2	_((unsigned w0, unsigned w1));
extern truc mklocsym	_((int flg, unsigned u));
extern truc mkfixnum	_((unsigned n));
extern truc mksfixnum	_((int n));
extern truc mkint	_((int sign, word2 *arr, int len));
extern truc mkgf2n  _((word2 *arr, int len));
extern truc mk0gf2n _((word2 *arr, int len));
extern truc mkfloat	_((int prec, numdata *nptr));
extern truc fltzero	_((int prec));
extern truc mk0float	_((numdata *nptr));
extern truc mkchar	_((int ch));
extern truc mkbstr	_((byte *arr, unsigned len));
extern truc mkstr	_((char *str));
extern truc mkstr0	_((unsigned len));
extern truc mkbstr0	_((unsigned len));
extern truc mknullstr	_((void));
extern truc mknullbstr	_((void));
extern truc mkvect0	_((unsigned len));
extern truc mkrecord	_((int flg, truc *ptr, unsigned len));
extern truc mkstack	_((void));
extern truc mkstream	_((FILE *file, int mode));
extern truc mk0stream	_((FILE *file, int mode));
extern truc mk0fun	_((truc op));
extern truc mkpair	_((int flg, truc sym1, truc sym2));
extern truc mkunode	_((truc op));
extern truc mkbnode	_((truc op));
extern truc mkspecnode	_((truc fun, truc *argptr, int k));
extern truc mkfunode	_((truc fun, int n));
extern truc mkfundef	_((int argc, int argoptc, int varc));
extern truc mkntuple	_((int flg, truc *arr, int n));
extern truc mkcompnode	_((int flg, int n));

/* terminal.c */
extern void initerm	_((void));
extern void inputprompt	 _((void));
extern void dumpinput	_((void));
extern char *treadline	_((void));
extern void historyout	_((int flg));

extern truc historsym, savinsym, bufovflsym;

/* sysdep.c */
extern void stacklimit	_((void));
extern long stkcheck	_((void));
extern long timer	    _((void));
extern long datetime	_((int tim[6]));
extern int sysrand	    _((void));
extern void prologue	_((void));
extern int epilogue	    _((void));
extern char *getworkdir _((void));
extern int setworkdir   _((char *pfad));
#ifdef ATARIST
extern int	VDI_handle;
#endif

/* syntchk.c */
extern void inisyntchk	_((void));
extern int chknargs	_((truc fun, int n));

extern int	s_dum, s_0, s_01, s_02, s_0u, s_1, s_1u, s_12, s_bV,
		s_rr, s_vr, s_ii, s_iI, s_bs, s_nv, s_rrr, s_iii,
		s_12ii, s_12rn, s_13, s_14, s_2, s_23, s_3, s_0uii,
		s_iiii, s_4, s_Viiii, s_iiiII;

#define NARGS_FALSE	0
#define NARGS_OK	1
#define NARGS_VAR     255

/**************************************************************************/
