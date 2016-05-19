/*-*- mode: c;-*-*/
#if !defined(_all_types_inc_)
#define _all_types_inc_
/***************************
 **
 ** Purpose
 **   this include all node types to may them easier to use
 **/

#include "integer.h"
#include "pair.h"
#include "primitive.h"
#include "symbol.h"
#include "text.h"
#include "tuple.h"
#include "type.h"
/* */
#include "apply.h"
#include "reader.h"
#include "dump.h"
#include "node.h"
/* */
extern void forceArg(Node arg, Target result);
extern unsigned checkArgs(Node args, const char* name, unsigned min, ...);
extern int forceArgs(Node args, ...);
extern void bindValue(Pair env, Symbol symbol, const Node value, Pair* result);
/* */
/***************************
 ** end of file
 **************************/
#endif
