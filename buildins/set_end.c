#include "prefix.inc"

void SUBR(set_end) {
    Node head;
    Node tail;
    checkArgs(args, "set-end", 1, t_pair, NIL);
    forceArgs(args, &head, &tail, 0);

    if (!isType(head, t_pair)) {
        ASSIGN(result, NIL);
    }

    list_SetEnd(head.pair, tail);
    ASSIGN(result, true_v);
}
