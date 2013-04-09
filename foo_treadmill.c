
#include "treadmill.h"

extern bool   test_result;
extern bool   test_atom;
extern Size   test_size;
extern Target test_target;

extern void fooness() {
    test_result = node_Allocate(_zero_space,
                                test_atom,
                                test_size,
                                test_target);
}

/** x86_64
.globl fooness
        .type   fooness, @function
fooness:
        pushq   %rbp                    # push frame pointer
        movq    %rsp, %rbp              # set new frame pointer

        movq    test_size(%rip), %rdx   # test_size, test_size.6
        movzbl  test_atom(%rip), %eax   # test_atom, test_atom.7
        movzbl  %al, %esi               # test_atom.7, D.3205
        movq    _zero_space(%rip), %rdi # _zero_space, _zero_space.8
        movq    test_target(%rip), %rcx # test_target, test_target
        call    node_Allocate           #
        movb    %al, test_result(%rip)  # D.3207, test_result
        leave
        ret
---
objdump --disassemble foo_treadmill.o

foo_treadmill.o:     file format elf64-x86-64

Disassembly of section .text:

0000000000000000 <fooness>:
   0:   48 83 ec 08             sub    $0x8,%rsp
   4:   0f b6 35 00 00 00 00    movzbl 0(%rip),%esi        # b <fooness+0xb>
   b:   48 8b 0d 00 00 00 00    mov    0(%rip),%rcx        # 12 <fooness+0x12>
  12:   48 8b 15 00 00 00 00    mov    0(%rip),%rdx        # 19 <fooness+0x19>
  19:   48 8b 3d 00 00 00 00    mov    0(%rip),%rdi        # 20 <fooness+0x20>
  20:   e8 00 00 00 00          callq  25 <fooness+0x25>
  25:   88 05 00 00 00 00       mov    %al,0(%rip)        # 2b <fooness+0x2b>
  2b:   48 83 c4 08             add    $0x8,%rsp
  2f:   c3                      retq
---
http://www.technovelty.org/c/position-independent-code-and-x86-64-libraries.html

by convention the return value is left in register eax

The IP (%rip) relative move is really the trick here.
We know from the code that it has to move the value of the global variable(s) here
The zero value is simply a place holder.
the compiler currently does not determine the required address.
It leaves behind a relocation link patch up.
---
readelf --relocs   foo_treadmill.o

Relocation section '.rela.text' at offset 0x1a00 contains 6 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000000007  001100000002 R_X86_64_PC32     0000000000000000 test_atom + fffffffffffffffc
00000000000e  001200000002 R_X86_64_PC32     0000000000000000 test_target + fffffffffffffffc
000000000015  001300000002 R_X86_64_PC32     0000000000000000 test_size + fffffffffffffffc
00000000001c  001400000002 R_X86_64_PC32     0000000000000000 _zero_space + fffffffffffffffc
000000000021  001500000002 R_X86_64_PC32     0000000000000000 node_Allocate + fffffffffffffffc
000000000027  001600000002 R_X86_64_PC32     0000000000000000 test_result + fffffffffffffffc
**/
