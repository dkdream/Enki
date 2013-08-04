/*-*- mode: c;-*-*/
#if !defined(_ea_text_buffer_h_)
#define _ea_text_buffer_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include <stdlib.h>
#include <string.h>

struct text_buffer
{
    char *buffer;
    int   size;
    int   position;
};

typedef struct text_buffer TextBuffer;

#define BUFFER_INITIALISER { 0, 0, 0 }

extern inline void buffer_init(TextBuffer *tbuf) __attribute__((always_inline nonnull(1)));
extern inline void buffer_init(TextBuffer *tbuf) {
    tbuf->buffer   = 0;
    tbuf->size     = 0;
    tbuf->position = 0;
}

extern inline void buffer_reset(TextBuffer *tbuf) __attribute__((always_inline nonnull(1)));
extern inline void buffer_reset(TextBuffer *tbuf) {
    tbuf->position = 0;
}

extern inline unsigned buffer_marker(TextBuffer *tbuf) __attribute__((always_inline nonnull(1)));
extern inline unsigned buffer_marker(TextBuffer *tbuf) {
    if (0 > tbuf->position) return 0;
    return tbuf->position;
}

extern inline void buffer_extendBy(TextBuffer *tbuf, unsigned count) __attribute__((always_inline nonnull(1)));
extern inline void buffer_extendBy(TextBuffer *tbuf, unsigned count) {

    while ((tbuf->position + count + 2) > tbuf->size) {
        if (tbuf->buffer) {
            tbuf->buffer = realloc(tbuf->buffer, tbuf->size *= 2);
        } else {
            tbuf->buffer = malloc(tbuf->size = 32);
        }
    }
}


extern inline void buffer_append(TextBuffer *tbuf, int chr) __attribute__((always_inline nonnull(1)));
extern inline void buffer_append(TextBuffer *tbuf, int chr)
{
    buffer_extendBy(tbuf, 1);

    tbuf->buffer[tbuf->position++] = chr;
}

extern inline void buffer_join(TextBuffer *tbuf, TextBuffer *data) __attribute__((always_inline nonnull(1)));
extern inline void buffer_join(TextBuffer *tbuf, TextBuffer *data)
{
    if (!data) return;

    char *string = data->buffer;
    int      len = data->position;

    if (0 >  len) return;
    if (0 == len) return;

    buffer_extendBy(tbuf, len);

    for ( ; 0 < len; --len) {
        tbuf->buffer[tbuf->position++] = *(string++);
    }
}

extern inline void buffer_add(TextBuffer *tbuf, const char *string) __attribute__((always_inline nonnull(1)));
extern inline void buffer_add(TextBuffer *tbuf, const char *string)
{
    if (!string) return;

    int len = strlen(string);

    if (0 >  len) return;
    if (0 == len) return;

    buffer_extendBy(tbuf, len);

    for ( ; 0 < len; --len) {
        tbuf->buffer[tbuf->position++] = *(string++);
    }
}

extern inline const char *buffer_contents(TextBuffer *tbuf) __attribute__((always_inline nonnull(1)));
extern inline const char *buffer_contents(TextBuffer *tbuf)
{
    buffer_append(tbuf, 0);
    tbuf->position--;
    return tbuf->buffer;
}

extern inline void buffer_free(TextBuffer *tbuf) __attribute__((always_inline nonnull(1)));
extern inline void buffer_free(TextBuffer *tbuf)
{
    if (tbuf->buffer) {
        free(tbuf->buffer);
    }
    buffer_init(tbuf);
}

/***************************
 ** end of file
 **************************/
#endif
