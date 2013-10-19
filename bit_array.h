/*-*- mode: c;-*-*/
#if !defined(_ea_bit_array_h_)
#define _ea_bit_array_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>

struct bit_array {
    char *buffer;
    unsigned size;
    unsigned marker;
};

typedef struct bit_array BitArray;

#define BITS_INITIALISER { 0, 0, 0 }

extern inline void bits_init(BitArray *array) __attribute__((always_inline nonnull(1)));
extern inline void bits_init(BitArray *array) {
    array->buffer = 0;
    array->size   = 0;
    array->marker = 0;
}

extern inline void bits_reset(BitArray *array) __attribute__((always_inline nonnull(1)));
extern inline void bits_reset(BitArray *array) {
    array->marker = 0;
}

extern inline unsigned bits_marker(const BitArray *array) __attribute__((always_inline nonnull(1)));
extern inline unsigned bits_marker(const BitArray *array) {
    return array->marker;
}

extern inline unsigned bits_size(const BitArray *array) __attribute__((always_inline nonnull(1)));
extern inline unsigned bits_size(const BitArray *array) {
    return array->size;
}

extern inline unsigned long bits_count(const BitArray *array) __attribute__((always_inline nonnull(1)));
extern inline unsigned long bits_count(const BitArray *array) {
    unsigned long count = array->size;
    return count * CHAR_BIT;
}


extern inline void bits_extendTo(BitArray *array, const unsigned count) __attribute__((always_inline nonnull(1)));
extern inline void bits_extendTo(BitArray *array, const unsigned count)
{
    while (count >= array->size) {
        if (array->buffer) {
            array->buffer = realloc(array->buffer, array->size *= 2);
        } else {
            array->buffer = malloc(array->size = 32);
        }
    }
    array->marker = count + 1;
}

extern inline bool bits_get(const BitArray *array, const unsigned index) __attribute__((always_inline nonnull(1)));
extern inline bool bits_get(const BitArray *array, const unsigned index)
{
    const unsigned offset = index / CHAR_BIT;
    const unsigned mask   = 1 << (index % CHAR_BIT);

    if (offset >= array->marker) return false;

    char slot = array->buffer[offset];

    if (mask & slot) return true;

    return false;
}

extern inline void bits_set(BitArray *array, const unsigned index, const bool value) __attribute__((always_inline nonnull(1)));
extern inline void bits_set(BitArray *array, const unsigned index, const bool value)
{
    const unsigned offset = index / CHAR_BIT;
    const unsigned mask   = 1 << (index % CHAR_BIT);

    if (offset >= array->marker) {
        if (!value) return;
        bits_extendTo(array, offset);
    }

    char slot = array->buffer[offset];

    if (value) {
        array->buffer[offset] = slot | mask;
    } else {
        array->buffer[offset] = slot & (~mask);
    }
}

extern inline int bits_walk(const BitArray *array, const int last) __attribute__((always_inline nonnull(1)));
extern inline int bits_walk(const BitArray *array, const int last)
{
    int index = (0 > last) ? 0 : (last + 1);

    const unsigned boundry = 1 << CHAR_BIT;

    unsigned offset = index / CHAR_BIT;
    unsigned mask   = 1 << (index % CHAR_BIT);

    for (;; ++offset) {
        if (offset >= array->marker) return -1;

        char slot = array->buffer[offset];

        for (; mask < boundry ; ++index) {
            if (mask & slot) return index;
            mask = mask << 1;
        }
        mask = 1;
    }
}

extern inline void bits_free(BitArray *array)  __attribute__((always_inline nonnull(1)));
extern inline void bits_free(BitArray *array) {
    if (array->buffer) {
        free(array->buffer);
    }
    bits_init(array);
}

/***************************
 ** end of file
 **************************/
#endif
