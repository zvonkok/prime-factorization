/*****************************************************************/
/* file mainloop.c

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
WWW	http://www.mathematik.uni-muenchen.de/~forster

The latest version of ARIBAS can be obtained by anonymous ftp from

    ftp.mathematik.uni-muenchen.de

directory

    pub/forster/aribas
*/
/*****************************************************************/
/*
** mainloop.c
** mainloop, error handling and help system
**
** date of last change
** 96-10-18:	architec[]
** 97-01-26:	aripath, argload
** 97-04-11:	changed findhelpfile(), helptopic(), cfgfile()
** 97-04-18:	changed Fsystem()
** 97-05-27:	option -b (batch mode)
** 97-07-12:	reorg commandline options
** 97-08-02:	getenv
** 97-08-28:	improved findhelpfile()
*/
/*------------------------------------------------------------*/
#include "common.h"
#include <setjmp.h>


#ifdef DOSorUNiX
#include <signal.h>
#endif
#ifdef MsDOS
#define genDOS
#endif
#ifdef DjGPP
#define genDOS
#endif
/*------------------------------------------------------------*/

#ifdef ARCHITEC
static char architec[] = ARCHITEC;
#else
static char architec[] = "???";
#endif

#ifdef MYFUN
static char Version[] = "1.53(+), Nov. 2004";
#else
static char Version[] = "1.53, Nov. 2004";
#endif
static int version_no = 153;
static char Email[] = "forster@mathematik.uni-muenchen.de";

#ifdef DTRACE
PUBLIC FILE *DTraceF;
PUBLIC int DTraceWrite(char *mess);
PUBLIC char DTraceZeile[80];
#endif
/*--------------------------------------------------------------*/

/********* prototypes of exported functions ************/
PUBLIC int error	_((truc source, char *message, truc obj));
PUBLIC void setinterrupt  _((int flg));
PUBLIC void reset	_((char *message));
PUBLIC void faterr	_((char *mess));

#ifdef DOSorUNiX
PUBLIC void ctrlcreset	_((int sig));
#endif

PUBLIC int Unterbrech = 0;
PUBLIC truc *res1Ptr, *res2Ptr, *res3Ptr;
PUBLIC truc helpsym;
PUBLIC truc apathsym;

/*-----------------------------------------------------*/
PRIVATE char *HelpFile = "aribas.hlp";
PRIVATE char *InitLabel = "-init";

#ifdef genUNiX
#define MAXCMDLEN	256
PRIVATE char *CfgFile = ".arirc";
#else
#define MAXCMDLEN	128
PRIVATE char *CfgFile = "aribas.cfg";
#endif

struct options {
	int mem;
	int cols;
	int verbose;
	int batchflg;
	char *helppath;
	char *aripath;
	char *loadinit;
	char *loadname;
	char *home;
	int argc;
	char **argv;
	char helpbuf[MAXPFADLEN+2];
	char pathbuf[MAXPFADLEN+4];
	char inibuf[MAXPFADLEN+2];
	char homebuf[MAXPFADLEN+2];
};

PRIVATE FILE *findcfg	_((struct options *popt));
PRIVATE void iniopt	_((struct options *popt, char *argv0));
PRIVATE int cfgfile	_((struct options *popt));
PRIVATE int main0	_((int argc, char *argv[]));
PRIVATE int commandline	 _((int argc, char *argv[], struct options *popt));
PRIVATE int findhelpfile _((struct options *popt));
PRIVATE void initialize	 _((struct options *popt));
PRIVATE void inimain	_((struct options *popt));
PRIVATE void title	_((void));
PRIVATE int argload	_((char *fil, int verb));
PRIVATE int mainloop	_((void));
PRIVATE void toprespush	   _((truc obj));
PRIVATE truc Fhalt	   _((int argn));
PRIVATE truc Fversion	   _((int argn));
PRIVATE void resetcleanup  _((char *message));

