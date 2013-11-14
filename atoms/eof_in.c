#include "prefix.inc"

void SUBR(eof_in) {
    Node file;

    checkArgs(args, "eof_in", 1, t_infile);
    forceArgs(args, &file, 0);

    if (!isType(file, t_infile)) {
        fatal("read-line: not an infile");
    }

    FILE* in = ((OSFile)(file.reference))->file;

    if (feof(in)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
