/*-*- mode: c;-*-*/
#if !defined(_apply_h_)
#define _apply_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

extern void expand(const Node expr, const Node env, Target result);
extern void encode(const Node expr, const Node env, Target result);
extern void apply(Node fun, Node args, const Node env, Target result);
extern void eval(const Node expr, const Node env, Target result);
extern void eval_begin(Node body, Node env, Target last);
extern void eval_block(Symbol escape, Node body, Node env, Target last);

extern void dump_enki_stack();
extern void pushTrace(const Node context);
extern void popTrace();

/***************************
 ** end of file
 **************************/
#endif
