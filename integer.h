/*-*- mode: c;-*-*/
#if !defined(_ea_integer_h_)
#define _ea_integer_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

struct integer {
    long value;
};
extern bool integer_Create(long value, Integer*);
extern bool integer_Add(Integer, Integer, Integer*);
extern bool integer_Mult(Integer, Integer, Integer*);
extern bool integer_Subtract(Integer, Integer, Integer*);

/***************************
 ** end of file
 **************************/
#endif
