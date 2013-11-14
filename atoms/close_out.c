#include "prefix.inc"

void SUBR(close_out) {
    Node file;
    checkArgs(args, "close-out", 1, t_outfile);
    forceArgs(args, &file, 0);

    if (!isType(file, t_outfile)) {
        fatal("close-out: not an outfile");
    }

    FILE* out = ((OSFile)(file.reference))->file;

    if (fclose(out)) {
        fatal("close-out: error closing os-file");
    }

    ((OSFile)(file.reference))->file = 0;

    setType(file, s_opaque);
}
