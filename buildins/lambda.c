#include "prefix.inc"

void SUBR(lambda)
{
    pair_Create(args, env, result.pair);
    setConstructor(*result.reference, s_lambda);
}

