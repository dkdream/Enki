/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "symbol_list.h"
#include "symbol.h"

/* */
#include <stdlib.h>
#include <string.h>

/* */

extern bool symbol_list_Allocate(unsigned int size, SymbolList* target) {
    if (!target) return false;

    const unsigned int fullsize
        = sizeof(struct symbol_list)
        + (sizeof(Symbol) * size);

    SymbolList result = malloc(fullsize);

    if (!result) return false;

    memset(result, 0, fullsize);

    result->size = size;
    result->used = 0;

    *target = result;

    return true;
}

extern unsigned int symbol_list_Count(SymbolList list) {
    unsigned int count = 0;

    for ( ; list ; list = list->next ) {
        unsigned int inx = 0;
        for ( ; inx < list->used ; ++inx) {
            if (list->value[inx]) {
                count += 1;
            }
        }
    }

    return count;
}

extern bool symbol_list_Copy(SymbolList list, SymbolList* target) {
    if (!target) return false;
    if (!list) {
        *target = list;
        return true;
    }

    unsigned int count = symbol_list_Count(list);

    if (1 > count) return false;

    if (!symbol_list_Allocate(count, target)) return false;

    SymbolList result = *target;

    unsigned int jnx = 0;
    for ( ; list ; list = list->next ) {
        unsigned int inx  = 0;
        for ( ; inx < list->used ; ++inx) {
            Symbol value = list->value[inx];
            if (value) {
                result->value[jnx] = value;
                ++jnx;
            }
        }
    }
    result->used = jnx;

    return true;
}

extern bool symbol_list_Find(SymbolList list, Symbol symbol, unsigned int * index) {
    unsigned int current = 0;

    for ( ; list ; list = list->next ) {
        unsigned int inx = 0;

        for ( ; inx < list->used ; ++inx) {
            if (symbol == list->value[inx]) {
                if (index) *index = current + inx;
                return true;
            }
        }

        current += list->size;
    }

    return false;
}

extern bool symbol_list_Remove(SymbolList list, Symbol symbol, unsigned int * index) {
    unsigned int current = 0;

    for ( ; list ; list = list->next ) {
        unsigned int inx = 0;

        for ( ; inx < list->used ; ++inx) {
            if (symbol == list->value[inx]) {
                if (index) *index = current + inx;
                list->value[inx] = (Symbol)0;
                return true;
            }
        }

        current += list->size;
    }

    return false;
}

extern bool symbol_list_Add(SymbolList list, Symbol symbol, unsigned int *target) {
    if (!list)   return false;
    if (!target) return false;

    if (symbol_list_Find(list, symbol, target)) {
        return true;
    }

    unsigned int current = 0;

    for ( ; ; list = list->next ) {
        unsigned int inx  = 0;

        for ( ; inx < list->used ; ++inx) {
             if (!list->value[inx]) {
                 current += inx;
                 list->value[inx] = symbol;
                 *target = current;
                 return true;
             }
        }

        if (list->used < list->size) {
            inx      = list->used;
            current += inx;
            list->used += 1;
            list->value[inx] = symbol;
            *target = current;
            return true;
        }

        current += list->size;

        if (list->next) continue;

        if (!symbol_list_Allocate(list->size, &list->next)) return false;
    }
}

extern bool symbol_list_Element(SymbolList list, unsigned int index, Symbol *target) {
    for ( ; list ; list = list->next ) {
        if (index > list->size) {
            index -= list->size;
        } else {
            if (index < list->used) {
                *target = list->value[index];
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

extern void symbol_list_Release(SymbolList list) {
    SymbolList next;
    for ( ; list ; list = next ) {
        next = list->next;
        free(list);
    }
}

extern void symbol_list_Print(FILE* output, SymbolList list) {
    if (!list) {
        fprintf(output, "nil");
        return;
    }

    fprintf(output, "( ");
    for ( ; list ; list = list->next ) {
        unsigned int inx = 0;
        for ( ; inx < list->used ; ++inx) {
            fprintf(output, ":'%s' ", (const char *) list->value[inx]->value);
        }
        for ( ; inx < list->size ; ++inx) {
            fprintf(output, "nil ");
        }
    }
    fprintf(output, ")");
}

/*****************
 ** end of file **
 *****************/

