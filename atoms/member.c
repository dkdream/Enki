#include "prefix.inc"

void SUBR(member)
{
    Node tst = NIL;
    Node lst = NIL;

    forceArgs(args, &tst, &lst, 0);

    while (isType(lst, t_pair)) {
        Node elm = NIL;

        pair_GetCar(lst.pair, &elm);

        if (node_Match(tst, elm)) {
            ASSIGN(result, true_v);
            break;
        }

        pair_GetCdr(lst.pair, &lst);
    }
}
