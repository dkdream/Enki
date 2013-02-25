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
    struct set_cell *rest;
    Node first;
};


/* this is only used inside of a set */
struct set_block {
    // internals of block
    struct set_block  *next; // next block
    unsigned int       size;
    Set_cell           list[];
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

// used to create root set
extern bool set_CreateIn(struct gc_treadmill *Space, unsigned, enum node_type, Set*);

/* */
extern void set_Print(FILE*, Set);

/* marcos */
extern inline bool set_Create(unsigned, enum node_type, Set*) __attribute__((always_inline));
extern inline bool set_Create(unsigned size, enum node_type type, Set *target) {
    return set_CreateIn(_zero_space, size, type, target);
}

/***************************
 ** end of file
 **************************/
#endif
