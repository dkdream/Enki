/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#define USE_HASH

#include "hash.h"
#include "symbol.h"
#include "node.h"
#include "treadmill.h"
#include "dump.h"

/* */
#include <error.h>

#define Hash_Block_Size 10

/* */
static inline bool entry_Create(Symbol symbol, Node value, Hash_entry next, Hash_entry* result) {

    Reference target;

    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct hash_entry),
                       0,
                       &target))
        return false;

    Hash_entry entry = target;

    entry->next   = next;
    entry->symbol = symbol;
    entry->value  = value;

    *result = entry;

    return true;
}

static inline bool hash_block_Create(Hash_block *result) {
    Reference target;

    if (!node_Allocate(_zero_space,
                       false,
                       asSize(sizeof(struct hash_block), sizeof(Hash_entry) * Hash_Block_Size),
                       0,
                       &target)) return false;

    *result = target;

    return true;
}

static inline Hash_entry getEntry(Hash_block here, unsigned index) {
    for ( ; ; here = here->next) {
        if (!here) {
            return false;
        }
        long max = getCount(here) - 1;
        if (index < max) break;
        index -= max;
    }
    return here->list[index];
}

static inline Hash_entry* refEntry(Hash_block here, unsigned index) {
    for ( ; ; here = here->next) {
        if (!here) {
            return false;
        }
        long max = getCount(here) - 1;
        if (index < max) break;
        index -= max;
    }

    return &(here->list[index]);
}


extern bool hash_Create(unsigned size, Hash *result) {
    Reference target;

    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct hash),
                       sizeof(struct hash_state),
                       &target))
        return false;

    Hash hash = target;

    if (!hash_block_Create(&hash->first)) return false;

    Hash_state state = hash->state;

    state->count    = 0;
    state->fullsize = Hash_Block_Size;

    *result = hash;

    return true;
}

extern bool hash_Find(Hash table, Symbol name, Node *target) {
    if (!table)              return false;
    if (!name)               return false;

    Hash_state state = table->state;
    if (1 > state->fullsize) return false;

    unsigned  index = name->hashcode % state->fullsize;
    Hash_entry list = getEntry(table->first, index);

    if (!list) return false;

    for ( ; list ; list = list->next) {
        if (list->symbol == name) {
            if (target) *target = list->value;
            return true;
        }
    }

    return false;
}

extern bool hash_Change(Hash table, Symbol name, Node value) {
    if (!table)       return false;
    if (!name)        return false;
    if (isNil(value)) return false;

    Hash_state state = table->state;
    if (1 > state->fullsize) return false;

    unsigned    index    = name->hashcode % state->fullsize;
    Hash_entry *location = refEntry(table->first, index);
    Hash_entry  list     = *(location);

    if (!list) {
        state->count += 1;
        return entry_Create(name, value, 0, location);
    }

    for ( ; ; list = list->next) {
        if (list->symbol == name) {
            list->value = value;
            return true;
        }
        if (!list->next) {
            state->count += 1;
            return entry_Create(name, value, 0, &(list->next));
        }
    }
}

extern bool hash_Add(Hash table, Symbol name, Node value) {
    if (!table)              return false;
    if (!name)               return false;

    Hash_state state = table->state;
    if (1 > state->fullsize) return false;

    unsigned    index    = name->hashcode % state->fullsize;
    Hash_entry *location =  refEntry(table->first, index);
    Hash_entry  list     = *(location);

    for ( ; list ; list = list->next) {
        if (list->symbol == name) {
            darken_Node(value);
            list->value = value;
            return true;
        }
    }

    if (!entry_Create(name, value, *(location), location)) {
        return false;
    }

    state->count += 1;
    return true;
}

extern bool hash_Remove(Hash table, Symbol name) {
    if (!table) return false;
    if (!name)  return false;

    Hash_state state = table->state;
    if (1 > state->fullsize) return false;

    unsigned    index    = name->hashcode % state->fullsize;
    Hash_entry *location = refEntry(table->first, index);
    Hash_entry  list     = *(location);

    if (!list) return true;

    if (list->symbol != name) {
        for ( ; ; ) {
            location = &(list->next);
            list     = list->next;

            if (!list) return true;
            if (list->symbol == name) break;
        }
    }

    state->count -= 1;
    darken_Node(list->next);
    *(location) = list->next;
    return true;
}

// expand the hash table (for faster access)
extern bool hash_Expand(Hash table, unsigned size) {
    if (!table) return false;
    Hash_state state = table->state;
    if (size < state->fullsize) return true;
    return false;
}

// contract the hash table (for sparce tables)
extern bool hash_Contract(Hash table, unsigned size) {
    if (!table) return false;
    Hash_state state = table->state;
    if (size < state->fullsize) return true;
    return false;
}

/* */
extern void hash_Print(FILE* output, Hash table) {
    if (!output) return;
    if (!table) {
        fprintf(output, "nil");
        return;
    }

    Hash_state state = table->state;

    fprintf(output, "hash_%p{", table);

    unsigned int total = 0;

    Hash_block here = table->first;
    for ( ; here ; here = here->next) {
        long index = getCount(here) - 1;
        for ( ; 0 < index ; --index) {
            Hash_entry list = here->list[index];
            for ( ; list ; list = list->next) {
                if (total) fprintf(output, " ");
                fprintf(output, "%s -> ", symbol_Text(list->symbol));
                dump(output, list->value);
                total += 1;
            }
        }
    }
    fprintf(output, "}");

    if (total != state->count) {
        //VM_ERROR("Hash Count Error: seen %u cached %u", total, table->count);
    }

    return;
}



/*****************
 ** end of file **
 *****************/

