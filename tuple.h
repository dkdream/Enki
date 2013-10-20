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
#include "bit_array.h"

struct tuple {
    Node item[1];
};

extern bool tuple_Create(unsigned size, Tuple* target);
extern bool tuple_Convert(Pair pair, Tuple* target);
extern bool tuple_SetItem(Tuple tuple, unsigned index, const Node value);
extern bool tuple_GetItem(Tuple tuple, unsigned index, Target value);
extern bool tuple_Fill(Tuple tuple, Pair list);

extern bool tuple_Map(Operator func, Tuple tuple, const Node env, Target target);
extern bool tuple_Filter(Selector func, Tuple tuple, const Node env, Target target);
extern bool tuple_FoldLeft(Flexor func, Pair pair, const Node init, const Node env, Target target);
extern bool tuple_FoldRight(Flexor func, Pair pair, const Node init, const Node env, Target target);
extern bool tuple_Reverse(Pair pair, Pair* target);
extern bool tuple_Find(Selector func, Pair pair, const Node env, Target target);
extern bool tuple_Section(Tuple tuple, unsigned start, unsigned end, Tuple* target);
extern bool tuple_Select(Tuple tuple, unsigned count, BitArray *array, Tuple* target);

/******************
  inline functions
 ******************/

/***************************
 ** end of file
 **************************/
#endif
