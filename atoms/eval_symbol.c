#include "prefix.inc"

void SUBR(eval_symbol)
{
    GC_Begin(2);
    Node symbol, tmp;
    Variable entry;

    GC_Protect(tmp);

    pair_GetCar(args.pair, &symbol);

    // lookup symbol in the current enviroment
    if (!alist_Entry(env.pair, symbol.symbol, &entry)) {
        if (!alist_Entry(enki_globals.pair,  symbol.symbol, &entry)) goto error;
    }

    tmp = entry->value;

    if (isForced(tmp)) {
        tuple_GetItem(tmp.tuple, 0, &(entry->value));
        tmp = entry->value;
    }

    ASSIGN(result, tmp);

    GC_End();
    return;

 error:
    GC_End();
    if (!isSymbol(symbol)) {
        fatal("undefined variable: <non-symbol>");
    } else {
        fatal("undefined variable: %s", symbol_Text(symbol.symbol));
    }
}
