#include "prefix.inc"

void SUBR(eval_pair)
{
    GC_Begin(5);
    Node obj, head, tail, tmp;

    GC_Protect(obj);
    GC_Protect(head);
    GC_Protect(tail);
    GC_Protect(tmp);

    // (subr_eval_pair obj)
    pair_GetCar(args.pair, &obj);
    pair_GetCar(obj.pair, &head);
    pair_GetCdr(obj.pair, &tail);

    pushTrace(obj);

    // first eval the head
    eval(head, env, &head);

    if (fromCtor(head, s_delay)) {
        Node dexpr, denv;
        tuple_GetItem(head.tuple, 1, &dexpr);
        tuple_GetItem(head.tuple, 2, &denv);
        eval(dexpr, denv, &tmp);
        tuple_SetItem(head.tuple, 0, tmp);
        setConstructor(head.tuple, s_forced);
        head = tmp;
    }

    if (fromCtor(head, s_fixed)) {
        // apply Fixed to un-evaluated arguments
        Node func = NIL;
        tuple_GetItem(head.tuple, fxd_eval, &func);
        apply(func, tail, env, result);
        goto done;
    }

    // evaluate the arguments
    list_Map(tail.pair, eval, env, &tail.pair);

    // now apply the head to the evaluated arguments
    apply(head, tail, env, result);

 done:
    popTrace();

    GC_End();
}
