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
#include "dump.h"

/* */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

extern void retype_global_symboltable();

typedef enum {
    ic_found,
    ic_added,
    ic_error,
} insert_code;

typedef enum {
    cc_lesser  = -1,
    cc_equal   = 0,
    cc_greater = 1,
} compare_code;

struct _internal_Row {
    unsigned lock;
    Header   first;
};

struct _internal_Table {
    unsigned lock;
    unsigned size;
    struct _internal_Row row[1];
};

static Node external_references;

static struct _internal_Table *_global_typetable = 0;

Constant void_s = 0;

Constant opaque_s = 0;
Constant zero_s   = 0;

Constant boolean_s = 0;
Constant undefined_s = 0;
Constant unit_s = 0;

Constant t_ASTree = 0;

Constant t_nil = 0;

Constant t_integer = 0;
Constant t_pair = 0;
Constant t_symbol = 0;
Constant t_text = 0;
Constant t_tuple = 0;
Constant t_arrow = 0;

Constant t_buffer = 0;
Constant t_closed = 0;
Constant t_continuation = 0;
Constant t_infile = 0;
Constant t_outfile = 0;

static void make_sort(const char* value, Constant* target) {
    Symbol symbol = 0;

    if (!symbol_Convert(value, &symbol)) return;

    sort_Create(symbol, target);
}

static void make_basetype(const char* value, Constant sort, Constant* target) {
    Symbol symbol = 0;

    if (!symbol_Convert(value, &symbol)) return;

    type_Create(symbol, sort, target);
}

#define MK_BTYPE(x) make_basetype(#x, zero_s, &t_ ##x)
#define MK_OTYPE(x) make_basetype(#x, opaque_s, &t_ ##x)

extern void init_global_typetable(Clink *roots) {
    if (_global_typetable) return;

    clink_Manage(roots, &external_references, true);

    pair_Create(NIL,NIL, &external_references.pair);

    const unsigned int rows = 1000;
    const unsigned int fullsize
        = sizeof(struct _internal_Table)
        + (sizeof(struct _internal_Row) * rows);

    struct _internal_Table * result = malloc(fullsize);

    memset(result, 0, fullsize);

    result->size = rows;

    _global_typetable = result;

    make_sort("Void", &void_s);

    make_sort("Opaque", &opaque_s);
    make_sort("Zero",   &zero_s);

    make_sort("Boolean",   &boolean_s);
    make_sort("Undefined", &undefined_s);
    make_sort("Unit",      &unit_s);

    MK_BTYPE(ASTree);

    MK_BTYPE(nil);

    MK_BTYPE(integer);
    MK_BTYPE(pair);
    MK_BTYPE(symbol);
    MK_BTYPE(text);
    MK_BTYPE(tuple);
    MK_BTYPE(arrow);

    MK_OTYPE(buffer);
    MK_OTYPE(infile);
    MK_OTYPE(outfile);
    MK_OTYPE(continuation);
    MK_OTYPE(closed);

    retype_global_symboltable();
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
            if (isIdentical(group->kind.type, s_type))   continue;
            if (isIdentical(group->kind.type, s_sort))   continue;
            if (isIdentical(group->kind.type, s_axiom))  continue;
            if (isIdentical(group->kind.type, s_rule))   continue;

            fprintf(stderr, "%s:%u",filename, line);
            fatal("found a non-type-system object in row %d of the type-system-table", row);
        }
    }
}

extern bool type_Create(Symbol symbol, Constant sort, Constant* target) {
    if (!symbol) return false;
    if (!sort)   return false;

    const HashCode hashcode = symbol->hashcode;

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_type)) continue;

        Constant test = (Constant) asReference(group);

        if (test->code != tc_constant) continue;
        if (test->name != symbol) continue;

        make_Axiom(test, sort);

        ASSIGN(target, test);

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct constant));

    if (!entry) return false;

    entry->kind.type.symbol = s_type;
    entry->kind.constructor = s_type;
    entry->kind.constant    = 1;

    Constant result = (Constant) asReference(entry);

    result->hashcode = hashcode;
    result->code     = tc_constant;
    result->name     = symbol;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    make_Axiom(result, sort);

    ASSIGN(target, result);

    return true;
}

extern bool sort_Create(Symbol symbol, Constant* target) {
    if (!symbol) return false;

    const HashCode hashcode = symbol->hashcode;

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_sort)) continue;

        Constant test = (Constant) asReference(group);

        if (test->name != symbol) continue;

        ASSIGN(target, test);

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct constant));

    if (!entry) return false;

    entry->kind.type.symbol = s_sort;
    entry->kind.constructor = s_sort;
    entry->kind.constant = 1;

    Constant result = (Constant) asReference(entry);

    result->hashcode = symbol->hashcode;
    result->code     = tc_sort;
    result->name     = symbol;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(target, result);

    return true;
}

