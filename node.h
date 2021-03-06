/*-*- mode: c;-*-*/
#if !defined(_ea_node_h_)
#define _ea_node_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include "hashcode.h"

extern HashCode node_HashCode(Node node);
extern bool     node_Match(Node left, Node right);
extern bool     node_Iso(long depth, Node left, Node right);
extern void     node_TypeOf(Node value, Target result);

/* marcos */

/***************************
 ** end of file
 **************************/
#endif