PRIVATE truc Shelp	_((void));
PRIVATE int helpintro	_((void));
PRIVATE int helptopic	_((char *topic));
PRIVATE void displaypage _((char *txtarr[]));

#ifdef DOSorUNiX
PRIVATE truc systemsym;
PRIVATE truc Fsystem	_((void));
PRIVATE truc getenvsym;
PRIVATE truc Fgetenv	_((void));
#endif

PRIVATE truc hlpfilsym;

PRIVATE truc res1sym, res2sym, res3sym;
PRIVATE truc verssym;
PRIVATE truc haltsym;
PRIVATE truc haltret;

PRIVATE jmp_buf globenv;
PRIVATE int setjumpflg = 0;
PRIVATE int mainret = 0;

#ifdef TT      /* nur fuer Test-Zwecke */
PRIVATE truc ttsym;
PRIVATE truc Ftt    _((void));
#endif /* TT */

/*---------------------------------------------------------------*/
main(argc,argv)
int argc;
char *argv[];
{
	int ret;

	ret = main0(argc,argv);
#ifdef DTRACE
        DTraceF = fopen("DTraceF.txt","w");
        if(DTraceF == NULL) {
                ret = EXITREQ;
        }
#endif
	if(ret != EXITREQ)
		ret = mainloop();
	else
		ret = 0;
#ifdef DTRACE
        if(DTraceF)
                fclose(DTraceF);
#endif
	closelog();
	dealloc();
	epilogue();
	exit(ret);
}
/*------------------------------------------------------------------*/
PRIVATE int main0(argc,argv)
int argc;
char *argv[];
{
	struct options opt;
	int ret;
	int verb;

	iniopt(&opt,argv[0]);
	cfgfile(&opt);
	commandline(argc,argv,&opt);
	findhelpfile(&opt);

	prologue();
	initialize(&opt);

	verb = opt.verbose;
	if(verb)
		title();
	if(opt.loadinit) {
		if(verb) {
		    fnewline(tstdout);
		    fprintline(tstdout,"(** loading init code **)");
		}
		ret = loadaux(opt.loadinit,verb,InitLabel);
	}
	iniargv(opt.argc,opt.argv);
	if(opt.loadname) {
		ret = argload(opt.loadname,verb);
		if(opt.batchflg)
		    ret = EXITREQ;
	}
	else
		ret = 0;
	return(ret);
}
/*------------------------------------------------------------------*/
#ifdef DTRACE
PUBLIC DTraceWrite(mess)
char *mess;
{
     fprintf(DTraceF,mess);
}
#endif
/*------------------------------------------------------------------*/
PRIVATE void iniopt(popt,argv0)
struct options *popt;
char *argv0;
{
	char *home;
	char *str;
	int n;

	home = popt->homebuf;
	home[0] = 0;
#ifdef genUNiX
	str = getenv("HOME");
	if(str != NULL)
		strncopy(home,str,MAXPFADLEN);
#endif
#ifdef genDOS
	if(argv0 != NULL) {
		n = strncopy(home,argv0,MAXPFADLEN);
		while((--n >= 0) && !issepdir(home[n]))
			;
		home[n] = 0;
	}
#endif
	popt->home = home;
	popt->mem = popt->cols = 0;
	popt->verbose = 1;
	popt->batchflg = 0;
	popt->loadname = NULL;
	popt->helppath = NULL;
	popt->aripath = NULL;
	popt->loadinit = NULL;
}
/*------------------------------------------------------------------*/
PRIVATE FILE *findcfg(popt)
struct options *popt;
{
	FILE *fil;
	char *buf, *str;
	int n;

