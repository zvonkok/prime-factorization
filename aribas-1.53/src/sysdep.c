/****************************************************************/
/* file sysdep.c

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
** sysdep.c
** sytemabhaengige Funktionen
** fuer stack check, timer, random seed
** sowie appl_init fuer ATARI-Version
**
** date of last change
** 1994-09-15
** 1996-10-22	SysDUM
** 2002-04-27   datetime
*/

#include "common.h"

#ifdef MsDOS
#include <dos.h>
#endif

#ifdef ATARIST
#include <tos.h>
#endif

#ifdef DOSorUNiX
#include <signal.h>
#endif

#ifdef DOSorTOS
#include <time.h>
#endif

#ifdef MsWIN32
#include <time.h>
#endif
/*
** if you have (under UNIX) compiling problems with the system
** dependent functions, define the symbol SysDUM or TimeDUM
** Then dummy functions will be substituted; the ARIBAS function
** timer and the random initialization will not work properly,
** but the rest will be unchanged.
** Unjustified error message "Too deeply nested recursion"
** can be avoided by defining StackDUM
*/
#ifdef genUNiX
#define StackDUM
#ifdef SysDUM
#define TimeDUM
#endif
#endif

#ifdef UNiXorGCC
#ifndef TimeDUM
#include <time.h>
#ifdef SCOUNiX
#include <sys/types.h>
#include <sys/timeb.h>
#else	/* ifndef SCOUNiX */
#include <sys/time.h>
static struct timezone t_z = {0,0};
#endif
#endif	/* TimeDUM */
#endif

#ifndef DirDUM
#if defined(genUNiX) || defined(DjGPP)
#include <unistd.h>
#endif
#ifdef Win32CON
#include <dir.h>
#endif
#endif

#ifdef ATARIST
#include <vdi.h>
#include <aes.h>
int contrl[12],
    intin[128],
    intout[128],
    ptsin[128],
    ptsout[128];

int work_in[12],
    work_out[57];

PUBLIC int VDI_handle;
#endif

PUBLIC void stacklimit	_((void));
PUBLIC long stkcheck	_((void));
PUBLIC long timer	    _((void));
PUBLIC long datetime	_((int tim[6]));
PUBLIC int sysrand	    _((void));
PUBLIC void prologue	_((void));
PUBLIC int epilogue	    _((void));
PUBLIC char *getworkdir   _((void));
PUBLIC int setworkdir   _((char *pfad));

/*-------------------------------------------------------------*/
PRIVATE long StackLimit;

#ifdef MsDOS
#define STACKLEN	50000	/* changed for MSVC, 98-04-12 */
extern unsigned _stklen = STACKLEN;
#endif

#ifdef genUNiX
#define STACKLEN	400000
#endif

#ifdef MsWIN32
#define STACKLEN	128000
#endif

#ifdef DjGPP
#define STACKLEN	128000
#endif

/*-------------------------------------------------------------*/
#ifdef ATARIST
PUBLIC void stacklimit()
{
	extern long _StkLim;	/* im Startmodul TCSTSTK.O */

	StackLimit = _StkLim;
}
#endif

#ifdef MsDOS
PUBLIC void stacklimit()
{
	char ptr;

	StackLimit = (long)&ptr - (long)_stklen;
}
#endif

#if defined(UNiXorGCC) || defined(MsWIN32)
PUBLIC void stacklimit()
{
	char ptr;

	StackLimit = (long)&ptr - STACKLEN;
}
#endif
/*-------------------------------------------------------------*/
/*
** Returns length of free stack; used in EVAL.C
** not very portable!
*/

PUBLIC long stkcheck()
{
	long len;
#ifdef StackDUM
	len = 32000;
#else
	extern long StackLimit;
	char stkptr;

	len = (long)&stkptr - StackLimit;
#endif
	return(len);
}
/*-------------------------------------------------------------*/
#ifdef ATARIST
#define TMULT	5
#endif
#ifdef MsDOS
#define TMULT	54
#endif
#ifdef MsWIN32
#define TMULT   1
#endif

#if defined(DOSorTOS) || defined(MsWIN32)
PUBLIC long timer()
{
	long t = clock();
	t *= TMULT;
	return(t);
}
#endif

