#include "prefix.inc"

void SUBR(concat_text) {
    static TextBuffer buffer = BUFFER_INITIALISER;
    static char data[20];
    buffer_reset(&buffer);

    checkArgs(args, "concat-text", 2, t_text, t_text);

    while (isType(args, t_pair)) {
        Node text;

        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (isType(text, t_text)) {
            buffer_add(&buffer, text_Text(text.text));
            continue;
        }

        if (isType(text, s_symbol)) {
            buffer_add(&buffer, symbol_Text(text.symbol));
            continue;
        }

        if (isType(text, t_integer)) {
            long value = text.integer->value;
            sprintf(data, "%ld", value);
            buffer_add(&buffer, data);
            continue;
        }

        fatal("wrong type of argument to: %s", "concat-text");
    }

    text_Create(buffer, result.text);
}
