#include "prefix.inc"

static inline void eval_binding(Node binding, Node env, Target entry)
{
    GC_Begin(2);
    Node symbol, expr, value;

    GC_Protect(expr);
    GC_Protect(value);

    list_GetItem(binding.pair, 0, &symbol);
    list_GetItem(binding.pair, 1, &expr);

    eval(expr, env, &value);

    pair_Create(symbol, value, entry.pair);

    GC_End();
}

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
        list_Map(bindings.pair, eval_binding, env, &(env2.pair));

        list_SetEnd(env2.pair, env);

        eval_begin(body, env2, result);
    }
}

