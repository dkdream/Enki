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
Space _zero_space = (Space)0;

static struct gc_treadmill the_zero_space;

static inline void fast_lock(int *address) {
    return;
}

static inline void fast_unlock(int *address) {
    return;
}

extern void init_zero_space() {
    if (_zero_space) return;

    VM_DEBUG(1, "init_zero_space");

    if (!space_Init(&the_zero_space)) return;
    _zero_space = &the_zero_space;
}

extern void final_zero_space() {
    if (0 == _zero_space) return;

    VM_DEBUG(1, "final_zero_space");
}

extern inline bool initialize_StartNode(const Space space, const Header) __attribute__((always_inline));
extern inline bool initialize_StartNode(const Space space, const Header node) {
    if (!node) return false;

    memset(node, 0, sizeof(struct gc_header));

    node->count  = 0;
    node->color  = nc_orange;
    node->atom   = 1;
    node->inside = 0;
    node->kind   = nc_unknown;
    node->space  = space;
    node->before = node->after = 0;

    return true;
}

extern inline bool insert_Before(const Header, const Header) __attribute__((always_inline));
extern inline bool insert_Before(const Header mark, const Header node) {
    if (!mark)        VM_ERROR("no mark");
    if (!node)        VM_ERROR("no node");
    if (node->before) VM_ERROR("node 0x%p is not unlinked", node);
    if (node->after)  VM_ERROR("node 0x%p is not unlinked", node);

    const Header before = mark->before;

    node->before = before;
    node->after  = mark;

    mark->before  = node;
    before->after = node;

    return true;
}

extern inline bool insert_After(const Header, const Header) __attribute__((always_inline));
extern inline bool insert_After(const Header mark, const Header node) {
    if (!mark)        VM_ERROR("no mark");
    if (!node)        VM_ERROR("no node");
    if (node->before) VM_ERROR("node 0x%p is not unlinked", node);
    if (node->after)  VM_ERROR("node 0x%p is not unlinked", node);

    const Header after  = mark->after;

    node->after  = after;
    node->before = mark;

    mark->after   = node;
    after->before = node;

    return true;
}