	buf = popt->inibuf;
	fil = fopen(CfgFile,"r");
	if(fil) {
		strcopy(buf,CfgFile);
		return(fil);
	}
	n = strncopy(buf,popt->home,MAXPFADLEN);
	buf[n] = SEP_DIR[0];
	strncopy(buf+n+1,CfgFile,MAXPFADLEN-n);
	fil = fopen(buf,"r");
	if(fil)
		return(fil);
#ifdef genUNiX
	str = getenv("ARIRC");
	if(str != NULL) {
		strncopy(buf,str,MAXPFADLEN);
		fil = fopen(buf,"r");
	}
	if(fil) {
		return(fil);
	}
#endif
	return(NULL);
}
/*------------------------------------------------------------------*/
PRIVATE int cfgfile(popt)
struct options *popt;
{
	FILE *fil;
	char linebuf[IOBUFSIZE+2];
	char *str0, *str, *buf;
	int n, ch;

	fil = findcfg(popt);
	if(fil == NULL) {
		return(0);
	}
	while(fgets(linebuf,IOBUFSIZE,fil)) {
	    str0 = trimblanks(linebuf,0);
	    if(str0[0] == '-') {
		ch = toupcase(str0[1]);
		str = trimblanks(str0 + 2,1);
		switch(ch) {
		case 'M':
		    popt->mem = str2int(str,&n);
		    break;
		case 'C':
		    popt->cols = str2int(str,&n);
		    break;
		case 'P':
		    buf = popt->pathbuf;
		    strncopy(buf,str,MAXPFADLEN);
		    popt->aripath = buf;
		    break;
		case 'H':
		    buf = popt->helpbuf;
		    strncopy(buf,str,MAXPFADLEN);
		    popt->helppath = buf;
		    break;
		case 'Q':
		    popt->verbose = 0;
		    break;
		case 'V':
		    popt->verbose = 1;
		    break;
		case 'I':
		    if(strcmp(str0,InitLabel) == 0) {
			popt->loadinit = popt->inibuf;
		    }
		    break;
		}
	    }
	    if(popt->loadinit)
		break;
	}
	fclose(fil);
	return 0;
}
/*------------------------------------------------------------------*/
PRIVATE int commandline(argc,argv,popt)
int argc;
char *argv[];
struct options *popt;
{
	char *str, *buf;
	int ch;
	int n, k;

	k = 0;
	while(++k < argc && argv[k][0] == '-') {
		ch = argv[k][1];
		str = argv[k] + 2;
	  nochmal:
		switch(toupcase(ch)) {
		case 'M':	/* memory for heap */
		    if(str[0] == 0 && k+1 < argc && argv[k+1][0] != '-')
			str = argv[++k];
		    popt->mem = str2int(str,&n);
		    break;
		case 'C':	 /* columns */
		    if(str[0] == 0 && k+1 < argc && argv[k+1][0] != '-')
			str = argv[++k];
		    popt->cols = str2int(str,&n);
		    break;
		case 'H':	  /* helppath */
		    if(str[0] == 0 && k+1 < argc && argv[k+1][0] != '-')
			str = argv[++k];
		    buf = popt->helpbuf;
		    strncopy(buf,str,MAXPFADLEN);
		    popt->helppath = buf;
		    break;
		case 'P':	  /* aripath */
		    if(str[0] == 0 && k+1 < argc && argv[k+1][0] != '-')
			str = argv[++k];
		    buf = popt->pathbuf;
		    strncopy(buf,str,MAXPFADLEN);
		    popt->aripath = buf;
		    break;
		case 'Q':
		    popt->verbose = 0;
		    if((ch = *str++))
			goto nochmal;
		    break;
		case 'V':
		    popt->verbose = 1;
		    if((ch = *str++))
			goto nochmal;
		    break;
		case 'B':	      /* batch mode */
		    popt->batchflg = 1;
		    if((ch = *str++))
			goto nochmal;
		    break;
		default:
		    ;
		}
	}
	if(k < argc) {
		popt->loadname = argv[k];
	}
	popt->argc = argc - k;
	popt->argv = argv + k;
	return(argc);
}
/*------------------------------------------------------------------*/
PRIVATE int findhelpfile(popt)
struct options *popt;
{
	FILE *fil;
	char *searchpath;
	char path[MAXPFADLEN+2];
	int n, erf;

