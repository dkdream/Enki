#include "prefix.inc"

void SUBR(eq)
{
    Node left; Node right;
    checkArgs(args, "==", 2, NIL, NIL);
    forceArgs(args, &left, &right, 0);

    if (node_Match(left,right)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