extern inline bool extract_Node(const Header) __attribute__((always_inline));
extern inline bool extract_Node(const Header node) {
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

extern inline Color space_Hidden(const Space space) __attribute__((always_inline));
extern inline Color space_Hidden(const Space space) {
    if (!space) return nc_unknown;

    switch (space->visiable) {
    case nc_blue:
        return nc_orange;

    case nc_orange:
        return nc_blue;

    default:
        VM_ERROR("Space (0x%p) has an invalid color\n", space);
    }
    return nc_unknown;
}

extern bool darken_Node(const Node node) __attribute__((always_inline));
extern bool darken_Node(const Node node) {
    if (!node.reference) return true;

    const Header value = node.reference;
    const Space     space = value->space;

    if (!space) return true;

    if (value->color == space->visiable) return true;

    VM_DEBUG(5, "darkening (%s) node 0x%p",
             nodeTypename(node),
             node.reference);

    const Header scan   = space->scan;
    const Header top    = space->top;

    if (value == scan) {
        if (top != scan) {
            VM_ERROR("%s", "found a clear/white node in the gray chain");
            return false;
        }
    }

    if (!extract_Node(value)) {
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
        if (!insert_Before(scan, value)) {
            VM_ERROR("unable to insert the node into gray list");
            return false;
        }
#else
        if (!insert_After(top, value)) {
            VM_ERROR("unable to append the node unto gray list");
            return false;
        }
#endif

    value->color = space->visiable;
    return true;
}

extern bool scan_Node(const Node node, const bool depth) __attribute__((always_inline));
extern bool scan_Node(const Node node, const bool depth) {
    if (!node.reference) return false;

    const Header reference = node.reference;
    const Space     space     = reference->space;

    if (!space) return true;

    VM_DEBUG(5, "scanning (%s) node 0x%p in space 0x%p",
             nodeTypename(node),
             reference,
             space);

    if (reference->color != space->visiable) {
        VM_ERROR("scan error: scanning a clear or white node 0x%p", reference);
        return false;
    }

    bool scan_hash() {
        Hash value = node.hash;
        VM_DEBUG(5, "scan begin (%s)", nodeTypename(node));
        if (!darken_Node(value->first))  return false;
        VM_DEBUG(5, "scan end (%s)", nodeTypename(node));
        return true;
    };
    bool scan_hash_block() {
        struct hash_block *value = node.hash_block;
        VM_DEBUG(5, "scan begin (%s)", nodeTypename(node));
        if (!darken_Node(value->next))  return false;
        unsigned inx = value->size;
        for ( ; 0 < inx-- ; ) {
            if (!darken_Node(value->list[inx])) return false;
        }
        VM_DEBUG(5, "scan end (%s)", nodeTypename(node));
        return true;
    };
    bool scan_hash_entry() {
        Hash_entry value = node.hash_entry;
        VM_DEBUG(5, "scan begin (%s)", nodeTypename(node));
        if (!darken_Node(value->next))   return false;
        if (!darken_Node(value->symbol)) return false;
        if (!darken_Node(value->value))  return false;
        VM_DEBUG(5, "scan end (%s)", nodeTypename(node));
        return true;
    };
    bool scan_set() {
        Set value = node.set;
        VM_DEBUG(5, "scan begin (%s)", nodeTypename(node));
        if (!darken_Node(value->first))  return false;
        VM_DEBUG(5, "scan end (%s)", nodeTypename(node));
        return true;
    };
    bool scan_set_block() {
        struct set_block *value = node.set_block;
        VM_DEBUG(5, "scan begin (%s)", nodeTypename(node));
        if (!darken_Node(value->next))  return false;
        unsigned inx = value->size;
        for ( ; 0 < inx-- ; ) {
            if (!darken_Node(value->list[inx])) return false;
        }
        VM_DEBUG(5, "scan end (%s)", nodeTypename(node));
        return true;
    };
    bool scan_set_cell() {
        Set_cell value = node.set_cell;
        VM_DEBUG(5, "scan begin (%s)", nodeTypename(node));
        if (!darken_Node(value->first)) return false;
        if (!darken_Node(value->rest))  return false;
        VM_DEBUG(5, "scan end (%s)", nodeTypename(node));
        return true;
    };
    bool type_unfinished() {
        VM_ERROR("%s is not finished", node_type_Name(hasType(node)));
        return false;
    };

    switch (hasType(reference)) {
    case nt_count:         return true;
    case nt_hash:          return scan_hash();
    case nt_hash_block:    return scan_hash_block();
    case nt_hash_entry:    return scan_hash_entry();
    case nt_input:         return true;
    case nt_integer:       return true;
    case nt_output:        return true;
    case nt_primitive:     return true;
    case nt_set:           return scan_set();
    case nt_set_block:     return scan_set_block();
    case nt_set_cell:      return scan_set_cell();
    case nt_symbol:        return true;
    case nt_text:          return true;

        /*********************************/

    case nt_unknown: return true;
    }

    VM_ERROR("Invalid Type %u", (unsigned) hasType(reference));
    return false;
}

extern bool node_Darken(Node node) {
    return darken_Node(node);
}

extern bool node_ExternalInit(const EA_Type type, const Header result) {
    if (!result) return false;

    result->count  = 0;
    result->color  = nc_unknown;
    result->atom   = 1;
    result->inside = 0;
    result->kind   = type;
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
                          enum node_type type,
                          unsigned int extend,
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

    unsigned base = 0;

    switch (type) {
    case nt_unknown:
        VM_ERROR("can not allocate an unknown type");
        return false;
        /*********************************/
    case nt_count:         base = sizeof(struct count);         break;
    case nt_hash:          base = sizeof(struct hash);          break;
    case nt_hash_block:    base = sizeof(struct hash_block);    break;
    case nt_hash_entry:    base = sizeof(struct hash_entry);    break;
    case nt_input:         base = sizeof(struct input);         break;
    case nt_integer:       base = sizeof(struct integer);       break;
    case nt_output:        base = sizeof(struct output);        break;
    case nt_primitive:     base = sizeof(struct primitive);     break;
    case nt_set:           base = sizeof(struct set);           break;
    case nt_set_block:     base = sizeof(struct set_block);     break;
    case nt_set_cell:      base = sizeof(struct set_cell);      break;
    case nt_symbol:        base = sizeof(struct symbol);        break;
    case nt_text:          base = sizeof(struct text);          break;

        /*********************************/
    }

    if (0 == base) {
        VM_ERROR("Invalid Type %u", (unsigned) type);
        return false;
    }

    bool variable = true;

    switch (type) {
    case nt_unknown:       variable = true; break;
        /*********************************/
    case nt_count:         variable = false; break;
    case nt_hash:          variable = true;  break; // need to make fix sized
    case nt_hash_block:    variable = true;  break; // need to make fix sized
    case nt_hash_entry:    variable = false; break;
    case nt_input:         variable = false; break;
    case nt_integer:       variable = false; break;
    case nt_output:        variable = false; break;
    case nt_primitive:     variable = true;  break; // can never be fix size
    case nt_set:           variable = true;  break; // need to make fix sized
    case nt_set_block:     variable = true;  break; // need to make fix sized
    case nt_set_cell:      variable = false; break;
    case nt_symbol:        variable = true;  break; // can never be fix size
    case nt_text:          variable = true;  break; // can never be fix size
    }

    const unsigned int fullsize = base + extend;

    const Header header = fresh_atom(fullsize);

    const Reference result = (*target.reference) = asReference(header);

    VM_DEBUG(4, "allocating node 0x%p of type \'%s\' and size %d from space 0x%p",
             result,
             node_type_Name(type),
             fullsize,
             space);

    header->kind  = type;
    header->space = space;

    if (!space) return true;

    header->color = space->visiable;

    // insert into the black chain
    if (!insert_Before(space->free, result)) {
        VM_ERROR("unable to add to 0x%p to black list of 0x%p",
                 result,
                 space);
        return false;
    }

    // only return (true) after its in the black chain
    return true;
}

extern bool space_Init(const Space space) {
    if (!space) return false;

    VM_DEBUG(5, "init space 0x%p", space);

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

    VM_DEBUG(5, "scan space 0x%p %u", space, upto);

    for ( ; 0 < upto ; --upto) {
        const Header scan = space->scan;

        if (scan == space->top) {
            return true;
        }

        space->scan = scan->before;

        if (!scan_Node(scan, true)) {
            VM_ERROR("unable to scan node (%s) 0x%p", nodeTypename(scan), scan);
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

    VM_DEBUG(5, "flip space 0x%p begin", space);

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
    if (!extract_Node(root)) {
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
        VM_DEBUG(5, "flip space 0x%p end", space);
        return true;
    }

    // concatinate the old white and old clear chains
    if (!extract_Node(bottom)) {
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
            if (!node_Darken(value)) {
                VM_ERROR("unable to darken a clink value");
            }
        }
    }
    /* CLINK_UNLOCK */

    VM_DEBUG(5, "flip space 0x%p end", space);
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

#if 0
// roots are added/removed using Clink(s)
extern bool space_AddRoot(const Space space, Node node) {
    if (!space) return false;
    if (!node.reference) return false;

    if (isType(node, nt_extended)) {
        Extended  value = node.extended;
        Scanner scanner = extended_Scanner(value->type);
        if (!scanner) {
            VM_ERROR("unable to scanner an unregistered extended type: %s(0x%p) hashcode %lld",
                     symbol_Text(value->type),
                     value,
                     value->type->hashcode);
            return false;
        }
    }

    Quote root = &space->start_root;

    if (isNil(root->value)) {
        Set rootSet; // this is safe because without a root set, scan is disabled
        // this need to be in this space not zero_space
        if (!set_CreateIn(space, 30, nt_unknown, &rootSet)) {
            VM_ERROR("unable to allocate root set for 0x%p", space);
        }
        root->value = (Node)rootSet;
    }

    VM_DEBUG(5, "adding root 0x%p to 0x%p", node.reference, space);

    return set_Add(root->value.set, node);
}

extern bool space_RemoveRoot(const Space space, Node node) {
    if (!space)          return false;
    if (!node.reference) return false;

    const Quote root = &space->start_root;

    if (isNil(root->value)) {
        VM_DEBUG(5, "no root set for 0x%p", space);
        return false;
    }

    VM_DEBUG(5, "removing root 0x%pb from 0x%p", node.reference, space);

    return set_Remove(root->value.set, node);
}
#endif

/*****************
 ** end of file **
 *****************/
