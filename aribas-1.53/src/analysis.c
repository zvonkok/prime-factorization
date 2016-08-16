/****************************************************************/
/* file analysis.c

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
** analysis.c
** transcendental functions
**
** date of last change
** 95-02-21
** 97-04-14:	changed inipi
*/

#include "common.h"

PUBLIC void inianalys	_((void));
PUBLIC int lognum	_((int prec, numdata *nptr, word2 *hilf));
PUBLIC int expnum	_((int prec, numdata *nptr, word2 *hilf));

/*--------------------------------------------------------*/
PRIVATE truc expsym, logsym, sqrtsym, sinsym, cossym;
PRIVATE truc tansym, atansym, atan2sym, asinsym, acossym;
PRIVATE truc pisym;

PRIVATE truc inipi	_((int prec));
PRIVATE int Gget1flt	_((truc symb, numdata *nptr));
PRIVATE truc Fsqrt	_((void));
PRIVATE truc Fexp	_((void));
PRIVATE truc Flog	_((void));
PRIVATE truc Fsin	_((void));
PRIVATE truc Fcos	_((void));
PRIVATE truc Ftan	_((void));
PRIVATE truc Gtrig	_((truc symb));
PRIVATE truc Fatan	_((void));
PRIVATE truc Fatan2	_((void));
PRIVATE truc Fasin	_((void));
PRIVATE truc Facos	_((void));
PRIVATE truc Garcus	_((truc symb));

PRIVATE int atannum	_((int prec, numdata *nptr1, numdata *nptr2,
			   word2 *hilf));
PRIVATE int atanprep	_((int prec, numdata *nptr1, numdata *nptr2,
			   word2 *x, int *segptr));
PRIVATE int trignum	_((int prec, numdata *nptr, word2 *hilf, truc symb));
PRIVATE int expovfl	_((numdata *nptr, word2 *hilf));
PRIVATE long redmod	_((int prec, numdata *nptr, word2 *modul, int modlen,
			   word2 *hilf));
PRIVATE int exp0	_((int prec, word2 *x, int n, word2 *z, word2 *hilf));
PRIVATE int exp0aux	_((word2 *x, int n, unsigned a, int k, word2 *temp));
PRIVATE int exp1aux	_((word2 *x, int n, unsigned a, int k, word2 *temp));
PRIVATE int sin0	_((int prec, word2 *x, int n, word2 *z, word2 *hilf));
PRIVATE int cos0	_((int prec, word2 *x, int n, word2 *z, word2 *hilf));
PRIVATE int log0	_((int prec, word2 *x, int n, word2 *z, word2 *hilf));
PRIVATE unsigned log1_16  _((unsigned x));
PRIVATE int atan0	_((int prec, word2 *x, int n, word2 *z, word2 *hilf));

PRIVATE int curfltprec;