extern bool find_Axiom(Constant element, Constant class) {
    if (!element) return false;
    if (!class)   return false;

    if (class == void_s) return false;

    HashCode hashcode = class->hashcode;

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_axiom)) continue;

        Axiom test = (Axiom) asReference(group);

        if (test->element != element) continue;
        if (test->class   != class)   continue;
        return true;
    }

    return false;
}

extern bool make_Axiom(Constant element, Constant class) {
    if (!element) return false;
    if (!class)   return false;

    if (class == void_s) return false;

    if (class->code != tc_sort) return false;

    HashCode hashcode = class->hashcode;

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_axiom)) continue;

        Axiom test = (Axiom) asReference(group);

        if (test->element != element) continue;
        if (test->class   != class)   continue;
        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct axiom));

    if (!entry) return false;

    entry->kind.type.symbol = s_axiom;
    entry->kind.constructor = s_axiom;
    entry->kind.constant    = 1;

    Axiom result = (Axiom) asReference(entry);

    result->hashcode = hashcode;
    result->code     = tc_axiom;
    result->element  = element;
    result->class    = class;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    return true;
}

extern bool find_Rule(Symbol functor, Constant xxx, Constant yyy, Constant zzz) {
    if (!functor) return false;
    if (!xxx)     return false;
    if (!yyy)     return false;
    if (!zzz)     return false;

    if (xxx == void_s) return false;
    if (yyy == void_s) return false;
    if (zzz == void_s) return false;

    if (xxx->code != tc_sort) return false;
    if (yyy->code != tc_sort) return false;
    if (zzz->code != tc_sort) return false;

    HashCode hashcode = functor->hashcode;

    const int row = hashcode % _global_typetable->size;
    Header group  = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_rule)) continue;

        Rule test = (Rule) asReference(group);

        if (test->functor != functor) continue;
        if (test->xxx     != xxx)     continue;
        if (test->yyy     != yyy)     continue;
        if (test->zzz     != zzz)    continue;

        return true;
    }
    return false;
}

// each rule is for a functor
// functors:
//   Pi    - (xxx -> yyy):zzz (dependent function types)
//   Sigma - (xxx, yyy):zzz   (dependent tuple types)
extern bool make_Rule(Symbol functor, Constant xxx, Constant yyy, Constant zzz) {
    if (!functor) return false;
    if (!xxx)     return false;
    if (!yyy)     return false;
    if (!zzz)     return false;

    if (xxx == void_s) return false;
    if (yyy == void_s) return false;
    if (zzz == void_s) return false;

    if (xxx->code != tc_sort) return false;
    if (yyy->code != tc_sort) return false;
    if (zzz->code != tc_sort) return false;

    HashCode hashcode = functor->hashcode;

    const int row = hashcode % _global_typetable->size;
    Header group  = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_rule)) continue;

        Rule test = (Rule) asReference(group);

        if (test->functor != functor) continue;
        if (test->xxx     != xxx)     continue;
        if (test->yyy     != yyy)     continue;
        if (test->zzz     != zzz)     continue;

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct rule));

    if (!entry) return false;

    entry->kind.type.symbol = s_rule;
    entry->kind.constructor = s_rule;
    entry->kind.constant    = 1;

    Rule result = (Rule) asReference(entry);

    result->hashcode = hashcode;
    result->code     = tc_rule;
    result->functor  = functor;
    result->xxx      = xxx;
    result->yyy      = yyy;
    result->zzz      = zzz;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    return true;
}

extern bool type_Contains(const Constant type, const Node value) {
    if (!type) return false;

    Kind kind = asKind(type);

    if (!kind) return false;

    if (!isAType(kind->type)) return false;

    Constant vtype = kind->type.constant;

    if (type == vtype) return true;
}

static bool find_Parent(Constant element, unsigned offset, Target result) {
    Constant buffer[100];
    Constant hold = 0;

    const unsigned fullcount = 99;

    memset(buffer, 0, sizeof(Constant) * 100);

    unsigned row = 0;
    unsigned max = _global_typetable->size;
    for (; row < max; ++row) {
        Header group = _global_typetable->row[row].first;
        for ( ; group; group = group->after) {
            if (!isIdentical(group->kind.constructor, s_axiom)) continue;

            Axiom test = (Axiom) asReference(group);

            if (test->element != element) continue;

            if (0 < offset) {
                --offset;
                continue;
            }

            Constant parent = (Constant)(test->class);
            ASSIGN(result, parent);

            return true;
        }
    }

    return false;
}

extern bool compute_Type(Node value, Target result) {
    if (isNil(value)) {
        ASSIGN(result, t_nil);
        return true;
    }

    if (isAType(value)) {
        Constant type = value.constant;

        if (tc_constant == type->code) {
            return find_Parent(type, 0, result);
        }

        if (tc_sort == type->code) {
            return find_Parent(type, 0, result);
        }

        return false;
    }

    Node type = getType(value);

    if (isNil(type)) {
        ASSIGN(result, value);
        return false;
    }

    ASSIGN(result, type);
    return true;
}

/*****************
 ** end of file **
 *****************/

