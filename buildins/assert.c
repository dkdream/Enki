#include "prefix.inc"

void SUBR(assert)
{
    Node test; Node message;

    unsigned count = checkArgs(args, "assert", 1, NIL);

    fetchArgs(args, &test, &message, 0);

    if (!isNil(test)) {
        ASSIGN(result, true_v);
        return;
    }

    if (1 < count) {
        fprintf(stderr, "assert: ");
        print(stderr, message);
    } else {
        fprintf(stderr, "assert falure");
    }
    fprintf(stderr, "\n");
    fflush(stderr);
    fatal(0);
}
