#include "prefix.inc"

void SUBR(encode_let) {
    Node locals; Node lenv;

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
    list_Map(environ_Let, locals.pair, env, &lenv);
    list_SetEnd(lenv.pair, env);

    //list_Map(encode, args.pair, lenv, result);

    encode(args, lenv, result);
}

