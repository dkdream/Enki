/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **
 **
 **
 ** Henry Baker's incremental non-moving garbage collector (tri-color marking)
 **
 **   this is only used inside of a node
 **   each node is in one of the node chains.
 **      o. the grey  chain  (to   list) (top]-scan)
 **      o. the black chain  (live list) (scan]-[free)
 **      o. the white chain  (free list) (free-[bottom)
 **      o. the clear chain  (from list) (bottom]-[top)
 **
 **   Invarints:
 **     there are alway at least three nodes: root, up, down
 **     the top and bottom pointers NEVER point to live nodes (like root)
 **     the top node seperate the clear and gray chains
 **     the bottom node seperate the white and gray chains
 **     the free   pointer can never equal the top pointer
 **     the bottom pointer can never equal the top pointer
 **
 **     if bottom == top.before    ; the clear chain is empty
 **     if scan   == top           ; the grey  chain is empty
 **     if free   == bottom        ; the white chain is empty
 **     if scan   == free.before   ; the black chain is empty
 **
 **
 **   -- option one
 **   darken (n) = begin (breath-first traversal)
 **      insert (n) after top
 **   end
 **
 **   -- option two
 **   gray it (n) begin (depth-first traversal)
 **      insert before scan
 **   end
 **
 **   marker = begin
 **    for each node in the grey list
 **         check each child node (c)
 **           if it has visiable color then dont move it
 **           otherwise set it color to visiable and gray it (c)
 **         set scan to before scan - making the node black
 **    when the gray chain is empty (scan == top) then
 **       all living nodes are in the black chain
 **   end
 **
 **   allocater = begin
 **     if we allocate a new   node we added it to the black list (before free) with visiable color
 **     if we reuse    a white node we added it to the black list (move free pointer to next node) with visiable color
 **     if scan pointer == top pointer
 **         flip
 **     end
 **   end
 **
 **   mutater = begin
 **     if we are working with a node it must be black or gray.
 **   end
 ***/
#ifdef debug_Treadmill
#define debug_THIS
#endif

#include "treadmill.h"
#include "all_types.inc"
#include "node.h"
#include "debug.h"

/* */
#include <string.h>
#include <stdlib.h>


/* */
static inline void fast_lock(int *address) {
    return;
}

static inline void fast_unlock(int *address) {
    return;
}

static Target *clink_Slots(Clink *link) {
    return (Target *)(link + 1);
}

extern inline bool initialize_StartNode(const Space space, const Header) __attribute__((always_inline));
extern inline bool initialize_StartNode(const Space space, const Header node) {
    if (!node) return false;

    memset(node, 0, sizeof(struct gc_header));

    node->count  = 0;
    node->color  = nc_unknown;
    node->atom   = 1;
    node->inside = 0;
    node->prefix = 0;
    node->space  = space;
    node->before = node->after = 0;

    return true;
}

extern inline Color space_Hidden(const Space space) __attribute__((always_inline));
extern inline Color space_Hidden(const Space space) {
    if (!space) return nc_unknown;

    switch (space->visiable) {
    case nc_blue:
        return nc_orange;

    case nc_orange:
        return nc_blue;

    default:
        VM_ERROR("Space (%p) has an invalid color\n", space);
    }
    return nc_unknown;
}

extern bool insert_After(const Header mark, const Header node) {
    if (!mark)        VM_ERROR("no mark");
    if (!node)        VM_ERROR("no node");
    if (node->before) VM_ERROR("node %p is not unlinked (before %p)", node, node->before);
    if (node->after)  VM_ERROR("node %p is not unlinked (after %p)", node, node->after);

    const Header after  = mark->after;

    node->after  = after;
    node->before = mark;

    mark->after   = node;
    after->before = node;

    return true;
}

extern bool insert_Before(const Header mark, const Header node) {
    if (!mark)        VM_ERROR("no mark");
    if (!node)        VM_ERROR("no node");
    if (node->before) VM_ERROR("node %p is not unlinked (before %p)", node, node->before);
    if (node->after)  VM_ERROR("node %p is not unlinked (after %p)", node, node->after);

    const Header before = mark->before;

    node->before = before;
    node->after  = mark;

    mark->before  = node;
    before->after = node;

    return true;
}

extern bool extract_From(const Header node) {
    if (!node) VM_ERROR("no node");

    if (!node->before) {
        if (!node->after) return true;
        VM_ERROR("node is not linked corectly");
    }
    if (!node->after) VM_ERROR("node is not linked corectly");

    const Header before = node->before;
    const Header after  = node->after;

    if (before->after != node) VM_ERROR("node is not linked corectly");
    if (after->before != node) VM_ERROR("node is not linked corectly");

    before->after = after;
    after->before = before;

    node->before = 0;
    node->after  = 0;

    return true;
}


//extern inline bool darken_Node(const Node node) __attribute__((always_inline));
extern bool darken_Node(const Node node) {
    if (!node.reference) return true;

    const Header header = asHeader(node);
    const Space  space = header->space;

    if (!space) return true;

    if (header->color == space->visiable) return true;

    VM_DEBUG(5, "darkening node %p (%d)[%d]",
             node,
             getKind(node),
             header->count);

    const Header scan   = space->scan;
    const Header top    = space->top;

    if (header == scan) {
        if (top != scan) {
            return false;
        }
    }

    if (!extract_From(header)) {
        return false;
    }

    if (top == scan) {
        if (!insert_After(top, header)) {
            return false;
        }
        header->color = space->visiable;
        space->scan  = header;
        return true;
    }

#ifndef OPTION_TWO
        if (!insert_After(top, header)) {
            return false;
        }
#else
        if (!insert_Before(scan, header)) {
            return false;
        }
#endif

    header->color = space->visiable;
    return true;
}

extern inline bool scan_Node(const Header header) __attribute__((always_inline));
extern inline bool scan_Node(const Header header) {
    if (!header) return false;

    const Space space = header->space;

    if (!space) return true;

    if (header->color != space->visiable) {
        VM_ERROR("scan error: scanning a clear or white node %p", header);
        return false;
    }

    if (header->atom) return true;
    if (1 > header->count) return true;

    VM_DEBUG(5, "scanning header %p (%d)[%d] in space %p",
             header,
             header->kind,
             header->count,
             space);

    Reference *slot = (Reference*) asReference(header);

    VM_DEBUG(5, "scan begin (%p)", slot);

    if (header->prefix) {
        int inx;
        for (inx = 1; inx < header->count; ++inx) {
            if (!darken_Node(slot[inx])) return false;
        }
    } else {
        int inx;
        for (inx = 0; inx < header->count; ++inx) {
            if (!darken_Node(slot[inx])) return false;
        }
    }

    VM_DEBUG(5, "scan end (%p)", slot);
    return true;
}

extern bool node_ExternalInit(const EA_Type type, const Header result) {
    if (!result) return false;

    result->count  = 0;
    result->color  = nc_unknown;
    result->atom   = 1;
    result->inside = 0;
    result->prefix = 0;
    result->space  = 0;
    result->before = result->after = 0;

    return true;
}

static inline bool space_CanFlip(const Space space) {
    const Header scan = space->scan;
    const Header top  = space->top;
    return (scan == top);
}

extern bool node_Allocate(const Space space,
                          bool atom,
                          Size size_in_char,
                          Size prefix_in_char,
                          Target target)
{
    if (!target.reference) return false;

    if (space) {
        unsigned count = 100;
        // scan first
        if (!space_Scan(space, count)) {
            return false;
        }
        // can we flip
        if (space_CanFlip(space)) {
            if (!space_Flip(space)) {
                return false;
            }
        }
    }

    if (0 < prefix_in_char) {
        if (atom) {
            VM_ERROR("unable to allocate an prefixed atomic node");
            return false;
        }
    }

    const Header header = fresh_tuple(toCount(size_in_char), prefix_in_char);

    VM_DEBUG(4, "allocating header %p (%d)[%d] for node %p size %d from space %p",
             header,
             header->kind,
             header->count,
             asReference(header),
             size_in_char,
             space);

    header->atom = (atom ? 1 : 0);

    if (!space) return true;

    header->inside = 1;
    header->color  = space->visiable;
    header->space  = space;

    // insert into the black chain
    if (!insert_Before(space->free, header)) {
        VM_ERROR("unable to add to node %p to black list of %p",
                 asReference(header),
                 space);
        return false;
    }

    target.reference[0] = asReference(header);

    // only return (true) after its in the black chain
    return true;
}

extern bool space_Init(const Space space) {
    if (!space) return false;

    VM_DEBUG(5, "init space %p", space);

    memset(space, 0, sizeof(struct gc_treadmill));

    const Header up   = &space->start_up;
    const Header down = &space->start_down;
    const Header root = &space->start_root;

    space->visiable = nc_blue;

    if (!initialize_StartNode(space, root)) return false;
    if (!initialize_StartNode(space, up))   return false;
    if (!initialize_StartNode(space, down)) return false;

    // create a two node circular doubly link list
    up->after   = up->before   = down;
    down->after = down->before = up;

    // insert root before down (after up)
    if (!insert_Before(down, root)) return false;

    // set root to visible
    root->color = space->visiable;
    up->color   = space_Hidden(space);
    down->color = space_Hidden(space);

    space->top    = up;
    space->scan   = root;
    space->bottom = down;
    space->free   = down;

    // Inital state
    // white chain empty   (free == bottom)
    // clear chain empty   (bottom == top.before)
    // black chain empty   (scan == free.before)
    // gray chain one node (scan == root)

    space->start_clinks.before = &space->start_clinks;
    space->start_clinks.after  = &space->start_clinks;
    space->start_clinks.index  = ROOT_COUNT;
    space->start_clinks.index  = 0;

    return true;
}

extern bool space_Scan(const Space space, unsigned int upto) {
    if (!space) return false;

    VM_DEBUG(5, "scan space %p %u", space, upto);

    for ( ; 0 < upto ; --upto) {
        const Header scan = space->scan;

        if (scan == space->top) {
            return true;
        }

        space->scan = scan->before;

        if (!scan_Node(scan)) {
            VM_ERROR("unable to scan node (%d) %p", getKind(scan), scan);
            return false;
        }
    }

    return true;
}

extern bool space_Flip(const Space space) {
    if (!space) return false;

    Header top    = space->top;
    Header scan   = space->scan;

    if (scan != top) return true;

    VM_DEBUG(5, "flip space %p begin", space);

    Header bottom = space->bottom;
    Header free   = space->free;
    Header root   = &(space->start_root);

    if (root->space != space) {
        VM_ERROR("%s", "the start root is NOT in its allocated space !!!");
        return true;
    }

    // set top and bottom to old visible (new hidden)
    top->color    = space->visiable;
    bottom->color = space->visiable;

    // set root to new visiable
    root->color = space->visiable = space_Hidden(space);

    // extract the root from the old black chain
    if (!extract_From(root)) {
        VM_ERROR("unable to extract the start root from old black");
        return false;
    }

    // if there are no white node this is simple
    bool simple = (free == bottom);

    if (simple) {
        // set free to the head of the clear chain
        free = bottom->after;
    }

    // add the root to the front of the new gray
    if (!insert_Before(free, root)) {
        VM_ERROR("unable to add the start root from new grep");
        return false;
    }

    // swap top and bottom (roll the wheel)
    // changes black to clean
    // -- if this is simple this also changes clear to white
    space->top    = bottom;
    space->bottom = top;

    if (simple) {
        space->scan = root; // set the start of the new gray chain
        VM_DEBUG(5, "flip space %p end", space);
        return true;
    }

    // concatinate the old white and old clear chains
    if (!extract_From(bottom)) {
        VM_ERROR("unable concatinate old white and old clear");
        return false;
    }

    // seperate the new gray from the new clear
    if (!insert_Before(root, bottom)) {
        VM_ERROR("unable seperate new gray from new clear");
        return false;
    }

    space->scan = root; // set the start of the new gray chain

    Clink *start = &space->start_clinks;
    Clink *end   = &space->start_clinks;

    /* CLINK_LOCK */
    do {
        const unsigned max = start->max;
        unsigned size = start->index;
        Target *slots = clink_Slots(start);

        if (size > max) {
            VM_ERROR("invalid clink (%p)", start);
        }

        for (; 0 < size-- ;) {
            if (!slots[size].reference) continue;

            Reference value = slots[size].reference[0];

            VM_DEBUG(4, "darkening root node %p (%d) in space %p",
                     value,
                     getKind(value),
                     space);

            darken_Node(value);
        }
        start = start->after;
    } while ( start != end );
    /* CLINK_UNLOCK */

    VM_DEBUG(5, "flip space %p end", space);
    return true;
}

extern void clink_Init(Clink *link, unsigned max) {
    if (!link) return;

    link->before = link;
    link->after  = link;
    link->max    = max;
    link->index  = 0;
    link->before = 0;
    link->after  = 0;

    const Space space = _zero_space;

    if (!space) return;

    /* CLINK_LOCK */

    Clink *mark   = &space->start_clinks;
    Clink *before = mark->before;

    link->before = before;
    link->after  = mark;

    mark->before  = link;
    before->after = link;

    /* CLINK_UNLOCK */
}

extern bool clink_Manage(Clink *link, Target slot) {
    if (!link)           return false;
    if (!slot.reference) return false;
    if (!link->before)   return false;
    if (link->max <= link->index) return false;

    Target *array = clink_Slots(link);

    array[link->index] = slot;

    ++(link->index);

    return true;
}

extern void clink_Final(Clink *link) {
    if (!link)         return;
    if (!link->before) return;
    if (!link->after)  return;

    link->index = 0;

    bool error = false;

    /* CLINK_LOCK */

    Clink *before = link->before;
    Clink *after  = link->after;

    if (before->after != link) error = true;
    if (after->before != link) error = true;

    if (!error) {
        before->after = after;
        after->before = before;

        link->before = 0;
        link->after  = 0;
    }

    /* CLINK_UNLOCK */

    if (error) {
        VM_ERROR("clink is not linked corectly");
    }
}

/*****************
 ** end of file **
 *****************/
