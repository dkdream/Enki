#include "prefix.inc"

void SUBR(read_sexpr) {
    static TextBuffer buffer = BUFFER_INITIALISER;
    buffer_reset(&buffer);

    Node file;

    checkArgs(args, "read-sexpr", 1, t_infile);
    forceArgs(args, &file, 0);

    if (!isType(file, t_infile)) {
        fatal("read-line: not an infile");
    }

    FILE* in = ((OSFile)(file.reference))->file;

    if (feof(in)) {
        ASSIGN(result, NIL);
        return;
    }

    readExpr(in, result);
}
