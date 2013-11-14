#include "prefix.inc"

void SUBR(tuple) {
    unsigned size = 0;
    bool     dotted = false;

    list_State(args.pair, &size, &dotted);

    if (dotted) fatal("dotted list in: tuple");

    if (1 > size) {
        ASSIGN(result, NIL);
        return;
    }

    GC_Begin(2);
    Node tuple;
    GC_Protect(tuple);

    tuple_Create(size, &tuple.tuple);
    tuple_Fill(tuple.tuple, args.pair);
    ASSIGN(result, tuple);

    GC_End();
}
