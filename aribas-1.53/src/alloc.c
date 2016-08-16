/****************************************************************/
/* file alloc.c

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
** alloc.c
** memory allocation and garbage collection functions
**
** date of last change
** 1995-03-29
** 1996-10-04	changed iniconfig
** 1997-04-18	various SIZE defines, changed mvsymtab, tempfree
** 1997-09-06	memory allocation #ifdef M_LARGE
** 1997-12-26	small changes in iniconfig
** 1998-01-06	fixed small bug in moveobj
** 2002-04-05   changed some configuration constants
*/

#include "common.h"

/*-------------------------------------------------------------*/
/* configuration constants */

#define SMALL_BLOCK0SIZE     4000	/* unit is sizeof(truc) = 4 */
#define MED_BLOCK0SIZE	     8000
#define BIG_BLOCK0SIZE	    16000
#define SMALL_BLOCKSIZE	    12240	/* multiple of 255 */
#define MED_BLOCKSIZE	    16320	/* multiple of 255, < 2**14 */
#define BIG_BLOCKSIZE	    65280	/* multiple of 255, < 2**16 */

#ifdef M_SMALL

#define HASHTABSIZE	 509	/* size of hash table (prime) */
#define BLOCKMAX	  16
#define ARIBUFSIZE	5000	/* size of bignum buffer (word2's) */
#define BLOCK0SIZE	SMALL_BLOCK0SIZE
#define BLOCKSIZE	SMALL_BLOCKSIZE
#define RESERVE		6000	/* soviel Bytes sollen freibleiben */
#define WORKSTKSIZE	6000	/* size of evaluation+work stack (word4's) */
#define ARGSTKSIZE	7000	/* size of argument+save stack (word4's) */

#endif

#ifdef M_LARGE
#include <assert.h>

#define HASHTABSIZE	    1009	/* size of hash table (prime) */
#define BLOCKMAX	     192    /* must be < 255 */
#define BLOCK0SIZE     BIG_BLOCK0SIZE
#define BLOCKSIZE      BIG_BLOCKSIZE
#define RESERVE	       16000
#define WORKSTKSIZE    BIG_BLOCKSIZE
#define ARGSTKSIZE     16000

#ifdef MEM
#if (MEM >= 1) && (MEM <= 32)
#define MEM_DEFAULT    (MEM*1024)
#endif
#endif /* MEM */
#ifdef INTSIZE
#if (INTSIZE >= 20) && (INTSIZE <= 300)
#define ARIBUFSIZE      (INTSIZE * 209)
#endif
#else
#define ARIBUFSIZE      10000   /* size of bignum buffer (word2's) */
#endif /* INTSIZE */
#endif /* M_LARGE */


#ifndef MEM_DEFAULT

#ifdef ATARIST
#define MEM_DEFAULT	512
#endif
#ifdef MsDOS
#define MEM_DEFAULT	300
#endif
#ifdef DjGPP
#define MEM_DEFAULT	2048
#endif
#ifdef MsWIN32
#define MEM_DEFAULT	2048
#endif
#ifdef genUNiX
#define MEM_DEFAULT	2048
#endif

#endif /* MEM_DEFAULT */

/*-------------------------------------------------------------*/

PUBLIC truc *Symbol;
PUBLIC truc *Memory[BLOCKMAX+1];
PUBLIC trucptr *Symtab;		/* symbol table */
PUBLIC size_t hashtabSize;
PUBLIC truc *WorkStack;		/* evaluation stack (also work stack) */
PUBLIC truc *evalStkPtr, *workStkPtr;
PUBLIC truc *ArgStack;		/* argument stack (also save stack) */
PUBLIC truc *argStkPtr, *saveStkPtr;
PUBLIC truc *basePtr;

PUBLIC word2 *AriBuf, *PrimTab;
PUBLIC word2 *AriScratch, *AuxBuf;
PUBLIC size_t aribufSize, auxbufSize, scrbufSize;
		/* unit is sizeof(word2) */

