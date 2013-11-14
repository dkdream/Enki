#include "prefix.inc"

void SUBR(cdr) {
    Pair pair;

    checkArgs(args, "car", 1, t_pair);
    forceArgs(args, &pair, 0);

    pair_GetCdr(pair, result);
}
