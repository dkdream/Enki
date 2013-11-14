#include "prefix.inc"

void SUBR(car) {
    Pair pair;
    checkArgs(args, "car", 1, t_pair);
    forceArgs(args, &pair, 0);

    pair_GetCar(pair, result);
}
