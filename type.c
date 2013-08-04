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

typedef struct type_constant* TypeCnt;
typedef struct type_index*    TypeInx;
typedef struct type_label*    TypeLbl;
typedef struct type_branch*   TypeBrn;


//  s_base  is a base    type (type constant)
//  s_type  is a compond type (index,label,union,tuple,record)

//  s_sort  is a sort
//  s_axiom is a axiom pair   (c:s)
//  s_rule  is a rule  triple (in=s1,out=s2,kind=s3)
//  s_name  is a variable reference used in a formula

struct name {
    Sort sort;
};

struct axiom {
    HashCode hashcode;
    Sort element;
    Sort class;
};

struct rule {
    HashCode hashcode;
    Symbol   functor;
    Sort xxx, yyy, kind;
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

Sort void_s   = 0;
Sort zero_s   = 0;
Sort symbol_s = 0;
Sort opaque_s = 0;

Type t_any = 0;
Type t_block = 0;
Type t_buffer = 0;
Type t_comma = 0;
Type t_delay = 0;
Type t_fixed = 0;
Type t_forced = 0;
Type t_form = 0;
Type t_infile = 0;
Type t_integer = 0;
Type t_lambda = 0;
Type t_opaque = 0;
Type t_outfile = 0;
Type t_pair = 0;
Type t_path = 0;
Type t_primitive = 0;
Type t_semi = 0;
Type t_symbol = 0;
Type t_text = 0;
Type t_true = 0;
Type t_tuple = 0;
Type t_void = 0;

static void make_sort(const char* value, Sort* target) {
    Symbol symbol = 0;

    if (!symbol_Convert(value, &symbol)) return;

    sort_Create(symbol, target);
}

static void make_basetype(const char* value, Sort sort, Type* target) {
    Symbol symbol = 0;

    if (!symbol_Convert(value, &symbol)) return;

    type_Create(symbol, sort, target);
}

#define MK_BTYPE(x) make_basetype(#x, zero_s, &t_ ##x)

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

    make_sort("Void", &void_s);
    make_basetype("void", void_s, &t_void);

    make_sort("Symbol", &symbol_s);
    make_basetype("symbol", symbol_s, &t_symbol);

    make_sort("Opaque", &opaque_s);
    make_basetype("opaque", opaque_s, &t_opaque);

    make_sort("Zero", &zero_s);

    MK_BTYPE(any);
    MK_BTYPE(block);
    MK_BTYPE(buffer);
    MK_BTYPE(comma);
    MK_BTYPE(delay);
    MK_BTYPE(fixed);
    MK_BTYPE(forced);
    MK_BTYPE(form);
    MK_BTYPE(infile);
    MK_BTYPE(integer);
    MK_BTYPE(lambda);
    MK_BTYPE(outfile);
    MK_BTYPE(pair);
    MK_BTYPE(path);
    MK_BTYPE(primitive);
    MK_BTYPE(semi);
    MK_BTYPE(text);
    MK_BTYPE(true);
    MK_BTYPE(tuple);

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

extern bool sort_Create(Symbol symbol, Sort* target) {
    if (!symbol) return false;

    const HashCode hashcode = symbol->hashcode;

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_sort)) continue;

        Sort test = (Sort) asReference(group);

        if (test->name != symbol) continue;

        ASSIGN(target, test);

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct sort));

    if (!entry) return false;

    entry->kind.type        = (Node)s_sort;
    entry->kind.constructor = (Node)s_sort;
    entry->kind.constant = 1;

    Sort result = (Sort) asReference(entry);

    result->hashcode = symbol->hashcode;
    result->name     = symbol;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(target, result);

    return true;
}

extern bool make_Axiom(Sort element, Sort class) {
    if (!element) return false;
    if (!class)   return false;

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

    entry->kind.type        = (Node)s_axiom;
    entry->kind.constructor = (Node)s_axiom;
    entry->kind.constant    = 1;

    Axiom result = (Axiom) asReference(entry);

    result->hashcode = hashcode;
    result->element  = element;
    result->class    = class;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    return true;
}

