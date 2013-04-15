
#include "all_types.inc"
#include "treadmill.h"

int bar(int value) {
     return sizeof(void*) * value;
}

/* m32
bar:
	pushl	%ebp	        # save frame-pointer
	movl	%esp, %ebp	# make new frame
	movl	8(%ebp), %eax	# value, result (%eax)
	sall	$2, %eax	# shift-left %eax : (4 * value)
	popl	%ebp	        # resort frame-pointer
	ret
*/

unsigned int text_csize(Text value) {
    return value->size;
}

/* m32
text_csize:
        pushl   %ebp            # save frame-pointer
        movl    %esp, %ebp      # make new frame
        movl    8(%ebp), %eax   # value, %eax
        movl    (%eax), %eax    # mem[%eax], result
        popl    %ebp            # resort frame-pointer
        ret
*/

HashCode text_hashcode(Text value) {
     return value->hashcode;
}

/* m32
text_hashcode:
        pushl   %ebp            # save frame-pointer
        movl    %esp, %ebp      # make new frame
        movl    8(%ebp), %eax   # value, %eax
        movl    8(%eax), %edx   # mem[%eax + 8], %edx
        movl    4(%eax), %eax   # mem[%eax + 4], %eax
        popl    %ebp            # resort frame-pointer
        ret
*/

char* text_cstring(Text value) {
    return (char*)value->value;
}

/* m32
text_cstring:
        pushl   %ebp            # save frame-pointer
        movl    %esp, %ebp      # make new frame
        movl    8(%ebp), %eax   # value, result (%eax)
        addl    $12, %eax       # ajust pointer (3*4):%eax
        popl    %ebp            # resort frame-pointer
        ret
*/

unsigned hash_code_size() {
    return sizeof(HashCode);
}

/* m32
hash_code_size:
        pushl   %ebp    #
        movl    %esp, %ebp      #,
        movl    $8, %eax        # sizeof(HashCode)
        popl    %ebp    #
        ret
*/

HashCode hash_code_add(HashCode value) {
    return value + 1;
}

/* m32
hash_code_add:
        pushl   %ebp            # save frame-pointer
        movl    %esp, %ebp      # make new frame
        subl    $8, %esp        # alloc scratch
        movl    8(%ebp), %eax   # value.l_bits, %eax
        movl    %eax, -8(%ebp)  # %eax, tmp0
        movl    12(%ebp), %eax  # value.h_bits, %eax
        movl    %eax, -4(%ebp)  # %eax, tmp1
        movl    -8(%ebp), %eax  # tmp0, result.l_bits
        movl    -4(%ebp), %edx  # tmp1, result.h_bits
        addl    $1, %eax        # add 1, result.l_bits
        adcl    $0, %edx        # carry, result.h_bits
        leave                   # %ebp -> %esp, resort frame-pointer
        ret
*/

struct Fooness {
    long one;
    long two;
    long three;
};

struct Fooness fooness_fill() {
    struct Fooness value;
    value.one   = 1;
    value.two   = 2;
    value.three = 3;
    return value;
}

/* m32 (note: fooness_fill has a hidden argument (Fooness*)
fooness_fill:
        pushl   %ebp            # save frame-pointer
        movl    %esp, %ebp      # make new frame
        subl    $16, %esp       # alloc scratch
        movl    8(%ebp), %eax   # <hidden-ptr>, <result>
        movl    $1, -12(%ebp)   # value.one
        movl    $2, -8(%ebp)    # value.two
        movl    $3, -4(%ebp)    # value.three
        movl    -12(%ebp), %edx # value.one %edx
        movl    %edx, (%eax)    # %edx, <result>.one
        movl    -8(%ebp), %edx  # value.two, %edx
        movl    %edx, 4(%eax)   # %edx, <result>.two
        movl    -4(%ebp), %edx  # value.three, %edx
        movl    %edx, 8(%eax)   # %edx, <result>.three
        leave                   # %ebp -> %esp, resort frame-pointer
        ret     $4              # return and remove <hidden>
 */

struct Fooness* fooness_fillp(struct Fooness* hidden) {
    struct Fooness value;
    value.one   = 1;
    value.two   = 2;
    value.three = 3;

    *hidden = value;

    return hidden;
}

