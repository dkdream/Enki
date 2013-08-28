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


//  s_sort   is a sort         (sort constant)
//  s_axiom  is a axiom pair   (c:s)
//  s_rule   is a rule  triple (in=s1,out=s2,kind=s3)
//  s_base   is a base type    (type constant)
//  s_branch is a branch type  (any,tuple,record,

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
Type t_buffer = 0;
Type t_false = 0;
Type t_infile = 0;
Type t_integer = 0;
Type t_nil = 0;
Type t_opaque = 0;
Type t_outfile = 0;
Type t_pair = 0;
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

    make_sort("Symbol", &symbol_s);
    make_basetype("symbol", symbol_s, &t_symbol);

    make_sort("Opaque", &opaque_s);
    make_basetype("opaque", opaque_s, &t_opaque);

    make_sort("Zero", &zero_s);

    MK_BTYPE(any);
    MK_BTYPE(buffer);
    MK_BTYPE(false);
    MK_BTYPE(infile);
    MK_BTYPE(integer);
    MK_BTYPE(nil);
    MK_BTYPE(outfile);
    MK_BTYPE(pair);
    MK_BTYPE(text);
    MK_BTYPE(true);
    MK_BTYPE(tuple);
    MK_BTYPE(void);

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
            if (isIdentical(group->kind.type, s_sort))   continue;
            if (isIdentical(group->kind.type, s_axiom))  continue;
            if (isIdentical(group->kind.type, s_rule))   continue;
            if (isIdentical(group->kind.type, s_base))   continue;
            if (isIdentical(group->kind.type, s_branch)) continue;
            if (isIdentical(group->kind.type, s_index))  continue;
            if (isIdentical(group->kind.type, s_label))  continue;

            if (isIdentical(group->kind.type, s_type))  continue;
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

static long entry_Index(const Type entry) {
    if (!entry) return 0;
    switch (entry->code) {
    case tc_index:
        return (long)(((TypeInx) entry)->index);

    case tc_label:
        return (long)(((TypeLbl) entry)->label);

    default:
        return (long)(entry);
    }
}

static bool entry_Greater(const Type left, const Type right) {
    return entry_Index(left) > entry_Index(right);
}

static bool match_Tag(const Type left, const Type right, Type* result) {
    if (!left) return false;
    if (!right) return false;

    if (left->code != right->code) return false;

    if (left->code == tc_index) {
        TypeInx lindex = ((TypeInx) left);
        TypeInx rindex = ((TypeInx) right);

        if (lindex->index != rindex->index) return false;
        if (lindex->slot == rindex->slot) {
            *result = left;
            return true;
        }

        Type hold;

        if (!type_Any(lindex->slot,rindex->slot, &hold)) return false;

        return type_Index(lindex->index, hold, result);
    }

    if (left->code == tc_label) {
        TypeLbl llabel = ((TypeLbl) left);
        TypeLbl rlabel = ((TypeLbl) right);

        if (llabel->label != rlabel->label) return false;

        if (llabel->slot == rlabel->slot) {
            *result = left;
            return true;
        }

        Type hold;

        if (!type_Any(llabel->slot,rlabel->slot, &hold)) return false;

        return type_Label(llabel->label, hold, result);
    }

    return false;
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

    entry->kind.type        = (Node) s_branch;
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

static bool any_IsMember(TypeBrn set, Type type) {
    for (; set ;) {
        Type left  = set->left;
        Type right = set->right;

        if (isIdentical(left, type)) return true;

        if (right->code == tc_any) {
            set = (TypeBrn) right;
            continue;
        }

        return isIdentical(right,type);
    }

    return false;
}

static bool any_Cons(const Type left, const Type right, Type* result) {
    if (match_Tag(left,right,result)) return true;

    const Sort sort = left->sort;
    return branch_Cons(tc_any,
                       sort,
                       left,
                       right,
                       result);
}

static bool any_Add(const Type left, TypeBrn right, Type* result) {
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
        return any_Cons(left, (Type) right, result);
    }

    if (right->code == tc_any) {
        if (!any_Add(left, (TypeBrn) rright, &hold)) return false;
    } else {
        if (left < rright) {
            if (!any_Cons(left, rright, &hold)) return false;
        } else {
            if (!any_Cons(rright, left, &hold)) return false;
        }
    }

    return any_Cons(rleft, hold, result);
}

static bool any_Merge(TypeBrn left, TypeBrn right, Type* result) {
    Type hold;
    Type lleft  = left->left;
    Type rleft  = right->left;

    if (lleft == rleft) {
        /* head == lleft & head == rleft */
        if (!type_Any(left->right, right->right, &hold)) return false;
        return any_Cons(rleft, hold, result);
    }

    if (lleft < rleft) {
        /* head == lleft */
        if (!type_Any(left->right, (Type)right, &hold)) return false;
        return any_Cons(lleft, hold, result);
    }

    if (lleft > rleft) {
        /* head == rleft */
        if (!type_Any(right->right, (Type)left, &hold)) return false;
        return any_Cons(rleft, hold, result);
    }

    return false;
}

/*
 * build anys a list of non anys
 * any(a,a)   == a
 * any(a,b)   == any(b,a)
 * any(a,b,c) == any(a,any(b,c))
 */
