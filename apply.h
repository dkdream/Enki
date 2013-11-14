/*-*- mode: c;-*-*/
#if !defined(_apply_h_)
#define _apply_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

extern void expand(const Node expr, const Node env, Target result) HOT;
extern void encode(const Node expr, const Node env, Target result) HOT;
extern void analysis(const Node expr, const Node env, Target result) HOT; // add type annotation
extern void eval(const Node expr, const Node env, Target result) HOT;
extern void apply(Node fun, Node args, const Node env, Target result) HOT;

// infer type of expr (if possible)
extern bool analysis_infer(const Node expr, const Node env, Constant *type);
// check type of expr (if possible)
extern bool analysis_check(const Node expr, const Node env, const Constant type);

extern void eval_begin(Node body, Node env, Target last) HOT;
extern void eval_block(const Symbol escape, const Constant type, Node body, Node env, Target last) HOT;
extern void eval_escape(Node node, Node result) __attribute__ ((noreturn));

extern void dump_enki_stack();
extern void pushTrace(const Node context);
extern void popTrace();

/***************************
 ** end of file
 **************************/
#endif
