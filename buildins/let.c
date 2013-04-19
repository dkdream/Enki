#include "prefix.inc"

void SUBR(let)
{ //Fixed
    Node env2     = NIL;
    Node bindings = NIL;
    Node body     = NIL;

    pair_GetCar(args.pair, &bindings);
    pair_GetCdr(args.pair, &body);

    if (!isType(bindings, t_pair)) {
        eval_begin(body, env, result);
    } else {
        list_Map(eval_binding, bindings.pair, env, &(env2.pair));

        list_SetEnd(env2.pair, env);

        eval_begin(body, env2, result);
    }
}

