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
#include "bit_array.h"
#include "symbol.h"

struct pair {
    Node car;
    Node cdr;
};

struct variable {
    Symbol   label;
    Node     value;
    Constant type;
};

extern bool pair_Create(const Node car, const Node cdr, Target target);
extern bool pair_SetCar(Pair pair, const Node car);
extern bool pair_SetCdr(Pair pair, const Node cdr);
extern bool pair_GetCar(Pair pair, Target car);
extern bool pair_GetCdr(Pair pair, Target cdr);

// construct a list
extern bool list_Make(Pair* target, ...);

// when using a pair as a list (proper when dotted != true)
extern bool list_State(Pair pair, unsigned *count, bool *dotted);

// convert a generic list into a proper list
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

// convert a tuple into a (proper) list
extern bool list_Convert(Tuple tuple, Pair* target);

// create a variable entry
extern bool variable_Create(const Symbol label, const Node value, const Constant type, Variable* target);

// find the entry=(label,value,type) for label
extern bool alist_Entry(Pair pair, const Symbol label, Target entry);

// prepend the entry(label,value,type) to list
extern bool alist_Add(Pair pair, const Symbol label, const Node value, const Constant type, Target target);

//  create a new list then map input to output by func
extern bool list_Map(Pair pair, const Operator func, const Node env, Target target);

// create a new list from the selected parts of an old list
extern bool list_Filter(Pair pair, const Predicate func, const Node env, Target target);

// return the left fold of a list using the folder
extern bool list_FoldLeft(Pair pair, const Node init, const Folder func, const Node env, Target target);

// return a new (proper) list that is the reverse of an old (proper) list
extern bool list_Reverse(Pair pair, Target target);

// fill the bit array based on what elements the predicate selected from the (proper) list
extern bool list_Find(Pair pair, const Predicate func, const Node env, BitArray *array);

// create a new list from elements of an old (proper) list
extern bool list_Select(Pair pair, BitArray *array, Target target);

// update select parts of the current (proper) list (update all when array==null)
extern bool list_Update(Pair pair, const Operator func, const Node env, BitArray *array);

// convert a tuple into a new (proper) list
extern bool list_Curry(Tuple typle, const Operator func, const Node env, BitArray *array, Pair* target);

// split
// when predicate(car(cdr(pair)))
//    target = cdr(pair)
//    setcdr(pair,null)
//    exit t
// exit f

// split the list in two at the first tail with a head that is selected by predicate
extern bool list_SplitFirst(Pair pair, const Predicate func, const Node env, Target target);

// split the list in two at the last tail with a head that is selected by predicate
extern bool list_SplitLast(Pair pair, const Predicate func, const Node env, Target target);

// forwarded from treadmill.h
extern bool darken_Node(const Node node);

/******************
  inline functions
 ******************/

// find the value for label
static inline bool alist_Get(Pair pair, const Symbol label, Target value)  __attribute__((always_inline, nonnull));
static inline bool alist_Get(Pair pair, const Symbol label, Target value) {
    Variable variable;

    if (!alist_Entry(pair, label, &variable)) return false;

    ASSIGN(value, variable->value);

    return true;
}

// set the value for label
static inline bool alist_Set(Pair pair, const Symbol label, const Node value) __attribute__((always_inline, nonnull));
static inline bool alist_Set(Pair pair, const Symbol label, const Node value) {
    Variable variable;

    if (!alist_Entry(pair, label, &variable)) return false;

    darken_Node(value);

    variable->value = value;

    return true;
}

// prepend the entry(label,void,type) to list
static inline bool alist_Declare(Pair pair, const Symbol label, const Node type, Target target) __attribute__((always_inline, nonnull));
static inline bool alist_Declare(Pair pair, const Symbol label, const Constant type, Target target) {
    return alist_Add(pair, label, void_v, type, target);
}


/***************************
 ** end of file
 **************************/
#endif