/* m32
fooness_fillp:
        pushl   %ebp            # save frame-pointer
        movl    %esp, %ebp      # make new frame
        subl    $16, %esp       # alloc scratch
        movl    $1, -12(%ebp)   # value.one
        movl    $2, -8(%ebp)    # value.two
        movl    $3, -4(%ebp)    # value.three
        movl    8(%ebp), %eax   # hidden, <result>
        movl    -12(%ebp), %edx # value.one, tmp61
        movl    %edx, (%eax)    # tmp61, <result>.one
        movl    -8(%ebp), %edx  # value.two, tmp62
        movl    %edx, 4(%eax)   # tmp62, <result>.two
        movl    -4(%ebp), %edx  # value.three, tmp63
        movl    %edx, 8(%eax)   # tmp63, <result>.three
        movl    8(%ebp), %eax   # hidden, D.2285
        leave                   # %ebp -> %esp, resort frame-pointer
        ret
*/

/*
struct gc_header {
    struct gc_treadmill *space;
    struct gc_header    *before;
    struct gc_header    *after;
    struct gc_kind       kind;
};

struct gc_kind {
    Node type;
    unsigned long count : BITS_PER_WORD - 8 __attribute__((__packed__));
    struct {
        enum gc_color color  : 2;
        unsigned int  atom   : 1; // is this a tuple of values
        unsigned int  live   : 1; // is this alive
        unsigned int  inside : 1; // is this inside a space (malloc/free by the treadmill)
    } __attribute__((__packed__));
};
*/

unsigned gc_kind_size() {
    return sizeof(struct gc_kind);
}

/*
gc_kind_size:
        pushl   %ebp        #
        movl    %esp, %ebp  #,
        movl    $8, %eax    #, D.2987
        popl    %ebp        #
        ret
*/

unsigned gc_header_size() {
    return sizeof(struct gc_header);
}

/*
gc_header_size:
        pushl   %ebp       #
        movl    %esp, %ebp #
        movl    $20, %eax  #
        popl    %ebp       #
        ret
*/


struct gc_kind *barness_kind_ptr() {
    return &((((Header)0) - 1)->kind);
}

/* m32
barness_kind_ptr:
        pushl   %ebp        #
        movl    %esp, %ebp  #
        movl    $-8, %eax   #
        popl    %ebp        #
        ret
*/

struct gc_header *barness_header_ptr() {
    return (((Header)0) - 1);
}

/* m32
barness_header_ptr:
        pushl   %ebp       #
        movl    %esp, %ebp #
        movl    $-20, %eax #
        popl    %ebp       #
        ret

*/

void *barness_type() {
    return (((Header)0) - 1)->kind.type.reference;
}

/* m32
barness_type:
        pushl   %ebp    #
        movl    %esp, %ebp      #
        movl    $-20, %eax      #
        movl    12(%eax), %eax  # <variable>.kind.type.reference
        popl    %ebp            #

*/

void *shmay_type() {
    return (((Kind)0) - 1)->type.reference;
}

/* m32
shmay_type:
        pushl   %ebp    #
        movl    %esp, %ebp      #,
        movl    $-8, %eax       #, D.3005
        movl    (%eax), %eax    # <variable>.type.reference, D.3006
        popl    %ebp    #
        ret
*/

unsigned long barness_count() {
    return (((Header)0) - 1)->kind.count;
}

/* m32
barness_count:
        pushl   %ebp            #
        movl    %esp, %ebp      #
        movl    $-20, %eax      # header
        movl    16(%eax), %eax  # header.count
        andl    $16777215, %eax # mask 0x00FFFFFF
        popl    %ebp            #
        ret
 */

unsigned int barness_atom() {
    return (((Header)0) - 1)->kind.atom;
}

/* m32
barness_atom:
        pushl   %ebp            #
        movl    %esp, %ebp      #
        movl    $-20, %eax      #
        movzbl  19(%eax), %eax  #
        shrb    $2, %al         #
        andl    $1, %eax        #
        movzbl  %al, %eax       #
        popl    %ebp            #
        ret
*/

unsigned int barness_inside() {
    return (((Header)0) - 1)->kind.inside;
}

/* m32
barness_inside:
        pushl   %ebp            #
        movl    %esp, %ebp      #
        movl    $-20, %eax      # header
        movzbl  19(%eax), %eax  #
        shrb    $4, %al         #
        andl    $1, %eax        #
        movzbl  %al, %eax       #
        popl    %ebp            #
        ret
*/

void opr_sub(Integer left, Integer right, Reference *result)
{
    integer_Create(left->value - right->value, (Integer*)result);
}