	if(popt->helppath != NULL) {
		n = strcopy(path,popt->helppath);
		path[n] = SEP_DIR[0];
		strncopy(path+n+1,HelpFile,MAXPFADLEN-n);
		fil = fopen(path,"r");
		if(fil != NULL) {
			goto found;
		}
		else {
			path[n] = 0;
			fil = fopen(path,"r");
			if((fil != NULL) && (getc(fil) > 0)) {
			    goto found;
			}
		}
	}
#ifdef genDOS
	if(strlen(popt->home) > 0) {
		n = strcopy(path,popt->home);
		path[n] = SEP_DIR[0];
		strncopy(path+n+1,HelpFile,MAXPFADLEN-n);
		fil = fopen(path,"r");
		if(fil != NULL)
			goto found;
	}
#endif
#ifdef genUNiX
	searchpath = getenv("PATH");
	if(searchpath == NULL || *searchpath == 0) {
		goto notfound;
	}
	erf = findfile(searchpath,HelpFile,path);
	if(erf) {
		goto found1;
	}
#endif
  notfound:
	popt->helppath = NULL;
	return(aERROR);
  found:
	fclose(fil);
  found1:
	strcopy(popt->helpbuf,path);
	popt->helppath = popt->helpbuf;
	return(0);
}
/*------------------------------------------------------------------*/
PRIVATE void initialize(popt)
struct options *popt;
{
	memalloc(popt->mem);
	inicont();		/* must be called first */
	inialloc();
	inistore();
	inisyntchk();
	iniarith();
	inianalys();
	inieval();
	inifile();
	iniarray();
	initerm();
	iniscan();
	iniparse();
	iniprint(popt->cols);
	inimain(popt);
#ifdef MYFUN
	inimyfun();
#endif
	initend();
}
/*------------------------------------------------------------------*/
PRIVATE void inimain(popt)
struct options *popt;
{
	int sflg;
	char *str;

	helpsym	   = newsymsig("help",sSBINARY, (wtruc)Shelp, s_01);

	if(popt->helppath != NULL)
		str = popt->helppath;
	else
		str = HelpFile;
	hlpfilsym  = mksym(str,&sflg);

	if(popt->pathbuf != NULL)
		str = popt->pathbuf;
	else
		str = "";
	apathsym   = mksym(str,&sflg);

	res1sym	   = newsym("_",  sSYSTEMVAR, zero);
	res1Ptr	   = SYMBINDPTR(&res1sym);
	res2sym	   = newsym("__", sSYSTEMVAR, zero);
	res2Ptr	   = SYMBINDPTR(&res2sym);
	res3sym	   = newsym("___",sSYSTEMVAR, zero);
	res3Ptr	   = SYMBINDPTR(&res3sym);

