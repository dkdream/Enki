#include "prefix.inc"

void SUBR(not) {
    Node value;

    checkArgs(args, "not", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isNil(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
