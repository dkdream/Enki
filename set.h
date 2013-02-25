/*-*- mode: c;-*-*/
#if !defined(_ea_set_h_)
#define _ea_set_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"

#include <stdbool.h>
#include <stdio.h>

struct set_cell {
    Set_cell rest;
    Node     first;
};


/* this is only used inside of a set */
struct set_block {
    unsigned int size;
    Set_block next; // next block
    Set_cell  list[];
};

struct set {
    enum node_type type;
    unsigned int   count;    // number of entries
    unsigned int   fullsize; // = sum(block[0..n].size)
    Set_block      first;
};

extern bool set_Contains(Set, Node);
extern bool set_Add(Set, Node);
extern bool set_Remove(Set, Node);
extern bool set_Clone(Set, Set*);
extern bool set_Union(Set target, Set);
extern bool set_Difference(Set target, Set);
extern bool set_Intersection(Set target, Set);
extern bool set_Create(unsigned, enum node_type, Set*);

/* */
extern void set_Print(FILE*, Set);

/* marcos */


/***************************
 ** end of file
 **************************/
#endif