	haltsym	   = newsymsig("halt", sFBINARY, (wtruc)Fhalt, s_01);
	verssym	   = newsymsig("version", sFBINARY, (wtruc)Fversion, s_01);

#ifdef DOSorUNiX
	systemsym  = newsymsig("system",sFBINARY,(wtruc)Fsystem,s_1);
	getenvsym  = newsymsig("getenv",sFBINARY,(wtruc)Fgetenv,s_1);
#endif

#ifdef	TT
	ttsym = newsymsig("tt",sFBINARY,(wtruc)Ftt,s_0);
#endif
}
/*------------------------------------------------------------------*/
#ifdef TT
PRIVATE truc Ftt()
{
	char *str;

	str = tmpnam(NULL);
	return(mkstr(str)); 
}
#endif /* TT */
/*------------------------------------------------------------------*/
static char *gpltxt[] = {
"ARIBAS comes with ABSOLUTELY NO WARRANTY. This is free software,",
"and you are welcome to redistribute it under the terms of the GNU",
"General Public License as published by the Free Software Foundation.\n",
NULL
};
/*------------------------------------------------------------------*/
PRIVATE void title()
{
	s2form(OutBuf,"~%ARIBAS Interpreter for Arithmetic, V~A (~A)",
		(wtruc)Version,(wtruc)architec);
	fprintline(tstdout,OutBuf);
	s1form(OutBuf,"Copyright (C) 1996-2004 O.Forster <~A>",(wtruc)Email);
	fprintline(tstdout,OutBuf);
	displaypage(gpltxt);
	fnewline(tstdout);
	fnewline(tstdout);
	fprintline(tstdout,"for help, type\040\040?");
	fprintline(tstdout,"to return from ARIBAS, type\040\040exit");
}
/*------------------------------------------------------------------*/
PRIVATE int argload(fil,verb)
char *fil;
int verb;
{
	char name[MAXPFADLEN+4];
	int ret;
	
	ret = findarifile(fil,name);
	if(verb) {
		fnewline(tstdout);
		s1form(OutBuf,"(** loading ~A **)",(wtruc)name);
		fprintline(tstdout,OutBuf);
	}
	ret = loadaux(name,verb,NULL);
	if(ret == aERROR) {
		s1form(OutBuf,"error while loading file ~A",(wtruc)name);
		fprintline(tstderr,OutBuf);
	}
	return(ret);
}
/*------------------------------------------------------------------*/
PRIVATE int mainloop()
{
	static char resprompt[] = "-: ";
	truc obj;
	int jres;

	setjumpflg = 1;
	for( ; ; ) {
		jres = setjmp(globenv);
		if(jres == HALTRET) {
			obj = haltret;
			goto printres;
		}
		if(STREAMtok(tstdin) == EOLTOK || jres) {
			inputprompt();
		}
		obj = tread(&tstdin,TERMINALINP);
		if(obj == exitsym || obj == eofsym)
			break;
		if(obj == historsym) {
			historyout(1);
			continue;
		}
		flinepos0(tstdout);
		obj = eval(&obj);
  printres:
		toprespush(obj);
		if(obj == breaksym) {
			if(*brkmodePtr == exitsym)
				break;
			else
				obj = errsym;
		}
		ffreshline(tstdout);
		if(obj != voidsym) {
			fprintstr(tstdout,resprompt);
			tprint(tstdout,obj);
			fnewline(tstdout);
		}
	}
	return(mainret);
}
/*------------------------------------------------------------------*/
PRIVATE void toprespush(obj)
truc obj;
{
	*res3Ptr = *res2Ptr;
	*res2Ptr = *res1Ptr;
	*res1Ptr = obj;
}
/*------------------------------------------------------------------*/
PRIVATE truc Fversion(argn)
int argn;
{
	if(argn == 0 || *argStkPtr != zero) {
		s2form(OutBuf,"ARIBAS Version ~A (~A)",
			(wtruc)Version,(wtruc)architec);
		fprintline(tstdout,OutBuf);
	}
	return(mkfixnum(version_no));
}
/*------------------------------------------------------------------*/
#ifdef DOSorUNiX
PRIVATE truc Fsystem()
{
	char command[MAXCMDLEN+2];
	int res;

	if(*FLAGPTR(argStkPtr) != fSTRING) {
		error(systemsym,err_str,*argStkPtr);
		goto errexit;
	}
	if(tempfree(1) == 0) {
		error(systemsym,err_memev,voidsym);
		goto errexit;
	}
	strncopy(command,STRINGPTR(argStkPtr),MAXCMDLEN);
	res = system(command);
	if(tempfree(0) == 0) {
		mainret = error(scratch("\nFATAL ERROR"),err_memory,voidsym);
		return(Sexit());
	}

	return(mksfixnum(res));
  errexit:
	return(mksfixnum(-1));
}
#endif
/*------------------------------------------------------------------*/
#ifdef DOSorUNiX
PRIVATE truc Fgetenv()
{
	char *estr;

	if(*FLAGPTR(argStkPtr) != fSTRING) {
		error(getenvsym,err_str,*argStkPtr);
		return(brkerr());
	}
	estr = getenv(STRINGPTR(argStkPtr));
	if(estr == NULL) {
		return(nullstring);
	}
	else {
		return(mkstr(estr));
	}
}
#endif
/*------------------------------------------------------------------*/
PUBLIC int error(source,message,obj)
truc source;
char *message;
truc obj;
{
	if(source != voidsym) {
		tprint(tstderr,source);
		fprintstr(tstderr,": ");
	}
	fprintstr(tstderr,message);
	if(obj != voidsym) {
		fprintstr(tstderr,": ");
		tprint(tstderr,obj);
	}
	fnewline(tstderr);
	return(aERROR);
}
/*------------------------------------------------------------------*/
PRIVATE truc Fhalt(argn)
int argn;
{
	if(argn == 1 && *FLAGPTR(argStkPtr) == fFIXNUM)
		haltret = *argStkPtr;
	else
		haltret = zero;
	resetarr();
	if(setjumpflg)
	    longjmp(globenv,HALTRET);
	else
	    exit(-2);
	return(haltret);
}
/*------------------------------------------------------------------*/
PUBLIC void setinterrupt(flg)
int flg;
{
	Unterbrech = flg;
}
/*------------------------------------------------------------------*/
#ifdef DOSorUNiX
PUBLIC void ctrlcreset(sig)
int sig;
{
	signal(sig,SIG_IGN);
#ifdef UNiXorGCC
	setinterrupt(1);
	signal(SIGINT,ctrlcreset);
#else
	resetcleanup("interrupted by CTRL-C");
	signal(SIGINT,ctrlcreset);
	if(setjumpflg)
	    longjmp(globenv,RESET);
	else
	    exit(-2);
#endif /* ?genUNiX */
}
#endif /* DOSorUNiX */
/*------------------------------------------------------------------*/
PRIVATE void resetcleanup(message)
char *message;
{
	*brkbindPtr = zero;
	resetarr();
	clearcompile();
	historyout(0);
	fnewline(tstderr);
	fprintline(tstderr,message);
	fprintline(tstderr,"** RESET **");
}
/*------------------------------------------------------------------*/
PUBLIC void reset(message)
char *message;
{
	resetcleanup(message);
	if(setjumpflg)
	    longjmp(globenv,RESET);
	else
	    exit(-2);
}
/*------------------------------------------------------------------*/
PUBLIC void faterr(mess)
char *mess;
{
	fputs("\n FATAL ERROR: ",stderr);
	fputs(mess,stderr);
	fputs("\n",stderr);
	exit(aERROR);
}
/*------------------------------------------------------------------*/
/********************************************************************/
/*
** Text for help introduction
*/
static char *help1txt[] = {
"The simplest way to use ARIBAS is as a calculator for big integer arithmetic",
"\t+, -, *\t have the usual meaning",
"\t**\t denotes exponentiation",
"\tdiv, mod calculate the quotient resp. remainder of integer division",
"\t/\t denotes floating point division",
"Simply enter the expression you want to calculate at the ARIBAS prompt ==>",
"followed by a full stop, for example",
"\t==> (23*57 - 13) div 7.",
"After pressing RETURN, the result (here 185) will appear.",
"You can also assign the result of a calculation to a variable, as in",
"\tp := 2**127 - 1.",
"and later use this variable, for example",
"\tx := 1234**(p-1) mod p.",
"The three most recent results are stored in the pseudo variables",
"_, __, and ___. Suppose you have calculated",
"\t==> sqrt(2).",
"\t-: 1.41421356",
"Then you can use the result at the next prompt for example in the",
"expression arcsin(_/2).",
"IMPORTANT: To mark the end of your input, you must type a full stop '.'",
"\t   and then press the RETURN (ENTER) key.\n",
NULL};
/*------------------------------------------------------------------*/
static char *help2txt[] = {
"The for loop and while loop in ARIBAS have a syntax similar to",
"MODULA-2. For example, the sequence",
"\tx := 1;",
"\tfor i := 2 to 100 do",
"\t    x := x*i;",
"\tend;",
"\tx.",
"calculates the factorial of 100.",
"You can define your own functions in ARIBAS. For example, a recursive",
"version of the factorial function can be defined by",
"\tfunction fac(n: integer): integer;",
"\tbegin",
"\t    if n <= 1 then",
"\t\treturn 1;",
"\t    else",
"\t\treturn n*fac(n-1);",
"\t    end;",
"\tend.",
"After you have entered this, the function fac will be at your disposal and",
"\t==> fac(100).",
"will calculate the factorial of 100.\n",
NULL};
/*------------------------------------------------------------------*/
static char *help3txt[] = {
"A list of all keywords and names of builtin functions is returned",
"by the command\n",
"\t==> symbols(aribas).\n",
"For most of the symbols in this list, you can get a short online",
"help using the help function. For example\n",
"\t==> help(factor16).\n",
"will print an information on the function factor16 to the screen.\n",
"For more information, read the documentation.\n",
"To leave ARIBAS, type\040\040exit",
NULL};
/*------------------------------------------------------------------*/
PRIVATE truc Shelp()
{
	truc *ptr;
	char *topic;
	int argn;

	argn = *ARGCOUNTPTR(evalStkPtr);
	if(argn >= 1) {
		ptr = ARG1PTR(evalStkPtr);
		if(*FLAGPTR(ptr) == fSYMBOL) {
			topic = SYMNAMEPTR(ptr);
			helptopic(topic);
			return(voidsym);
		}
	}
	helpintro();
	return(voidsym);
}
/*------------------------------------------------------------------*/
PRIVATE int helpintro()
{
	static char *gotonext = "Press RETURN to see the next help screen.";

	displaypage(help1txt);
	fnewline(tstdout);
	fprintstr(tstdout,gotonext); fflush(stdout);
	getchar();

	displaypage(help2txt);
	fnewline(tstdout);
	fprintstr(tstdout,gotonext); fflush(stdout);
	getchar();

	displaypage(help3txt);
	return(0);
}
/*------------------------------------------------------------------*/
PRIVATE void displaypage(txtarr)
char *txtarr[];
{
	char *str;
	int i = 0;

	while((str = txtarr[i]) != NULL) {
		fnewline(tstdout);
		fprintstr(tstdout,str);
		i++;
	}
}
/*------------------------------------------------------------------*/
#define TOPICMARKER	'?'
#define TOPICEND	'#'
#define PAGEFULL	25

