#include "prefix.inc"

void SUBR(delay)
{
    Tuple tuple, expr;

    pair_GetCar(args.pair, &expr);

    tuple_Create(3, &tuple);
    tuple_SetItem(tuple, 0, NIL);
    tuple_SetItem(tuple, 1, expr);
    tuple_SetItem(tuple, 2, env);

    setConstructor(tuple, s_delay);

    ASSIGN(result, tuple);
}

