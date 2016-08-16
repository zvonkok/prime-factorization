/****************************************************************/
/* file errtext.c

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
** errtext.c
** error messages
**
** date of last change
** 1995-03-22
** 2001-03-11: err_0brace
*/

char *err_funest  = "nested function definition not allowed";
char *err_funame  = "inadmissible function name";
char *err_2ident  = "duplicate or inadmissible identifier";
char *err_synt	  = "syntax error";
char *err_imp	  = "not yet implemented";
char *err_case	  = "this case should not happen";
char *err_args	  = "incorrect number of arguments";
char *err_pars	  = "bad parameter(s)";
char *err_varl	  = "bad variable list";
char *err_parl	  = "bad parameter list";
char *err_unvar	  = "undeclared variable";
char *err_evstk	  = "evaluation stack overflow";
char *err_astk	  = "argument stack overflow";
char *err_pstk	  = "parse stack overflow";
char *err_savstk  = "save stack overflow";
char *err_wrkstk  = "work stack overflow";
char *err_memory  = "memory space exhausted";
char *err_memev	  = "insufficient memory for evaluation";
char *err_2large  = "too large piece of memory requested";
char *err_rec	  = "too deeply nested recursion";
char *err_intr	  = "user interrupt";
char *err_garb	  = "garbage collection failed";
char *err_pbase	  = "only allowed values are 2,8,10,16";
char *err_int	  = "integer number expected";
char *err_intt	  = "integer or gf2nint expected";
char *err_intvar  = "integer variable expected";
char *err_odd	  = "odd integer expected";
char *err_oddprim = "odd prime expected";
char *err_2big	  = "number too big";
char *err_p4int	  = "non-negative integer < 2**31 expected";
char *err_pfix	  = "non-negative integer < 2**16 expected";
char *err_fix	  = "integer -2**16 < x < 2**16 expected";
char *err_pnum	  = "positive number expected";
char *err_p0num	  = "non-negative number expected";
char *err_float	  = "float number expected";
char *err_num	  = "number expected";
char *err_2long	  = "string too long";
char *err_iovfl	  = "input buffer overflow";
char *err_bool	  = "boolean or integer value expected";
char *err_div	  = "division by zero";
char *err_bas1	  = "basis must be /= 1";
char *err_ovfl	  = "arithmetic overflow";
char *err_range	  = "argument out of range";
char *err_irange  = "index out of range";
char *err_inadm	  = "inadmissible input";
char *err_buf	  = "buffer too short";
char *err_brstr	  = "broken string";
char *err_bystr	  = "byte_string expected";
char *err_vbystr  = "byte_string variable expected";
char *err_str	  = "string expected";
char *err_strsym  = "string or symbol expected";
char *err_arr	  = "array, string or byte_string expected";
char *err_syarr	  = "symbol array, string or byte_string expected";
char *err_sarr	  = "bad subarray indices";
char *err_vect	  = "vector expected";
char *err_nil	  = "pointer is nil";
char *err_vpoint  = "pointer variable expected";
char *err_stkv	  = "stack variable expected";
char *err_stke	  = "stack is empty";
char *err_stkbig  = "stack too big";
char *err_vasym	  = "variable argument: symbol expected";
char *err_vsym	  = "variable symbol expected";
char *err_lval    = "lval expected";
char *err_sym	  = "symbol expected";
char *err_gsym	  = "global symbol expected";
char *err_sym2	  = "non-constant symbol expected";
char *err_field	  = "bad field identifier";
char *err_ubound  = "unbound symbol";
char *err_ufunc	  = "undefined function";
char *err_var	  = "bad variable";
char *err_call	  = "not an admissible function";
char *err_open	  = "can't open file";
char *err_filex	  = "file exists already";
char *err_filv	  = "file variable expected";
char *err_outf	  = "output file expected";
char *err_tout	  = "text output file expected";
char *err_bout	  = "binary output file expected";
char *err_inpf	  = "input file expected";
char *err_tinp	  = "text input file expected";
char *err_binp	  = "binary input file expected";
char *err_char	  = "character expected";
char *err_bchar	  = "bad character input";
char *err_chr	  = "integer < 2**16 expected";
char *err_type	  = "invalid argument type";
char *err_btype	  = "bad type specification";
char *err_mism	  = "type mismatch";
char *err_rparen  = "unexpected ')'";
char *err_0rparen = "')' expected";
char *err_0lparen = "'(' expected";
char *err_0brace  = "'}' expected";
char *err_0rbrack = "']' expected";
char *err_eof	  = "unexpected end of file";
char *err_bltin	  = "built-in symbol cannot be made unbound";

/********************************************************************/
