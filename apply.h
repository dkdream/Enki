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
extern bool apply(Node fun, Node args, Node env, Target result);
extern bool expand(Node expr, Node env, Target result);
extern bool encode(Node expr, Node env, Target result);
extern bool eval(Node expr, Node env, Target result);

/***************************
 ** end of file
 **************************/
#endif
