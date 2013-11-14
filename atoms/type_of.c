#include "prefix.inc"

void SUBR(type_of)
{
    Node value, type;
    int count = checkArgs(args, "type-of", 1, NIL);

    ASSIGN(result,NIL);

    if (1 < count) {
        forceArgs(args, &value, &type, 0);
        setType(value, type);
    } else {
        pair_GetCar(args.pair, &value);
        node_TypeOf(value, result);
    }
}
