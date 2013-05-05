/*-*- mode: c;-*-*/
#if !defined(_ea_dump_h_)
#define _ea_dump_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include "text_buffer.h"
#include <stdio.h>

extern bool print(FILE *fp, Node result);
extern bool dump(FILE *fp, Node result);
extern void prettyPrint(FILE *fp, Node result);

extern bool buffer_print(TextBuffer *tbuf, Node result);
extern bool buffer_dump(TextBuffer *tbuf, Node result);
extern bool buffer_dumpTree(TextBuffer *tbuf, unsigned indent, Node result);
extern void buffer_prettyPrint(TextBuffer *tbuf, Node result);

/***************************
 ** end of file
 **************************/
#endif