PUBLIC void inialloc	_((void));
PUBLIC int memalloc	_((int mem));
PUBLIC void dealloc	_((void));
PUBLIC void resetarr	_((void));
PUBLIC int initend	_((void));
PUBLIC int tempfree	_((int flg));
PUBLIC int inpack	_((truc obj, truc pack));
PUBLIC char *stringalloc  _((unsigned int size));
PUBLIC unsigned getblocksize  _((void));
PUBLIC size_t new0	_((unsigned int size));
PUBLIC truc newobj	_((int flg, unsigned int size, trucptr *ptraddr));
PUBLIC truc new0obj	_((int flg, unsigned int size, trucptr *ptraddr));
PUBLIC unsigned obj4size  _((int type, truc *ptr));
PUBLIC void cpy4arr	_((truc *ptr1, unsigned len, truc *ptr2));

/*--------------------------------------------------------*/

typedef struct {
	byte	flag;
	byte	flg2;
	word2	curbot;
	word2	blkceil;
	word2	blkbot;
} blkdesc;

PRIVATE char *Stringpool;
PRIVATE char *Stringsys;

PRIVATE size_t symBot, userBot;
PRIVATE size_t memBot, memCeil;
PRIVATE size_t argstkSize, workstkSize, blockSize, block0Size;
PRIVATE int curblock, noofblocks, auxindex0, maxblocks;
PRIVATE blkdesc blockinfo[BLOCKMAX+1];

PRIVATE word4 gccount = 0;

PRIVATE truc gcsym, memavsym;

PRIVATE void iniconfig	_((int mem));
PRIVATE void inisymtab	_((void));
PRIVATE void iniblock	_((void));
PRIVATE void memstatistics	_((long slot[4]));
PRIVATE void displaymem		_((long s[]));
PRIVATE void gcstatistics	_((void));
PRIVATE int memshrink	_((int nnew, int nold));
PRIVATE truc Fmemavail	_((int argn));
PRIVATE void nextblock	_((unsigned int size));
PRIVATE void clearbufs	_((void));
PRIVATE truc Fgc	_((int argn));
PRIVATE int garbcollect	 _((int mode));
PRIVATE void prepgc	_((void));
PRIVATE void endgc	_((void));
PRIVATE void mvsymtab	_((void));
PRIVATE void mvargstk	_((void));
PRIVATE void mvevalstk	_((void));
PRIVATE void moveobj	_((truc *x));
PRIVATE int toupdate	_((truc *x));
PRIVATE int datupdate	_((int flg));

#define FREE		0
#define HALFFULL	1
#define FULL		2
#define RESERVED	4
#define NOAGERL		8

