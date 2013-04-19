#include "prefix.inc"

void SUBR(add)
{
    Integer left; Integer right;
    forceArgs(args, &left, &right, 0);
    integer_Create((left->value) + (right->value), result.integer);
}
