/*-*- mode: c;-*-*/
#if !defined(_ea_pair_h_)
#define _ea_pair_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

struct pair {
    Node car;
    Node cdr;
};

extern inline bool pair_Create(const Node car, const Node cdr, Pair* target)  __attribute__((always_inline));
extern inline bool pair_Create(const Node car, const Node cdr, Pair* target) {
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct pair),
                       0,
                       target))
        return false;

    Pair result = (*target);

    result->car = car;
    result->cdr = cdr;

    return true;
}

/***************************
 ** end of file
 **************************/
#endif
