/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "text.h"
#include "symbol.h"
#include "treadmill.h"

/* */
#include <string.h>

/* */
struct cu_data {
    unsigned    length;
    const char* start;
};

typedef struct cu_data CuData;

/*
 * the hash code is computed but xor-ing the longs together
 * but a text may not fill the last long so when to text
 * are combined the one cant just xor the hashs to get the new
 * hash one must rotate the bytes of the trailing texts hash
 * before xor-ing it to hash of the leading text.
 *
 *  adjust   = head.size % sizeof(unsigned long)
 *  new.hash = head.hash ^ rotate(adjust, tail.hash)
 *  new.size = head.size + tail.size
 *
 */
static inline HashCode hash_ajust(unsigned int size, HashCode hash) {
    unsigned int adjust = size % sizeof(HashCode);

    if (0 == adjust) return hash;

    union {
        HashCode hash;
        char map[sizeof(HashCode)];
    } input, output;

    input.hash  = hash;
    output.hash = 0;

    int from = 0;
    for ( ; from < sizeof(HashCode); ++from) {
        int to = from + adjust;
        if (to < sizeof(HashCode)) {
            output.map[to] = input.map[from];
        } else {
            to -= sizeof(HashCode);
            output.map[to] = input.map[from];
        }
    }

    return output.hash;
}

static inline HashCode hash_full(CuData value) {
    HashCode result = 5381;

    unsigned int length = value.length;
    const char*  begin  = value.start;

    for ( ; length-- ; ) {
        int val = begin[length];
        result = ((result << 5) + result) + val;
    }

    return result;
}

extern bool text_Create(CuData value, Text *target) {
    const unsigned     size = value.length;
    const HashCode hashcode = hash_full(value);
    const int         cells = size / sizeof(const unsigned long);

    if (!node_Allocate(_zero_space,
                       true,
                       asSize(sizeof(struct text), sizeof(unsigned long) * cells),
                       0,
                       target))
        return false;

    Text result = *target;

    result->size     = size;
    result->hashcode = hashcode;

    memcpy(result->value, value.start, size);

    return result;
}

extern bool text_Make(const char* text, Text *target) {
    const unsigned size = (text ? strlen(text) : 0);
    CuData        value = { size, text };
    return text_Create(value, target);
}

extern bool text_Append(Text head, Text tail, Text *target) {
    if (!head) {
        if (tail) {
            *target = tail;
            return true;
        }
        return text_Make((const char*) 0, target);
    }

    if (!tail) {
        *target = head;
        return true;
    }

    unsigned int       size = head->size + tail->size;
    const HashCode hashcode = head->hashcode ^ hash_ajust(head->size, tail->hashcode);
    const int         cells = size / sizeof(const unsigned long);

    if (!node_Allocate(_zero_space,
                       true,
                       asSize(sizeof(struct text), sizeof(unsigned long) * cells),
                       0,
                       target)) return false;

    Text result = *target;

    result->size     = size;
    result->hashcode = hashcode;

    char* at = (char*)result->value;

    memcpy(at, head->value, head->size);
    memcpy(at + head->size, tail->value, tail->size);

    return result;
}

/*****************
 ** end of file **
 *****************/