PRIVATE int helptopic(topic)
char *topic;
{
	FILE *hfile;
	char *path;
	int i, len;
	int found = 0;

	path = SYMname(hlpfilsym);
	hfile = fopen(path,"r");
	if(hfile == NULL) {
		error(helpsym,err_open,scratch(path));
		return(aERROR);
	}
	len = strlen(topic);
	while(fgets(OutBuf,IOBUFSIZE,hfile)) {
		if(OutBuf[0] == TOPICMARKER &&
		   strncmp(OutBuf+1,topic,len) == 0 &&
		   OutBuf[len+1] <= ' ') {
			found = 1;
			break;
		}
	}
	if(found) {
		while(fgets(OutBuf,IOBUFSIZE,hfile) &&
		      OutBuf[0] == TOPICMARKER)
			;
		fprintstr(tstdout,OutBuf);
		for(i=0; i<PAGEFULL; i++) {
			if(!fgets(OutBuf,IOBUFSIZE,hfile) ||
			    OutBuf[0] == TOPICEND)
				break;
			fprintstr(tstdout,OutBuf);
		}
	}
	else {
		s1form(OutBuf,"no help available for ~A",(wtruc)topic);
		fprintline(tstdout,OutBuf);
	}
	fclose(hfile);
	return(0);
}
/********************************************************************/





