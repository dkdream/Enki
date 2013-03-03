/*-*- mode: c;-*-*/
#if !defined(_ea_reader_h_)
#define _ea_reader_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include <stdio.h>

extern bool readExpr(FILE *fp, Target result);
extern void readFile(FILE *stream);

/***************************
 ** end of file
 **************************/
#endif