PRIVATE word2 LOG2DAT[]	 =	/* log(2) */
#ifdef FPREC_HIGH
{322, 0x154C, 0xB783, 0x64F1, 0xEC3F, 0x53DA, 0x501E, 0xF281, 0x9316, 
0xDB4A, 0xF949, 0x56C9, 0xC921, 0xBE2E, 0x341C, 0x94F0, 0xF58D, 0xCA8, 0x4A00, 
0xD287, 0x3D7, 0x554B, 0xE00C, 0x5497, 0x75DF, 0xFB0C, 0x2D06, 0xECA4, 0x850, 
0xEE6E, 0xEC2F, 0xEF22, 0x5B8A, 0x364F, 0x3C9F, 0x78B6, 0x39CE, 0x897A, 0x8438,
0x1E23, 0x3316, 0x52AB, 0xC60C, 0xA6C4, 0x1A63, 0x62B, 0xEDD, 0xE8F7, 0x449F, 
0x3EA8, 0xC51C, 0x26FA, 0xA415, 0x6425, 0x84E0, 0xF958, 0x767D, 0xC5E5, 0x23FA,
0x8A0E, 0xB31D, 0xC0B1, 0xBD0D, 0x3A49, 0x6AB0, 0x85DB, 0xADD8, 0xC8DA, 0xB4AF,
0x175E, 0x374E, 0xA892, 0xFFF3, 0xF07A, 0x891E, 0xDEA, 0x2625, 0x8F68, 0x339D, 
0x9C38, 0x72F1, 0xCECB, 0x45AE, 0xAC9F, 0x7CEB, 0x5F6F, 0x15C0, 0xE761, 0x2096,
0x6C47, 0x9D42, 0xFBBD, 0xD18B, 0x972C, 0xC724, 0xBD67, 0x11BB, 0xAB1, 0x38B9, 
0xA0C2, 0x26FD, 0x4738, 0xAEBD, 0xD24A, 0x696D, 0x61C1, 0xD5E3, 0x2413, 0xC29, 
0x156E, 0x7487, 0xDC4E, 0x4460, 0x9518, 0x646A, 0x901E, 0x2658, 0xD762, 0x3958,
0xD737, 0xCE2, 0xEF2F, 0x207C, 0xC4E9, 0xB61C, 0x2AC5, 0x7D05, 0xBEBA, 0x9BA2, 
0x5733, 0x1A0C, 0x839, 0xE499, 0x60, 0x302, 0x6AF5, 0x6319, 0x6213, 0xD2F9, 
0x3D0B, 0x28D5, 0x5C1, 0x86B9, 0xCEE8, 0x2B20, 0x36E0, 0x49F2, 0xF3D9, 0x16FA, 
0xBBB, 0x2109, 0xC994, 0x83ED, 0x4221, 0xD3C5, 0x8C66, 0x22B8, 0x5E92, 0xA3CF, 
0x6B1C, 0xFD44, 0x61AF, 0xB982, 0x9538, 0x5C1F, 0x268A, 0x755, 0xFBCF, 0x5177, 
0x8D6F, 0x4EF9, 0x228A, 0x93D1, 0xA172, 0xDC8E, 0x731C, 0x2554, 0x44A0, 0x889B,
0x30AF, 0xE6D3, 0x96D4, 0x9834, 0x8F96, 0xB6C6, 0x5570, 0x73EE, 0x1AE2, 0xA195,
0x7598, 0x853D, 0xB365, 0x2DB3, 0x4D16, 0xC18B, 0x5064, 0xB518, 0x5F50, 0xB31B,
0x1B2D, 0x735D, 0x78F, 0x6CB1, 0x6C60, 0x3CDB, 0xAE31, 0x7B9D, 0xB1E1, 0x5179, 
0x955D, 0xD2C, 0x1735, 0xA54, 0xC48, 0x7AA3, 0x5CFE, 0xB601, 0x74D, 0x8E82, 
0x5E14, 0x7F8A, 0x6A9C, 0xA337, 0x3564, 0x9B33, 0x2566, 0x95D, 0xD1D6, 0x1E0B, 
0x4C1A, 0x514C, 0x9393, 0x4E65, 0xCCCC, 0xCD33, 0xB479, 0xE732, 0xC943, 0x90E5,
0xDB89, 0x775, 0x1746, 0xB396, 0x1400, 0x23DE, 0x7D2E, 0xFA15, 0xFC1E, 0x9D6D,
0xEE56, 0x51A2, 0x8FE5, 0x30F8, 0x610D, 0xFB90, 0xFB5B, 0xCA11, 
#else
{66,
#endif
0x7F4, 0xD5C6, 
0xF3F, 0x97C5, 0xDA2D, 0xE3A2, 0x2F20, 0xA187, 0x655F, 0x3248, 0x3830, 0xA6BD, 
0xF5DF, 0x48CA, 0x9D65, 0x87B1, 0x72CE, 0xF74B, 0x7657, 0xA0EC, 0x256F, 0x603B,
0xB136, 0x9BC3, 0xB9EA, 0x387E, 0x317C, 0xDA11, 0x1ACB, 0xE8C5, 0x224A, 0xCA16,
0x3E96, 0xB825, 0x1169, 0x3B29, 0x2757, 0x2144, 0xC138, 0xAE35, 0xED2E, 0x1B10,
0x4AFA, 0x52FB, 0x5595, 0xAC98, 0x6DEB, 0x7620, 0xE7B8, 0xFA2B, 0x8BAA, 0x175B,
0x8A0D, 0xB62D, 0x7298, 0x4326, 0x40F3, 0xF6AF, 0x3F2, 0xB398, 0xC9E3, 0x79AB, 
0xD1CF, 0x17F7, 0xB172};

PRIVATE word2 PI4THDAT[] =	 /* pi/4 */
#ifdef FPREC_HIGH
{322, 0x23A9, 0xE8F3, 0xBEC7, 0xC97F, 0x59E7, 0x1C9E, 0x900B, 0x4031, 
0xB5A8, 0xC82, 0x4698, 0x702F, 0xD55E, 0xFEF6, 0x6E74, 0xD7CE, 0xF482, 0x1D03, 
0xD172, 0xEA15, 0xF032, 0x92EC, 0xC64B, 0xCA01, 0x5983, 0xD2BF, 0x378C, 0xF401,
0x6FB8, 0xAF42, 0x2BD7, 0x5151, 0x3320, 0x254B, 0xE6CC, 0x1447, 0xDB7F, 0xBB1B,
0xCED4, 0x6CBA, 0x44CE, 0x14ED, 0xCF9B, 0xDBEB, 0xDA3E, 0x8918, 0x865A, 0x27B0,
0x1797, 0xD831, 0x9027, 0x53ED, 0xB06A, 0x1AE, 0x4130, 0x382F, 0xE5DB, 0x530E, 
0xAD9E, 0x9406, 0xF8FF, 0x37BD, 0x3DBA, 0x1E76, 0xC975, 0x46DE, 0x6026, 0xDCB2,
0xC1D4, 0x7026, 0xD27C, 0xFAB4, 0x36C3, 0x8492, 0x3402, 0x35C9, 0x4DF4, 0xC08F,
0x90A6, 0xB7DC, 0x86FF, 0xDDC1, 0x8D8F, 0xEA98, 0x93B4, 0x5AA9, 0xD5B0, 0x9127,
0xD006, 0x481C, 0x2170, 0xDD76, 0xB81B, 0xD7AF, 0xCEE2, 0x2970, 0x1F61, 0xE7ED,
0x515B, 0xA186, 0x233B, 0xC3A2, 0xA090, 0x964F, 0x99B2, 0xC05D, 0x4E6B, 0x5947,
0x287C, 0xCAA6, 0x1FBE, 0xFC14, 0x2E8E, 0x8EF9, 0x4DE, 0xC2DB, 0xDBBB, 0x4CE8, 
0x2AD4, 0xE9CA, 0x2583, 0xBDA, 0xB615, 0x6834, 0x1A94, 0xE23C, 0x6AF4, 0x2718, 
0x99C3, 0x5B26, 0xBDBA, 0x9A10, 0x8871, 0xE6D7, 0xA787, 0x3C12, 0x1A72, 0x801, 
0xA921, 0xD120, 0x4B82, 0x108E, 0xE0FD, 0x5BFC, 0x43DB, 0xAB31, 0x74E5, 0x4FA0,
0x8E2, 0x46E2, 0xBAD9, 0x88C0, 0x7709, 0x5D6C, 0x7A61, 0x1757, 0xBBE1, 0x200C, 
0x177B, 0x2B18, 0x521F, 0x6A64, 0x3EC8, 0x273, 0xD876, 0x864, 0xD98A, 0xFA06, 
0xF12F, 0xEE6B, 0x1AD2, 0xD226, 0xCEE3, 0x619D, 0x4A25, 0x94E0, 0x1E8C, 0x33D7,
0xDB09, 0xAE8C, 0xABF5, 0xE4C7, 0xA6E1, 0xF85, 0xB397, 0xC7D, 0x5D06, 0x7157, 
0x8AEA, 0xEF0A, 0x58DB, 0x8504, 0xECFB, 0xBA64, 0xDF1C, 0x21AB, 0xA855, 0x7A33,
0x450, 0x170D, 0xAD33, 0xC42D, 0x8AAA, 0x8E5A, 0x1572, 0x510, 0x98FA, 0x2618, 
0x15D2, 0x6AE5, 0xEA95, 0x497C, 0x3995, 0x1718, 0x9558, 0xCBF6, 0xDE2B, 0x52C9,
0x6F4C, 0x5DF0, 0xB5C5, 0xA28F, 0xEC07, 0x83A2, 0x9B27, 0x8603, 0x180E, 0x772C,
0xE39E, 0xCE3B, 0x2E36, 0x5E46, 0x3290, 0x217C, 0xCA18, 0x6C08, 0xF174, 0x9804,
0x4ABC, 0x354E, 0x670C, 0x966D, 0x7096, 0x2907, 0x9ED5, 0x52BB, 0x2085, 0xF356,
0x1C62, 0xAD96, 0xDCA3, 0x5D23, 0x8365, 0xCF5F, 0xFD24, 0x3FA8,
#else
{66,
#endif
0x6916, 0xD39A,
0x1C55, 0x4836, 0x98DA, 0xBF05, 0xA163, 0x7CB8, 0xC200, 0x5B3D, 0xECE4, 0x6651,
0x4928, 0x1FE6, 0x7C4B, 0x2411, 0xAE9F, 0x9FA5, 0x5A89, 0x6BFB, 0xEE38, 0xB7ED,
0xF406, 0x5CB6, 0xBFF, 0xED6B, 0xA637, 0x42E9, 0xF44C, 0x7EC6, 0x625E, 0xB576, 
0xE485, 0xC245, 0x6D51, 0x356D, 0x4FE1, 0x1437, 0xF25F, 0xA6D, 0x302B, 0x431B, 
0xCD3A, 0x19B3, 0xEF95, 0x4DD, 0x8E34, 0x879, 0x514A, 0x9B22, 0x3B13, 0xBEA6, 
0x20B, 0xCC74, 0x8A67, 0x4E08, 0x2902, 0x1CD1, 0x80DC, 0x628B, 0xC4C6, 0xC234, 
0x2168, 0xDAA2, 0xC90F};

PRIVATE word2 ATAN4DAT[] =	 /* atan(1/4) */
#ifdef FPREC_HIGH
{322, 0x6B3F, 0x5870, 0xADCF, 0xE549, 0x1C7F, 0x82B2, 0x9F6F, 0x5450, 
0xE8F, 0xB2E1, 0x95F5, 0x9CE5, 0x998F, 0x64AE, 0x9F2F, 0xDE06, 0xF67, 0xF39, 
0x111B, 0x3858, 0x25DF, 0x7580, 0x7910, 0xC3A6, 0x5FC3, 0xD1A8, 0xD87A, 0xE0C5,
0x559D, 0xDDFE, 0x68B1, 0x81C0, 0x1970, 0x7B17, 0xE38E, 0x6D8A, 0xCB0F, 0x5873,
0x6156, 0x89FD, 0xD3FF, 0x5798, 0xD222, 0x62A3, 0x2B89, 0x5A2C, 0x260F, 0xCCE9,
0x293D, 0xE516, 0x4EFB, 0xBD90, 0x4872, 0x20F, 0x4EE8, 0xE6DA, 0x95AF, 0x4E41, 
0x3EB, 0xACB3, 0x4BB, 0xFA89, 0x7C7B, 0xE015, 0xB5E, 0xD3B4, 0xE52, 0x8AAD, 
0x23A1, 0xC362, 0x18B1, 0x55A6, 0x43E0, 0xD662, 0x5797, 0x3973, 0x13D8, 0x973E,
0xF08F, 0x3D2F, 0x90F, 0x24A3, 0x1BEF, 0xE4D5, 0xBD6F, 0x59FF, 0x62B7, 0xDBB9, 
0x1D86, 0xBB9E, 0xAFC2, 0x44FD, 0xA3B, 0x2F5C, 0x6A0E, 0x683, 0x9A47, 0xFCE5, 
0xA135, 0x4C26, 0x53DD, 0x2C34, 0x24E0, 0xD837, 0xB3AE, 0xBC2B, 0xD2D4, 0x734B,
0x1532, 0x7380, 0x5C33, 0x575, 0xC233, 0x2399, 0xBAE6, 0x7055, 0xF5A3, 0x4C75, 
0x25A3, 0x2990, 0x818C, 0xAD7F, 0x8904, 0x4750, 0xFF60, 0x5C39, 0xFAA1, 0x9C67,
0xF168, 0xE82D, 0x6B10, 0xD0C5, 0xD5B4, 0x9B3A, 0x9454, 0x29C8, 0x4BE0, 0x81CB,
0x4EAB, 0xEAF8, 0x579A, 0x338B, 0x4F0D, 0x3013, 0xDBAF, 0xD9A3, 0xF5E0, 0x6F27,
0x5EDE, 0xB593, 0xCBCE, 0x3DD7, 0x5BCE, 0x2D5D, 0xA47C, 0x9F21, 0x2301, 0xCBE5,
0x11E3, 0x9164, 0xCBDC, 0x79E8, 0x4BA8, 0xCEF1, 0x17C3, 0x56EC, 0x494, 0x7880, 
0x80E9, 0x31E6, 0x23C7, 0xEA04, 0xE55F, 0xFA48, 0x7482, 0xE6EF, 0x8AEF, 0x170F,
0xAB2D, 0xC9E8, 0x9A8F, 0x6030, 0x5040, 0xC199, 0xF2AD, 0x8FA1, 0x8187, 0xF447,
0x7EED, 0x6403, 0xC38D, 0xC50D, 0x32FA, 0xEF27, 0x2E51, 0xCE00, 0x3E5A, 0x8682,
0x2519, 0xEA4, 0x2861, 0x3E51, 0xB4BB, 0x6267, 0xF41, 0x3D03, 0xFA02, 0x9927, 
0x4748, 0x5F53, 0x2703, 0x4487, 0xBCA2, 0x63CE, 0x7F56, 0x950C, 0x492E, 0x1875,
0xEEBA, 0xE60, 0xDA37, 0x9A95, 0x1906, 0x77BF, 0xE0DF, 0xB528, 0xFCFC, 0xA1D, 
0xDBFB, 0x2714, 0xFB05, 0x8AA1, 0x6928, 0xB682, 0x65A9, 0x50C7, 0x38B1, 0x4DD3,
0x1BED, 0x4652, 0xD4F7, 0xE21E, 0xA8AB, 0xE37, 0x8EC9, 0xAEA6, 0x1541, 0x16BE, 
0x3E03, 0xA59D, 0x5E8, 0x5574, 0x77B4, 0xA43E, 0x3638, 0x2903, 
#else
{66,
#endif
0x6F3D, 0xC10E, 
0xF52C, 0x8AB6, 0x7616, 0xFD3E, 0x172D, 0x2301, 0x3520, 0x463F, 0x9382, 0xA65, 
0xC31B, 0x1204, 0xD9EA, 0x1DE7, 0x6EE5, 0xF2CB, 0xCF96, 0x738F, 0xF848, 0x9C81,
0xEB38, 0x48D5, 0x234, 0xF724, 0x5756, 0x7231, 0xFDA2, 0xA695, 0xF8FC, 0xADD0, 
0x67A2, 0x60DE, 0xC0E6, 0x70EA, 0x83D0, 0x93E6, 0xC1C7, 0xF089, 0x8F0A, 0xC68F,
0xE960, 0xA316, 0xC16F, 0x5D94, 0x9A39, 0x4945, 0x64AE, 0x69D9, 0x2512, 0x9D9F,
0xDE8E, 0xE0DA, 0xE22C, 0xEA40, 0x6A9F, 0x85F9, 0x7DE8, 0xE7BD, 0x5B71, 0xBAC5,
0x5901, 0xEBF2, 0x3EB6};

#define LOG2(prec)	LOG2DAT + (*LOG2DAT - (prec))
#define PI4TH(prec)	PI4THDAT + (*PI4THDAT - (prec))
#define ATAN4(prec)	ATAN4DAT + (*ATAN4DAT - (prec))

/*----------------------------------------------------------------*/
PUBLIC void inianalys()
{
	expsym	  = newsymsig("exp",   sFBINARY, (wtruc)Fexp, s_rr);
	sqrtsym	  = newsymsig("sqrt",  sFBINARY, (wtruc)Fsqrt,s_rr);
	sinsym	  = newsymsig("sin",   sFBINARY, (wtruc)Fsin, s_rr);
	cossym	  = newsymsig("cos",   sFBINARY, (wtruc)Fcos, s_rr);
	tansym	  = newsymsig("tan",   sFBINARY, (wtruc)Ftan, s_rr);
	logsym	  = newsymsig("log",   sFBINARY, (wtruc)Flog, s_rr);
	atansym	  = newsymsig("arctan",sFBINARY, (wtruc)Fatan,s_rr);
	atan2sym  = newsymsig("arctan2",sFBINARY, (wtruc)Fatan2,s_rrr);
	asinsym	  = newsymsig("arcsin",sFBINARY, (wtruc)Fasin,s_rr);
	acossym	  = newsymsig("arccos",sFBINARY, (wtruc)Facos,s_rr);

	pisym	  = newsym("pi",   sSCONSTANT, inipi(FltPrec[MaxFltLevel]));
}
/*------------------------------------------------------------------*/
PRIVATE truc inipi(prec)
int prec;
{
	numdata acc;

	acc.sign = 0;
	acc.digits = PI4TH(prec);
	acc.len = prec;
	acc.expo = -(prec<<4) + 2;    /*   (pi/4)*4   */
	return(mk0float(&acc));
}
/*----------------------------------------------------------------*/
/*
** Setzt nptr->digits = AriBuf 
** und holt float aus argStkPtr nach nptr;
** setzt curfltprec und gibt prec = curfltprec + 1 zurueck.
** Im Fehlerfall Rueckgabewert = ERROR
*/
PRIVATE int Gget1flt(symb,nptr)
truc symb;
numdata *nptr;
{
	int prec, type;

	type = chknum(symb,argStkPtr);
	if(type == aERROR)
		return(aERROR);

	curfltprec = deffltprec();
	prec = curfltprec + 1;
	nptr->digits = AriBuf;
	getnumtrunc(prec,argStkPtr,nptr);
	return(prec);
}
/*----------------------------------------------------------------*/
PRIVATE truc Fsqrt()
{
	numdata acc;
	word2 *hilf, *x;
	long m;
	int prec;
	int sh, len, rlen;

	prec = Gget1flt(sqrtsym,&acc);
	if(prec == aERROR)
		return(brkerr());
	if(acc.sign) {
		error(sqrtsym,err_p0num,*argStkPtr);
		return(brkerr());
	}
	if((len = acc.len)) {
		sh = (curfltprec << 5) + 8;
		sh -= bitlen(*(AriBuf + len - 1)) + ((len - 1) << 4) - 1;
		len = shiftarr(AriBuf,len,sh);
		m = acc.expo - sh;
		if(m & 1) {
			len = shlarr(AriBuf,len,1);
			m -= 1;
		}
		x = AriScratch;
		hilf = AriScratch + aribufSize;
		cpyarr(AriBuf,len,x);
		acc.len = bigsqrt(x,len,AriBuf,&rlen,hilf);
		acc.expo = (m >> 1);
	}
	return(mkfloat(curfltprec,&acc));
}
/*----------------------------------------------------------------*/
PRIVATE truc Fexp()
{
	numdata acc;
	int prec;
	int ret;

	prec = Gget1flt(expsym,&acc);
	if(prec == aERROR)
		return(brkerr());
	ret = expnum(prec,&acc,AriScratch);
	if(ret == aERROR) {
		error(expsym,err_ovfl,voidsym);
		return(brkerr());
	}
	return(mkfloat(curfltprec,&acc));
}
/*----------------------------------------------------------------*/
PRIVATE truc Flog()
{
	numdata acc;
	int prec;
	int ret;

	prec = Gget1flt(logsym,&acc);
	if(prec == aERROR)
		return(brkerr());
	ret = lognum(prec,&acc,AriScratch);
	if(ret == aERROR) {
		error(logsym,err_pnum,*argStkPtr);
		return(brkerr());
	}
	return(mkfloat(curfltprec,&acc));
}
/*----------------------------------------------------------------*/
PRIVATE truc Fsin()
{
	return(Gtrig(sinsym));
}
/*----------------------------------------------------------------*/
PRIVATE truc Fcos()
{
	return(Gtrig(cossym));
}
/*----------------------------------------------------------------*/
PRIVATE truc Ftan()
{
	return(Gtrig(tansym));
}
/*----------------------------------------------------------------*/
PRIVATE truc Gtrig(symb)
truc symb;
{
	numdata acc, acc2;
	int prec;
	int ret;

	prec = Gget1flt(symb,&acc);
	if(prec == aERROR)
		return(brkerr());

	if(symb == sinsym || symb == cossym)
		ret = trignum(prec,&acc,AriScratch,symb);
	else {	/* symb == tansym */
		acc2.digits = AriScratch + aribufSize;
		cpynumdat(&acc,&acc2);
		ret = trignum(prec,&acc2,AriScratch,cossym);
		ret = trignum(prec,&acc,AriScratch,sinsym);
		ret = divtrunc(prec,&acc,&acc2,AriScratch);
	}
	if(ret == aERROR) {
		error(symb,err_ovfl,voidsym);
		return(brkerr());
	}
	return(mkfloat(curfltprec,&acc));
}
/*----------------------------------------------------------------*/
PRIVATE truc Fatan()
{
	truc res;

	ARGpush(constone);
	res = Fatan2();
	ARGpop();
	return(res);
}
/*----------------------------------------------------------------*/
PRIVATE truc Fatan2()
{
	numdata acc1, acc2;
	word2 *hilf;
	int type, prec;
	int ret;

	type = chknums(atan2sym,argStkPtr-1,2);
	if(type == aERROR)
		return(brkerr());
	acc1.digits = AriBuf;
	acc2.digits = AriScratch;
	hilf = AriScratch + aribufSize;
	curfltprec = fltprec(type);
	prec = curfltprec + 1;

	getnumtrunc(prec,argStkPtr-1,&acc1);
	getnumtrunc(prec,argStkPtr,&acc2);
	ret = atannum(prec,&acc1,&acc2,hilf);
	if(ret == aERROR) {
		error(atansym,err_ovfl,voidsym);
		return(brkerr());
	}
	return(mkfloat(curfltprec,&acc1));
}
/*----------------------------------------------------------------*/
PRIVATE truc Fasin()
{
	return(Garcus(asinsym));
}
/*----------------------------------------------------------------*/
PRIVATE truc Facos()
{
	return(Garcus(acossym));
}
/*----------------------------------------------------------------*/
PRIVATE truc Garcus(symb)
truc symb;
{
	numdata acc1, acc2;
	word2 *x, *y, *z, *hilf;
	int prec, prec2, ret, cmp, rlen;
	int n, m;

	prec = Gget1flt(symb,&acc1);
	if(prec == aERROR)
		return(brkerr());
	prec2 = prec + prec;

	x = acc1.digits;
	acc2.digits = y = AriScratch;
	z = AriScratch + aribufSize;
	hilf = z + prec2 + 2;
	setarr(z,prec2,0);
	z[prec2] = 1;

	n = alignfix(prec,&acc1);
	if(n == aERROR || (cmp = cmparr(x,n,z+prec,prec+1)) > 0) {
		error(symb,err_range,*argStkPtr);
		return(brkerr());
	}
	if(cmp == 0)		/* abs(x) = 1 */
		int2numdat(0,&acc2);
	else if(n == 0)		/* x = 0 */
		int2numdat(1,&acc2);
	else {
		m = multbig(x,n,x,n,y,hilf);
		m = sub1arr(y,m,z,prec2+1);	/* z = 1 - x*x */
		m = bigsqrt(y,m,z,&rlen,hilf);
		cpyarr(z,m,y);			/* y = sqrt(1 - x*x) */
		acc2.len = m;
		acc2.sign = 0;
		acc2.expo = -(prec<<4);
	}
	if(symb == asinsym)
		ret = atannum(prec,&acc1,&acc2,hilf);
	else {
		ret = atannum(prec,&acc2,&acc1,hilf);
		cpynumdat(&acc2,&acc1);
	}
	if(ret == aERROR) {
		error(symb,err_ovfl,voidsym);
		return(brkerr());
	}
	return(mkfloat(curfltprec,&acc1));
}
/*----------------------------------------------------------------*/
/*
** Ersetzt nptr1 durch den Arcus-Tangens des Quotienten
** aus nptr1 und nptr2
*/
PRIVATE int atannum(prec,nptr1,nptr2,hilf)
int prec;
numdata *nptr1, *nptr2;
word2 *hilf;
{
	word2 *x, *z;
	int seg, s, sign, m, n;

	x = hilf;
	hilf += prec << 1;
	z = nptr1->digits;

	n = atanprep(prec,nptr1,nptr2,x,&seg);
	if(n < 0)
		return(n);	/* aERROR */
	m = atan0(prec,x,n,z,hilf);
	nptr1->sign = sign = (seg < 0 ? MINUSBYTE : 0);
	if(sign)
		seg = -seg - 2;
	s = ((seg + 2) >> 1) & 0xFFFE;
	if(s) {
		n = multarr(PI4TH(prec),prec,s,hilf);
		if(seg & 2) {
			m = sub1arr(z,m,hilf,n);
		}
		else
			m = addarr(z,m,hilf,n);
	}
	nptr1->len = m;
	nptr1->expo = -(prec << 4);
	return(m);
}
/*----------------------------------------------------------------*/
/*
** Falls Zahl nptr2 groesser als Zahl in nptr1,
** wird nptr1 durch nptr2 dividiert,
** andernfalls wird nptr2 durch nptr1 dividiert.
** Ist len der Rueckgabewert so erhaelt man
**	Ergebnis = (x,len) * (2**16)**(-prec)
** Platz x muss genuegend lang sein
** Arbeitet destruktiv auf nptr1 und nptr2 !!!
*/
PRIVATE int atanprep(prec,nptr1,nptr2,x,segp)
int prec;
numdata *nptr1, *nptr2;
word2 *x;
int *segp;
{
	numdata *temp;
	long diff, sh;
	int n, m, sign1, sign2;
	int cmp;

	sign1 = nptr1->sign;
	sign2 = nptr2->sign;
	n = alignfloat(prec+1,nptr1);
	m = alignfloat(prec+1,nptr2);
	if(!n) {
		if(!m)
			return(aERROR);
		else {
			*segp = (sign2 ? 8 : 0);
			return(0);
		}
	}
	else if(!m) {
		*segp = (sign1 ? -4 : 4);
		return(0);
	}
	if(!sign1)
		*segp = (sign2 ? 4 : 0);
	else
		*segp = (sign2 ? -8 : -4);
	if(sign1 != sign2) {
		temp = nptr1;
		nptr1 = nptr2;
		nptr2 = temp;
	}

	diff = nptr1->expo - nptr2->expo;
	if(diff == 0)
		cmp = (nptr1->digits[prec] > nptr2->digits[prec]);
	else
		cmp = (diff > 0);

	if(cmp) {	       /* nptr1 groesser */
		*segp += 2;
		temp = nptr1;
		nptr1 = nptr2;
		nptr2 = temp;
	}
	n = divtrunc(prec,nptr1,nptr2,x);
	cpyarr(nptr1->digits,n,x);
	sh = (prec << 4) + nptr1->expo;
	n = lshiftarr(x,n,sh);
	return(n);
}
/*----------------------------------------------------------------*/
PRIVATE int trignum(prec,nptr,hilf,symb)
int prec;
numdata *nptr;
word2 *hilf;
truc symb;
{
	word2 *x;
	int m, n;

	m = redmod(prec,nptr,PI4TH(prec),prec,hilf);
	if(m & 1) {
		nptr->len =
		sub1arr(nptr->digits,nptr->len,PI4TH(prec),prec);
	}
	x = hilf;
	hilf += prec;
	n = nptr->len;
	cpyarr(nptr->digits,n,x);
	if(symb == cossym)
		m += 2;
	if((m+1) & 2)
		nptr->len = cos0(prec,x,n,nptr->digits,hilf);
	else
		nptr->len = sin0(prec,x,n,nptr->digits,hilf);
	nptr->sign = (m & 4 ? MINUSBYTE : 0);
	nptr->expo = -(prec << 4);
	return(nptr->len);
}
/*----------------------------------------------------------------*/
/*
** Die durch nptr dargestellte Zahl z wird ersetzt durch exp(z)
** Bei overflow wird aERROR zurueckgegeben
*/
PUBLIC int expnum(prec,nptr,hilf)
int prec;
numdata *nptr;
word2 *hilf;
{
	word2 *x;
	long m;
	int ovfl, n;

	ovfl = expovfl(nptr,hilf);
	if(ovfl > 0)
		return(aERROR);
	else if(ovfl < 0) {
		int2numdat(0,nptr);
		return(0);
	}
	m = redmod(prec+1,nptr,LOG2(prec+1),prec+1,hilf);
	x = hilf;
	hilf += prec;
	n = nptr->len - 1;
	cpyarr(nptr->digits+1,n,x);
	nptr->len = exp0(prec,x,n,nptr->digits,hilf);
	nptr->sign = 0;
	nptr->expo = m - (prec << 4);
	return(nptr->len);
}
/*----------------------------------------------------------------*/
PRIVATE int expovfl(nptr,hilf)
numdata *nptr;
word2 *hilf;
{
	int n;

	if(nptr->expo <= 1) {
		cpyarr(nptr->digits,nptr->len,hilf);
		n = lshiftarr(hilf,nptr->len,nptr->expo);
		if(n == 0 || (n <= 2 && big2long(hilf,n) < exprange))
			return(0);
	}
	return(nptr->sign ? -1 : 1);
}
/*----------------------------------------------------------------*/
/*
** Die durch nptr gegebene Zahl wird destruktiv dargestellt als
**   ((nptr->digits,nptr->len) + ret*(modul,modlen)) * (2**16)**(-prec),
** wobei ret der Rueckgabewert ist.
** Die Zahl (nptr->digits,nptr->len) ist nicht negativ und < (2**16)**prec
** ret kann auch negativ sein
*/
PRIVATE long redmod(prec,nptr,modul,modlen,hilf)
int prec, modlen;
numdata *nptr;
word2 *modul, *hilf;
{
	word2 *x, *quot;
	word4 u;
	long ret;
	int n, len, rlen;

	x = nptr->digits;
	len = lshiftarr(x,nptr->len,(prec << 4) + nptr->expo);
	quot = hilf + prec + 1;
	n = divbig(x,len,modul,modlen,quot,&rlen,hilf);
	if(n <= 2)
		u = big2long(quot,n);
	if(n > 2 || u >= 0x80000000) {
		error(scratch("redmod"),err_ovfl,voidsym);
		return(LONGERROR);
	}
	else
		ret = u;
	if(nptr->sign) {
		ret = -ret;
		if(rlen) {
			rlen = sub1arr(x,rlen,modul,modlen);
			ret--;
		}
	}
	nptr->len = rlen;
	return(ret);
}
/*----------------------------------------------------------------*/
PUBLIC int lognum(prec,nptr,hilf)
int prec;
numdata *nptr;
word2 *hilf;
{
	word2 *x, *z;
	word2 aa[2];
	word4 u;
	long expo;
	int m, n, len;

	if(nptr->sign || nptr->len == 0)
		return(aERROR);
	x = nptr->digits;
	z = hilf;
	hilf += prec + 2;

	normfloat(prec,nptr);
	n = shlarr(x,prec,1);
	expo = nptr->expo + (prec << 4) - 1;
	len = log0(prec,x,n,z,hilf);
	cpyarr(z,len,x);
	if(expo) {
		u = (expo > 0 ? expo : -expo);
		m = long2big(u,aa);
		n = multbig(LOG2(prec),prec,aa,m,z,hilf);
		if(expo > 0) {
			len = addarr(x,len,z,n);
			nptr->sign = 0;
		}
		else if(cmparr(x,len,z,n) >= 0) {
			len = subarr(x,len,z,n);
			nptr->sign = 0;
		}
		else {
			len = sub1arr(x,len,z,n);
			nptr->sign = MINUSBYTE;
		}
	}
	if(len == 0)
		int2numdat(0,nptr);
	else {
		nptr->len = len;
		nptr->expo = -(prec << 4);
	}
	return(len);
}
/*----------------------------------------------------------------*/
/*
** Berechnet die Exponentialfunktion von (x,n) * (2**16)**(-prec)
** Ist len der Rueckgabewert, so erhaelt man
**	Resultat = (z,len) * (2**16)**(-prec)
** Es wird vorausgesetzt, dass n <= prec ist
** Platz hilf muss mindestens prec + 2 lang sein
** Platz z muss mindestens prec + 1 lang sein
*/

PRIVATE int exp0(prec,x,n,z,hilf)
int prec, n;
word2 *x, *z, *hilf;
{
	int len;

	setarr(z,prec,0);
	z[prec] = 1;
	len = prec + 1;
	while(--n >= 0)
		len = exp0aux(z,len,x[n],prec-n,hilf);
	return(len);
}
/*----------------------------------------------------------------*/
/*
** Multipliziert (x,n) mit exp(a * (2**16)**(-k))
** Platz temp muss mindestens n + 2 lang sein
** Arbeitet destruktiv auf x !!!
*/
PRIVATE int exp0aux(x,n,a,k,temp)
word2 *x, *temp;
unsigned a;
int n, k;
{
	int i, m;
	word2 rest;

	if(a == 0)
		return(n);
	temp++;
	cpyarr(x,n,temp);
	m = n;
	for(i=1; m>k; i++) {
		m = multarr(temp+k-1,m-k+1,a,temp-1) - 1;
		m = divarr(temp,m,i,&rest);
		n = addarr(x,n,temp,m);
	}
	return(n);
}
/*----------------------------------------------------------------*/
/*
** Dividiert (x,n) durch exp(a * (2**16)**(-k))
** Platz temp muss mindestens n + 2 lang sein
** Arbeitet destruktiv auf x !!!
*/
PRIVATE int exp1aux(x,n,a,k,temp)
word2 *x, *temp;
unsigned a;
int n, k;
{
	int i, m;
	word2 rest;

	if(a == 0)
		return(n);
	temp++;
	cpyarr(x,n,temp);
	m = n;
	for(i=1; m>k; i++) {
		m = multarr(temp+k-1,m-k+1,a,temp-1) - 1;
		m = divarr(temp,m,i,&rest);
		n = (i&1 ? subarr(x,n,temp,m) : addarr(x,n,temp,m));
	}
	return(n);
}
/*----------------------------------------------------------------*/
/*
** Berechnet die Funktion sin(x) von (x,n) * (2**16)**(-prec)
** Ist len der Rueckgabewert so erhaelt man
**	Resultat = (z,len) * (2**16)**(-prec)
*/
PRIVATE int sin0(prec,x,n,z,hilf)
int prec, n;
word2 *x, *z, *hilf;
{
	word2 *temp, *temp1, *x2;
	unsigned i;
	int len, m, m2;

	m = (prec + 1) << 1;
	temp = hilf + m;
	temp1 = temp + m;
	x2 = temp1 + m;
	cpyarr(x,n,temp);
	m = n;
	cpyarr(temp,m,z);
	len = m;
	m2 = multfix(prec,x,n,x,n,x2,hilf);
	for(i=2; m>0; i+=2) {
		m = multfix(prec,x2,m2,temp,m,temp1,hilf);
		cpyarr(temp1,m,temp);
		m = divarr(temp,m,i*(i+1),hilf);
		if(i & 2)
			len = subarr(z,len,temp,m);
		else
			len = addarr(z,len,temp,m);
	}
	return(len);
}
/*----------------------------------------------------------------*/
/*
** Berechnet die Funktion cos(x) von (x,n) * (2**16)**(-prec)
** Ist len der Rueckgabewert so erhaelt man
**	Resultat = (z,len) * (2**16)**(-prec)
*/
PRIVATE int cos0(prec,x,n,z,hilf)
int prec, n;
word2 *x, *z, *hilf;
{
	word2 *temp, *temp1, *x2;
	int len, m, m2;
	unsigned i;

	m = (prec + 1) << 1;
	temp = hilf + m;
	temp1 = temp + m;
	x2 = temp1 + m;
	for(i=0; i<prec; i++)
		z[i] = 0;
	z[prec] = 1;
	len = prec + 1;
	m2 = multfix(prec,x,n,x,n,x2,hilf);
	cpyarr(x2,m2,temp);
	m = m2;
	for(i=1; ; i+=2) {
		m = divarr(temp,m,i*(i+1),hilf);
		if(i & 2)
			len = addarr(z,len,temp,m);
		else
			len = subarr(z,len,temp,m);
		if(m <= 1)
			break;
		m = multfix(prec,x2,m2,temp,m,temp1,hilf);
		cpyarr(temp1,m,temp);
	}
	return(len);
}
/*----------------------------------------------------------------*/
/*
** Berechnet die Funktion log(x) von (x,n) * (2**16)**(-prec)
** Ist len der Rueckgabewert so erhaelt man
**	Resultat = (z,len) * (2**16)**(-prec)
** Es wird vorausgesetzt, dass (2**16)**prec <= (x,n) < 2*(2**16)**prec
** Platz hilf muss prec + 2 lang sein
** Arbeitet destruktiv auf x !!!
*/
PRIVATE int log0(prec,x,n,z,hilf)
int prec, n;
word2 *x, *z, *hilf;
{
	unsigned u;
	int k, len;

	setarr(z,prec,0);

	k = prec - 1;
	u = log1_16(x[k]);
	len = 0;

	while(1) {
		n = exp1aux(x,n,u,prec-k,hilf);
		while(n <= prec) {
			u--;
			n = exp0aux(x,n,1,prec-k,hilf);
		}
		len = incarr(z+k,len,u);
		u = x[k];
		if(u == 0)
			u = x[k-1];
		else if(u == 1)
			u = 0xFFFF;
		else
			continue;
		if(--k < 0)
			break;
		if(len)
			len++;
	}
	while(len > 0 && z[len-1] == 0)
		len--;
	return(len);
}
/*----------------------------------------------------------------*/
/*
** berechnet 2**16 * log(1 + x*(2**-16))
** gerundet auf ganze Zahl
*/
PRIVATE unsigned log1_16(x)
unsigned x;
{
static word4 logtab[] = {0x49A58844,0x108598B5,0x04081596,0x01008055,
			 0x00400801,0x00100080,0x00040008,0x00010000};
	/*  log(2**m/(2**m - 1)) * 2**32 fuer m = 2,4,...,16 */

	word4 *logptr;
	word4 xx,d,z;
	int m;

	logptr = logtab;
	z = 0;
	xx = x;
	xx <<= 16;
	m = 1;
	while(m < 16) {
		d = xx;
		d >>= 1;
		d += 0x80000000;
		d >>= m;
		if(xx >= d) {
			xx -= d;
			z += *logptr;
		}
		else {
			m += 2;
			logptr++;
		}
	}
	return(z >> 16);
}
/*----------------------------------------------------------------*/
/*
** Berechnet die Funktion atan(x) von (x,n) * (2**16)**(-prec)
** Ist len der Rueckgabewert so erhaelt man
**	Resultat = (z,len) * (2**16)**(-prec)
** Es wird vorausgesetzt, dass (x,n) < (1/2)*(2**16)**prec
** Platz hilf muss 8*(prec+1) lang sein
*/
PRIVATE int atan0(prec,x,n,z,hilf)
int prec, n;
word2 *x, *z, *hilf;
{
	word2 *u, *v, *y, *temp, *temp1, *x2, *arctan;
	int i, k, m, m1, m2, len;

	i = (prec+1) << 1;
	u = temp = hilf;
	v = temp1 = temp + i;
	y = x2 = temp1 + i;
	hilf = x2 + i;

	len = 0;				/* z = 0 */
	setarr(u,prec-1,0);
	u[prec-1] = 0x4000;			/* u = 1/4 */
	while(cmparr(x,n,u,prec) >= 0) {	/* x = (x-u)/(1+u*x) */
		arctan = ATAN4(prec);
		cpyarr(x,n,v);
		k = shrarr(v,n,2);
		setarr(v+k,prec-k,0);
		v[prec] = 1;
		n = subarr(x,n,u,prec);
		n = divfix(prec,x,n,v,prec+1,y,hilf);
		cpyarr(y,n,x);
		len = addarr(z,len,arctan,prec);
	}
	len = addarr(z,len,x,n);
	cpyarr(x,n,temp);
	m = n;
	m2 = multfix(prec,x,n,x,n,x2,hilf);
	for(i=3; m>1; i+=2) {
		m = multfix(prec,x2,m2,temp,m,temp1,hilf);
		cpyarr(temp1,m,temp);
		m1 = divarr(temp1,m,i,hilf);
		if(i & 2)
			len = subarr(z,len,temp1,m1);
		else
			len = addarr(z,len,temp1,m1);
	}
	return(len);
}
/******************************************************************/