// each rule is for a functor
// functors:
//   Pi    - (xxx -> yyy):kind (dependent function types)
//   Sigma - (xxx, yyy):kind   (dependent tuple types)
extern bool make_Rule(Symbol functor, Sort xxx, Sort yyy, Sort kind) {
    if (!functor) return false;
    if (!xxx)     return false;
    if (!yyy)     return false;
    if (!kind)    return false;

    HashCode hashcode = functor->hashcode;

    const int row = hashcode % _global_typetable->size;
    Header group  = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_rule)) continue;

        Rule test = (Rule) asReference(group);

        if (test->functor != functor) continue;
        if (test->xxx     != xxx)     continue;
        if (test->yyy     != yyy)     continue;
        if (test->kind    != kind)    continue;

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct rule));

    if (!entry) return false;

    entry->kind.type        = (Node)s_rule;
    entry->kind.constructor = (Node)s_rule;
    entry->kind.constant    = 1;

    Rule result = (Rule) asReference(entry);

    result->hashcode = hashcode;
    result->functor  = functor;
    result->xxx      = xxx;
    result->yyy      = yyy;
    result->kind     = kind;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    return true;
}

extern bool type_Create(Symbol symbol, Sort sort, Type* target) {
    if (!symbol) return false;
    if (!sort)   return false;

    const HashCode hashcode = symbol->hashcode;

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_base)) continue;

        Type test = (Type) asReference(group);

        if (test->sort != sort)        continue;
        if (test->code != tc_constant) continue;

        if (((TypeCnt)test)->name != symbol)      continue;

        ASSIGN(target, test);

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct type_constant));

    if (!entry) return false;

    entry->kind.type        = (Node) s_base;
    entry->kind.constructor = (Node) s_base;
    entry->kind.constant    = 1;

    TypeCnt result = (TypeCnt) asReference(entry);

    result->hashcode = hashcode;
    result->sort     = sort;
    result->code     = tc_constant;
    result->name     = symbol;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(target, result);

    return true;
}

static bool branch_Cons(const enum type_code kind,
                        const Sort sort,
                        const Type left,
                        const Type right,
                        Type* result)
{
    const HashCode hashcode = hash_merge(left->hashcode,
                                         right->hashcode);

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_branch)) continue;

        Type test = (Type) asReference(group);

        if (test->sort != sort) continue;
        if (test->code != kind) continue;

        TypeBrn branch = (TypeBrn) test;

        if (isIdentical(branch->left, left)) {
            if (isIdentical(branch->right, right)) {
                ASSIGN(result, test);
                return true;
            }
        }
    }

    Header entry = fresh_atom(0, sizeof(struct type_branch));

    if (!entry) return false;

    entry->kind.type        = (Node) s_type;
    entry->kind.constructor = (Node) s_branch;
    entry->kind.constant    = 1;

    TypeBrn branch = (TypeBrn) asReference(entry);

    branch->hashcode = hashcode;
    branch->sort     = sort;
    branch->code     = kind;
    branch->left     = left;
    branch->right    = right;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(result, branch);
    return true;
}

static bool union_IsMember(TypeBrn set, Type type) {
    for (; set ;) {
        Type left  = set->left;
        Type right = set->right;

        if (isIdentical(left, type)) return true;

        if (right->code == tc_union) {
            set = (TypeBrn) right;
            continue;
        }

        return isIdentical(right,type);
    }

    return false;
}

static bool union_Cons(const Type left, const Type right, Type* result) {
    const Sort sort = left->sort;
    return branch_Cons(tc_union,
                       sort,
                       left,
                       right,
                       result);
}

static bool union_Add(const Type left, TypeBrn right, Type* result) {
    Type hold;
    Type rleft  = right->left;
    Type rright = right->right;

    if (left == rleft) {
        ASSIGN(result, right);
        return true;
    }

    if (left == rright) {
        ASSIGN(result, right);
        return true;
    }

    /* (left != rleft) && (left != rright) */

    if (left < rleft) {
        return union_Cons(left, (Type) right, result);
    }

    if (right->code == tc_union) {
        if (!union_Add(left, (TypeBrn) rright, &hold)) return false;
    } else {
        if (left < rright) {
            if (!union_Cons(left, rright, &hold)) return false;
        } else {
            if (!union_Cons(rright, left, &hold)) return false;
        }
    }

    return union_Cons(rleft, hold, result);
}

static bool union_Merge(TypeBrn left, TypeBrn right, Type* result) {
    Type hold;
    Type lleft  = left->left;
    Type rleft  = right->left;

    if (lleft == rleft) {
        /* head == lleft & head == rleft */
        if (!type_Union(left->right, right->right, &hold)) return false;
        return union_Cons(rleft, hold, result);
    }

    if (lleft < rleft) {
        /* head == lleft */
        if (!type_Union(left->right, (Type)right, &hold)) return false;
        return union_Cons(lleft, hold, result);
    }

    if (lleft > rleft) {
        /* head == rleft */
        if (!type_Union(right->right, (Type)left, &hold)) return false;
        return union_Cons(rleft, hold, result);
    }

    return false;
}

