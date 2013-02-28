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

extern bool pair_Create(const Node car, const Node cdr, Pair* target);
extern bool pair_SetCar(Pair pair, const Node car);
extern bool pair_SetCdr(Pair pair, const Node cdr);

// when using a pair as a list
extern bool list_State(Pair pair, unsigned *count, bool *dotted);
extern bool list_UnDot(Pair pair);
extern bool list_SetItem(Pair pair, unsigned index, const Node value);
extern bool list_GetItem(Pair pair, unsigned index, Target value);

// replace the nil/value at the end
extern bool list_SetEnd(Pair pair, const Node value);
extern bool list_GetEnd(Pair pair, Target value);

/***************************
 ** end of file
 **************************/
#endif
