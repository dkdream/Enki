
int bar(int value) {
     return sizeof(void*) * value;
}

/*
bar:
        pushq   %rbp            # save frame-pointer
        movq    %rsp, %rbp      # make new frame
        movl    %edi, -4(%rbp)  # value, t1
        movl    -4(%rbp), %eax  # t1, result (%rax)
        cltq                    # extend %eax to %rax
        salq    $3, %rax        # shift-left %rax
        leave
        ret
*/
