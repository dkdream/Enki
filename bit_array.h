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
#include <stdio.h>

struct bit_array {
    char *buffer;
    unsigned size;
    unsigned marker;
};

typedef struct bit_array BitArray;

#define BITS_INITIALISER { 0, 0, 0 }

extern inline void bits_init(BitArray *array) __attribute__((always_inline nonnull));
extern inline void bits_init(BitArray *array) {
    array->buffer = 0;
    array->size   = 0;
    array->marker = 0;
}

extern inline void bits_reset(BitArray *array) __attribute__((always_inline nonnull));
extern inline void bits_reset(BitArray *array) {
    array->marker = 0;

    const unsigned max = array->size;
    unsigned    offset = 0;
    for (; offset < max ; ++offset) {
        array->buffer[offset] = 0;
    }
}

extern inline unsigned bits_marker(const BitArray *array) __attribute__((always_inline nonnull));
extern inline unsigned bits_marker(const BitArray *array) {
    return array->marker;
}

extern inline unsigned bits_size(const BitArray *array) __attribute__((always_inline nonnull));
extern inline unsigned bits_size(const BitArray *array) {
    return array->size;
}

extern inline unsigned long bits_count(const BitArray *array) __attribute__((always_inline nonnull));
extern inline unsigned long bits_count(const BitArray *array) {
    unsigned long count = array->size;
    return count * CHAR_BIT;
}

extern inline void bits_extendTo(BitArray *array, const unsigned count) __attribute__((always_inline nonnull));
extern inline void bits_extendTo(BitArray *array, const unsigned count)
{
    while (count >= array->size) {
        if (array->buffer) {
            array->buffer = realloc(array->buffer, array->size *= 2);
        } else {
            array->buffer = malloc(array->size = 32);
        }
        const unsigned max = array->size;
        unsigned    offset = array->marker;
        for (; offset < max ; ++offset) {
            array->buffer[offset] = 0;
        }
    }
    array->marker = count + 1;
}

extern inline bool bits_get(const BitArray *array, const unsigned index) __attribute__((always_inline nonnull));
extern inline bool bits_get(const BitArray *array, const unsigned index)
{
    const unsigned offset = index / CHAR_BIT;
    const unsigned mask   = 1 << (index % CHAR_BIT);

    if (offset >= array->marker) return false;

    char slot = array->buffer[offset];

    if (mask & slot) return true;

    return false;
}

extern inline void bits_set(BitArray *array, const unsigned index, const bool value) __attribute__((always_inline nonnull));
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

extern inline void bits_normalize(BitArray *array) __attribute__((always_inline nonnull));
extern inline void bits_normalize(BitArray *array) {
    unsigned offset = array->marker;
    for (; 0 < offset ;) {
        --offset;
        if (0 == array->buffer[offset]) continue;
        array->marker = offset + 1;
        return;
    }
}

extern inline void bits_or(BitArray *target, BitArray *source) __attribute__((always_inline nonnull));
extern inline void bits_or(BitArray *target, BitArray *source) {
    unsigned boundry = source->marker;
    unsigned offset  = 0;

    for (; offset < boundry ; ++offset) {
        char slot = source->buffer[offset];
        if (0 == slot) continue;
        if (offset >= target->marker) {
            bits_extendTo(target, offset);
        }
        target->buffer[offset] |= slot;
    }
}

extern inline void bits_xor(BitArray *target, BitArray *source) __attribute__((always_inline nonnull));
extern inline void bits_xor(BitArray *target, BitArray *source) {
    unsigned boundry = source->marker;
    unsigned offset  = 0;

    for (; offset < boundry ; ++offset) {
        char slot = source->buffer[offset];
        if (0 == slot) continue;
        if (offset >= target->marker) {
            bits_extendTo(target, offset);
        }
        target->buffer[offset] ^= slot;
    }

    bits_normalize(target);
}

extern inline void bits_and(BitArray *target, BitArray *source) __attribute__((always_inline nonnull));
extern inline void bits_and(BitArray *target, BitArray *source) {
    unsigned offset  = (target->marker < source->marker ? target->marker : source->marker);

    for (; 0 < offset ;) {
        --offset;

        if (offset >= source->marker) {
            target->buffer[offset] = 0;
            continue;
        }

        char slot = target->buffer[offset] & source->buffer[offset];

        target->buffer[offset] = slot;

        if (0 != slot) {
            target->marker = offset + 1;
            break;
        }
    }

    for (; 0 < offset ;) {
        --offset;
        target->buffer[offset] &= source->buffer[offset];
    }
}

extern inline void bits_section(BitArray *array, const unsigned bottom, const unsigned top, const bool value) __attribute__((always_inline nonnull));
extern inline void bits_section(BitArray *array, const unsigned bottom, const unsigned top, const bool value)
{
    if (bottom >= top) return;

    const unsigned boundry = 1 << (CHAR_BIT - 1);

    if (value) {
        bits_extendTo(array, top / CHAR_BIT);

        unsigned   mask = 0;
        unsigned  count = bottom;
        unsigned offset = bottom / CHAR_BIT;
        char       slot = array->buffer[offset];

        // mark to a byte boundry
        for (; mask < boundry; ++count) {
            if (count >= top) break;
            mask = mask | (1 << (count % CHAR_BIT));
        }

        array->buffer[offset] = slot | mask;
        ++offset;

        for (;; ++offset) {
            if (offset >= array->marker)   return;
            if ((count + CHAR_BIT) >= top) break;
            array->buffer[offset] = -1;
            count += CHAR_BIT;
        }

        mask = 0;
        slot = array->buffer[offset];

        // mark the tail
        for (; mask < boundry ; ++count) {
            if (count >= top) break;
            mask = mask | (1 << (count % CHAR_BIT));
        }

        array->buffer[offset] = slot | mask;

    } else {
        unsigned offset = bottom / CHAR_BIT;

        if (offset >= array->marker)  return;

        unsigned  mask = 0;
        unsigned count = bottom;
        char      slot = array->buffer[offset];

        // mark to a byte boundry
        for (; mask < boundry; ++count) {
            if (count >= top) break;
            mask = mask | (1 << (count % CHAR_BIT));
        }

        array->buffer[offset] = slot & (~mask);
        ++offset;

        for (;; ++offset) {
            if (offset >= array->marker)   return;
            if ((count + CHAR_BIT) >= top) break;
            array->buffer[offset] = 0;
            count += CHAR_BIT;
        }

        mask = 0;
        slot = array->buffer[offset];

        // mark the tail
        for (; mask < boundry ; ++count) {
            if (count >= top) break;
            mask = mask | (1 << (count % CHAR_BIT));
        }

        array->buffer[offset] = slot & (~mask);
    }
}

extern inline unsigned long bits_found(const BitArray *array) __attribute__((always_inline nonnull));
extern inline unsigned long bits_found(const BitArray *array)
{
    const unsigned boundry = 1 << CHAR_BIT;

    unsigned long found = 0;
    unsigned     offset = 0;
    unsigned       mask = 1;

    for (;; ++offset) {
        if (offset >= array->marker) return found;

        char slot = array->buffer[offset];

        for (; mask < boundry ;) {
            if (mask & slot) ++found;
            mask = mask << 1;
        }

        mask = 1;
    }
}

extern inline int bits_walk(const BitArray *array, const int last) __attribute__((always_inline nonnull));
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

extern inline void bits_free(BitArray *array)  __attribute__((always_inline nonnull));
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
