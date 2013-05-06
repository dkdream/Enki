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

extern bool print(FILE *fp, Node value);
extern bool dump(FILE *fp, Node value);
extern void prettyPrint(FILE *fp, Node value);

extern bool buffer_print(TextBuffer *tbuf, Node value);
extern bool buffer_dump(TextBuffer *tbuf, Node value);
extern bool buffer_dumpTree(TextBuffer *tbuf, unsigned indent, Node value);
extern void buffer_prettyPrint(TextBuffer *tbuf, Node value);

/***************************
 ** end of file
 **************************/
#endif
