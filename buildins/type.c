#include "prefix.inc"

void SUBR(type)
{ //Fixed
    Node symbol;
    pair_GetCar(args.pair, &symbol);

    if (!isType(symbol, s_symbol)) {
        ASSIGN(result, symbol);
        return;
    }

    type_Create(symbol.symbol, zero_s, result.type);
}

