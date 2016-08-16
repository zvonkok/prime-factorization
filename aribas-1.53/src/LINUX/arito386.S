/*************************************************************/
/*
** ARIBAS by O. Forster
** file arito386.s
** Assembler-Code (GNU386-Assembler) fuer primitive
** Arithmetikfunktionen
*/
/*
** date of last change
** 95-09-27
*/
/*--------------------------------------------------------------
**
** Bemerkung: ints sind 4 Bytes lang; Rueckgabe erfolgt in EAX
**
** int multarr(word2 *x, int n, unsigned a, word2 *z);
** int mult4arr(word2 *x, int n, word4 a, word2 *z);
** int divarr(word2 *x, int n, unsigned a, word2 *rest);
** int div4arr(word2 *x, int n, word4 a, word4 *rest);
** unsigned modarr(word2 *x, int n, unsigned a);
** word4 mod4arr(word2 *x, int n, word4 a);
*/
	.text
/*
** Funktion
** int multarr(word2 *x, int n, unsigned a, word2 *z);
** int mult4arr(word2 *x, int n, word4 a, word2 *z);
** Das Array (x,n) wird mit a multipliziert, in der Funktion
** multarr muss a eine 16-Bit-Zahl sein.
** Produkt in z, Laenge = Returncode
** Es wird auch bei multarr vorausgesetzt, dass a als
** 4-byte-Zahl auf dem Stack uebergeben wird.
*/

	.globl multarr, mult4arr
multarr:
mult4arr:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi

	movl    8(%ebp),%esi    /* address of x */
	movl    20(%ebp),%edi   /* address of z */
	movl    12(%ebp),%ecx   /* n */
	cmpl	$0,%ecx
	jle		mult0exit
	cld                     /* direction upwards */
	cmpl    $1,16(%ebp)     /* a = 1 ? */
	ja		multstart
	jb		mult0exit		/* multiplication by 0 */
							/* else multiplication by 1 */
	rep
	movsw
	movl    12(%ebp),%eax   /* n */
	jmp		multexit
  multstart:
	movl    $0,%ebx         /* first carry = 0 */
	jmp		mloopentry
  multloop:
	lodsl                   /* %eax <-- (%esi) and increase %esi by 4 */
	mull    16(%ebp)        /* multiply by a */
	addl	%ebx,%eax
	adcl	$0,%edx
	movl    %edx,%ebx       /* next carry */
	stosl                   /* (%edi) <-- %eax and increase %edi by 4 */
  mloopentry:
	subl	$2,%ecx
	jge	multloop

  mul_L0:
	cmpl    $-1,%ecx        /* n odd? */
	jne		mul_L10
	movzwl  (%esi),%eax     /* n is odd */
	mull    16(%ebp)        /* multiply by a */
	addl	%ebx,%eax
	adcl	$0,%edx
	movl    %edx,%ebx       /* last carry < 2**16 */
	movl    12(%ebp),%ecx   /* n */
	cmpl	$0,%ebx
	je		mul_L1
	addl	$2,%ecx
	stosl                   /* (%edi) <-- %eax and increase %edi by 4 */
	movw	%bx,(%edi)
	jmp	multweiter
  mul_L1:
	cmpl	$0x10000,%eax
	jb		mul_L2
	incl	%ecx
	stosl                   /* (%edi) <-- %eax and increase %edi by 4 */
	jmp		multweiter
  mul_L2:
	movw	%ax,(%edi)
	jmp		multweiter

  mul_L10:                      /* n is even */
	movl	12(%ebp),%ecx	/* n */
	cmpl	$0x10000,%ebx
	jb		mul_L11
	addl	$2,%ecx
	movl	%ebx,(%edi)
	jmp		multweiter
  mul_L11:
	cmpl	$0,%ebx
	jz		multweiter
	incl	%ecx
	movw	%bx,(%edi)

  multweiter:
	movl	%ecx,%eax
	jmp		multexit
  mult0exit:
	movl	$0,%eax
  multexit:
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	ret

/*---------------------------------------------------------*/
/*
** Assembler-Routine fuer die C-Funktionen
** int divarr(word2 *x, int n, unsigned a, word2 *rest);
** int div4arr(word2 *x, int n, word4 a, word4 *rest);
** Arbeitet destruktiv auf x; x wird Quotient, rest in *rest
** In der Funktion divarr muss a eine 16-Bit-Zahl sein.
** Rueckgabewert Laenge des Quotienten
** Es wird auch bei divarr vorausgesetzt, dass a als
** 4-byte-Zahl auf dem Stack uebergeben wird.
*/
	.data
