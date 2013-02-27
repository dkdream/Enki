/*-*- mode: c;-*-*/
#if !defined(_ea_dump_h_)
#define _ea_dump_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include <stdio.h>

extern bool print(FILE *fp, Node result);
extern bool dump(FILE *fp, unsigned indent, Node result);
extern void prettyPrint(FILE *fp, Node result);

/***************************
 ** end of file
 **************************/
#endif
