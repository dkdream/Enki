#include "prefix.inc"

void SUBR(open_buffer) {
    Reference buffer = 0;

    if (!opaque_Create(t_buffer, sizeof(struct text_buffer), &buffer)) {
        fatal("failed to allocate opaque object");
    }

    buffer_init((TextBuffer *)buffer);

    ASSIGN(result, buffer);
}
