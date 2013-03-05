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
#include <stdio.h>

#define ROOT_COUNT 5

typedef struct gc_clink      Clink;
typedef enum   gc_color      Color;
typedef struct gc_header    *Header;
typedef struct gc_treadmill *Space;

struct gc_clink {
    Clink*   before;
    Clink*   after;
    unsigned max;
    unsigned index;
};

enum gc_color {
    nc_unknown,
    nc_blue,
    nc_orange,
};

#define BITS_PER_WORD (sizeof(long) * 8)
#define WORD_SIZE     (sizeof(long))
#define POINTER_SIZE  (sizeof(Node))

struct gc_header {
    struct {
        struct gc_treadmill *space;
        struct gc_header    *before;
        struct gc_header    *after;
    } __attribute__((__packed__));

    struct {
        unsigned long count : BITS_PER_WORD - 12 __attribute__((__packed__));
        union {
            unsigned int flags : 12 __attribute__((__packed__));
            struct {
                enum gc_color color  : 2;
                unsigned int  atom   : 1; // is this a tuple of values
                unsigned int  live   : 1; // is this alive
                unsigned int  inside : 1; // is this inside a space (malloc/free by the treadmill)
                unsigned int  prefix : 1; // the first slot is a pointer an atomic segment. (implies !atom)
                unsigned int  kind   : 6; // the kind (used by c-code) (and prettyprint)
            } __attribute__((__packed__));
        } __attribute__((__packed__));
    } __attribute__((__packed__));
};

struct gc_treadmill {
    unsigned long count;
    Color  visiable;
    Header free;   // start of the free    list
    Header bottom; // start of the hidden  list
    Header top;    // end   of the hidden  list
    Header scan;   // end   of the resting list
    /**/
    struct gc_header start_up;
    struct gc_header start_down;
    struct gc_header start_root;
    struct {
        Clink  start_clinks;
        Target array[ROOT_COUNT];
    } __attribute__((__packed__));
};

extern Space _zero_space;

extern bool space_Init(const Space); // this will erase the contents of the space
extern bool space_Flip(const Space);
extern bool space_Scan(const Space, unsigned int);

extern void clink_Init(Clink *link, unsigned max)  __attribute__((nonnull));
extern bool clink_Manage(Clink *link, Target slot) __attribute__((nonnull));
extern void clink_End(Clink *link)                 __attribute__((nonnull));

#define GC_Begin(MAX) \
   struct { Clink link__; Target array[MAX];} __LOCAL_GC; \
   clink_Init((Clink*)(& __LOCAL_GC), MAX)

#define GC_Protect(NAME) \
   clink_Manage((Clink*)(& __LOCAL_GC), &(NAME))

#define GC_End() \
   clink_End((Clink*)(& __LOCAL_GC))

//extern bool node_ExternalInit(const EA_Type, Header);

extern bool darken_Node(const Node node);
extern bool node_Allocate(Space space,
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

extern inline unsigned long getCount(const Node value) __attribute__((always_inline));
extern inline unsigned long getCount(const Node value) {
    if (!value.reference) return 0;
    Header header = asHeader(value);
    return (unsigned long)(header->count);
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

extern inline bool isKind(const Node value, const EA_Type kind)  __attribute__((always_inline));
extern inline bool isKind(const Node value, const EA_Type kind) {
    if  (!value.reference) return false;
    Header header = asHeader(value);
    return kind == header->kind;
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

extern inline Reference init_atom(Header, unsigned long) __attribute__((nonnull always_inline));
extern inline Reference init_atom(Header header,
                                  unsigned long size_in_chars)
{
    unsigned long fullcount = toCount(size_in_chars);
    memset(header, 0, sizeof(struct gc_header));

    header->count  = fullcount;
    header->color  = nc_unknown;
    header->atom   = 1;
    header->live   = 1;

    return asReference(header);
}

extern inline Reference init_tuple(Header, bool, unsigned long) __attribute__((nonnull always_inline));
extern inline Reference init_tuple(Header header,
                                   bool prefix,
                                   unsigned long size_in_pointers)
{
    unsigned long fullcount = size_in_pointers;

    memset(header, 0, sizeof(struct gc_header));

    header->count  = fullcount;
    header->color  = nc_unknown;
    header->atom   = 1;
    header->live   = 1;
    header->prefix = (prefix ? 1 : 0);

    Reference reference = asReference(header);

    if (prefix) {
        Reference *slots = reference;
        slots[0] = (slots + (fullcount + 1));
    }

    return asReference(header);
}

extern inline Header fresh_atom(bool, unsigned long) __attribute__((always_inline));
extern inline Header fresh_atom(bool inside, unsigned long size_in_chars) {
    unsigned long fullcount = toCount(size_in_chars);
    unsigned long fullsize  = toSize(fullcount);
    fullsize += sizeof(struct gc_header);

    Header header = (Header) malloc(fullsize);

    memset(header, 0, fullsize);

    header->count  = fullcount;
    header->color  = nc_unknown;
    header->atom   = 1;
    header->live   = 1;
    header->inside = (inside ? 1 : 0);

    return header;
}

extern inline Header fresh_tuple(bool, unsigned long, unsigned long) __attribute__((always_inline));
extern inline Header fresh_tuple(bool inside,
                                 unsigned long size_in_chars,
                                 unsigned long prefix_in_chars)
{
    bool prefix = (0 < prefix_in_chars);
    unsigned long fullcount = toCount(size_in_chars);
    unsigned long fullsize  = toSize(fullcount);
    fullsize += sizeof(struct gc_header);

    if (prefix) {
        fullsize += POINTER_SIZE;
        fullsize += prefix_in_chars;
    }

    Header header = (Header) malloc(fullsize);

    memset(header, 0, fullsize);

    header->count  = fullcount;
    header->color  = nc_unknown;
    header->live   = 1;
    header->inside = (inside ? 1 : 0);
    header->prefix = (prefix ? 1 : 0);

    if (prefix) {
        Reference *slots = asReference(header);
        slots[0] = (slots + (fullcount + 1));
    }

    return header;
}

/* macros */

/***************************
 ** end of file
 **************************/
#endif
