/*-*- mode: c;-*-*/
#if !defined(_ea_string_h_)
#define _ea_string_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include "text_buffer.h"

/* */
struct text {
    HashCode hashcode;
    // size in chars
    unsigned int  size;
    unsigned long value[1];
};

extern bool text_Create(TextBuffer value, Text*);

/* macros */
extern inline const char* text_Text(Text) __attribute__((always_inline nonnull(1)));
extern inline const char* text_Text(Text text) {
    if (!text) return "";
    return (const char*)(text->value);
}

/***************************
 ** end of file
 **************************/
#endif
