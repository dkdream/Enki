/*-*- mode: c;-*-*/
#if !defined(_ea_tuple_h_)
#define _ea_tuple_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include "primitive.h"

struct tuple {
    Node item[1];
};

extern bool tuple_Create(unsigned size, Tuple* target);
extern bool tuple_Convert(Pair pair, Tuple* target);
extern bool tuple_SetItem(Tuple tuple, unsigned index, const Node value);
extern bool tuple_GetItem(Tuple tuple, unsigned index, Target value);
extern bool tuple_Fill(Tuple tuple, Pair list);
extern bool tuple_Map(Operator func, Tuple tuple, const Node env, Target target);

/******************
  inline functions
 ******************/

/***************************
 ** end of file
 **************************/
#endif
