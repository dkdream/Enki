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

struct primitive {
    Symbol   label;
    Operator function;
};

extern Node      f_quote;       // quote a syntax tree
extern Node      f_let;         // extend/overide enviroment
extern Node      f_lambda;      //
extern Primitive p_eval_symbol;
extern Primitive p_eval_pair;
extern Primitive p_apply_expr;
extern Primitive p_apply_form;

extern bool primitive_Create(Symbol label, Operator function, Primitive*);

/* macros */


/***************************
 ** end of file
 **************************/
#endif
