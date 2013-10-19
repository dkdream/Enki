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
    int size;
    int marker;
};

typedef struct bit_array BitArray;

#define ARRAY_INITIALISER { 0, 0, 0 }

extern inline void array_init(BitArray *array) __attribute__((always_inline nonnull(1)));
extern inline void array_init(BitArray *array) {
    array->buffer = 0;
    array->size   = 0;
    array->marker = 0;
}

extern inline void array_reset(BitArray *array) __attribute__((always_inline nonnull(1)));
extern inline void array_reset(BitArray *array) {
    array->marker = 0;
}

extern inline unsigned array_marker(const BitArray *array) __attribute__((always_inline nonnull(1)));
extern inline unsigned array_marker(const BitArray *array) {
    if (0 > array->marker) return 0;
    return array->marker;
}

extern inline void array_extendBy(BitArray *array, unsigned count) __attribute__((always_inline nonnull(1)));
extern inline void array_extendBy(BitArray *array,
                                  unsigned count)
{
    while ((array->marker + count + 2) > array->size) {
        if (array->buffer) {
            array->buffer = realloc(array->buffer, array->size *= 2);
        } else {
            array->buffer = malloc(array->size = 32);
        }
    }
}

extern inline bool array_get(const BitArray *array, unsigned index) __attribute__((always_inline nonnull(1)));
extern inline bool array_get(const BitArray *array,
                             unsigned index)
{
    const unsigned offset = index / CHAR_BIT;
    const unsigned mask   = 1 << (index % CHAR_BIT);

    if (offset < array->marker) return false;

    char slot = array->buffer[offset];

    if (mask & slot) return true;

    return false;
}

extern inline void array_set(BitArray *array, unsigned index, bool value) __attribute__((always_inline nonnull(1)));
extern inline void array_set(BitArray *array,
                             unsigned index,
                             bool     value)
{
    const unsigned offset = index / CHAR_BIT;
    const unsigned mask   = 1 << (index % CHAR_BIT);

    if (offset >= array->marker) {
        if (!value) return;
        array_extendBy(array, offset);
        array->marker = offset;
    }

    char slot = array->buffer[offset];

    if (value) {
        array->buffer[offset] = slot | mask;
    } else {
        array->buffer[offset] = slot & (~mask);
    }
}

extern inline int array_walk(const BitArray *array, int last) __attribute__((always_inline nonnull(1)));
extern inline int array_walk(const BitArray *array,
                             const int last)
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

extern inline void array_free(BitArray *array)  __attribute__((always_inline nonnull(1)));
extern inline void array_free(BitArray *array) {
    if (array->buffer) {
        free(array->buffer);
    }
    array_init(array);
}

/***************************
 ** end of file
 **************************/
#endif
