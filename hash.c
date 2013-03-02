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

/* */
#include <error.h>

#define Hash_Block_Size 10

extern bool entry_Create(Symbol symbol, Node value, Hash_entry next, Hash_entry* target);
extern bool hash_block_Create(struct hash_block **target);

/* */
#if 0
static inline bool entry_Create(Symbol symbol, Node value, Hash_entry next, Hash_entry* target) {
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct hash_entry),
                       0,
                       target))
        return false;

    Hash_entry result = *target;

    result->next   = next;
    result->symbol = symbol;
    result->value  = value;

    return true;
}

static inline bool hash_block_Create(struct hash_block **target) {
    if (!node_Allocate(_zero_space,
                       false,
                       asSize(sizeof(struct hash_block), sizeof(Hash_entry) * Hash_Block_Size),
                       0,
                       target)) return false;

    struct hash_block *result = *target;

    result->size = Hash_Block_Size;

    return true;
}

extern bool hash_Create(unsigned size, Hash *target) {
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct hash),
                       0,
                       target))
        return false;

    Hash result = *target;

    if (!hash_block_Create(&result->first)) return false;

    result->count    = 0;
    result->fullsize = Hash_Block_Size;

    return true;
}
#endif

extern bool hash_Find(Hash table, Symbol name, Node *target) {
    if (!table)              return false;
    if (!name)               return false;
    if (1 > table->fullsize) return false;

    unsigned  index = name->hashcode % table->fullsize;
    Hash_entry      list = (Hash_entry)0;
    Hash_block here = table->first;

    for ( ; ; here = here->next) {
        if (!here) {
            return false;
        }
        if (index < here->size) break;
        index -= here->size;
    }

    list = here->list[index];

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
    if (1 > table->fullsize) return false;

    unsigned  index = name->hashcode % table->fullsize;
    Hash_entry *location = (Hash_entry*)0;
    Hash_block here = table->first;

    for ( ; ; here = here->next) {
        if (!here) {
            return false;
        }
        if (index < here->size) break;
        index -= here->size;
    }

    location = &(here->list[index]);

    Hash_entry list = *(location);

    if (!list) {
        table->count += 1;
        return entry_Create(name, value, 0, location);
    }

    for ( ; ; list = list->next) {
        if (list->symbol == name) {
            list->value = value;
            return true;
        }
        if (!list->next) {
            table->count += 1;
            return entry_Create(name, value, 0, &(list->next));
        }
    }
}

extern bool hash_Add(Hash table, Symbol name, Node value) {
    if (!table)              return false;
    if (!name)               return false;
    if (1 > table->fullsize) return false;

    unsigned  index = name->hashcode % table->fullsize;
    Hash_entry *location = (Hash_entry*)0;
    Hash_block here = table->first;

    for ( ; ; here = here->next) {
        if (!here) {
            return false;
        }
        if (index < here->size) break;
        index -= here->size;
    }

    location = &(here->list[index]);

    Hash_entry list = *(location);

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

    table->count += 1;
    return true;
}

extern bool hash_Remove(Hash table, Symbol name) {
    if (!table) return false;
    if (!name)  return false;
    if (1 > table->fullsize) return true;

    unsigned  index = name->hashcode % table->fullsize;
    Hash_entry *location = (Hash_entry*)0;
    Hash_block here = table->first;

    for ( ; ; here = here->next) {
        if (!here) {
            return false;
        }
        if (index < here->size) break;
        index -= here->size;
    }

    location = &(here->list[index]);

    Hash_entry list = *(location);

    if (!list) return true;

    if (list->symbol != name) {
        for ( ; ; ) {
            location = &(list->next);
            list     = list->next;

            if (!list) return true;
            if (list->symbol == name) break;
        }
    }

    table->count -= 1;
    darken_Node(list->next);
    *(location) = list->next;
    return true;
}

// expand the hash table (for faster access)
extern bool hash_Expand(Hash table, unsigned size) {
    if (!table) return false;
    if (size < table->fullsize) return true;
    return false;
}

// contract the hash table (for sparce tables)
extern bool hash_Contract(Hash table, unsigned size) {
    if (!table) return false;
    if (size > table->fullsize) return true;
    return false;
}

/* */
extern void hash_Print(FILE* output, Hash table) {
    if (!output) return;
    if (!table) {
        fprintf(output, "nil");
        return;
    }

    fprintf(output, "hash_%p{", table);

    unsigned int total = 0;

    Hash_block here = table->first;
    for ( ; here ; here = here->next) {
        unsigned index = here->size;
        for ( ; 0 < index ; --index) {
            Hash_entry list = here->list[index];
            for ( ; list ; list = list->next) {
                if (total) fprintf(output, " ");
                fprintf(output, "%s -> ", symbol_Text(list->symbol));
                node_Print(output, list->value);
                total += 1;
            }
        }
    }
    fprintf(output, "}");

    if (total != table->count) {
        //VM_ERROR("Hash Count Error: seen %u cached %u", total, table->count);
    }

    return;
}



/*****************
 ** end of file **
 *****************/

