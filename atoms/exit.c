#include "prefix.inc"

void SUBR(exit)
{
    Node value = NIL;
    forceArgs(args, &value, 0);

    if (isType(value, t_integer)) {
        exit(value.integer->value);
    } else {
        exit(0);
    }
}
