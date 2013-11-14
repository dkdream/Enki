#include "prefix.inc"

void SUBR(dump)
{
    Node value = NIL;

    if (isType(args, t_pair)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        prettyPrint(stdout, value);
    }

    while (isType(args, t_pair)) {
        printf(" ");
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        prettyPrint(stdout, value);
    }

    fflush(stdout);
    ASSIGN(result,NIL);
}
