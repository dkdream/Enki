#include "prefix.inc"

static void environ_Let(Node local, Node env, Target result)
{
    Node symbol;
    pair_GetCar(local.pair, &symbol);
    pair_Create(symbol, NIL, result.pair);
}

void SUBR(encode_let) {
    Node locals; Pair lenv;

    pair_GetCar(args.pair, &locals);

    if (!isType(locals, t_pair)) {
        encode(args, env, result);
        return;
    }

    /*
    ** given args=(locals . body)
    **
    ** (set locals (car body))
    ** (set lenv (map (lambda (binding) (box (car binding))) locals))
    ** (set-end lenv env)
    **
    ** result=(encode args lenv)
    */
    list_Map(locals.pair, environ_Let, env, &lenv);
    list_SetEnd(lenv, env);

    //list_Map(encode, args.pair, lenv, result);

    encode(args, lenv, result);
}

