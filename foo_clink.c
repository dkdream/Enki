#include "reference.h"
#include "treadmill.h"

void foo_clink(Reference a,
               Reference b,
               Reference c)
{
    GC_Begin(3);

    GC_Inline_Protect(a);
    GC_Inline_Protect(b);
    GC_Inline_Protect(c);

    GC_End();
}



