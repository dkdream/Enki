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

extern Primitive    _enclose;
extern Primitive    _apply;
extern Primitive    _closure;
extern Primitive    _hash;
extern Primitive    _set;
extern Primitive    _entry;
extern Primitive    _context;
extern Primitive    _one_or_more;
extern Primitive    _add;
extern Primitive    _depth;
extern Primitive    _contract;
extern Primitive    _drop;

extern bool primitive_Create(Symbol label, Operator function, unsigned int extend, Primitive*);

/* macros */


/***************************
 ** end of file
 **************************/
#endif
