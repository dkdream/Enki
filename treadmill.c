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

#include "treadmill.h"
#include "all_types.inc"
#include "node.h"

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

extern inline bool initialize_StartNode(const Space space, const Header) __attribute__((always_inline));
extern inline bool initialize_StartNode(const Space space, const Header node) {
    if (!node) return false;

    memset(node, 0, sizeof(struct gc_header));

    node->count  = 0;
    node->color  = nc_orange;
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
    if (node->before) VM_ERROR("node %p is not unlinked", node);
    if (node->after)  VM_ERROR("node %p is not unlinked", node);

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
    if (node->before) VM_ERROR("node %p is not unlinked", node);
    if (node->after)  VM_ERROR("node %p is not unlinked", node);

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

extern bool scan_Node(const Node node) __attribute__((always_inline));
extern bool scan_Node(const Node node) {
    if (!node.reference) return false;

    const Header reference = asHeader(node.reference);
    const Space  space     = reference->space;

    if (!space) return true;

    VM_DEBUG(5, "scanning (%d) node %p in space %p",
             getKind(node),
             reference,
             space);

    if (reference->color != space->visiable) {
        VM_ERROR("scan error: scanning a clear or white node %p", reference);
        return false;
    }

    if (reference->atom) return true;

    Reference *slot = (Reference *)node.reference;

    if (1 > reference->count) return true;

    VM_DEBUG(5, "scan begin (%p)", slot);
    if (reference->prefix) {
        int inx;
        for (inx = 1; inx < reference->count; ++inx) {
            if (!darken_Node(slot[inx])) return false;
        }
    } else {
        int inx;
        for (inx = 0; inx < reference->count; ++inx) {
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

    const Header    header = fresh_tuple(toCount(size_in_char), prefix_in_char);
    const Reference result = (*target.reference) = asReference(header);

    VM_DEBUG(4, "allocating node %p of size %d from space %p",
             result,
             size_in_char,
             space);

    header->atom = (atom ? 1 : 0);
    header->kind = 0;

    if (!space) return true;

    header->inside = 1;
    header->color  = space->visiable;
    header->space  = space;

    // insert into the black chain
    if (!insert_Before(space->free, result)) {
        VM_ERROR("unable to add to %p to black list of %p",
                 result,
                 space);
        return false;
    }

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

    Clink *start = space->start_clinks.after;
    Clink *end   = &space->start_clinks;

    /* CLINK_LOCK */
    for ( ; start != end ; ) {
        unsigned   size  = start->size;
        Reference *array = start->array;

        start = start->after;

        for ( ; size-- ; ) {
            Reference value = array[size];
            if (!darken_Node(value)) {
                VM_ERROR("unable to darken a clink value");
            }
        }
    }
    /* CLINK_UNLOCK */

    VM_DEBUG(5, "flip space %p end", space);
    return true;
}

extern bool clink_Init(Clink *link, unsigned size, Reference* array) {
    if (!link)     return false;
    if (!array)    return false;
    if (0 >= size) return false;

    link->before = link;
    link->after  = link;
    link->size   = size;
    link->array  = array;

    for ( ; size-- ; ) {
        array[size] = 0;
    }

    const Space space = _zero_space;

    if (!space) return true;

    /* CLINK_LOCK */

    Clink *mark   = &space->start_clinks;
    Clink *before = mark->before;

    link->before = before;
    link->after  = mark;

    mark->before  = link;
    before->after = link;

    /* CLINK_UNLOCK */

    return true;
}

extern bool clink_Drop(Clink *link) {
    if (!link) return false;

    link->size = 0;

    const Space space = _zero_space;

    if (!space) return true;

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

    if (error) VM_ERROR("clink is not linked corectly");

    return true;
}

/*****************
 ** end of file **
 *****************/
