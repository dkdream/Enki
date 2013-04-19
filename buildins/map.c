#include "prefix.inc"


void SUBR(map) {
    Node function;
    Node list;

    checkArgs(args, "map", 1, NIL, t_pair);
    forceArgs(args, &function, &list, 0);

    if (isNil(function)) {
        ASSIGN(result, list);
        return;
    }

    if (!isType(list, t_pair)) {
        call_with(function, list, env, result);
        return;
    }

    Pair pair = list.pair;

    GC_Begin(4);

    Node input;
    Node output;
    Node first;

    GC_Protect(input);
    GC_Protect(output);
    GC_Protect(first);

    call_with(function, pair->car, env, &output);

    pair_Create(output,NIL, &first.pair);

    Pair last = first.pair;

    for (; isType(pair->cdr.pair, t_pair) ;) {
        pair   = pair->cdr.pair;
        input  = pair->car;
        output = NIL;

        call_with(function, input, env, &output);

        pair_Create(output,NIL, &(last->cdr.pair));

        last = last->cdr.pair;
    }

    if (!isNil(pair->cdr.pair)) {
        input  = pair->cdr;
        output = NIL;

        call_with(function, input, env, &output);
        pair_SetCdr(last, output);
    }

    ASSIGN(result, first);

    GC_End();
}
