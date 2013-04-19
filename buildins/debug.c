#include "prefix.inc"

void SUBR(debug)
{
    Node value = NIL;
    long level = 1;
    if (isType(args, t_pair)) {
        pair_GetCar(args.pair, &value);
        if (isType(value, t_integer)) {
            level = value.integer->value;
            pair_GetCdr(args.pair, &args);
        }
    }
    if (ea_global_debug <= level) {
        while (isType(args, t_pair)) {
            pair_GetCar(args.pair, &value);
            pair_GetCdr(args.pair, &args);
            print(stderr, value);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    ASSIGN(result,NIL);
}
