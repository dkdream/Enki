#include "prefix.inc"

void SUBR(fprint) {
    FILE* out = 0;

    checkArgs(args, "fprint", 1, t_outfile);

    if (!isType(args, t_pair)) {
         fatal("missing first argument to fprint\n");
    } else {
        Node outfile = NIL;

        pair_GetCar(args.pair, &outfile);
        pair_GetCdr(args.pair, &args);

        if (!isType(outfile, t_outfile)) {
            fatal("first argument to fprint is not an outfile\n");
        }

        out = ((OSFile)(outfile.reference))->file;
    }

    while (isType(args, t_pair)) {
        Node text;
        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (!isType(text, t_text)) {
            fatal("invalid argument to fprint\n");
        }

        fprintf(out, "%s", text_Text(text.text));
    }
}
