/*-*- mode: c;-*-*/
#if !defined(_ea_hashcode_h_)
#define _ea_hashcode_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "text_buffer.h"

typedef unsigned long long HashCode;


static inline HashCode hash_full(TextBuffer value) __attribute__((always_inline));
static inline HashCode hash_full(TextBuffer value) {
    HashCode result = 5381;

    unsigned int length = value.position;
    const char*  begin  = value.buffer;

    for ( ; length-- ; ) {
        int val = begin[length];
        result = ((result << 5) + result) + val;
    }

    return result;
}


/* hash_merge: commutative, associative */
/* Forall a, b    where a:HashCode, b:HashCode            then hash_merge(a,b) = hash_merge(b,a) */
/* Forall a, b, c where a:HashCode, b:HashCode c:HashCode then hash_merge(hash_merge(a,b),c) = hash_merge(a,hash_merge(b,c)) */
static inline HashCode hash_merge(HashCode left, HashCode right) __attribute__((always_inline));
static inline HashCode hash_merge(HashCode left, HashCode right) {
    return (left ^ right);
}

/***************************
 ** end of file
 **************************/
#endif
