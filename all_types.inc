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
/* */
#include "apply.h"
#include "reader.h"
#include "dump.h"
#include "node.h"
/* */
#ifdef USE_HASH
#include "hash.h"
#endif

#ifdef USE_SET
#include "set.h"
#endif
/* */
/***************************
 ** end of file
 **************************/
#endif
