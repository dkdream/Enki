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
typedef struct gc_kind      *Kind;
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

#define BOX_TAG_MASK 0x03
#define BOX_TAG      0x00
#define BOX_NIL      0x00

#define FIX_TAG_MASK 0x03
#define FIX_TAG      0x01
#define FIX_SHIFT    2

#define BOOL_TAG_MASK 0x03
#define BOOL_TAG      0x02
#define BOOL_TRUE     0xf2
#define BOOL_FALSE    0x02

struct gc_kind {
    Node type;
    unsigned long count : BITS_PER_WORD - 8 __attribute__((__packed__));
    struct {
        enum gc_color color  : 2;
        unsigned int  atom   : 1; // is this a tuple of values
        unsigned int  live   : 1; // is this alive
        unsigned int  inside : 1; // is this inside a space (malloc/free by the treadmill)
    } __attribute__((__packed__));
};

struct gc_header {
    struct gc_treadmill *space;
    struct gc_header    *before;
    struct gc_header    *after;
    struct gc_kind       kind;
};

struct gc_treadmill {
    unsigned long count;
    Color  visiable;
    Header free;   // start of the free    list
    //    Header bottom; // start of the hidden  list
    //    Header top;    // end   of the hidden  list
    Header scan;   // end   of the resting list
    /**/
    struct gc_header top;
    struct gc_header bottom;
    struct gc_header root;
    struct {
        Clink  start_clinks;
        Target array[ROOT_COUNT];
    } __attribute__((__packed__));
};

extern Space _zero_space;

extern void space_Init(const Space); // this will erase the contents of the space
extern void space_Flip(const Space);
extern void space_Scan(const Space, unsigned int);

#define Debug_treadmill() \
     fprintf(stderr, "Debug_treadmill %s:%d begin\n", __FILE__,  __LINE__); \
     space_Scan(_zero_space, 1000); \
     space_Flip(_zero_space); \
     space_Scan(_zero_space, 1000); \
     space_Flip(_zero_space); \
     space_Scan(_zero_space, 1000); \
     space_Flip(_zero_space); \
     space_Scan(_zero_space, 1000); \
     space_Flip(_zero_space); \
     fprintf(stderr, "end\n")

extern void clink_Init(Clink *link, unsigned max)    __attribute__((nonnull));
extern void clink_Manage(Clink *link, Target slot)   __attribute__((nonnull));
extern void clink_UnManage(Clink *link, Target slot) __attribute__((nonnull));
extern void clink_Final(Clink *link)                 __attribute__((nonnull));

#define GC_Begin(MAX) \
    struct { Clink link__; Target array[MAX];} __LOCAL_GC;    \
    clink_Init((Clink*)(& __LOCAL_GC), MAX)

#define GC_Protect(NAME) \
   clink_Manage((Clink*)(& __LOCAL_GC), &(NAME))

#define GC_UnProtect(NAME) \
   clink_UnManage((Clink*)(& __LOCAL_GC), &(NAME))

#define GC_End() \
    clink_Final((Clink*)(& __LOCAL_GC))

//extern bool node_ExternalInit(const EA_Type, Header);

extern inline bool boxed_Tag(const Node node) __attribute__((always_inline));
extern inline bool boxed_Tag(const Node node) {
    return (BOX_TAG == (node.value & BOX_TAG_MASK));
}

extern inline bool fixed_Tag(const Node node) __attribute__((always_inline));
extern inline bool fixed_Tag(const Node node) {
    return (FIX_TAG == (node.value & FIX_TAG_MASK));
}

extern inline long fixed_Value(const Node node) __attribute__((always_inline));
extern inline long fixed_Value(const Node node) {
    if (!fixed_Tag(node)) return 0;
    return (node.value >> FIX_SHIFT);
}

extern inline bool bool_Tag(const Node node) __attribute__((always_inline));
extern inline bool bool_Tag(const Node node) {
    return (BOOL_TAG == (node.value & BOOL_TAG_MASK));
}
extern inline bool bool_Value(const Node node) __attribute__((always_inline));
extern inline bool bool_Value(const Node node) {
    return (node.value == BOOL_TRUE);
}

extern bool darken_Node(const Node node);
extern void check_Node(const Node node);
extern bool node_Allocate(const Space space, bool atom, Size size_in_char, Target);

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

extern inline Kind asKind(const Node value) __attribute__((always_inline));
extern inline Kind asKind(const Node value) {
    Header header = asHeader(value);
    if (!header) return (Kind)0;
    return &(header->kind);
}


extern inline Node getType(const Node value) __attribute__((always_inline));
extern inline Node getType(const Node value) {
    Kind kind = asKind(value);
    if (!kind) return NIL;
    return kind->type;
}

extern inline bool setType(const Node value, const Node type) __attribute__((always_inline));
extern inline bool setType(const Node value, const Node type) {
    Kind kind = asKind(value);
    if (!kind) return false;
    kind->type = type;
    return true;
}

extern inline bool isType(const Node value, const Node type) __attribute__((always_inline));
extern inline bool isType(const Node value, const Node type) {
    Kind kind = asKind(value);
    if (!kind) return false;
    return kind->type.reference == type.reference;
}

extern inline bool isAtomic(const Node value) __attribute__((always_inline));
extern inline bool isAtomic(const Node value) {
    Kind kind = asKind(value);
    if (!kind) return true;
    if (kind->atom) return true;
    return false;
}

extern inline unsigned long getCount(const Node value) __attribute__((always_inline));
extern inline unsigned long getCount(const Node value) {
    Kind kind = asKind(value);
    if (!kind) return 0;
    return (unsigned long)(kind->count);
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

    header->kind.count  = fullcount;
    header->kind.color  = nc_unknown;
    header->kind.atom   = 1;
    header->kind.live   = 1;
    header->kind.inside = 0;

    return asReference(header);
}

extern inline Reference init_tuple(Header, unsigned long) __attribute__((nonnull always_inline));
extern inline Reference init_tuple(Header header, unsigned long size_in_pointers)
{
    unsigned long fullcount = size_in_pointers;

    memset(header, 0, sizeof(struct gc_header));

    header->kind.count  = fullcount;
    header->kind.color  = nc_unknown;
    header->kind.atom   = 1;
    header->kind.live   = 1;
    header->kind.inside = 0;

    return asReference(header);
}

extern inline Header fresh_atom(bool, unsigned long) __attribute__((always_inline));
extern inline Header fresh_atom(bool inside, unsigned long size_in_chars) {
    unsigned long fullcount = toCount(size_in_chars);
    unsigned long fullsize  = toSize(fullcount);
    fullsize += sizeof(struct gc_header);

    Header header = (Header) malloc(fullsize);

    memset(header, 0, fullsize);

    header->kind.count  = fullcount;
    header->kind.color  = nc_unknown;
    header->kind.atom   = 1;
    header->kind.live   = 1;
    header->kind.inside = (inside ? 1 : 0);

    return header;
}

extern inline Header fresh_tuple(bool, unsigned long) __attribute__((always_inline));
extern inline Header fresh_tuple(bool inside, unsigned long size_in_chars)
{
    unsigned long fullcount = toCount(size_in_chars);
    unsigned long fullsize  = toSize(fullcount);
    fullsize += sizeof(struct gc_header);

    Header header = (Header) malloc(fullsize);

    memset(header, 0, fullsize);

    header->kind.count  = fullcount;
    header->kind.color  = nc_unknown;
    header->kind.live   = 1;
    header->kind.inside = (inside ? 1 : 0);

    return header;
}

/* macros */

/***************************
 ** end of file
 **************************/
#endif
