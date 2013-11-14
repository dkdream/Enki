/*-*- mode: c;-*-*/
#if !defined(_ea_primitive_h_)
#define _ea_primitive_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

typedef void (*Operator)(Node args, Node env, Target result);
typedef bool (*Analyser)(Node args, Node env, Target result);

/* List/Pair operations */
typedef bool (*Folder)(Node left, Node right, Node env, Target result);
typedef bool (*Predicate)(Node value, Node env);

/* Tuple operations */
typedef bool (*Flexor)(unsigned index, Node left, Node right, Node env, Target result);
typedef bool (*Selector)(unsigned index, Node value, Node env);

struct atomic {
    Symbol   label;
    Operator evaluator;
};

struct primitive {
    Symbol   label;
    Operator evaluator;
    Analyser analyser;
};

struct composite {
    Symbol   label;
    Operator evaluator;
    Analyser analyser;
    Operator encoder;
};

extern Primitive p_eval_symbol;
extern Primitive p_eval_pair;
extern Primitive p_eval_tuple;
extern Primitive p_apply_lambda;
extern Primitive p_apply_form;

extern bool atomic_Create(Symbol label, Operator evaluator, Atomic*);
extern bool primitive_Create(Symbol label, Operator evaluator, Analyser analyser, Primitive*);
extern bool composite_Create(Symbol label, Operator evaluator, Analyser analyser, Operator encoder, Composite*);

/* macros */


/***************************
 ** end of file
 **************************/
#endif
