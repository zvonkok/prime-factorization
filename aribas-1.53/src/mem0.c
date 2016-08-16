/****************************************************************/
/* file mem0.c

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
** mem0.c
** Entschluesselung der truc's zu echten Pointern
** Kann zur Beschleunigung in Assembler geschrieben werden
**
** date of last change
** 1994-02-15
*/


#include "common.h"

#ifdef ATARIST
#define ASSEMB
#endif

#ifndef ASSEMB
/*
** Falls ASSEMB definiert ist, werden die folgenden Routinen
** durch Assembler-Code ersetzt.
*/
PUBLIC truc *Taddress	_((truc x));
PUBLIC truc *Saddress	_((truc x));
PUBLIC truc *TAddress	_((truc *p));
PUBLIC truc *SAddress	_((truc *p));
PUBLIC int Tflag	_((truc x));
PUBLIC int Symflag	_((truc x));

/*----------------------------------------------------------------*/
PUBLIC truc *Taddress(x)
truc x;
{
	variant v;
	size_t offs;

	v.xx = x;
	offs = v.pp.ww;
	return(Memory[v.pp.b1] + offs);
}
/*----------------------------------------------------------------*/
PUBLIC truc *Saddress(x)
truc x;
{
	variant v;
	size_t offs;

	v.xx = x;
	offs = v.pp.ww;
	return(Symbol + offs);
}
/*----------------------------------------------------------------*/
PUBLIC truc *TAddress(p)
truc *p;
{
	variant v;
	size_t offs;

	v.xx = *p;
	offs = v.pp.ww;
	return(Memory[v.pp.b1] + offs);
}
/*----------------------------------------------------------------*/
PUBLIC truc *SAddress(p)
truc *p;
{
	size_t offs;

	offs = *((word2 *)p + 1);
	return(Symbol + offs);
}
/*----------------------------------------------------------------*/
PUBLIC int Tflag(x)
truc x;
{
	variant v;

	v.xx = x;
	return(v.pp.b0);
}
/*-------------------------------------------------------------*/
PUBLIC int Symflag(x)
truc x;
{
	variant v;
	size_t offs;

	v.xx = x;
	offs = v.pp.ww;
	return(*(byte *)(Symbol + offs));
}
/*-------------------------------------------------------------*/
#else
#undef ASSEMB
#endif
/***************************************************************/
