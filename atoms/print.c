#include "prefix.inc"


void SUBR(print)
{
    Node value = NIL;

    while (isType(args, t_pair)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        print(stdout, value);
    }

    fflush(stdout);
    ASSIGN(result,NIL);
}
