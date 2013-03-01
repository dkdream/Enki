/*-*- mode: c;-*-*/
#if !defined(_apply_h_)
#define _apply_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

extern bool apply(Node fun, Node args, Node env, Target result);
extern bool expand(Node expr, Node env, Target result);
static bool encode(Node expr, Node env, Target result);
static bool eval(Node expr, Node env, Target result);

/***************************
 ** end of file
 **************************/
#endif
