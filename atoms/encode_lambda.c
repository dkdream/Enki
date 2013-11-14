#include "prefix.inc"

static void environ_Lambda(Node symbol, Node env, Target result)
{
    pair_Create(symbol, NIL, result.pair);
}

void SUBR(encode_lambda) {
    Node formals; Node body; Pair lenv;

    pair_GetCar(args.pair, &formals);
    pair_GetCdr(args.pair, &body);

    /*
    ** given args=(formal . body)
    **
    ** (set lenv (map box formal))
    ** (set-end lenv env)
    **
    ** result=(cons formals (encode body lenv))
    */

    list_Map(formals.pair, environ_Lambda, env, &lenv);

    list_SetEnd(lenv, env);

    encode(body, lenv, &(body.pair));

    pair_Create(formals, body, result.pair);
}

