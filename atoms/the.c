#include "prefix.inc"

void SUBR(the) {
    Node type;
    Node value;

    checkArgs(args, "the", 2, NIL, NIL);
    fetchArgs(args, &type, &value, 0);

    if (!isType(value, type)) {
        fprintf(stderr, "the : ");
        print(stderr, value);
        fprintf(stderr, " :is not a: ");
        print(stderr, type);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    ASSIGN(result, value);
}
