/*-*- mode: c;-*-*/
#if !defined(_ea_primitive_h_)
#define _ea_primitive_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

typedef ResultCode (*Operator)(Node args, Node env, Target result);

struct primitive {
    Symbol       label;
    Operator     function;
    unsigned int size;
    unsigned int buffer[];
};

extern bool primitive_Create(Symbol label, Operator function, unsigned int extend, Primitive*);

/* macros */


/***************************
 ** end of file
 **************************/
#endif
