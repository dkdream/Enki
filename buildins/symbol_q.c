#include "prefix.inc"

void SUBR(symbol_q) {
    Node value;
    checkArgs(args, "symbol?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isType(value, s_symbol)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