/* m32
opr_sub:
        pushl   %ebp            #
        movl    %esp, %ebp      #,
        pushl   %esi            #
        pushl   %ebx            #
        subl    $16, %esp       #,

        movl    16(%ebp), %esi  # result, result.12

        movl    8(%ebp), %eax   # left, tmp62
        movl    4(%eax), %edx   # <variable>.value, D.3294
        movl    (%eax), %eax    # <variable>.value, D.3294

        movl    12(%ebp), %ecx  # right, tmp63
        movl    4(%ecx), %ebx   # <variable>.value, D.3295
        movl    (%ecx), %ecx    # <variable>.value, D.3295

        subl    %ecx, %eax      # D.3295, D.3296
        sbbl    %ebx, %edx      # D.3295, D.3296

        movl    %eax, (%esp)    # D.3296,
        movl    %edx, 4(%esp)   # D.3296,
        movl    %esi, 8(%esp)   # result.12,
        call    integer_Create  #

        addl    $16, %esp       #,
        popl    %ebx    #
        popl    %esi    #
        popl    %ebp    #
        ret

*/

Reference alloc_gc(Size size, bool atom, Reference type)
{
    Reference result;

    if (!node_Allocate(_zero_space,
                       atom,
                       size,
                       &result)) return 0;

    return result;
}

/* m32
alloc_gc:
        pushl   %ebp               #
        movl    %esp, %ebp         #,
        subl    $56, %esp          #,

        movl    12(%ebp), %eax     # atom, tmp65
        movb    %al, -28(%ebp)     # tmp65, atom

        movl    $0, %eax           #, D.3303
        leal    -12(%ebp), %eax    #, tmp66

        movzbl  -28(%ebp), %ecx    # atom, D.3304

        movl    %eax, 12(%esp)     # D.3303,
        movl    8(%ebp), %eax      # size, tmp67
        movl    %eax, 8(%esp)      # tmp67,
        movl    %ecx, 4(%esp)      # D.3304,
        movl    _zero_space, %edx  # _zero_space, _zero_space.13
        movl    %edx, (%esp)       # _zero_space.13,
        call    node_Allocate      #

        xorl    $1, %eax           # D.3307 (??)
        testb   %al, %al           # D.3307 (??)
        je      .L38               #
        movl    $0, %eax           #, D.3310
        jmp     .L39               #
.L38:
        movl    -12(%ebp), %eax # result, D.3310
.L39:
        leave
        ret
*/

/*
(xchg eax tos)       ## exchange top-of-stack and bottom-of-frame
(push frame)         ## push old frame
(push bos)           ## push old bottom-of-stack
(mov eax frame)      ## set new frame
(mov tos bos)        ## set new  bottom-of-stack
(push (const 0))     ## alloc ret
(mov top eax)        ## hold address of result
(push eax)           ## push address of result
(push (arg 0))       ## push size
(push (arg 1))       ## push atom
(push  eax)          ## push &ret
(call node_Allocate) ## call
(drop 4)             ## clear stack
(let ((pass (new-label))
      (exit (new-label)))
   (xor (const 1) eax)  ## mask result
   (test al al)         ## test result
   (je pass)            ##
   (mov (const 0) eax)
   (jmp exit)
   (label pass)
   (pop eax)
   (label exit))
(mov frame scr)  ## mov the frame(old-tos) to scr
(mov bos tos)    ## clear stack/locals
(pop bos)        ## pop the old bottom-of-stack
(pop frame)      ## pop the old frame
(mov scr tos)    ## set old top-of-stack
(ret)
*/


bool foo_true()
{
    return true;
}

/*
foo_true:
        pushl   %ebp    #
        movl    %esp, %ebp      #,
        movl    $1, %eax        #, D.3313
        popl    %ebp    #
        ret
*/

bool foo_false()
{
    return false;
}

/*
foo_false:
        pushl   %ebp    #
        movl    %esp, %ebp      #,
        movl    $0, %eax        #, D.3316
        popl    %ebp    #
        ret
*/

int  xx_one = 1;
int  xx_two = 2;
bool xx_test;

int foo_if()
{
    if (!xx_test) return xx_one;
    else return xx_two;
}

/*
foo_if:
        pushl   %ebp            #
        movl    %esp, %ebp      #
        movzbl  xx_test, %eax   # xx_test
        xorl    $1, %eax        # (xor 1 xx_test) == 0 if xx_test == 1
        testb   %al, %al        # set ZF to 1 if al == 0
        je      .L46            # jump if ZF == 1
        movl    xx_one, %eax    #
        jmp     .L47            #
.L46:
        movl    xx_two, %eax    #
.L47:
        popl    %ebp            #
        ret
*/
