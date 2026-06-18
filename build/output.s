	.file	"techlang"
	.text
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rax
	.cfi_def_cfa_offset 16
	movl	$42, 4(%rsp)
	movl	$.Lfmt, %edi
	movl	$42, %esi
	xorl	%eax, %eax
	callq	printf@PLT
	movl	$.Lfmt.1, %edi
	movl	$.Lstr, %esi
	xorl	%eax, %eax
	callq	printf@PLT
	popq	%rax
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	.Lfmt,@object                   # @fmt
	.section	.rodata.str1.1,"aMS",@progbits,1
.Lfmt:
	.asciz	"%d\n"
	.size	.Lfmt, 4

	.type	.Lstr,@object                   # @str
.Lstr:
	.asciz	"hello from techlang!"
	.size	.Lstr, 21

	.type	.Lfmt.1,@object                 # @fmt.1
.Lfmt.1:
	.asciz	"%s\n"
	.size	.Lfmt.1, 4

	.section	".note.GNU-stack","",@progbits
