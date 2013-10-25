#include "prefix.inc"

void SUBR(define)
{ //Fixed
    Node symbol  = NIL;
    Node expr    = NIL;
    Node value   = NIL;
    Node globals = NIL;

    fetchArgs(args, &symbol, &expr, 0);

    if (!isType(symbol, s_symbol)) {
        fprintf(stderr, "\nerror: non-symbol identifier in define: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    eval(expr, env, &value);

    VM_DEBUG(1, "defining %s", symbol_Text(symbol.symbol));

    pair_GetCdr(enki_globals.pair, &globals);
    alist_Add(globals.pair, symbol.symbol, value, getType(value).constant, &globals.pair);
    pair_SetCdr(enki_globals.pair, globals);

    ASSIGN(result, value);
}

