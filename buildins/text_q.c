#include "prefix.inc"

void SUBR(text_q) {
    Node value;
    checkArgs(args, "text?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isType(value, t_text)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