#ifdef UNiXorGCC
PUBLIC long timer()
{
#ifdef TimeDUM
	return(0);
#else
#ifdef SCOUNiX
	struct timeb tb;

	ftime(&tb);
	return(tb.time*1000 + tb.millitm);
#else
	struct timeval tv;

	gettimeofday(&tv,&t_z);
	return(tv.tv_sec * 1000 + tv.tv_usec/1000);
#endif
#endif
}
#endif
/*-------------------------------------------------------------*/
PUBLIC long datetime(tim)
int tim[6];
{
#ifdef TimeDUM
    for(k=0; k<6; k++)
        tim[k] = 0;
    tim[2] = 1;
	return(0);
#else
    time_t secs;
    struct tm *gmt;
    long t0 = 946684800;    /* seconds from Jan 1, 1970 to Jan 1, 2000 */

    secs = time(NULL);
    gmt = gmtime(&secs);
    tim[0] = gmt->tm_year;
    tim[1] = gmt->tm_mon;
    tim[2] = gmt->tm_mday;
    tim[3] = gmt->tm_hour;
    tim[4] = gmt->tm_min;
    tim[5] = gmt->tm_sec;
    return (long)secs - t0;
#endif
}
/*-------------------------------------------------------------*/
#ifdef ATARIST
PUBLIC int sysrand()
{
       return clock();
}
#endif

#if defined(MsDOS) || defined(MsWIN32)
PUBLIC int sysrand()
{
/*	randomize(); */ /* works with BORLAND C++ */
    srand((unsigned)time(NULL));
	return(rand());
}
#endif

#ifdef UNiXorGCC
PUBLIC int sysrand()
{
#ifdef SCOUNiX
	struct timeb tb;

	ftime(&tb);
	return(tb.time*1000 + tb.millitm);
#else
#ifndef TimeDUM
	struct timeval tv;

	gettimeofday(&tv,&t_z);
	return(tv.tv_usec);
#else
	return timer() + 37421;
#endif
#endif
}
#endif
/*------------------------------------------------------------------*/
PUBLIC char *getworkdir()
{
    static char pfad[MAXPFADLEN];
    int res;

#if defined(Win32GUI)
    res = getwwdir(pfad,MAXPFADLEN);    /* from file winproc.c */
    if(res == 0)
        pfad[0] = '\0';
#elif defined(Win32CON) || defined(DjGPP) || defined(genUNiX)
    if (getcwd(pfad,MAXPFADLEN) == NULL)
        pfad[0] = '\0';
#else
    pfad[0] = '\0';
#endif
    return pfad;
}
/*------------------------------------------------------------------*/
/*
** Returns 0 on failure
*/
PUBLIC int setworkdir(pfad)
char *pfad;
{
    int res;
    int drive;

#if defined(Win32GUI)
    return setwwdir(pfad);    /* from file winproc.c */
#elif defined(Win32CON)
    if(strncmp(pfad+1,":\\",2) == 0) {
        drive = toupcase(pfad[0]) - 'A';
        if(drive >= 0 && drive < 26)
            setdisk(drive);
    }
    res = chdir(pfad);
    return (!res);
#elif defined(genUNiX) || defined(DjGPP)
    res = chdir(pfad);
    return (!res);
#endif
    return 0;
}
/*------------------------------------------------------------------*/
PUBLIC void prologue()
{
#ifdef ATARIST

	int i, handle;
	int dum;

	if(appl_init() < 0) {
		puts("error in appl_init");
		exit(1);
	}
	for(i=1; i<10; i++)
		work_in[i] = 0;
	work_in[10] = 2;
	work_in[0] = graf_handle(&dum, &dum, &dum, &dum);
	v_opnvwk(work_in, &handle, work_out);
	if(handle <= 0) {
		puts("error during program initialization");
		exit(2);
	}
	v_hide_c(handle);
	v_enter_cur(handle);
	VDI_handle = handle;
#endif

#ifdef DOSorUNiX
	signal(SIGINT,ctrlcreset);
#endif
	return;
}
/*------------------------------------------------------------------*/
PUBLIC int epilogue()
{
	int ret = 0;
#ifdef ATARIST
	v_exit_cur(VDI_handle);
	v_show_c(VDI_handle,1);
	v_clsvwk(VDI_handle);
	ret = appl_exit();
#endif
	return(ret);
}
/*****************************************************************/
