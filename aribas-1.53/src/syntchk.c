/****************************************************************/
/* file syntchk.c

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
** syntchk.c
** tools for syntax checks
**
** date of last change
** 95-03-26
*/

#include "common.h"

PUBLIC void inisyntchk	_((void));
PUBLIC int chknargs	_((truc fun, int n));

enum signatures {
	S_dum,
	S_0,
	S_01,
	S_02,
	S_0u,
	S_1,
	S_1u,
	S_12,
	S_bV,
	S_rr,
	S_vr,
	S_ii,
	S_iI,
	S_bs,
	S_nv,
	S_rrr,
	S_iii,
	S_12ii,
	S_12rn,
	S_13,
    S_14,
	S_2,
	S_23,
	S_3,
	S_0uii,
	S_iiii,
	S_4,
	S_Viiii,
	S_iiiII,
	SIGMAX
};

PUBLIC int s_dum = S_dum,
	   s_0	 = S_0,
	   s_01	 = S_01,
	   s_02	 = S_02,
	   s_0u	 = S_0u,
	   s_1	 = S_1,
	   s_1u	 = S_1u,
	   s_12	 = S_12,
	   s_bV	 = S_bV,
	   s_rr	 = S_rr,
	   s_vr	 = S_vr,
	   s_ii	 = S_ii,
	   s_iI	 = S_iI,
	   s_bs	 = S_bs,
	   s_nv	 = S_nv,
	   s_rrr = S_rrr,
	   s_iii = S_iii,
	   s_12ii = S_12ii,
	   s_12rn = S_12rn,
	   s_13 = S_13,
	   s_14 = S_14,
	   s_2 = S_2,
	   s_23 = S_23,
	   s_3 = S_3,
	   s_0uii = S_0uii,
	   s_iiii = S_iiii,
	   s_4 = S_4,
	   s_Viiii = S_Viiii,
	   s_iiiII = S_iiiII;

PRIVATE char *SigString[SIGMAX];

#define MAXBYTE		255

/*--------------------------------------------------------------------*/
PUBLIC void inisyntchk()
{

	SigString[s_dum] = "";

	SigString[s_0] = "\001";
	SigString[s_01] = "\377\001\002";
	SigString[s_02] = "\377\001\003";
	SigString[s_0u] = "\377\001\377";
	SigString[s_1] = "\002";
	SigString[s_1u] = "\377\002\377";
	SigString[s_12] = "\377\002\003";
	SigString[s_bV] = "\002bV";
	SigString[s_rr] = "\002rr";
	SigString[s_vr] = "\002vr";
	SigString[s_ii] = "\002ii";
	SigString[s_iI] = "\002iI";
	SigString[s_bs] = "\002bs";
	SigString[s_nv] = "\002nv";
	SigString[s_rrr] = "\003rrr";
	SigString[s_iii] = "\003iii";
	SigString[s_12ii] = "\377\002\003ii";
	SigString[s_12rn] = "\377\002\003rn";
	SigString[s_13] = "\377\002\004";
	SigString[s_14] = "\377\002\005";
	SigString[s_2] = "\003";
	SigString[s_23] = "\377\003\004";
	SigString[s_3] = "\004";
	SigString[s_0uii] = "\377\001\377ii";
	SigString[s_iiii] = "\004iiii";
	SigString[s_4] = "\005";
	SigString[s_Viiii] = "\005Viiii";
	SigString[s_iiiII] = "\005iiiII";
}
/*--------------------------------------------------------------------*/
/*
** check number of arguments of builtin functions
*/
PUBLIC int chknargs(fun,n)
truc fun;
int n;
{
	struct symbol *sptr;
	char *ss;
	int k, k1, k2;
	int ret;
	int sflg;

	sptr = symptr(fun);
	sflg = *FLAGPTR(sptr);
	if(sflg == sFBINARY || sflg == sSBINARY) {
		ss = SigString[sptr->cc.yy.ww];
		k = (byte)ss[0];
		if(k != MAXBYTE && n+1 == k)
			ret = NARGS_OK;
		else if(k == MAXBYTE) {
			k1 = (byte)ss[1];
			k2 = (byte)ss[2];
			if(n+1 < k1 || (k2 != MAXBYTE && n+1 > k2))
				ret = NARGS_FALSE;
			else
				ret = NARGS_VAR;
		}
		else
			ret = NARGS_FALSE;
	}
	else {	/* user defined function */
		ret = NARGS_OK;		/* vorlaeufig */
	}
	return(ret);
}
/**********************************************************************/
