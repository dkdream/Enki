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

typedef struct set       *Set;
typedef struct set_state *Set_state;
typedef struct set_block *Set_block;
typedef struct set_cell  *Set_cell;

struct set_cell {
    Set_cell rest;
    Node     first;
};

/* this is only used inside of a set */
struct set_block {
    Set_block next; // next block
    Set_cell  list[];
};

struct set_state {
    unsigned int count;    // number of entries
    unsigned int fullsize; // = sum(block[0..n].size)
};

struct set {
    Set_state state;
    Set_block first;
};

extern bool set_Create(unsigned, Set*);
extern bool set_Contains(Set, Node);
extern bool set_Add(Set, Node);
extern bool set_Remove(Set, Node);
extern bool set_Clone(Set, Set*);
extern bool set_Union(Set target, Set);
extern bool set_Difference(Set target, Set);
extern bool set_Intersection(Set target, Set);

/* */
extern void set_Print(FILE*, Set);

/* marcos */


/***************************
 ** end of file
 **************************/
#endif
