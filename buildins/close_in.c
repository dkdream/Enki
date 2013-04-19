#include "prefix.inc"

void SUBR(close_in) {
    Node file;
    checkArgs(args, "close-in", 1, t_infile);
    forceArgs(args, &file, 0);

    if (!isType(file, t_infile)) {
        fatal("close-in: not an infile");
    }

    FILE* in = ((OSFile)(file.reference))->file;

    if (fclose(in)) {
        fatal("close-in: error closing os-file");
    }

    ((OSFile)(file.reference))->file = 0;

    setType(file, s_opaque);
}
