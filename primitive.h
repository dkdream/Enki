/*-*- mode: c;-*-*/
#if !defined(_ea_primitive_h_)
#define _ea_primitive_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

typedef enum fixed_slots {
    fxd_name,
    fxd_eval,
    fxd_encode,
} FXD_Slots;

typedef void (*Operator)(Node args, Node env, Target result);

/* List/Pair operations */
typedef bool (*Folder)(Node left, Node right, Node env, Target result);
typedef bool (*Predicate)(Node value, Node env);

/* Tuple operations */
typedef bool (*Flexor)(unsigned index, Node left, Node right, Node env, Target result);
typedef bool (*Selector)(unsigned index, Node value, Node env);

struct primitive {
    Symbol   label;
    Operator function;
};

extern Primitive p_eval_symbol;
extern Primitive p_eval_pair;
extern Primitive p_eval_tuple;
extern Primitive p_apply_lambda;
extern Primitive p_apply_form;

extern bool primitive_Create(Symbol label, Operator function, Primitive*);

/* macros */


/***************************
 ** end of file
 **************************/
#endif
