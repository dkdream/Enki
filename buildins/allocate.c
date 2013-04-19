#include "prefix.inc"

void SUBR(allocate) {
    Node type; Node size;
    checkArgs(args, "allocate", 2, s_symbol, t_integer);

    ASSIGN(result,NIL);

    forceArgs(args, &type, &size, 0);

    long slots = size.integer->value;

    if (1 > slots) return;

    Tuple value;

    tuple_Create(slots, &value);

    setType(value, type.symbol);

    ASSIGN(result,value);
}
