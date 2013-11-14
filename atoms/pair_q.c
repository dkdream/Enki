#include "prefix.inc"

void SUBR(pair_q) {
    Node value;
    checkArgs(args, "pair?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isType(value, t_pair)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
