#include "prefix.inc"

void SUBR(if)
{ //Fixed
    GC_Begin(2);

    Node tst, val, t_expr, e_body;

    GC_Protect(val);

    list_GetItem(args.pair, 0, &tst);
    list_GetItem(args.pair, 1, &t_expr);
    list_GetTail(args.pair, 1, &e_body);

    eval(tst, env, &val);

    if (isNil(val)) {
        eval_begin(e_body, env, result);
    } else {
        eval(t_expr, env, result);
    }
    GC_End();
}

