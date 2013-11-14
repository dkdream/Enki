#include "prefix.inc"

void SUBR(nil_q) {
    Node value;

    checkArgs(args, "nil?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isNil(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