extern bool type_Any(const Type left, const Type right, Type* result) {
    if (!left)  return false;
    if (!right) return false;

    /* forall(A) any(A,A) == A */
    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    unsigned option = 0;

    option += (left->code  == tc_any) ? 1 : 0;
    option += (right->code == tc_any) ? 2 : 0;

    Type hold;

    switch (option) {
    case 0:
        /* enforce address order of any members */
        if (entry_Greater(left, right)) {
            return any_Cons(right, left, result);
        }
        return any_Cons(left, right, result);

    case 1: /* left == tc_any, right != tc_any */
        return any_Add(right, (TypeBrn)left, result);

    case 2: /* left != tc_any, right == tc_any */
        return any_Add(left, (TypeBrn)right, result);

    case 3:
        return any_Merge((TypeBrn) left, (TypeBrn) right, result);
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

    entry->kind.type        = (Node) s_index;
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
    if (match_Tag(left,right,result)) return true;

    const Sort sort = left->sort;
    return branch_Cons(tc_tuple,
                       sort,
                       left,
                       right,
                       result);
}

static bool tuple_Add(const Type left, TypeBrn right, Type* result) {
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
        return tuple_Cons(left, (Type) right, result);
    }

    if (right->code == tc_any) {
        if (!tuple_Add(left, (TypeBrn) rright, &hold)) return false;
    } else {
        if (left < rright) {
            if (!tuple_Cons(left, rright, &hold)) return false;
        } else {
            if (!tuple_Cons(rright, left, &hold)) return false;
        }
    }

    return tuple_Cons(rleft, hold, result);
}

static bool tuple_Merge(TypeBrn left, TypeBrn right, Type* result) {
    Type hold;
    Type lleft  = left->left;
    Type rleft  = right->left;

    if (lleft == rleft) {
        /* head == lleft & head == rleft */
        if (!type_Tuple(left->right, right->right, &hold)) return false;
        return tuple_Cons(rleft, hold, result);
    }

    if (lleft < rleft) {
        /* head == lleft */
        if (!type_Tuple(left->right, (Type)right, &hold)) return false;
        return tuple_Cons(lleft, hold, result);
    }

    if (lleft > rleft) {
        /* head == rleft */
        if (!type_Tuple(right->right, (Type)left, &hold)) return false;
        return tuple_Cons(rleft, hold, result);
    }

    return false;
}

/*
 * Tuples := index(i,a) | tuple(a,b) where a,b in Tuple
 *
 * tuple(a,a) == a
 * tuple(a,b) == tuple(b,a)
 * tuple(a,tuple(b,c)) == tuple(tuple(a,b),c)
 */
extern bool type_Tuple(const Type left, const Type right, Type* result) {
    if (!left)  return false;
    if (!right) return false;


    if (left->code != tc_index) {
        if (left->code != tc_tuple) return false;
    }

    if (right->code != tc_index) {
        if (right->code != tc_tuple) return false;
    }

    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    unsigned option = 0;

    option += (left->code  == tc_tuple) ? 1 : 0;
    option += (right->code == tc_tuple) ? 2 : 0;

    switch (option) {
    case 0:
        /* enforce order of tuple members */
        if (entry_Greater(left, right)) {
            return tuple_Cons(right, left, result);
        }
        return tuple_Cons(left, right, result);

    case 1: /* left == tc_tuple, right != tc_tuple */
        return tuple_Add(right, (TypeBrn)left, result);

    case 2: /* left != tc_tuple, right == tc_tuple */
        return tuple_Add(left, (TypeBrn)right, result);

    case 3:
        return tuple_Merge((TypeBrn) left, (TypeBrn) right, result);
    }

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

    entry->kind.type        = (Node) s_label;
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

static bool record_Cons(const Type left, const Type right, Type* result) {
    if (match_Tag(left,right,result)) return true;

    const Sort sort = left->sort;
    return branch_Cons(tc_record,
                       sort,
                       left,
                       right,
                       result);
}

static bool record_Add(const Type left, TypeBrn right, Type* result) {
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
        return record_Cons(left, (Type) right, result);
    }

    if (right->code == tc_any) {
        if (!record_Add(left, (TypeBrn) rright, &hold)) return false;
    } else {
        if (left < rright) {
            if (!record_Cons(left, rright, &hold)) return false;
        } else {
            if (!record_Cons(rright, left, &hold)) return false;
        }
    }

    return record_Cons(rleft, hold, result);
}

static bool record_Merge(TypeBrn left, TypeBrn right, Type* result) {
    Type hold;
    Type lleft  = left->left;
    Type rleft  = right->left;

    if (lleft == rleft) {
        /* head == lleft & head == rleft */
        if (!type_Record(left->right, right->right, &hold)) return false;
        return record_Cons(rleft, hold, result);
    }

    if (lleft < rleft) {
        /* head == lleft */
        if (!type_Record(left->right, (Type)right, &hold)) return false;
        return record_Cons(lleft, hold, result);
    }

    if (lleft > rleft) {
        /* head == rleft */
        if (!type_Record(right->right, (Type)left, &hold)) return false;
        return record_Cons(rleft, hold, result);
    }

    return false;
}

/*
 * Records := label(i,a) | record(a,b) where a,b in Record
 * record(a,a) == a
 * record(a,b) == record(b,a)
 * record(a,record(b,c)) == record(record(a,b),c)
 */
extern bool type_Record(const Type left, const Type right, Type* result) {
    if (!left)  return false;
    if (!right) return false;

    if (left->code != tc_label) {
        if (left->code != tc_record) return false;
    }

    if (right->code != tc_label) {
        if (right->code != tc_record) return false;
    }

    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    unsigned option = 0;

    option += (left->code  == tc_record) ? 1 : 0;
    option += (right->code == tc_record) ? 2 : 0;

    switch (option) {
    case 0:
        /* enforce order of record members */
        if (entry_Greater(left, right)) {
            return record_Cons(right, left, result);
        }
        return record_Cons(left, right, result);

    case 1: /* left == tc_record, right != tc_record */
        return record_Add(right, (TypeBrn)left, result);

    case 2: /* left != tc_record, right == tc_record */
        return record_Add(left, (TypeBrn)right, result);

    case 3:
        return record_Merge((TypeBrn) left, (TypeBrn) right, result);
    }

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

