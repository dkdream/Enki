/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "set.h"
#include "node.h"
#include "treadmill.h"

/* */
#include <stdio.h>
#include <stdbool.h>
#include <error.h>

#define SET_BLOCK_SIZE 10

/* */
static inline bool cell_Create(Node first, Set_cell rest, Set_cell* target) {
    if (isNil(first)) return false;
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct set_cell),
                       0,
                       target))
        return false;

    Set_cell result = *target;

    node_Live(first);
    node_Live(rest);

    result->first = first;
    result->rest  = rest;

    return true;
}

static inline bool cell_Count(Set_cell at, unsigned int *count) {
    if (!count) return false;

    unsigned total = 0;

    for ( ; at ; at = at->rest) {
        total += 1;
    }

    *count = total;

    return true;
}

static inline bool cell_First(Set_cell current, Node* target) {
    if (!current) return false;
    if (!target)  return false;

    node_Live(current->first);
    *target = current->first;

    return true;
}

static inline bool cell_Next(Set_cell current, Set_cell* target) {
    if (!current)       return false;
    if (!current->rest) return false;
    if (!target)        return false;

    node_Live(current->rest);
    *target = current->rest;

    return true;
}

static inline bool set_block_Create(const Space space, struct set_block **target) {
    if (!node_Allocate(space,
                       false,
                       asSize(sizeof(struct set_block), sizeof(Set_cell) * SET_BLOCK_SIZE),
                       0,
                       target))
        return false;

    struct set_block *result = *target;

    result->size = SET_BLOCK_SIZE;

    return true;
}

static inline bool allowInSet(Set set, Node value) {
    if (!set)         return false;
    if (isNil(value)) return false;
    if (nt_unknown == set->type) return true;
    return (set->type == hasType(value));
}

static inline bool matchSets(Set left, Set right) {
    if (!left)  return false;
    if (!right) return false;
    return (left->type == right->type);
}

static inline bool hashForSet(Set set, Node value, HashCode *target) {
    if (!set)         return false;
    if (!target)      return false;
    if (isNil(value)) return false;

    HashCode hashcode = 0;
    if (nt_unknown != set->type) {
        if (set->type != hasType(value)) return false;
        hashcode = node_HashCode(value);
    } else {
        union {
            Node     input;
            HashCode output;
        } convert;

        convert.output = 0;
        convert.input  = value;
        hashcode ^= convert.output;
    }

    *target = hashcode;

    return true;
}

static inline bool matchForSet(Set set, Node left, Node right) {
    if (!set)         return false;
    if (isNil(left))  return false;
    if (isNil(right)) return false;
    if (nt_unknown == set->type) {
        return (left.reference == right.reference);
    } else {
        return node_Match(left, right);
    }
}

extern bool set_Contains(Set set, Node value) {
    if (!allowInSet(set, value)) return false;
    if (0 == set->fullsize)      return false;
    if (0 == set->count)         return false;

    HashCode hashcode = 0;

    if (!hashForSet(set, value, &hashcode)) return false;

    unsigned index = hashcode % set->fullsize;
    Set_cell      list = (Set_cell)0;
    Set_block here = set->first;

    for ( ; ; here = here->next) {
        if (!here) {
            VM_ERROR("Set Error");
            return false;
        }
        if (index < here->size) break;
        index -= here->size;
    }

    list = here->list[index];

    if (!list) return false;

    for ( ; list ; list = list->rest) {
        if (matchForSet(set, list->first, value)) return true;
    }

    return false;
}

extern bool set_Add(Set set, Node value) {
    if (!allowInSet(set, value)) return false;
    if (0 == set->fullsize)      return false;

    HashCode hashcode = 0;

    if (!hashForSet(set, value, &hashcode)) return false;

    unsigned index = hashcode % set->fullsize;
    Set_cell *location = (Set_cell*)0;
    Set_block here = set->first;

    for ( ; ; here = here->next) {
        if (!here) {
            VM_ERROR("Set Error");
            return false;
        }
        if (index < here->size) break;
        index -= here->size;
    }

    location = &(here->list[index]);

    Set_cell list = *(location);

    if (!list) {
        cell_Create(value, 0, location);
        set->count += 1;
        return true;
    }

    for ( ; ; list = list->rest) {
        if (matchForSet(set, list->first, value)) return true;
        if (!list->rest) {
            cell_Create(value, 0, &list->rest);
            set->count += 1;
            return true;
        }
    }
}

