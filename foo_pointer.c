
int bar(int value) {
     return sizeof(void*) * value;
}

/* m64
bar:
        pushq   %rbp            # save frame-pointer
        movq    %rsp, %rbp      # make new frame
        movl    %edi, -4(%rbp)  # value, t1
        movl    -4(%rbp), %eax  # t1, result (%rax)
        cltq                    # extend %eax to %rax
        salq    $3, %rax        # shift-left %rax
        leave                   # resort frame-pointer
        ret
*/

/* m32
bar:
	pushl	%ebp	        # save frame-pointer
	movl	%esp, %ebp	# make new frame
	movl	8(%ebp), %eax	# value, result (%eax)
	sall	$2, %eax	# shift-left %eax
	popl	%ebp	        # resort frame-pointer
	ret
*/
