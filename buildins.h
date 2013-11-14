/*-*- mode: c;-*-*/
#if !defined(_ea_buildins_h_)
#define _ea_buildins_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

#define SUBR(NAME) void opr_##NAME(Node args, Node env, Target result)
#define APPLY(NAME,ARGS,ENV,RESULT) opr_##NAME(ARGS,ENV,RESULT)

#include "buildins/SUBR.lst"

/***************************
 ** end of file
 **************************/
#endif
