#include "prefix.inc"

void SUBR(level)
{
    Node value = NIL;

    integer_Create(ea_global_debug, &value.integer);

    if (isType(args, t_pair)) {
        Node nvalue;
        pair_GetCar(args.pair, &nvalue);
        if (isType(nvalue, t_integer)) {
            ea_global_debug = nvalue.integer->value;
        }
    }

    ASSIGN(result,value);
}
