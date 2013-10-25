#include "prefix.inc"

void SUBR(set)
{ //Fixed
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;

    fetchArgs(args, &symbol, &expr, 0);

    if (!isSymbol(symbol)) {
        fprintf(stderr, "\nerror: non-symbol identifier in set: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    Variable entry;

    if (!alist_Entry(env.pair, symbol.symbol, &entry)) {
        fprintf(stderr, "\nerror: cannot set undefined variable: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    eval(expr, env, &(entry->value));

    ASSIGN(result,value);
}

