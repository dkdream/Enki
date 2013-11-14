#include "prefix.inc"

void SUBR(integer_q) {
    Node value;
    checkArgs(args, "integer?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isType(value, t_integer)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
