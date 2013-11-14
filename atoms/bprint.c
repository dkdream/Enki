#include "prefix.inc"

void SUBR(bprint) {
    TextBuffer *out = 0;

    checkArgs(args, "bprint", 1, t_buffer);

    if (!isType(args, t_pair)) {
        fatal("missing first argument to bprint\n");
    } else {
        Node buffer = NIL;

        pair_GetCar(args.pair, &buffer);
        pair_GetCdr(args.pair, &args);

        if (!isType(buffer, t_buffer)) {
            fatal("first argument to bprint is not an buffer\n");
        }

        out = ((TextBuffer *)(buffer.reference));
    }

    while (isType(args, t_pair)) {
        Node text;
        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (!isType(text, t_text)) {
            fatal("invalid argument to bprint\n");
        }

        buffer_add(out, text_Text(text.text));
    }
}
