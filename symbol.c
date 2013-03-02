/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "symbol.h"
#include "treadmill.h"

/* */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* */
Symbol _empty_symbol = (Symbol)0;

struct _internal_SymbolRow {
    unsigned lock;
    Header   first;
};

struct _internal_SymbolTable {
    unsigned lock;
    unsigned size;
    struct _internal_SymbolRow row[];
};

static struct _internal_SymbolTable *_global_symboltable = 0;

extern void init_global_symboltable() {
    if (_global_symboltable) return;

    _empty_symbol = (Symbol) asReference(fresh_atom(sizeof(struct symbol)));

    setKind(_empty_symbol, nt_symbol);

    const unsigned int rows = 1000;
    const unsigned int fullsize
        = sizeof(struct _internal_SymbolTable)
        + (sizeof(struct _internal_SymbolRow) * rows);

    struct _internal_SymbolTable * result = malloc(fullsize);

    memset(result, 0, fullsize);

    result->size = rows;

    _global_symboltable = result;
}

extern void final_global_symboltable() {
     if (0 ==  _global_symboltable) return;
}

extern void check_SymbolTable__(const char* filename, unsigned line) {
    if (!_global_symboltable) return;

    printf("begin scan for(%s:%u)\n", filename, line);

    int row = _global_symboltable->size;

    for ( ; row-- ; ) {
        Header group = _global_symboltable->row[row].first;

        for ( ; group; group = group->after) {
            Symbol test = (Symbol) asReference(group);

            printf("   symbol %s\n", symbol_Text(test));
        }
    }

    printf("end\n");
}

static inline HashCode hash_full(TextBuffer value) {
    HashCode result = 5381;

    unsigned int length = value.position;
    const char*  begin  = value.buffer;

    for ( ; length-- ; ) {
        int val = begin[length];
        result = ((result << 5) + result) + val;
    }

    return result;
}

extern bool symbol_Create(TextBuffer value, Symbol *target) {
    if (!value.buffer) {
        *target = _empty_symbol;
        return true;
    }

    if (0 == value.position) {
        *target = _empty_symbol;
        return true;
    }

    unsigned int size = value.position;
    HashCode hashcode = hash_full(value);

    const int       row = hashcode % _global_symboltable->size;
    const int     cells = size / sizeof(const unsigned long);
    const int remainder = size % sizeof(const unsigned long);

    Header group = _global_symboltable->row[row].first;

    for ( ; group; group = group->after) {
        Symbol test = (Symbol) asReference(group);

        if (test->size != size)         continue;
        if (test->hashcode != hashcode) continue;

        const unsigned long *checking = test->value;
        const unsigned long *end      = checking + cells;
        const unsigned long *source   = (const unsigned long *) value.buffer;

        while (checking < end) {
            if (*checking++ != *source++) goto next;
        }

        int inx = 0;
        for ( ; inx < remainder; ++inx) {
            if ((((const char*)checking)[inx]) != (((const char*)source)[inx])) goto next;
        }

        *target = test;

        return true;
    next:
        continue;
    }

    const unsigned int fullsize = (sizeof(struct symbol) + (size + 1));

    Header entry = fresh_atom(fullsize);

    if (!entry) return false;

    entry->after  = _global_symboltable->row[row].first;

    Symbol result = (*target) = (Symbol)asReference(entry);

    setKind(result, nt_symbol);
    result->size     = size;
    result->hashcode = hashcode;

    memcpy(result->value, value.buffer, size);

    ((char*)(result->value))[size] = 0;

    _global_symboltable->row[row].first = entry;

    return true;
}

extern bool symbol_Convert(const char* value, Symbol* target) {
    if (!value) return false;

    TextBuffer data = BUFFER_INITIALISER;
    data.position = strlen(value);
    data.buffer   = (char*)value;

    return symbol_Create(data, target);
}

/*****************
 ** end of file **
 *****************/

