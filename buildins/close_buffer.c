#include "prefix.inc"

void SUBR(close_buffer) {
    Node buffer;
    checkArgs(args, "close-buffer", 1, t_buffer);
    forceArgs(args, &buffer, 0);

    if (!isType(buffer, t_buffer)) {
        fatal("close-buffer: not an buffer");
    }

    TextBuffer *buff = ((TextBuffer *)(buffer.reference));

    text_Create(*(buff), result.text);

    buffer_free(buff);

    setType(buffer, s_opaque);
}
