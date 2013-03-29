/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "type.h"
#include "symbol.h"
#include "treadmill.h"
#include "debug.h"

/* */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

struct name {
    Sort sort;
};

struct axion {
    HashCode hashcode;
    Sort sort;
    Type type;
};

struct rule {
    HashCode hashcode;
    Symbol   group;
    Sort in, out, type;
};

struct _internal_Row {
    unsigned lock;
    Header   first;
};

struct _internal_Table {
    unsigned lock;
    unsigned size;
    struct _internal_Row row[1];
};

static struct _internal_Table *_global_typetable = 0;

extern void init_global_typetable() {
    if (_global_typetable) return;

    const unsigned int rows = 1000;
    const unsigned int fullsize
        = sizeof(struct _internal_Table)
        + (sizeof(struct _internal_Row) * rows);

    struct _internal_Table * result = malloc(fullsize);

    memset(result, 0, fullsize);

    result->size = rows;

    _global_typetable = result;
}

extern void final_global_typetable() {
     if (0 == _global_typetable) return;
}

extern void check_TypeTable__(const char* filename, unsigned line) {
    if (!_global_typetable) return;

    int row = _global_typetable->size;

    for ( ; row-- ; ) {
        Header group = _global_typetable->row[row].first;
        for ( ; group; group = group->after) {
            if (isIdentical(group->kind.type, s_base))  continue;
            if (isIdentical(group->kind.type, s_type))  continue;
            if (isIdentical(group->kind.type, s_sort))  continue;
            if (isIdentical(group->kind.type, s_axiom)) continue;
            if (isIdentical(group->kind.type, s_rule))  continue;
            if (isIdentical(group->kind.type, s_name))  continue;

            fprintf(stderr, "%s:%u",filename, line);
            fatal("found a non-type-system object in row %d of the type-system-table", row);
        }
    }
}

static inline HashCode hash_merge(HashCode init, unsigned length, void* buffer) {
    HashCode result = init;

    const char* begin = (const char*)buffer;

    for ( ; length-- ; ) {
        int val = begin[length];
        result = ((result << 5) + result) + val;
    }

    return result;
}

extern bool sort_Create(Symbol symbol, Sort* target) {
    if (!symbol) return false;

    const HashCode hashcode = symbol->hashcode;

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (isIdentical(group->kind.type, s_sort)) continue;

        Sort test = (Sort) asReference(group);

        if (test->name != symbol) continue;

        ASSIGN(target, test);

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct sort));

    if (!entry) return false;

    entry->kind.type = (Node)s_sort;


    Sort result = (Sort) asReference(entry);

    result->hashcode = symbol->hashcode;
    result->name     = symbol;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry->after;

    ASSIGN(target, result);

    return true;
}

extern bool type_Create(Symbol symbol, Sort sort, Type* target) {
    if (!symbol) return false;
    if (!sort)   return false;

    const HashCode hashcode = hash_merge(symbol->hashcode,
                                         sizeof(sort->hashcode),
                                         &(sort->hashcode));

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (isIdentical(group->kind.type, s_base)) continue;

        Type test = (Type) asReference(group);

        if (test->sort != sort)   continue;
        if (test->name != symbol) continue;

        ASSIGN(target, test);

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct type));

    if (!entry) return false;

    entry->kind.type = (Node)s_base;

    Type result = (Type) asReference(entry);

    result->hashcode = hashcode;
    result->sort     = sort;
    result->name     = symbol;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry->after;

    ASSIGN(target, result);

    return true;
}

extern bool rule_Create(Symbol symbol, Sort in, Sort out, Sort type, Rule* target) {
    if (!symbol) return false;
    if (!in)     return false;
    if (!out)    return false;
    if (!type)   return false;

    HashCode hashcode = symbol->hashcode;
    hashcode = hash_merge(hashcode,
                          sizeof(in->hashcode),
                          &(in->hashcode));

    hashcode = hash_merge(hashcode,
                          sizeof(out->hashcode),
                          &(out->hashcode));

    hashcode = hash_merge(hashcode,
                          sizeof(type->hashcode),
                          &(type->hashcode));

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (isIdentical(group->kind.type, s_rule)) continue;

        Rule test = (Rule) asReference(group);

        if (test->group != symbol) continue;
        if (test->in    != in)     continue;
        if (test->out   != out)    continue;
        if (test->type  != type)   continue;

        ASSIGN(target, test);

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct rule));

    if (!entry) return false;

    entry->kind.type = (Node)s_rule;

    Rule result = (Rule) asReference(entry);

    result->hashcode = hashcode;
    result->group    = symbol;
    result->in       = in;
    result->out      = out;
    result->type     = type;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry->after;

    ASSIGN(target, result);

    return true;
}

extern bool name_Create(Sort sort, Name* target) {
    if (!sort) return false;

    HashCode hashcode = sort->hashcode;
    const int row     = hashcode % _global_typetable->size;

    Header entry = fresh_atom(0, sizeof(struct name));

    if (!entry) return false;

    entry->kind.type = (Node)s_name;

    Name result = (Name) asReference(entry);

    result->sort = sort;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry->after;

    return true;
}

extern bool type_Pi(Type* target, ...) {
    return false;
}

extern bool type_Sigma(Type* target, ...) {
    return false;
}

extern bool type_Mu(Type* target, ...) {
    return false;
}

extern bool type_Delta(Type* target, ...) {
    return false;
}


/*****************
 ** end of file **
 *****************/

