/*-*- mode: c;-*-*/
#if !defined(_ea_pair_h_)
#define _ea_pair_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include "primitive.h"

struct pair {
    Node car;
    Node cdr;
};

typedef bool (*Folder)(Node left, Node right, Node env, Target result);
typedef bool (*Predicate)(Node value, Node env);

extern bool pair_Create(const Node car, const Node cdr, Pair* target);
extern bool pair_SetCar(Pair pair, const Node car);
extern bool pair_SetCdr(Pair pair, const Node cdr);
extern bool pair_GetCar(Pair pair, Target car);
extern bool pair_GetCdr(Pair pair, Target cdr);

// when using a pair as a list
extern bool list_State(Pair pair, unsigned *count, bool *dotted);
extern bool list_UnDot(Pair pair);

// index=0 -> car
// index=1 -> cadr
extern bool list_SetItem(Pair pair, unsigned index, const Node value);
extern bool list_GetItem(Pair pair, unsigned index, Target value);

// index=0 -> cdr
// index=1 -> cddr
extern bool list_SetTail(Pair pair, unsigned index, const Node value);
extern bool list_GetTail(Pair pair, unsigned index, Target value);

// replace the nil/value at the end
extern bool list_SetEnd(Pair pair, const Node value);
extern bool list_GetEnd(Pair pair, Target value);

//extern bool list_SplitFirst(Pair pair, const Node value, Target target);
//extern bool list_SplitLast(Pair pair, const Node value, Target target);

// find the entry=(label,value) for label
extern bool alist_Entry(Pair pair, const Node label, Pair* entry);
// find the value for label
extern bool alist_Get(Pair pair, const Node label, Target entry);
// set the value for label
extern bool alist_Set(Pair pair, const Node label, const Node value);
// prepend the entry(label,value) to list
extern bool alist_Add(Pair pair, const Node label, const Node value, Pair* target);

//
extern bool list_Map(Operator func, Pair pair, const Node env, Target target);
extern bool list_FoldLeft(Folder func, Pair pair, const Node init, const Node env, Target target);
extern bool list_Reverse(Pair pair, Pair* target);
extern bool list_Find(Predicate func, Pair pair, const Node env, Target target);

// split
// when predicate(car(cdr(pair)))
//    target = cdr(pair)
//    setcdr(pair,null)
//    exit t
// exit f
extern bool list_SplitFirst(Predicate func, Pair pair, const Node env, Pair* target);
extern bool list_SplitLast(Predicate func, Pair pair, const Node env, Pair* target);

/******************
  inline functions
 ******************/

/***************************
 ** end of file
 **************************/
#endif
