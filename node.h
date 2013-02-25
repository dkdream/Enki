/*-*- mode: c;-*-*/
#if !defined(_ea_node_h_)
#define _ea_node_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

#include <stdbool.h>
#include <stdio.h>

extern HashCode    node_HashCode(Node node);
extern bool        node_Match(Node left, Node right);
extern const char* node_type_Name(enum node_type type);
extern void        node_Print(FILE* output, Node node);
extern void        node_PrintFul(FILE* output, Node node);
extern void        node_PrintTree(FILE* output, unsigned level, Node node);

/* marcos */

/***************************
 ** end of file
 **************************/
#endif
