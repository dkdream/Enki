#include "prefix.inc"


void SUBR(encode)
{
    GC_Begin(3);

    Node expr, cenv;

    GC_Protect(expr);
    GC_Protect(cenv);

    // (encode expr env)
    // (encode expr)
    list_GetItem(args.pair, 0, &expr);
    list_GetItem(args.pair, 1, &cenv);

    if (isNil(cenv)) cenv = env;

    encode(expr, cenv, result);

    GC_End();
}
