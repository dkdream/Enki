#include "prefix.inc"

void SUBR(lambda)
{
    pair_Create(args, env, result.pair);
    setType(*result.reference, t_lambda);
}

