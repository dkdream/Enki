/*-*- mode: c;-*-*/
#if !defined(_symbol_table_h_)
#define _symbol_table_h_
/***************************
 **
 ** Purpose
 **   a non-gc hash, this is used by
 **   the parser to store
 **     o. (symbol -> enum)    maps
 **     o. (symbol -> context) maps
 **     o. (symbol -> offset)  maps
 **/
#include "ea_symbol.h"

/* */
#include <string.h>
#include <stdlib.h>

/* */

typedef void* SymbolValue;
typedef struct symbol_map   *SymbolMap;
typedef struct symbol_table *SymbolTable;

struct symbol_map {
    SymbolMap   next;
    Symbol      name;
    SymbolValue value;
};

struct symbol_table {
    SymbolMap  freelist;
    unsigned   entries; // number of entries
    unsigned   count;   // number of columns
    SymbolMap *column;
};

static inline bool symap_Create(SymbolMap next, SymbolMap *target) {
    SymbolMap at = malloc(sizeof(struct symbol_map));

    if (!at) return false;

    memset(at, 0, sizeof(struct symbol_map));

    at->next = next;

    *target = at;

    return true;
}

static inline bool symtbl_Init(unsigned count, SymbolTable table) {
    if (!count) return false;
    if (!table) return false;

    memset(table, 0, sizeof(struct symbol_table));

    SymbolMap *columns = malloc(sizeof(SymbolMap) * count);

    if (!columns) return false;

    memset(columns, 0, sizeof(SymbolMap) * count);

    table->count    = count;
    table->column   = columns;

    for ( ; count ; --count) {
        SymbolMap at;
        if (!symap_Create(table->freelist, &at)) return false;
        table->freelist = at;
    }

    return true;
}

static inline bool symtbl_Find(SymbolTable table, Symbol name, SymbolValue *target) {
    if (!table)          return false;
    if (!name)           return false;
    if (!table->count)   return false;
    if (!table->entries) return false;

    unsigned long code  = name->hashcode;
    unsigned      index = code % table->count;

    SymbolMap at = table->column[index];

    for ( ; at ; at = at->next) {
        if (at->name != name) continue;
        *target = at->value;
        return true;
    }

    return false;
}

static bool symtbl_Replace(SymbolTable table, Symbol name, SymbolValue value) {
    if (!table)          return false;
    if (!name)           return false;
    if (!table->count)   return false;

    unsigned long code  = name->hashcode;
    unsigned      index = code % table->count;

    SymbolMap at = table->column[index];

    for ( ; at ; at = at->next) {
        if (at->name != name) continue;
        at->value = value;
        return true;
    }

    at = table->freelist;

    if (at) {
        table->freelist = at->next;
    } else {
        if (!symap_Create(0, &at)) return false;
    }

    memset(at, 0, sizeof(struct symbol_map));

    at->name   = name;
    at->value  = value;
    at->next   = table->column[index];

    table->column[index]  = at;
    table->entries       += 1;

    return true;
}

static inline bool symtbl_Remove(SymbolTable table, Symbol name) {
    inline bool mapfree(SymbolMap map) {
        memset(map, 0, sizeof(struct symbol_map));
        map->next = table->freelist;
        table->freelist = map;
        table->entries -= 1;
        return true;
    }

    if (!table)          return false;
    if (!name)           return false;
    if (!table->count)   return true;
    if (!table->entries) return true;

    unsigned long code  = name->hashcode;
    unsigned      index = code % table->count;

    SymbolMap at = table->column[index];

    if (at->name == name) {
        table->column[index] = at->next;
        return mapfree(at);
    }

    for ( ; at->next ; ) {
        SymbolMap  prev = at;
        at = at->next;
        if (at->name != name) continue;
        prev->next = at->next;
        return mapfree(at);
    }

    return true;
}

static inline void symtbl_Release(SymbolTable table) {
    if (!table) return;

    for ( ; table->freelist ; ) {
        SymbolMap at = table->freelist;
        table->freelist = at->next;
        free(at);
    }

    unsigned index = table->count;

    for ( ; index-- ; ) {
        SymbolMap   at = table->column[index];
        SymbolMap next = 0;
        for ( ; at ; at = next) {
            next = at->next;
            free(at);
        }
    }

    free(table->column);

    memset(table, 0, sizeof(struct symbol_table));
}


/***************************
 ** end of file
 **************************/
#endif
