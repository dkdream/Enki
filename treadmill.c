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
#define debug_THIS

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
    return link->slots;
}

extern inline const char* colorName(enum gc_color value) __attribute__((always_inline));
extern inline const char* colorName(enum gc_color value) {
    switch (value) {
    case nc_unknown: return "unknown";
    case nc_blue:    return "blue";
    case nc_orange:  return "orange";
    default: return ":UNKNOWN:";
    }
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

#define space_Check(space) space_Check__(space, __FILE__, __LINE__)

extern inline void space_Check__(const Space space,
                                 const char *filename,
                                 unsigned int linenum) __attribute__((always_inline));
extern inline void space_Check__(const Space space,
                                 const char *filename,
                                 unsigned int linenum)
{
    if (!space) return;

    const char* error = 0;

    const Header top    = &(space->top);
    const Header bottom = &(space->bottom);
    const Header root   = &(space->root);
    const Header scan   = space->scan;
    const Header free   = space->free;

    bool check_top    = false;
    bool check_bottom = false;
    bool check_root   = false;
    bool check_scan   = false;
    bool check_free   = false;

    Header temp = top;

    // top -> root -> scan -> free -> bottom -> top
    for (; temp ;) {
        if (temp == scan) {
            // note: scan may be root
            check_scan = true;
        }
        temp = temp->after;
        if (temp == top) {
            check_top = true;
            break;
        }
        if (temp == root) {
            check_root = true;
        }
        if (temp == free) {
            // note: free may be bottom
            if (!check_root) {
                error = "top -> free: missing root";
                goto error;
            }
            if (!check_scan) {
                error = "top -> free: missing scan";
                goto error;
            }
            check_free = true;
        }
        if (temp == bottom) {
            if (!check_root) {
                error = "top -> bottom: missing root";
                goto error;
            }
            if (!check_scan) {
                error = "top -> bottom: missing scan";
                goto error;
            }
            if (!check_free) {
                error = "top -> bottom: missing free";
                goto error;
            }
            check_bottom = true;
        }
    }

    if (!check_top) {
        error = "forward list is not circular";
        goto error;
    }
    if (!check_bottom) {
        error = "bottom not in forward list";
        goto error;
    }
    if (!check_root) {
        error = "root not in forward list";
        goto error;
    }
    if (!check_scan) {
        error = "scan not in forward list";
        goto error;
    }
    if (!check_free) {
        error = "free not in forward list";
        goto error;
    }

    check_top    = false;
    check_bottom = false;
    check_root   = false;
    check_scan   = false;
    check_free   = false;

    temp = top;

    // top <- root <- scan <- free <- bottom <- top
    for (; temp ;) {
        temp = temp->before;
        if (temp == scan) {
            // note: scan may be root
            check_scan = true;
        }
        if (temp == top) {
            check_top = true;
            break;
        }
        if (temp == root) {
            check_root = true;
        }
        if (temp == free) {
            // note: free may be bottom
            if (check_root) {
                error = "free <- root <- top";
                goto error;
            }
            if (check_scan) {
                error = "free <- scan <- top";
                goto error;
            }
            check_free = true;
        }
        if (temp == bottom) {
            if (check_root) {
                error = "bottom <- root <- top";
                goto error;
            }
            if (check_scan) {
                error = "bottom <- scan <- top";
                goto error;
            }
            check_bottom = true;
        }
    }


    if (!check_top) {
        error = "backward list is not circular";
        goto error;
    }
    if (!check_bottom) {
        error = "bottom not in backward list";
        goto error;
    }
    if (!check_root) {
        error = "root not in backward list";
        goto error;
    }
    if (!check_scan) {
        error = "scan not in backward list";
        goto error;
    }
    if (!check_free) {
        error = "free not in backward list";
        goto error;
    }

    return;

 error:
    fprintf(stderr,
            "%s:%d  %s\n",
            filename,
            linenum,
            error);
    BOOM();
}

static void scan_ForDead__(const Header root, const Header top) {
    if (!root) return;
    if (!top)  return;

    Header cursor = root->before;

    for (; cursor != top; cursor = cursor->before) {
        if (cursor->kind.atom) continue;
        if (1 > cursor->kind.count) continue;

        Reference *slot = (Reference*) asReference(cursor);

        int inx;
        for (inx = 0; inx < cursor->kind.count; ++inx) {
            const Kind kind = asKind(slot[inx]);
            if (!kind) continue;
            if (!kind->live) {
                fatal("ForDead");
            }
        }
    }
}

extern bool insert_After(const Header mark, const Header node) {
    if (!mark)        VM_ERROR("no mark");
    if (!node)        VM_ERROR("no node");
    if (node->before) VM_ERROR("node %p is not unlinked (before %p)", node, node->before);
    if (node->after)  VM_ERROR("node %p is not unlinked (after %p)", node, node->after);

    const Header after = mark->after;

    node->after  = after;
    node->before = mark;

    mark->after   = node;
    after->before = node;

#if defined(CHECK_INSERTS)
    Header cursor = mark;

    for (;;) {
        if (cursor->before == node) break;
        cursor = cursor->before;
        if (cursor->after == mark) VM_ERROR("node not in the circle");
    }
#endif

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

#if defined(CHECK_INSERTS)
    Header cursor = mark;

    for (;;) {
        if (cursor->after == node) break;
        cursor = cursor->after;
        if (cursor->after == mark) VM_ERROR("node not in the circle");
    }
#endif

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

static void release_Header(const Header header) {
    if (!header)              return;
    if (!header->kind.inside) return;

    const Space space = header->space;

    if (!space) return;

    if (header->kind.color == space->visiable) {
        VM_ERROR("releasing a dark header");
    }

    extract_From(header);

    VM_DEBUG(1, "releasing node %p (%s,%s,%s,%s)[%u]"
             " (header %p)"
             " (space %p visiable %s scan %p top %p count %u)",
             asReference(header),
             colorName(header->kind.color),
             (header->kind.atom ? "atom" : "compound"),
             (header->kind.live ? "live" : "dead"),
             (header->kind.inside ? "in" : "out"),
             (unsigned) header->kind.count,
             header,
             space,
             colorName(space->visiable),
             space->scan,
             &(space->top),
             (unsigned) space->count);

    header->kind.color  = nc_unknown;
    header->kind.live   = 0;
    header->kind.inside = 0;
    header->kind.count  = 0;
    header->space       = 0;

    free(header);
}

extern bool darken_Node(const Node node) {
    if (!node.reference)  return true;
    if (!boxed_Tag(node)) return true;

    const Header header = asHeader(node);
    const Space   space = header->space;

    if (!header->kind.live) {
        fprintf(stderr, "found in space %p a dead ", space);
        dump(stderr, node);
        fprintf(stderr, "\n");
        fatal("GC BOOM");
    }

    if (!header->kind.inside) return true;
    if (!space) return true;

    if (header->kind.color == space->visiable) return true;

    VM_DEBUG(1, "darkening node %p (%s,%s,%s,%s)[%u]"
             " (header %p)"
             " (space %p visiable %s scan %p top %p count %u)",
             node.reference,
             colorName(header->kind.color),
             (header->kind.atom ? "atom" : "compound"),
             (header->kind.live ? "live" : "dead"),
             (header->kind.inside ? "in" : "out"),
             (unsigned) header->kind.count,
             header,
             space,
             colorName(space->visiable),
             space->scan,
             &(space->top),
             (unsigned) space->count);

    const Header scan   = space->scan;
    const Header free   = space->free;
    const Header top    = &(space->top);
    const Header bottom = &(space->bottom);

    if (header == scan) {
        VM_ERROR("darkening scan");
        return false;
    }

    if (header == top) {
        VM_ERROR("darkening top");
        return false;
    }

    if (header == bottom) {
        VM_ERROR("darkening bottom");
        return false;
    }

    if (header == free) {
        VM_ERROR("darkening free");
        return false;
    }

    header->kind.color = space->visiable;

    if (!extract_From(header)) {
        BOOM();
    }
    if (!insert_After(top, header)) {
        BOOM();
    }

    space->count += 1;

    return true;
}

extern void check_Node(const Node node) {
     if (!node.reference) return;

     if (!boxed_Tag(node)) return;

     const Kind   kind   = asKind(node);
     const Header header = asHeader(node);

     if (&(header->kind) != kind) {
         BOOM();
     }

     if (!kind->inside) return;

     const Space space  = header->space;

     if (!space) {
         BOOM();
     }

     if (!header->kind.live) {
         BOOM();
     }

     if (header->kind.atom) return;

     unsigned long max = header->kind.count;

     Reference *slot = (Reference*) asReference(header);

     int inx;
     for (inx = 0; inx < max; ++inx) {
         const Header temp = asHeader(slot[inx]);
         if (!temp) continue;
         if (!temp->kind.live) {
             fprintf(stderr, "dead node ");
             prettyPrint(stderr, slot[inx]);
             fprintf(stderr,"\n");
             BOOM();
         }
         check_Node(slot[inx]);
     }
}

extern inline void scan_Node(const Header header) __attribute__((always_inline));
extern inline void scan_Node(const Header header) {
    if (!header) return;

    VM_DEBUG(5, "scan header %p(%s,%s,%s,%s)[%u] begin ",
             header,
             colorName(header->kind.color),
             (header->kind.atom ? "atom" : "compound"),
             (header->kind.live ? "live" : "dead"),
             (header->kind.inside ? "in" : "out"),
             (unsigned) header->kind.count);

    if (!header->kind.inside) return;

    const Space space = header->space;

    if (!space) return;

    if (header->kind.color != space->visiable) {
        VM_ERROR("scan error: scanning a clear or white node %p", header);
        return;
    }

    if (header->kind.atom) return;
    if (1 > header->kind.count) return;

    darken_Node(header->kind.type);

    Reference *slot = (Reference*) asReference(header);

    VM_DEBUG(5, "scan reference (%p) begin", slot);

    int inx;
    for (inx = 0; inx < header->kind.count; ++inx) {
        darken_Node(slot[inx]);
    }

    VM_DEBUG(5, "scan reference (%p) end", slot);

    VM_DEBUG(5, "scan header (%p) end", header);
}

extern bool node_Allocate(const Space space,
                          bool atom,
                          Size size_in_char,
                          Target target)
{
    static unsigned counter = 0;

    VM_DEBUG(1, "node_Allocate(%p, %s, %u, %p)",
             space,
             (atom ? "atom" : "compound"),
             (unsigned) size_in_char,
             target.reference);

    if (!target.reference) return false;

    bool inside = (!space ? false : true);

    if (inside) {
        ++counter;
        if (__alloc_cycle < counter) {
            unsigned count = __scan_cycle;
            // scan first
            space_Scan(space, count);

            // can we flip
            if (space_CanFlip(space)) {
                space_Flip(space);
            }
            counter = 0;
        }
    }

    const Header  header = fresh_tuple(inside, size_in_char);
    const Reference node = asReference(header);
    const Kind      kind = asKind(node);

    if (&(header->kind) != kind) {
        VM_ERROR("header pointer error:  kind %p header.kind %p",
                 kind,
                 &(header->kind));
    }

    kind->atom = (atom ? 1 : 0);

    if (!inside) return true;

    kind->color   = space->visiable;
    header->space = space;

    space->count += 1;

    VM_DEBUG(1,
             "allocating node %p (%s,%s,%s,%s)[%u]"
             " (header %p)"
             " (space %p visiable %s scan %p top %p count %u)",
             node,
             colorName(kind->color),
             (kind->atom ? "atom" : "compound"),
             (kind->live ? "live" : "dead"),
             (kind->inside ? "in" : "out"),
             (unsigned) kind->count,
             (void*) header,
             space,
             colorName(space->visiable),
             space->scan,
             &(space->top),
             (unsigned) space->count);

    // insert into the black chain
    if (!insert_Before(space->free, header)) {
        VM_ERROR("unable to add to node %p to black list of %p",
                 node,
                 space);
        return false;
    }

    target.reference[0] = node;

    if (!boxed_Tag(node)) {
        VM_ERROR("unable to allocate a boxed node %p to black list of %p",
                 node,
                 space);

        return false;
    }

    // only return (true) after its in the black chain
    return true;
}

extern void space_Init(const Space space) {
    if (!space) return;

    VM_DEBUG(5, "init space %p", space);

    memset(space, 0, sizeof(struct gc_treadmill));

    const Header top    = &space->top;
    const Header bottom = &space->bottom;
    const Header root   = &space->root;

    space->visiable = nc_blue;

    init_atom(top,0);
    init_atom(bottom,0);
    init_atom(root,0);

    top->kind.live    = 0;
    bottom->kind.live = 0;

    top->space    = space;
    bottom->space = space;
    root->space   = space;

    // create a three node circular doubly link list
    top->after     = root;
    root->after    = bottom;
    bottom->after  = top;

    top->before    = bottom;
    bottom->before = root;
    root->before   = top;

    // set root to visible
    top->kind.color    = space_Hidden(space);
    bottom->kind.color = space_Hidden(space);
    root->kind.color   = space->visiable;

    space->scan = root;
    space->free = bottom;

    space_Check(space);

    // Inital state
    // white chain empty   (free == bottom)
    // clear chain empty   (bottom == top.before)
    // black chain empty   (scan == free.before)
    // gray chain one node (scan == root)

    clink_Init(&space->start_clinks,
               space->array,
               ROOT_COUNT);
}

extern void space_Scan(const Space space, unsigned int upto) {
    if (!space) return;

    if (1 > upto) return;

    VM_DEBUG(2, "scan space %p begin", space);

    space_Check(space);

    const Header top = &(space->top);

    for ( ; 0 < upto ; --upto) {
        const Header scan = space->scan;

        if (scan == top) break;

        VM_DEBUG(1, "found header %p(%s,%s,%s,%s)[%u] in space %p[%u]",
                 scan,
                 colorName(scan->kind.color),
                 (scan->kind.atom ? "atom" : "compound"),
                 (scan->kind.live ? "live" : "dead"),
                 (scan->kind.inside ? "in" : "out"),
                 (unsigned) scan->kind.count,
                 space,
                 (unsigned) space->count);

        scan_Node(scan);

        space->scan = scan->before;
    }

    space_Check(space);

    VM_DEBUG(2, "scan space %p end", space);
}

extern void space_Flip(const Space space) {
    if (!space) return;

    space_Check(space);

    const Header top    = &(space->top);
    const Header bottom = &(space->bottom);
    const Header root   = &(space->root);
    const Header scan   = space->scan;

    if (scan != top) return;

    VM_DEBUG(2, "flip space %p[%ld] begin", space, space->count);

    space->count = 0;

    Header free = space->free;

    if (root->space != space) {
        VM_ERROR("%s", "the start root is NOT in its allocated space !!!");
        return;
    }

    // set top and bottom to old visible (new hidden)
    top->kind.color    = space->visiable;
    bottom->kind.color = space->visiable;

    // set root to new visiable
    root->kind.color = space->visiable = space_Hidden(space);

    space_Check(space);

    VM_DEBUG(5, "moving root (1)");

    // extract the root from the old black chain
    if (!extract_From(root)) {
        VM_ERROR("unable to extract the start root from old black");
        return;
    }

    if (!insert_Before(free, root)) {
        VM_ERROR("unable to insert the root into new gray");
        return;
    }

    space->scan = root; // set the start of the new gray chain

    VM_DEBUG(5, "check free with bottom and top (2)");

    if (free == bottom) {
        if (bottom->after != top) {
            VM_DEBUG(5, "ajusting free");
            free = bottom->after;
            space->free = free;
        }
    }

    VM_DEBUG(5, "moving bottom (3)");

    // concatinate the old white and old clear chains into new white chain
    if (!extract_From(bottom)) {
        VM_ERROR("unable concatinate old white and old clear");
        return;
    }

    // empty old clear list
    if (!insert_Before(top, bottom)) {
        VM_ERROR("unable to create empty clear list");
        return;
    }

    VM_DEBUG(5, "marking node in free segment (4)");

#if defined(DO_RELEASE)
    for (;;) {
        const Header hold = free;

        if (hold == bottom) break;

        free = hold->after;

        release_Header(hold);
    }
#else
    {
        Header hold = free;

        VM_DEBUG(1, "free %p bottom %p",
                 hold,
                 bottom);

        for (; hold != bottom ; hold = hold->after) {
            hold->kind.live = 0;
            VM_DEBUG(1, "marking dead header %p(%s,%s,%s,%s)[%u] in space %p",
                     hold,
                     colorName(hold->kind.color),
                     (hold->kind.atom ? "atom" : "compound"),
                     (hold->kind.live ? "live" : "dead"),
                     (hold->kind.inside ? "in" : "out"),
                     (unsigned) hold->kind.count,
                     space);
        }
    }
#endif

    space->free = free;

    scan_ForDead__(root, top);

    VM_DEBUG(5, "moving top (5)");

    if (!extract_From(top)) {
        VM_ERROR("unable ot extract the top");
        return;
    }

    // make the old black chain gray
    if (!insert_Before(root, top)) {
        VM_ERROR("unable to change old black to new gray");
        return;
    }

    VM_DEBUG(5, "scan clinks (6)");

    Clink *start = &space->start_clinks;
    Clink *end   = &space->start_clinks;

    /* CLINK_LOCK */
    do {
        VM_DEBUG(5, "scaning clink %p(%d,%d)", start, start->max, start->index);

        const unsigned max = start->max;
        unsigned size = start->index;
        Target *slots = clink_Slots(start);

        if (size > max) {
            VM_ERROR("invalid clink (%p)", start);
        }

        for (; 0 < size-- ;) {
            if (!slots[size].reference) {
                VM_DEBUG(5, "slot is empty");
                continue;
            }

            const Reference value = slots[size].reference[0];

            VM_DEBUG(5, "checking clink[%d] reference %p value %p",
                     size,
                     slots[size].reference,
                     value);

            if (!value) continue;

            const Header header = asHeader(value);
            const Space  space  = header->space;

            VM_DEBUG(1, "found header %p(%s,%s,%s,%s)[%u] in space %p",
                     header,
                     colorName(header->kind.color),
                     (header->kind.atom ? "atom" : "compound"),
                     (header->kind.live ? "live" : "dead"),
                     (header->kind.inside ? "in" : "out"),
                     (unsigned) header->kind.count,
                     space);

            if (!header->kind.inside) continue;
            if (!space) continue;

            darken_Node(value);
        }
        start = start->after;

    } while (start != end);
    /* CLINK_UNLOCK */

    space_Check(space);

    VM_DEBUG(2, "flip space %p end", space);

    (void)release_Header;
}

extern void clink_Init(Clink *link, Target* slots, unsigned max) {
    if (!link)  return;
    if (!slots) return;

    link->before = 0;
    link->after  = 0;
    link->max    = max;
    link->index  = 0;
    link->slots  = slots;

    const Space space = _zero_space;

    if (!space) return;

    /* CLINK_LOCK */
    Clink *mark = &space->start_clinks;

    if (mark == link) {
        link->before = mark;
        link->after  = mark;
        return;
    }

    Clink *before = mark->before;

    link->before = before;
    link->after  = mark;

    mark->before  = link;
    before->after = link;

    /* CLINK_UNLOCK */
}

extern void clink_Manage(Clink *link, Target slot, bool zero) {
    if (!link)           goto error;
    if (!slot.reference) goto error;
    if (!link->before)   goto error;
    if (link->max <= link->index) goto error_space;

    Target *array = clink_Slots(link);

    if (zero) {
        ASSIGN(slot, NIL);
    }

    array[link->index] = slot;

    ++(link->index);

    return;

 error:
    fatal("clink Manage error");

 error_space:
    fatal("clink Manage error: to little space");
}

extern void clink_UnManage(Clink *link, Target slot) {
    if (!link)           goto error;
    if (!slot.reference) goto error;
    if (!link->before)   goto error;
    if (1 > link->index) goto error;

    int      max = link->index;
    unsigned inx = 0;
    bool     found = false;

    Target *array = clink_Slots(link);

    for (; inx < max ; ++inx) {
        Target hold = array[inx];
        if (hold.reference == slot.reference) {
            found = true;
            break;
        }
    }

    if (!found) goto error;

    --max;

    for (; inx < max ; ++inx) {
        array[inx] = array[inx+1];
    }

    --(link->index);

    return;

 error:
    fatal("clink UnManage error");
}

extern void clink_Final(Clink *link) {
    if (!link)         goto error;
    if (!link->before) goto error;
    if (!link->after)  goto error;

    link->index = 0;

    /* CLINK_LOCK */

    Clink *before = link->before;
    Clink *after  = link->after;

    if (before->after != link) goto error;
    if (after->before != link) goto error;

    before->after = after;
    after->before = before;

    link->before = 0;
    link->after  = 0;

    /* CLINK_UNLOCK */

    return;

 error:
    fatal("clink is not linked corectly");
}

/*****************
 ** end of file **
 *****************/
