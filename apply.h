/*-*- mode: c;-*-*/
#if !defined(_apply_h_)
#define _apply_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

extern void fatal(const char *reason, ...) __attribute__ ((noreturn format (printf, 1, 2)));

extern bool expand(const Node expr, const Node env, Target result);
extern bool encode(const Node expr, const Node env, Target result);
extern bool apply(Node fun, Node args, const Node env, Target result);
extern bool eval(const Node expr, const Node env, Target result);

extern void pushTrace(const Node context);
extern void popTrace();

/***************************
 ** end of file
 **************************/
#endif
