/*-*- mode: c;-*-*/
#if !defined(_ea_treadmill_h_)
#define _ea_treadmill_h_
/***************************
 **
 ** GC_Treadmill
 **
 ** Purpose
 **   Garbage Collection
 **
 ** Coding Invariant:
 **
 ** All gc_treadmill are in static sections (not allocated)
 **
 ** Currently there is only two treadmill
 **
 ** The the_zero_space treadmil for generation zero
 **
 ** The empty treadmill ((Space)0) for generation omega
 **
 ** All node allocated from the empty treadmill can never be garbage
 **
 ** ----------------------------------------------------------------
 **
 ** Each treadmill is assigned a positive integer starting with zero
 **
 ** No two treadmil are assigned the same integer
 **
 ** The empty treadmill is assigned the positive infinity
 **
 ** The number assigned to a treadmill is called the node generation
 **
 ** Each node in a treadmill can refer to node in the in the same
 ** generation or greater without reservation
 **
 ** It a node in a treadmill has a child node in a lower generation
 ** then the child node must be put into the reservation table of
 ** child node's treadmill along with the parent node treadmill
 **
 ** It a node in a treadmill has a child node in a lower generation
 ** then the child node must be put into the seen table of the parent
 ** node treadmill and marked as seen.
 **
 ** At the start of each cycle (space_Flip) the seen table is scaned,
 ** if a node was not seen then it is remove from the seen table and
 ** from the child nodes reservation (for treadmills generation) otherwise
 ** the child node is marked unseen.
 **
 ** a treadmill can ONLY call space_Flip if the next lower generation
 ** has fliped at leasted once.
 **
 ***************************
 **
 ** Clinks
 **
 ** Purpose
 **   -- theses are used by combiners to
 **      hold data acrossed calls to next
 **
 **  As part of the roots links processing
 **  there is a doublly-links list of Clinks
 **  this is processed after the root set for
 **  space zero.
 **
 **  Use: a c-function declares a clink then
 **       calls clint_Init then before it calls
 **       'return next' it must calls clink_Drop
 **       so the roots list dosent refer to
 **       a c-stack variable in a frame that
 **       is gone.
 **
 **/
#include "reference.h"
#include <stdlib.h>
#include <string.h>

struct gc_clink {
    struct gc_clink *before;
    struct gc_clink *after;
    unsigned         size;
    Reference       *array;
};

enum gc_color {
    nc_unknown,
    nc_blue,
    nc_orange,
};

typedef struct gc_clink      Clink;
typedef enum   gc_color      Color;
typedef struct gc_header    *Header;
typedef struct gc_treadmill *Space;

#define BITS_PER_WORD (sizeof(long) * 8)
#define WORD_SIZE     (sizeof(long))
#define POINTER_SIZE  (sizeof(void *))

struct gc_header {
    unsigned long count : BITS_PER_WORD - 12 __attribute__((__packed__));
    union {
        unsigned int flags : 12;
        struct {
            enum gc_color color  : 2;
            unsigned int  atom   : 1; // is this a tuple of values
            unsigned int  inside : 1; // is this inside a space (managed)
            unsigned int  kind   : 8; // is this a kind of c-object (symbol,string,...)
        } __attribute__((__packed__));
    } __attribute__((__packed__));

    struct gc_treadmill *space;
    struct gc_header    *before;
    struct gc_header    *after;
};

struct gc_treadmill {
    enum   gc_color   visiable;
    struct gc_header *free;   // start of the free    list
    struct gc_header *bottom; // start of the hidden  list
    struct gc_header *top;    // end   of the hidden  list
    struct gc_header *scan;   // end   of the resting list
    /**/
    struct gc_header  start_up;
    struct gc_header  start_down;
    struct gc_header  start_root;
    struct gc_clink   start_clinks;
};


extern bool space_Init(const Space); // this will erase the contents of the space
extern bool space_Flip(const Space);
extern bool space_Scan(const Space, unsigned int);

#if 0 // roots are added/removed using Clink(s)
extern bool space_AddRoot(const Space, Node);
extern bool space_RemoveRoot(const Space, Node);
#endif

extern bool clink_Init(Clink *link, unsigned size, Reference* array);
extern bool clink_Drop(Clink *link);

extern bool node_ExternalInit(const EA_Type, const Header);
extern bool node_Allocate(Space space, EA_Type type, Size extend, Target);
extern bool node_Darken(const Reference node);

extern inline Reference asReference(Header header) __attribute__((always_inline));
extern inline Reference asReference(Header header) {
    if (!header) return (Reference)0;
    return (Reference)(header + 1);
}

extern inline Header asHeader(Reference value) __attribute__((always_inline));
extern inline Header asHeader(Reference value) {
    if (!value) return (Header)0;
    return (((Header)value) - 1);
}

extern inline unsigned long toCount(unsigned long size_in_chars) __attribute__((always_inline));
extern inline unsigned long toCount(unsigned long size_in_chars) {
    return size_in_chars / POINTER_SIZE;
}

extern inline unsigned long toSize(unsigned long size_in_pointers) __attribute__((always_inline));
extern inline unsigned long toSize(unsigned long size_in_pointers) {
    return size_in_pointers * POINTER_SIZE;
}

extern inline Header fresh_atom(unsigned long size_in_chars) __attribute__((always_inline));
extern inline Header fresh_atom(unsigned long size_in_chars) {
    unsigned long fullcount = size_in_chars;
    fullcount += (POINTER_SIZE - 1);
    fullcount /= POINTER_SIZE;

    unsigned long fullsize = (fullcount * POINTER_SIZE);
    fullsize += sizeof(struct gc_header);

    Header header = (Header) malloc(fullsize);

    memset(header, 0, fullsize);

    header->count  = fullcount;
    header->color  = nc_unknown;
    header->atom   = 1;
    header->inside = 0;
    header->kind   = 0;
    header->space  = 0;
    header->before = header->after = 0;

    return asReference(header);
}

extern inline Header fresh_tuple(unsigned long size_in_pointers) __attribute__((always_inline));
extern inline Header fresh_tuple(unsigned long size_in_pointers) {
    unsigned long fullcount = size_in_pointers;
    unsigned long fullsize  = (fullcount * POINTER_SIZE);
    fullsize += sizeof(struct gc_header);

    Header header = (Header) malloc(fullsize);

    memset(header, 0, fullsize);

    header->count  = fullcount;
    header->color  = nc_unknown;
    header->atom   = 0;
    header->inside = 0;
    header->kind   = 0;
    header->space  = 0;
    header->before = header->after = 0;

    return header;
}

extern inline bool clink_Assign(Clink *link, unsigned index, Reference value) __attribute__((always_inline));
extern inline bool clink_Assign(Clink *link, unsigned index, Reference value) {
    if (!link)                return false;
    if (!link->array)         return false;
    if (!link->before)        return false;
    if (!link->after)         return false;
    if (link->size <= index)  return false;
    if (!node_Darken(value))  return false;
    link->array[index] = value;
    return true;
}

/* macros */

/***************************
 ** end of file
 **************************/
#endif