/*
 * build unions a list of non unions
 * union(a,a)   == a
 * union(a,b)   == union(b,a)
 * union(a,b,c) == union(a,union(b,c))
 */
extern bool type_Union(Type left, Type right, Type* result) {
    if (!left)  return false;
    if (!right) return false;

    /* forall(A) union(A,A) == A */
    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    /* currently unions are mono-sorted */
    if (!isIdentical(left->sort,right->sort)) return false;

    unsigned option = 0;

    option += (left->code  == tc_union) ? 1 : 0;
    option += (right->code == tc_union) ? 2 : 0;

    Type hold;

    switch (option) {
    case 0:
        /* enforce address order of union members */
        if (left > right) {
            hold = left;
            left = right;
            right = hold;
        }
        return union_Cons(left, right, result);

    case 1: /* left == tc_union, right != tc_union */
        hold = left;
        left = right;
        right = hold;
        /* fall thru */

    case 2: /* left != tc_union, right == tc_union */
        return union_Add(left, (TypeBrn)right, result);

    case 3:
        return union_Merge((TypeBrn) left, (TypeBrn) right, result);
    }

    return false;
}

extern bool type_Index(const unsigned index, const Type at, Type* result) {
    if (!at) return false;

    const Sort sort = at->sort;
    const HashCode hashcode = hash_merge((HashCode) index,
                                         at->hashcode);

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_index)) continue;

        Type test = (Type) asReference(group);

        if (test->sort != sort)     continue;
        if (test->code != tc_index) continue;

        TypeInx inx = (TypeInx) test;

        if (index == inx->index) {
            if (isIdentical(inx->slot, at)) {
                ASSIGN(result, test);
                return true;
            }
        }
    }

    Header entry = fresh_atom(0, sizeof(struct type_index));

    if (!entry) return false;

    entry->kind.type        = (Node) s_type;
    entry->kind.constructor = (Node) s_index;
    entry->kind.constant = 1;

    TypeInx inx = (TypeInx) asReference(entry);

    inx->hashcode = hashcode;
    inx->sort     = sort;
    inx->code     = tc_index;
    inx->index    = index;
    inx->slot     = at;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(result, inx);
    return true;
}

static bool tuple_Cons(const Type left, const Type right, Type* result) {
    const Sort sort = left->sort;
    return branch_Cons(tc_tuple,
                       sort,
                       left,
                       right,
                       result);
}

/* Tuples := index(i,a) | tuple(a,b) where a,b in Tuple */
/* tuple(a,b) == tuple(b,a) */
/* tuple(a,tuple(b,c)) == tuple(tuple(a,b),c) */
extern bool type_Tuple(const Type left, const Type right, Type* result) {
    if (!left)  return false;
    if (!right) return false;

    unsigned option = 0;

    option += (left->code == tc_union)  ? 1 : 0;
    option += (right->code == tc_union) ? 2 : 0;

    return false;
}

extern bool type_Label(const Symbol label, const Type at, Type* result) {
    if (!label) return false;
    if (!at)    return false;

    const Sort sort = at->sort;
    const HashCode hashcode = hash_merge(label->hashcode,
                                         at->hashcode);

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_label)) continue;

        Type test = (Type) asReference(group);

        if (test->sort != sort)     continue;
        if (test->code != tc_label) continue;

        TypeLbl field = (TypeLbl) test;

        if (isIdentical(field->label,label)) {
            if (isIdentical(field->slot, at)) {
                ASSIGN(result, test);
                return true;
            }
        }
    }

    Header entry = fresh_atom(0, sizeof(struct type_index));

    if (!entry) return false;

    entry->kind.type        = (Node) s_type;
    entry->kind.constructor = (Node) s_label;
    entry->kind.constant    = 1;

    TypeLbl field = (TypeLbl) asReference(entry);

    field->hashcode = hashcode;
    field->sort     = sort;
    field->code     = tc_label;
    field->label    = label;
    field->slot     = at;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(result, field);
    return true;
}

extern bool type_Record(const Type left, const Type right, Type* result) {
    return false;
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


extern bool type_Contains(const Type type, const Node value) {
    if (!type) return false;

    Kind kind = asKind(type);

    if (!kind) return false;

    if (!isAType(kind->type)) return false;

    Type vtype = kind->type.type;

    if (type == vtype) return true;
}

/*****************
 ** end of file **
 *****************/