_flg:
	.long	0

	.text

	.globl divarr, div4arr
div4arr:
	movl	$4,_flg
	jmp	divstart
divarr:
	movl	$2,_flg
  divstart:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi

	movl	8(%ebp),%esi		/* address of x */
	movl	16(%ebp),%ebx		/* a */
	cmpl	$0,%ebx
	jz		div0exit			/* division by 0 */
	movl	12(%ebp),%ecx		/* n */
	subl	$1,%ecx
	jl		div0exit
	addl	%ecx,%ecx
	addl	%ecx,%esi	/* address of x[n-1] */
	pushl	%esi		/* save address of x[n-1] */
	std				/* direction downwards */
	movl	$0,%edx
	movl	12(%ebp),%ecx	/* n */
	test	$1,%ecx 	/* is n odd? */
	je		divide		/* if n is even */
	movzwl	(%esi),%eax	/* here n is odd */
	divl	%ebx
	movw	%ax,(%esi)
	subl	$2,%esi
	decl	%ecx
	je		div_L1
  divide:
	subl	$2,%esi
	movl	%esi,%edi
  divloop:
	lodsl			/* %eax <-- (%esi) and decrease %esi by 4 */
	divl	%ebx
	stosl			/* (%edi) <-- %eax and decrease %edi by 4 */
	subl	$2,%ecx
	jg	divloop

  div_L1:			/* adjust n */
	movl	12(%ebp),%ecx	/* n */
	popl	%esi		/* address of x[n-1] */
	lodsw			/* %ax <-- (%esi) and decrease %esi by 2 */
	cmpw	$0,%ax
	jnz		divweiter
	decl	%ecx		/* decrease n if x[n-1] = 0 */
	jz		divweiter
	lodsw			/* %ax <-- (%esi) and decrease %esi by 2 */
	cmpw	$0,%ax
	jnz		divweiter
	decl	%ecx		/* decrease n if x[n-1] = 0 */
	jmp		divweiter

  div0exit:
	movl	$0,%ecx
	movl	$0,%edx
  divweiter:
	movl	%ecx,%eax
	movl	20(%ebp),%edi	/* address rest */
	cmpl	$4,_flg
	jl	div_L30 	/* division by word2 */
	movl	%edx,(%edi)
	jmp	div_L31
  div_L30:
	movw	%dx,(%edi)
  div_L31:
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	ret

/*---------------------------------------------------------*/
/*
** Assembler-Routine fuer die C-Funktion
** unsigned modarr(word2 *x, int n, unsigned a);
** word4 mod4arr(word2 *x, int n, word4 a);
** In der Funktion modarr muss a eine 16-Bit-Zahl sein.
** Das Array (x,n) bleibt erhalten
** Es wird auch bei modarr vorausgesetzt, dass a als
** 4-byte-Zahl auf dem Stack uebergeben wird.
*/
	.globl modarr, mod4arr
modarr:
mod4arr:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%esi

	movl	8(%ebp),%esi		/* address x */
	movl	16(%ebp),%ebx		/* a */
	cmpl	$0,%ebx
	jz		mod0exit		/* division by 0 */
	movl	12(%ebp),%ecx		/* n */
	subl	$1,%ecx
	jl		mod0exit
	addl	%ecx,%ecx
	addl	%ecx,%esi	/* address of x[n-1] */
	std			/* direction downwards */
	movl	$0,%edx
	movl	12(%ebp),%ecx	/* n */
	testl	$1,%ecx 	/* is n odd? */
	je		mod_L0		/* if n is even */
	movzwl	(%esi),%eax
	divl	%ebx
	subl	$2,%esi
	decl	%ecx
	je		modweiter
  mod_L0:
	subl	$2,%esi
  modloop:
	lodsl			/* %eax <-- (%esi) and decrease %esi by 4 */
	divl	%ebx
	subl	$2,%ecx
	jg		modloop

	jmp		modweiter
  mod0exit:
	movl	$0,%edx
  modweiter:
	movl	%edx,%eax
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
/*---------------------------------------------------------*/
