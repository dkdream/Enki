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
#define POINTER_SIZE  (sizeof(Node))

struct gc_header {
    union {
        unsigned long state __attribute__((__packed__));
        unsigned long count : BITS_PER_WORD - 12 __attribute__((__packed__));
        union {
            unsigned int flags : 12 __attribute__((__packed__));
            struct {
                enum gc_color color  : 2;
                unsigned int  atom   : 1; // is this a tuple of values
                unsigned int  inside : 1; // is this inside a space (malloc/free by the treadmill)
                unsigned int  prefix : 1; // the first slot is a pointer an atomic segment. (implies !atom)
                unsigned int  kind   : 6; // the kind (used by c-code) (and prettyprint)
            } __attribute__((__packed__));
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

extern bool clink_Init(Clink *link, unsigned size, Reference* array);
extern bool clink_Drop(Clink *link);

//extern bool node_ExternalInit(const EA_Type, struct gc_header *);

extern bool node_Allocate(struct gc_treadmill *space,
                          bool atom,
                          Size size_in_char,
                          Size prefix_in_char,
                          Target);

extern bool insert_After(const Header mark, const Header node);
extern bool insert_Before(const Header mark, const Header node);
extern bool extract_From(const Header mark);

extern inline Reference asReference(Header header) __attribute__((always_inline));
extern inline Reference asReference(Header header) {
    if (!header) return (Reference)0;
    return (Reference)(header + 1);
}

extern inline Header asHeader(const Node value) __attribute__((always_inline));
extern inline Header asHeader(const Node value) {
    if (!value.reference) return (Header)0;
    return (((Header)value.reference) - 1);
}

extern inline EA_Type getKind(const Node value) __attribute__((always_inline));
extern inline EA_Type getKind(const Node value) {
    if (!value.reference) return nt_unknown;
    Header header = asHeader(value);
    return header->kind;
}

extern inline bool setKind(const Node value, EA_Type kind) __attribute__((always_inline));
extern inline bool setKind(const Node value, EA_Type kind) {
    if (!value.reference) return false;
    Header header = asHeader(value);
    header->kind = kind;
    return true;
}

extern inline unsigned long asSize(unsigned base_sizeof, unsigned extend_sizeof)  __attribute__((always_inline));
extern inline unsigned long asSize(unsigned base_sizeof, unsigned extend_sizeof) {
    return base_sizeof + extend_sizeof;
}

extern inline unsigned long toCount(unsigned long size_in_chars) __attribute__((always_inline));
extern inline unsigned long toCount(unsigned long size_in_chars) {
    unsigned long fullcount = size_in_chars;
    fullcount += (POINTER_SIZE - 1);
    fullcount /= POINTER_SIZE;
    return fullcount;
}

extern inline unsigned long toSize(unsigned long size_in_pointers) __attribute__((always_inline));
extern inline unsigned long toSize(unsigned long size_in_pointers) {
    return size_in_pointers * POINTER_SIZE;
}

extern inline Header fresh_atom(unsigned long size_in_chars) __attribute__((always_inline));
extern inline Header fresh_atom(unsigned long size_in_chars) {
    unsigned long fullcount = toCount(size_in_chars);
    unsigned long fullsize  = toSize(fullcount);
    fullsize += sizeof(struct gc_header);

    Header header = (Header) malloc(fullsize);

    memset(header, 0, fullsize);

    header->count  = fullcount;
    header->color  = nc_unknown;
    header->atom   = 1;
    header->inside = 0;
    header->prefix = 0;
    header->space  = 0;
    header->before = header->after = 0;

    return header;
}

extern inline Header fresh_tuple(unsigned long, unsigned long) __attribute__((always_inline));
extern inline Header fresh_tuple(unsigned long size_in_pointers,
                                 unsigned long prefix_in_chars)
{
    bool prefix = (0 < prefix_in_chars);
    unsigned long fullcount = size_in_pointers;
    unsigned long fullsize  = toSize(fullcount);
    fullsize += sizeof(struct gc_header);

    if (prefix) {
        fullsize += POINTER_SIZE;
        fullsize += prefix_in_chars;
    }

    Header header = (Header) malloc(fullsize);

    header->count  = fullcount;
    header->color  = nc_unknown;
    header->atom   = 0;
    header->inside = 0;
    header->prefix = (prefix ? 1 : 0);
    header->space  = 0;
    header->before = header->after = 0;

    if (prefix) {
        Reference *slots = asReference(header);
        slots[0] = (slots + (fullcount + 1));
    }

    return header;
}

extern inline bool darken_Node(const Node node) __attribute__((always_inline));
extern inline bool darken_Node(const Node node) {
    if (!node.reference) return true;

    const Header value = asHeader(node);
    const Space  space = value->space;

    if (!space) return true;

    if (value->color == space->visiable) return true;

    VM_DEBUG(5, "darkening (%d) node %p",
             getKind(node),
             node.reference);

    const Header scan   = space->scan;
    const Header top    = space->top;

    if (value == scan) {
        if (top != scan) {
            VM_ERROR("%s", "found a clear/white node in the gray chain");
            return false;
        }
    }

    if (!extract_From(value)) {
        VM_ERROR("unable to extract the node from clear list");
        return false;
    }

    if (top == scan) {
        if (!insert_After(top, value)) {
            VM_ERROR("unable to add the node to gray list");
            return false;
        }
        value->color = space->visiable;
        space->scan  = value;
        return true;
    }

#ifndef OPTION_TWO
        if (!insert_After(top, value)) {
            VM_ERROR("unable to append the node unto gray list");
            return false;
        }
#else
        if (!insert_Before(scan, value)) {
            VM_ERROR("unable to insert the node into gray list");
            return false;
        }
#endif

    value->color = space->visiable;
    return true;
}

extern inline bool clink_Assign(Clink *link, unsigned index, const Node value) __attribute__((always_inline));
extern inline bool clink_Assign(Clink *link, unsigned index, const Node value) {
    if (!link)                return false;
    if (!link->array)         return false;
    if (!link->before)        return false;
    if (!link->after)         return false;
    if (link->size <= index)  return false;
    if (!darken_Node(value))  return false;
    link->array[index] = value.reference;
    return true;
}

/* macros */

/***************************
 ** end of file
 **************************/
#endif
