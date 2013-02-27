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

extern bool pair_SetCar(Pair, const Node car);
extern bool pair_SetCdr(Pair, const Node cdr);
extern bool pair_Create(const Node car, const Node cdr, Pair* target);

/***************************
 ** end of file
 **************************/
#endif
