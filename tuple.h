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

extern bool tuple_Create(unsigned size, Target target);
extern bool tuple_SetItem(Tuple tuple, unsigned index, const Node value);
extern bool tuple_GetItem(Tuple tuple, unsigned index, Target value);

// construct a tuple
extern bool tuple_Make(Tuple* target, const unsigned size, ...);

// fill a tuple from a (proper) list
extern bool tuple_Fill(Tuple tuple, Pair list);

// convert a (proper) list to a new tuple
extern bool tuple_Convert(Pair pair, Target target);

// create an new tuple then map input to output by func
extern bool tuple_Map(Tuple tuple, const Operator func, const Node env, Target target);

// create a new tuple from the selected parts of an old tuple
extern bool tuple_Filter(Tuple tuple, const Selector func, const Node env, Target target);

// create a new tuple from a section of an old tuple
extern bool tuple_Section(Tuple tuple, unsigned start, unsigned end, Target target);

// return the left fold of a tuple using the flexor
extern bool tuple_FoldLeft(Tuple tuple, const Node init, const Flexor func, const Node env, Target target);

// return the left fold of a tuple using the flexor
extern bool tuple_FoldRight(Tuple tuple, const Node init, const Flexor func, const Node env, Target target);

// create a new tuple reverse fill it from the input tuple
extern bool tuple_Reverse(Tuple input, Tuple* target);

// fill the bit array based on what elements the selector selected from the tuple
extern bool tuple_Find(Tuple tuple, const Selector func, const Node env, BitArray *array);

// create a new tuple from elements of an old tuple
extern bool tuple_Select(Tuple tuple, unsigned count, BitArray *array, Target target);

// update select parts of the current tuple (update all when array==null)
extern bool tuple_Update(Tuple tuple, const Operator func, const Node env, BitArray *array);

/******************
  inline functions
 ******************/

/***************************
 ** end of file
 **************************/
#endif
