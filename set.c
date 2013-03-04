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
#include "dump.h"

/* */
#include <stdio.h>
#include <stdbool.h>
#include <error.h>

#define SET_BLOCK_SIZE 10

/* */

static inline bool cell_Create(Node first, Set_cell rest, Set_cell *result) {
    if (isNil(first)) return false;

    Reference target;

    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct set_cell),
                       0,
                       &target))
        return false;

    Set_cell cell = target;

    darken_Node(first);
    darken_Node(rest);

    cell->first = first;
    cell->rest  = rest;

    *result = cell;

    return true;
}

static inline bool set_block_Create(const Space space, Set_block *result) {

    Reference target;

    if (!node_Allocate(space,
                       false,
                       asSize(sizeof(struct set_block), sizeof(Set_cell) * SET_BLOCK_SIZE),
                       0,
                       &target))
        return false;

    *result = target;

    return true;
}

extern bool set_Create(unsigned size, Set *result) {

    Reference target;

    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct set),
                       sizeof(struct set_state),
                       &target))
        return false;

    Set set = target;

    if (!set_block_Create(_zero_space, &(set->first))) return false;

    Set_state state = set->state;

    if (!state) return false;

    state->fullsize = SET_BLOCK_SIZE;

    *result = set;

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

    darken_Node(current->first);
    *target = current->first;

    return true;
}

static inline bool cell_Next(Set_cell current, Set_cell* target) {
    if (!current)       return false;
    if (!current->rest) return false;
    if (!target)        return false;

    darken_Node(current->rest);
    *target = current->rest;

    return true;
}

static inline bool allowInSet(Set set, Node value) {
    if (!set)         return false;
    if (isNil(value)) return false;
    return true;
}

static inline bool matchSets(Set left, Set right) {
    if (!left)  return false;
    if (!right) return false;
    return true;
}

static inline bool hashForSet(Set set, Node value, HashCode *target) {
    if (!set)         return false;
    if (!target)      return false;
    if (isNil(value)) return false;

    *target = node_HashCode(value);

    return true;
}

static inline bool matchForSet(Set set, Node left, Node right) {
    if (!set)         return false;
    if (isNil(left))  return false;
    if (isNil(right)) return false;
    return node_Match(left, right);
}

extern bool set_Contains(Set set, Node value) {
    if (!allowInSet(set, value)) return false;

    Set_state state = set->state;

    if (!state)            return false;
    if (0 == state->count) return false;

    HashCode hashcode = 0;

    if (!hashForSet(set, value, &hashcode)) return false;

    unsigned index = hashcode % state->fullsize;
    Set_cell  list = (Set_cell)0;
    Set_block here = set->first;

    for ( ; ; here = here->next) {
        if (!here) {
            return false;
        }
        long max = getCount(here) - 1;
        if (index < max) break;
        index -= max;
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

    Set_state state = set->state;

    if (!state)               return false;
    if (0 == state->fullsize) return false;

    HashCode hashcode = 0;

    if (!hashForSet(set, value, &hashcode)) return false;

    unsigned index = hashcode % state->fullsize;
    Set_cell *location = (Set_cell*)0;
    Set_block here = set->first;

    for ( ; ; here = here->next) {
        if (!here) {
            return false;
        }
        long max = getCount(here) - 1;
        if (index < max) break;
        index -= max;
    }

    location = &(here->list[index]);

    Set_cell list = *(location);

    if (!list) {
        cell_Create(value, 0, location);
        state->count += 1;
        return true;
    }

    for ( ; ; list = list->rest) {
        if (matchForSet(set, list->first, value)) return true;
        if (!list->rest) {
            cell_Create(value, 0, &list->rest);
            state->count += 1;
            return true;
        }
    }
}

extern bool set_Remove(Set set, Node value) {
    if (!allowInSet(set, value)) return false;

    Set_state state = set->state;
    if (!state)               return false;
    if (0 == state->fullsize) return false;

    HashCode hashcode = 0;

    if (!hashForSet(set, value, &hashcode)) return false;

    unsigned index = hashcode % state->fullsize;
    Set_cell *location = (Set_cell*)0;
    Set_block here = set->first;

    for ( ; ; here = here->next) {
        if (!here) {
            return false;
        }
        long max = getCount(here) - 1;
        if (index < max) break;
        index -= max;
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

    state->count -= 1;
    *(location) = list->rest;

    return true;
}

extern bool set_Clone(Set source, Set *target) {
    if (!source) return false;

    Set_state state = source->state;
    if (!state)               return false;
    if (0 == state->fullsize) return false;
    if (!set_Create(state->fullsize, target)) return false;

    return set_Union(*target, source);
}

extern bool set_Union(Set result, Set source) {
    if (!matchSets(result, source)) return false;

    Set_state state = source->state;
    if (!state)               return false;
    if (0 == state->fullsize) return false;

    Set_block block = source->first;

    for ( ; block ; block = block->next) {
        long      index = getCount(block) - 1;
        Set_cell *list  = block->list;
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

    Set_state state = source->state;
    if (!state)               return false;
    if (0 == state->fullsize) return false;

    state = result->state;
    if (!state) return false;

    struct set_block *block = result->first;

    for ( ; block ; block = block->next) {
        long      index = getCount(block) - 1;
        Set_cell *list  = block->list;
        for ( ; 0 < index--; ) {
            Set_cell *location = &(list[index]);
            Set_cell      here = *location;
            for ( ; here ; here = here->rest) {
                if (set_Contains(source, here->first)) {
                    state->count -= 1;
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

    Set_state state = result->state;
    if (!state) return false;

    Set_block block = result->first;

    for ( ; block ; block = block->next) {
        long      index = getCount(block) - 1;
        Set_cell *list  = block->list;
        for ( ; 0 < index--; ) {
            Set_cell *location = &(list[index]);
            Set_cell      here = *location;
            for ( ; here ; here = here->rest) {
                if (!set_Contains(source, here->first)) {
                    state->count -= 1;
                    *location = here->rest;
                    continue;
                }
                location = &(here->rest);
            }
        }
    }

    return true;
}

/* */
extern void set_Print(FILE* output, Set set) {
    if (!output) return;
    if (!set) {
        fprintf(output, "nil");
        return;
    }

    Set_state state = set->state;

    fprintf(output, "set_%p{", set);

    unsigned int total = 0;

    Set_block here = set->first;
    for ( ; here ; here = here->next) {
        long index = getCount(here) - 1;
        for ( ; 0 < index ; --index) {
            Set_cell list = here->list[index];
            for ( ; list ; list = list->rest) {
                if (total) fprintf(output, " ");
                dump(output, list->first);
                total += 1;
            }
        }
    }
    fprintf(output, "}");

    if (total != state->count) {
        //VM_ERROR("Set Count Error: seen %u cached %u", total, set->count);
    }
}

/*****************
 ** end of file **
 *****************/

