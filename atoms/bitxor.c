#include "prefix.inc"

void SUBR(bitxor)
{
    Node left; Node right;
    checkArgs(args, "^", 2, t_integer, t_integer);
    forceArgs(args, &left, &right, 0);
    integer_Create((left.integer->value) ^ (right.integer->value), result.integer);
}
