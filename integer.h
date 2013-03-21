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
    long long value;
};
extern bool integer_Create(long long value, Integer*);

/***************************
 ** end of file
 **************************/
#endif