extern bool set_Remove(Set set, Node value) {
    if (!allowInSet(set, value)) return false;
    if (0 == set->fullsize)      return false;

    HashCode hashcode = 0;

    if (!hashForSet(set, value, &hashcode)) return false;

    unsigned index = hashcode % set->fullsize;
    Set_cell *location = (Set_cell*)0;
    Set_block here = set->first;

    for ( ; ; here = here->next) {
        if (!here) {
            VM_ERROR("Set Error");
            return false;
        }
        if (index < here->size) break;
        index -= here->size;
    }

    location = &(here->list[index]);

    Set_cell list = *(location);

    if (!list) return true;

    if (!matchForSet(set, list->first, value)) {
        for ( ; ; ) {
            location = &(list->rest);
            list     = list->rest;

            if (!list) return true;
            if (!matchForSet(set, list->first, value)) break;
        }
    }

    set->count -= 1;
    *(location) = list->rest;

    return true;
}

extern bool set_Clone(Set source, Set *target) {
    if (0 == source->fullsize) return false;
    if (!set_Create(source->fullsize, source->type, target)) return false;
    return set_Union(*target, source);
}

extern bool set_Union(Set result, Set source) {
    if (!matchSets(result, source)) return false;
    if (0 == source->fullsize)      return true;

    Set_block block = source->first;

    for ( ; block ; block = block->next) {
        unsigned int index = block->size;
        Set_cell        *list  = block->list;
        for ( ; 0 < index--; ) {
            Set_cell here = list[index];
            for ( ; here ; here = here->rest) {
                if (!set_Add(result, here->first)) return false;
            }
        }
    }

    return true;
}

extern bool set_Difference(Set result, Set source) {
    if (!matchSets(result, source)) return false;
    if (0 == source->fullsize)      return true;

    struct set_block *block = result->first;

    for ( ; block ; block = block->next) {
        unsigned int index = block->size;
        Set_cell         *list = block->list;
        for ( ; 0 < index--; ) {
            Set_cell *location = &(list[index]);
            Set_cell      here = *location;
            for ( ; here ; here = here->rest) {
                if (set_Contains(source, here->first)) {
                    result->count -= 1;
                    *location = here->rest;
                    continue;
                }
                location = &(here->rest);
            }
        }
    }

    return true;
}

extern bool set_Intersection(Set result, Set source) {
    if (!matchSets(result, source)) return false;

    Set_block block = result->first;

    for ( ; block ; block = block->next) {
        unsigned int index = block->size;
        Set_cell         *list = block->list;
        for ( ; 0 < index--; ) {
            Set_cell *location = &(list[index]);
            Set_cell      here = *location;
            for ( ; here ; here = here->rest) {
                if (!set_Contains(source, here->first)) {
                    result->count -= 1;
                    *location = here->rest;
                    continue;
                }
                location = &(here->rest);
            }
        }
    }

    return true;
}

extern bool set_Create(unsigned size, enum node_type type, Set *target) {
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct set),
                       0,
                       target))
        return false;

    Set result = *target;

    if (!set_block_Create(_zero_space, &result->first)) return false;

    result->type     = type;
    result->fullsize = SET_BLOCK_SIZE;

    return true;
}

/* */
extern void set_Print(FILE* output, Set set) {
    if (!output) return;
    if (!set) {
        fprintf(output, "nil");
        return;
    }

    fprintf(output, "set_%p{", set);

    unsigned int total = 0;

    Set_block here = set->first;
    for ( ; here ; here = here->next) {
        unsigned index = here->size;
        for ( ; 0 < index ; --index) {
            Set_cell list = here->list[index];
            for ( ; list ; list = list->rest) {
                if (total) fprintf(output, " ");
                node_Print(output, list->first);
                total += 1;
            }
        }
    }
    fprintf(output, "}");

    if (total != set->count) {
        VM_ERROR("Set Count Error: seen %u cached %u", total, set->count);
    }


}

/*****************
 ** end of file **
 *****************/

