/*-*- mode: c;-*-*/
#if !defined(_ea_count_h_)
#define _ea_count_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

struct count {
    unsigned int value;
};
extern bool count_Create(unsigned int value, Count*);
extern bool count_Add(Count, Count, Count*);

/***************************
 ** end of file
 **************************/
#endif