/*------------------------------------------------------------*/
PUBLIC void inialloc()
{
	gcsym	= newsymsig("gc",	sFBINARY, (wtruc)Fgc, s_01);
	memavsym= newsymsig("memavail", sFBINARY, (wtruc)Fmemavail, s_01);
}
/*--------------------------------------------------------------*/
PRIVATE void iniconfig(mem)
int mem;
{
	int k;
	long memmax;

#ifdef M_LARGE
	assert(sizeof(word4) == 4);
	assert(sizeof(word2) == 2);
#endif
	argstkSize = ARGSTKSIZE;
	workstkSize = WORKSTKSIZE;
	hashtabSize = HASHTABSIZE;
	aribufSize = ARIBUFSIZE;
	blockSize = BLOCKSIZE;
	block0Size = BLOCK0SIZE;

	if(mem <= 0) {
		mem = MEM_DEFAULT;
	}
#ifdef M_LARGE
	else if(mem < 1000) {
		block0Size = MED_BLOCK0SIZE;
		if(mem < 512)
			mem = 512;
	}
#else
	else if(mem < 64)
		mem = 64;
#endif
#ifdef ATARIST
	else if(mem >= 1000) {
		blockSize = BIG_BLOCKSIZE;
		block0Size = MED_BLOCK0SIZE;
	}
#endif
#ifdef DOSorTOS
	else if(mem >= 200)
		blockSize = MED_BLOCKSIZE;
#endif
	memmax = blockSize;
	memmax *= BLOCKMAX;
	memmax /= 255;
	if(mem > memmax)
		mem = memmax;
	if(mem < 2000) {
		if(mem < 96) {
			maxblocks = 2;
		}
		else {
			for(k=3; k<BLOCKMAX; k++)
				if(mem/k <= blockSize/255)
					break;
			maxblocks = k;
		}
		blockSize = mem/maxblocks;
		blockSize *= 255;
		blockSize &= 0xFFFE;	/* make even */
	}
	else {	/* mem >= 2000 */
		maxblocks = (mem + blockSize/510)/(blockSize/255);
		if(maxblocks > BLOCKMAX)
			maxblocks = BLOCKMAX;
	}
}
/*--------------------------------------------------------------*/
/*
** allocate memory for Symtab,
** ArgStack, WorkStack, EvalStack,
** Symbol and Memory
** returns total amount of allocated memory (in kilobytes)
*/
PUBLIC int memalloc(mem)
int mem;
{
	int k;
	unsigned long memallsize;
	size_t size;
	void *ptr;

	stacklimit();

	iniconfig(mem);
	size = sizeof(trucptr) * hashtabSize;
	memallsize = size;
	ptr = malloc(size);
	if(ptr) {
		Symtab = (trucptr *)ptr;
		size = sizeof(truc) * argstkSize;
		memallsize += size;
		ptr = malloc(size);
	}
	if(ptr) {
		ArgStack = (truc *)ptr;
		size = sizeof(truc) * workstkSize;
		memallsize += size;
		ptr = malloc(size);
	}
	if(ptr) {
		WorkStack = (truc *)ptr;
		size = sizeof(word2)*(aribufSize + PRIMTABSIZE + 16);
		memallsize += size;
		ptr = malloc(size);
	}
	if(ptr) {
		AriBuf = (word2 *)ptr;
		PrimTab = AriBuf + aribufSize + 16;
		inisymtab();
		resetarr();
	}
	else
		faterr(err_memory);

	size = sizeof(truc) * block0Size;
	ptr = malloc(size);
#ifdef M_LARGE
	noofblocks = maxblocks;
	if(ptr) {
		memallsize += size;
		Memory[0] = (truc *)ptr;
	}
	else {
		goto errmem;
	}
	size = sizeof(truc)*blockSize*noofblocks;
	ptr = malloc(size);
	if(ptr) {
		memallsize += size;
		Memory[1] = (truc *)ptr;
		for(k=2; k<=noofblocks; k++)
			Memory[k] = Memory[k-1] + blockSize;
	}
	else {
		goto errmem;
	}
#else /* !M_LARGE */
	k = 0;
	while(ptr != NULL) {
		memallsize += size;
		Memory[k] = (truc *)ptr;
		if(++k > maxblocks)
			break;
		size = sizeof(truc) * blockSize;
		ptr = malloc(size);
	}
	noofblocks = k-1;
#endif /* ?M_LARGE */
	ptr = malloc(RESERVE);	/* test free memory */
	if(ptr != NULL)
		free(ptr);
	else {
		free(Memory[noofblocks]);
		noofblocks--;
		memallsize -= size;
	}
	if(noofblocks < 2)
		goto errmem;
	Symbol = Memory[0];
	symBot = 0;
	Stringpool = (char *)(Symbol + block0Size);
	iniblock();
	return((int)(memallsize >> 10));
  errmem:
	faterr(err_memory);
	return(0);
}
/*-------------------------------------------------------------*/
PUBLIC void dealloc()
{

#ifdef M_LARGE
	free(Memory[1]);
#else
	int i;
	for(i=noofblocks; i>=1; i--)
		if(blockinfo[i].blkbot == 0)
			free(Memory[i]);
#endif
	free(Symbol);
	free(AriBuf);
	free(WorkStack);
	free(ArgStack);
	free(Symtab);
}
/*-------------------------------------------------------------*/
PUBLIC void resetarr()
{
	workStkPtr = WorkStack - 1;
	evalStkPtr = WorkStack + workstkSize;
	argStkPtr  = ArgStack - 1;
	saveStkPtr = ArgStack + argstkSize;
	basePtr	   = ArgStack;
}
/* ------------------------------------------------------- */
PRIVATE void inisymtab()
{
	trucptr *sympt;
	int i;

	sympt = Symtab;
	i = hashtabSize;
	while(--i >= 0)
		*sympt++ = NULL;
}
/*---------------------------------------------------------*/
PRIVATE void iniblock()
{
	int split, m;
	int i;

	scrbufSize = 
		(blockSize / sizeof(word2)) * sizeof(truc);
#ifdef M_LARGE
	auxbufSize = 
	(noofblocks*blockSize/2)/sizeof(word2)*sizeof(truc) - scrbufSize;
#else /* !M_LARGE */
	auxbufSize = scrbufSize;
	if(noofblocks == 3)
		auxbufSize /= 2;
	else if(noofblocks < 3)
		auxbufSize = 0;
#endif /* ?M_LARGE */

	m = (noofblocks+1)/2 + 1;
	split = (noofblocks & 1) && (noofblocks < BLOCKMAX);
	if(split) {
		noofblocks++;
		for(i=noofblocks; i>=m; i--)
			Memory[i] = Memory[i-1];
	}
	blockinfo[0].flag = noofblocks;
	for(i=1; i<=noofblocks; i++) {
		blockinfo[i].flag = (i<m ? FREE : RESERVED);
		blockinfo[i].blkbot = 0;
		blockinfo[i].curbot = 0;
		blockinfo[i].blkceil = blockSize;
	}
	if(split) {
		blockinfo[m-1].blkceil = blockSize/2;
		blockinfo[m].blkbot = blockSize/2;
		blockinfo[m].curbot = blockSize/2;
	}
	curblock = 1;
	memBot = blockinfo[curblock].blkbot;
	memCeil = blockinfo[curblock].blkceil;
	AriScratch = (word2 *)Memory[noofblocks];

#ifdef M_LARGE
	auxindex0 = noofblocks/2+1;
#else /* !M_LARGE */
	auxindex0 = noofblocks-1;
#endif /* ?M_LARGE */
	AuxBuf = (word2 *)Memory[auxindex0];
}
/*---------------------------------------------------------*/
/*
** must be called at the end of initializations
** sets global variables userBot and Stringsys
** returns number of bytes used by system for symbols and symbol names
*/
PUBLIC int initend()
{
	size_t n;

	userBot = symBot;
	Stringsys = Stringpool;

	n = (char *)(Symbol + block0Size) - Stringsys;
	n += sizeof(truc)*userBot;
	return(n);
}
/*---------------------------------------------------------*/
/*
** Mit Argument flg > 0: Gibt die zweite Haelfte der Memory-Bloecke frei
** Mit Argument flg == 0: Allokiert von neuem die freigegebenen
** Memorybloecke
** Rueckgabewert: 1 bei Erfolg, 0 bei Fehler
*/
PUBLIC int tempfree(flg)
int flg;
{
#ifdef M_SMALL
	int i, m, res;
	size_t size;
	void *ptr;

	m = (noofblocks/2) + 1;
	if(blockinfo[m].blkbot > 0)
		m++;
	if(flg > 0) {
		garbcollect(1);
		if(blockinfo[1].flag == RESERVED)
			garbcollect(1);
		/* nun ist zweite Haelfte frei */
		for(i=noofblocks; i>=m; i--)
			free(Memory[i]);
	}
	else {
		size = blockSize * sizeof(truc);
		for(i=m; i<=noofblocks; i++) {
			ptr = malloc(size);
			if(ptr == NULL) {
			    res = memshrink(i-1,noofblocks);
			    if(res == 0) {
				noofblocks = i-1;
				return(0);
			    }
			    else
				break;
			}
			Memory[i] = (truc *)ptr;
		}
		AriScratch = (word2 *)Memory[noofblocks];
		AuxBuf = (word2 *)Memory[noofblocks-1];
	}
#endif /* M_SMALL */
	return(1);
}
/*---------------------------------------------------------*/
/*
** Reduziert die Anzahl der Memory-Bloecke von nold auf nnew
** Es wird vorausgesetzt, dass die derzeit aktiven Bloecke
** zur ersten Haelfte gehoeren und dass nnew < nold
** Rueckgabewert: 1 bei Erfolg, 0 bei Misserfolg
*/
PRIVATE int memshrink(nnew,nold)
int nnew, nold;
{
#ifdef M_SMALL
	int i,m,m1,split;

	m = (nold/2) + 1;
	split = (blockinfo[m].blkbot > 0);
	if(split)
		nnew--;
	if(nnew < 2)
		return(0);
	else if(nnew == 2)
		auxbufSize = 0;
	else if(nnew == 3)
		auxbufSize /= 2;
	m1 = nnew/2;
	for(i=m1; i<m; i++) {
		if(blockinfo[m].blkbot < blockinfo[m].curbot)
			return(0);
	}
	if(split) {
		for(i=m; i<=nnew; i++)
			Memory[i] = Memory[i+1];
		blockinfo[m-1].blkceil = blockinfo[m].blkceil;
		blockinfo[m].blkbot = 0;
	}
	m = (nnew+1)/2;	     
	if(nnew & 1) {
		nnew++;
		for(i=nnew; i>m; i--) 
			Memory[i] = Memory[i-1];
		blockinfo[m].blkceil /= 2;
		blockinfo[m+1].blkbot = blockinfo[m].blkceil;
	}
	for(i=nnew; i>m; i--) 
		blockinfo[i].flag = RESERVED;
	noofblocks = nnew;
#endif	/* M_SMALL */
	return(1);
}
/*---------------------------------------------------------*/
PUBLIC int inpack(obj,pack)
truc obj, pack;
{
	variant v;
	int sys;

	v.xx = obj;
	sys = (v.pp.ww < userBot);
	if(pack == arisym)
		return(sys);
	else if(pack == usersym)
		return(!sys);
	else
		return(0);
}
/*---------------------------------------------------------*/
PRIVATE void memstatistics(slot)
long slot[4];
{
	int i, flg;
	unsigned b,c;
	unsigned long nres = 0, nact = 0, nfree = 0, nsymb;
	unsigned s = sizeof(truc);

	for(i=1; i<=noofblocks; i++) {
		b = blockinfo[i].blkbot;
		c = blockinfo[i].blkceil;
		if((flg = blockinfo[i].flag) == RESERVED) {
		    nres += c - b;
		}
		else {
		    nact += c - b;
		    if(flg < FULL) {
			b = (i == curblock ? memBot : blockinfo[i].curbot);
			nfree += c - b;
		    }
		}
	}
	slot[0] = s * nres;
	slot[1] = s * nact;
	slot[2] = s * nfree;
	nsymb = Stringpool - (char *)(Symbol + symBot);
	slot[3] = nsymb;
}
/*---------------------------------------------------------*/
PRIVATE void displaymem(s)
long s[];
{
	int n;
	long diff;

	diff = s[1] - s[2];
	n = s2form(OutBuf,"~8D Bytes reserved; ~D Bytes active ",
        (wtruc)s[0],(wtruc)s[1]);
	s2form(OutBuf+n,"(~D used, ~D free)",(wtruc)diff,(wtruc)s[2]);
	fprintline(tstdout,OutBuf);
	s1form(OutBuf,
	  "~8D Bytes free for user defined symbols and symbol names", (wtruc)s[3]);
	fprintline(tstdout,OutBuf);
}
/*---------------------------------------------------------*/
PRIVATE void gcstatistics()
{
	fnewline(tstdout);
	s1form(OutBuf,"total number of garbage collections: ~D", (wtruc)gccount);
	fprintline(tstdout,OutBuf);
}
/*---------------------------------------------------------*/
PRIVATE truc Fmemavail(argn)
int argn;
{
	long s[4];
	unsigned f;
	int verbose;

	verbose = (argn == 0 || *argStkPtr != zero);

	memstatistics(s);
	if(verbose) {
		gcstatistics();
		displaymem(s);
	}
	f = s[2] >> 10;		/* free kilobytes */
	return(mkfixnum(f));
}
/*---------------------------------------------------------*/
PUBLIC char *stringalloc(size)
unsigned int size;	/* unit for size is sizeof(char) */
{
	if(Stringpool - size <= (char *)(Symbol + symBot))
		faterr(err_memory);
	Stringpool -= size;
	return(Stringpool);
}
/*---------------------------------------------------------*/
PUBLIC unsigned getblocksize()
{
	return(blockSize);
}
/*---------------------------------------------------------*/
PUBLIC size_t new0(size)
unsigned int size;	/* unit for size is sizeof(truc) */
{
	size_t loc;

	loc = symBot;
	symBot += size;
#ifdef ALIGN8
    if (symBot & 0x1)
        symBot++;
#endif
	if(Stringpool <= (char *)(Symbol + symBot))
		faterr(err_memory);
	return(loc);
}
/*---------------------------------------------------------*/
PUBLIC truc newobj(flg,size,ptraddr)
int flg;
unsigned int size;
trucptr *ptraddr;
{
	variant v;

	if(size > memCeil - memBot) {
		nextblock(size);
	}
	v.pp.b0 = flg;
	v.pp.b1 = curblock;
	v.pp.ww = memBot;
	*ptraddr = Memory[curblock] + memBot;

	memBot += size;
	return(v.xx);
}
/*---------------------------------------------------------*/
/*
** allocation from memory block 0
** (for symbols, not moved during garbage collection)
*/
PUBLIC truc new0obj(flg,size,ptraddr)
int flg;
unsigned int size;	/* unit for size is sizeof(truc) */
trucptr *ptraddr;
{
	variant v;
	size_t loc = new0(size);

	v.pp.b0 = flg;
	v.pp.b1 = 0;
	v.pp.ww = loc;
	*ptraddr = Symbol + loc;

	return(v.xx);
}
/*---------------------------------------------------------*/
PRIVATE void nextblock(size)
unsigned int size;
{
	int i,k;
	int collected = 0;

	blockinfo[curblock].curbot = memBot;
	blockinfo[curblock].flag =
		(memCeil - memBot >= NOAGERL ? HALFFULL : FULL);

	if(size > blockSize) {
		reset(err_2large);
	}
  nochmal:
	k = curblock;
	for(i=1; i<=noofblocks; i++) {
		if(++k > noofblocks)
			k = 1;
		if((blockinfo[k].flag <= HALFFULL) &&
		   (size <= blockinfo[k].blkceil - blockinfo[k].curbot)) {
			memBot = blockinfo[k].curbot;
			memCeil = blockinfo[k].blkceil;
			curblock = k;
			return;
		}
	}
	if(!collected && garbcollect(1)) {
		collected = 1;
		goto nochmal;
	}
	clearbufs();
	if(garbcollect(0))
		reset(err_memev);
	else
		faterr(err_garb);
}
/*------------------------------------------------------------*/
PRIVATE void clearbufs()
{
	*res3Ptr = zero;
	*res2Ptr = zero;
	*res1Ptr = zero;
	*brkbindPtr = zero;
}
/*------------------------------------------------------------*/
PRIVATE truc Fgc(argn)
int argn;
{
	garbcollect(1);
	return Fmemavail(argn);
}
/*------------------------------------------------------------*/
PRIVATE int garbcollect(mode)
int mode;	/* mode = 0: emergency collection */
{
	static int merk = 0;

	gccount++;
	if(merk++) {
		merk = 0;
		return(0);
	}
	prepgc();
	mvsymtab();
	if(mode > 0) {
		mvargstk();
		mvevalstk();
	}
	endgc();

	--merk;
	return(1);
}
/*------------------------------------------------------------*/
PRIVATE void prepgc()
{
	int i, first = 1;
	for(i=1; i<=noofblocks; i++) {
		blockinfo[i].curbot = blockinfo[i].blkbot;
		if(blockinfo[i].flag == RESERVED) {
			blockinfo[i].flag = FREE;
			if(first) {
				curblock = i;
				memBot = blockinfo[i].curbot;
				memCeil = blockinfo[i].blkceil;
				first = 0;
			}
		}
		else
			blockinfo[i].flag = RESERVED;
	}
}
/*------------------------------------------------------------*/
PRIVATE void endgc()
{
	int scratchind, auxind;

	blockinfo[curblock].curbot = memBot;

	if(blockinfo[1].flag == RESERVED) {
		scratchind = 1;
		auxind = 2;
	}
	else {
		scratchind = noofblocks;
		auxind = auxindex0;
	}
	AriScratch = (word2 *)Memory[scratchind];
	AuxBuf = (word2 *)Memory[auxind];
}
/*------------------------------------------------------------*/
PRIVATE void mvsymtab()
{
	int n, flg;
	truc *x;

	*res3Ptr = zero;
	n = 0;
	while((x = nextsymptr(n++)) != NULL) {
		flg = *FLAGPTR(x);
		if(flg & sGCMOVEBIND)
			moveobj(SYMBINDPTR(x));
	}
}
/*------------------------------------------------------------*/
PRIVATE void mvargstk()
{
	truc *ptr;

	ptr = ArgStack - 1;
	while(++ptr <= argStkPtr)
		moveobj(ptr);
}
/*------------------------------------------------------------*/
PRIVATE void mvevalstk()
{
	truc *ptr;

	ptr = WorkStack - 1;
	while(++ptr <= workStkPtr)
		moveobj(ptr);

	ptr = WorkStack + workstkSize;
	while(--ptr >= evalStkPtr)
		moveobj(ptr);
}
/*------------------------------------------------------------*/
PRIVATE void moveobj(x)
truc *x;
{
	int flg;
	unsigned int len;
	truc *ptr, *ptr2;

  nochmal:
	flg = toupdate(x);
	if(!flg)
		return;

	ptr = TAddress(x);
	if(*FLAGPTR(ptr) == GCMARK) {	/* update *x */
		*x = *ptr;
		*FLAGPTR(x) = flg;
		return;
	}
	len = obj4size(flg,ptr);

	if(len == 0)	/* this case should not happen */
		return;

	*x = newobj(flg,len,&ptr2);
	cpy4arr(ptr,len,ptr2);
	*ptr = *x;			/* put forwarding address */
	*FLAGPTR(ptr) = GCMARK;

	if(datupdate(flg) && (len >= 2)) {
		while(--len > 1)	/* first element always fixed */
			moveobj(++ptr2);
		/*** tail recursion elimination *******/
		x = ptr2 + 1;
		goto nochmal;
	}
}
/*------------------------------------------------------------*/
/*
** returns 0, if *x needs no update; else returns flag of *x
*/
PRIVATE int toupdate(x)
truc *x;
{
	int flg, seg;

	flg = *FLAGPTR(x);
	if(flg & FIXMASK)
		return(0);
	seg = *SEGPTR(x);
	if(seg == 0 || blockinfo[seg].flag != RESERVED)
		return(0);
	else {
		return(flg);
	}
}
/*------------------------------------------------------------*/
/*
** returns 0, if data of object are fixed, else returns 1
*/
PRIVATE int datupdate(flg)
int flg;
{
	if(flg == fSTREAM)
		return(0);
	else if(flg <= fVECTOR)
		return(1);
	else
		return(0);
}
/*------------------------------------------------------------*/
/*
** return size of objects (unit is sizeof(truc)=4)
** which are not fixed during garbage collection
*/
PUBLIC unsigned obj4size(type,ptr)
int type;
truc *ptr;
{
	unsigned int len;

	switch(type) {
	case fBIGNUM:
	case fGF2NINT:
		len = ((struct bigcell *)ptr)->len;
		return(SIZEOFBIG(len));
	case fSTRING:
	case fBYTESTRING:
		len = ((struct strcell *)ptr)->len;
		return(SIZEOFSTRING(len));
	case fSTREAM:
		return(SIZEOFSTREAM);
	case fVECTOR:
		len = ((struct vector *)ptr)->len;
		return(SIZEOFVECTOR(len));
	case fPOINTER:
	case fRECORD:
		len = ((struct record *)ptr)->len;
		return(SIZEOFRECORD(len));
	case fSPECIAL1:
	case fBUILTIN1:
		return(SIZEOFOPNODE(1));
	case fSPECIAL2:
	case fBUILTIN2:
		return(SIZEOFOPNODE(2));
	case fTUPLE:
	case fWHILEXPR:
	case fIFEXPR:
	case fFOREXPR:
	case fCOMPEXPR:
		len = ((struct compnode *)ptr)->len;
		return(SIZEOFCOMP(len));
	case fSPECIALn:
	case fBUILTINn:
	case fFUNCALL:
		len = *ARGCPTR(ptr);
		return(SIZEOFFUNODE(len));
	case fFUNDEF:
		return(SIZEOFFUNDEF);
	case fSTACK:
		return(SIZEOFSTACK);
	default:
		if(type >= fFLTOBJ) {
			len = fltprec(type);
			return(SIZEOFFLOAT(len));
		}
		else {
			error(gcsym,err_case,mkfixnum(type));
			return(0);
		}
	}
}
/*------------------------------------------------------------*/
/*
** kopiert das word4-Array (ptr1,len) nach ptr2
*/
PUBLIC void cpy4arr(ptr1,len,ptr2)
truc *ptr1, *ptr2;
unsigned int len;
{
	while(len--)
		*ptr2++ = *ptr1++;
}
/************************************************************************/
