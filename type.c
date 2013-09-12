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

static struct _internal_Table *_global_typetable = 0;

Constant void_s = 0;

Constant opaque_s = 0;
Constant zero_s   = 0;

Constant boolean_s = 0;
Constant undefined_s = 0;
Constant unit_s = 0;

Constant t_integer = 0;
Constant t_pair = 0;
Constant t_symbol = 0;
Constant t_text = 0;
Constant t_tuple = 0;

Constant t_buffer = 0;
Constant t_infile = 0;
Constant t_nil = 0;
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

    make_sort("Opaque", &opaque_s);
    make_sort("Zero", &zero_s);

    make_sort("Boolean",   &boolean_s);
    make_sort("Undefined", &undefined_s);
    make_sort("Unit",      &unit_s);

    MK_BTYPE(integer);
    MK_BTYPE(pair);
    MK_BTYPE(symbol);
    MK_BTYPE(text);
    MK_BTYPE(tuple);

    MK_BTYPE(buffer);
    MK_BTYPE(infile);
    MK_BTYPE(nil);
    MK_BTYPE(outfile);

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

    Header entry = fresh_atom(0, sizeof(struct type_constant));

    if (!entry) return false;

    entry->kind.type        = (Node)s_sort;
    entry->kind.constructor = (Node)s_sort;
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

extern bool find_Rule(Symbol functor, Constant xxx, Constant yyy, Constant zzz) {
    if (!functor) return false;
    if (!xxx)     return false;
    if (!yyy)     return false;
    if (!zzz)     return false;

    if (xxx == void_s) return false;
    if (yyy == void_s) return false;
    if (zzz == void_s) return false;

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

    entry->kind.type        = (Node)s_rule;
    entry->kind.constructor = (Node)s_rule;
    entry->kind.constant    = 1;

    Rule result = (Rule) asReference(entry);

    result->hashcode = hashcode;
    result->functor  = functor;
    result->xxx      = xxx;
    result->yyy      = yyy;
    result->zzz      = zzz;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    return true;
}

extern bool type_Create(Symbol symbol, Constant sort, Constant* target) {
    if (!symbol) return false;
    if (!sort)   return false;

    const HashCode hashcode = symbol->hashcode;

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_base)) continue;

        Constant test = (Constant) asReference(group);

        if (test->code != tc_constant) continue;
        if (test->name != symbol) continue;

        make_Axiom(test, sort);

        ASSIGN(target, test);

        return true;
    }

    Header entry = fresh_atom(0, sizeof(struct type_constant));

    if (!entry) return false;

    entry->kind.type        = (Node) s_base;
    entry->kind.constructor = (Node) s_base;
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

/*
 * Index
 *
 * index(x, void) == void
 */
extern bool type_Index(const unsigned index, const Base at, Base* result) {
    if (!at) return false;

    if (isIdentical(at, void_s)) {
        ASSIGN(result, void_s);
    }

    const HashCode hashcode = hash_merge((HashCode) index,
                                         at->hashcode);

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_index)) continue;

        Base test = (Base) asReference(group);

        if (test->code != tc_index) continue;

        Index inx = (Index) test;

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

    Index inx = (Index) asReference(entry);

    inx->hashcode = hashcode;
    inx->code     = tc_index;
    inx->index    = index;
    inx->slot     = at;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(result, inx);
    return true;
}

/*
 * Label
 *
 * label(x, void) == void
 */
extern bool type_Label(const Symbol label, const Base at, Base* result) {
    if (!label) return false;
    if (!at)    return false;

    if (isIdentical(at, void_s)) {
        ASSIGN(result, void_s);
    }

    const HashCode hashcode = hash_merge(label->hashcode,
                                         at->hashcode);

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_label)) continue;

        Base test = (Base) asReference(group);

        if (test->code != tc_label) continue;

        Label field = (Label) test;

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

    Label field = (Label) asReference(entry);

    field->hashcode = hashcode;
    field->code     = tc_label;
    field->label    = label;
    field->slot     = at;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(result, field);
    return true;
}

// compare (left, right) :=
//    (left <  right) -> cc_lesser
//    (left == right) -> cc_equal
//    (left  > right) -> cc_greater
static compare_code entry_Compare(const Base left, const Base right) {
    if (left == right) return cc_equal;

    if (!right) return cc_greater;
    if (!left)  return cc_lesser;

    if (left->code != right->code) {
        return (left->code < right->code) ? cc_lesser : cc_greater;
    }

    switch (left->code) {
    default:
        fatal("undefined option %d", left->code);

    case tc_index: {
        const Index lleft  = (Index) left;
        const Index rright = (Index) right;
        if (lleft->index != rright->index) {
            return (lleft->index < rright->index) ? cc_lesser : cc_greater;
        } else {
            return entry_Compare(lleft->slot, rright->slot);
        }
    }

    case tc_label: {
        const Label lleft  = (Label) left;
        const Label rright = (Label) right;
        if (lleft->label != rright->label) {
            return (lleft->label < rright->label)  ? cc_lesser : cc_greater;
        } else {
            return entry_Compare(lleft->slot, rright->slot);
        }
    }

    case tc_sort:
    case tc_constant:
        return (left < right) ? cc_lesser : cc_greater;

    case tc_tuple:
    case tc_record:
    case tc_any:
    case tc_all: {
        const Branch lleft  = (Branch) left;
        const Branch rright = (Branch) right;
        if (lleft->count != rright->count)  {
            return (lleft->count < rright->count)  ? cc_lesser : cc_greater;
        } else {
            const unsigned count = lleft->count;
            int inx;
            for (inx = 0; inx < count; ++inx) {
                const Base llinx = lleft->slots[inx];
                const Base rrinx = rright->slots[inx];
                compare_code test = entry_Compare(llinx, rrinx);
                if (test == cc_equal) continue;
                return test;
            }
            return cc_equal;
        }
    }
    }
}

// is (left < right)
static bool entry_Lesser(const Base left, const Base right) {
    if (!right) return false;
    if (!left)  return true;

    if (left->code != right->code) {
        return left->code < right->code;
    }

    switch (left->code) {
    default:
        fatal("undefined option %d", left->code);

    case tc_index: {
        const Index lleft  = (Index) left;
        const Index rright = (Index) right;
        if (lleft->index != rright->index) {
            return lleft->index < rright->index;
        } else {
            return entry_Lesser(lleft->slot, rright->slot);
        }
    }

    case tc_label: {
        const Label lleft  = (Label) left;
        const Label rright = (Label) right;
        if (lleft->label != rright->label) {
            return lleft->label < rright->label;
        } else {
            return entry_Lesser(lleft->slot, rright->slot);
        }
    }

    case tc_sort:
    case tc_constant:
        return left < right;

    case tc_tuple:
    case tc_record:
    case tc_any:
    case tc_all: {
        const Branch lleft  = (Branch) left;
        const Branch rright = (Branch) right;
        if (lleft->count != rright->count)  {
            return lleft->count < rright->count;
        } else {
            const unsigned count = lleft->count;
            int inx;
            for (inx = 0; inx < count; ++inx) {
                const Base llinx = lleft->slots[inx];
                const Base rrinx = rright->slots[inx];
                if (llinx == rrinx) continue;
                if (!entry_Lesser(llinx, rrinx)) continue;
                return true;
            }
            return false;
        }
    }
    }
}

// is (left > right)
static bool entry_Greater(const Base left, const Base right) {
    if (!left)  return false;
    if (!right) return true;

    if (left->code != right->code) {
        return left->code > right->code;
    }

    switch (left->code) {
    default:
        fatal("undefined option %d", left->code);

    case tc_index: {
        const Index lleft  = (Index) left;
        const Index rright = (Index) right;
        if (lleft->index != rright->index) {
            return lleft->index > rright->index;
        } else {
            return entry_Greater(lleft->slot, rright->slot);
        }
    }

    case tc_label: {
        const Label lleft  = (Label) left;
        const Label rright = (Label) right;
        if (lleft->label != rright->label) {
            return lleft->label > rright->label;
        } else {
            return entry_Greater(lleft->slot, rright->slot);
        }
    }

    case tc_sort:
    case tc_constant:
        return left > right;

    case tc_tuple:
    case tc_record:
    case tc_any:
    case tc_all: {
        const Branch lleft  = (Branch) left;
        const Branch rright = (Branch) right;
        if (lleft->count != rright->count)  {
            return lleft->count > rright->count;
        } else {
            const unsigned count = lleft->count;
            int inx;
            for (inx = 0; inx < count; ++inx) {
                const Base llinx = lleft->slots[inx];
                const Base rrinx = rright->slots[inx];
                if (llinx == rrinx) continue;
                if (!entry_Greater(llinx, rrinx)) continue;
                return true;
            }
            return false;
        }
    }
    }
}

// true = (added|found|merged), false = full
static bool insertAny_Ordered(Base type, const unsigned count, Base *slots) {
    if (!type) return true;

    int      inx;
    unsigned index;
    Symbol   label;
    Base     reference;
    Base     join;

    if (tc_index == type->code) {
        index     = ((Index) type)->index;
        reference = ((Index) type)->slot;
        goto index_loop;
    }
    if (tc_label == type->code) {
        label     = ((Label) type)->label;
        reference = ((Label) type)->slot;
        goto label_loop;
    }
    goto general_loop;

  index_loop:
    for (inx = 0; inx < count; ++inx) {
        const Base here = slots[inx];
        if (!here) {
            slots[inx] = type;
            return true;
        }

        if (type == here) return true;

        if (tc_index == here->code) {
            const unsigned at = ((Index) here)->index;
            if (at == index) goto merge_index;
        }

        if (entry_Greater(type, here)) continue;
        slots[inx] = type;
        type = here;
    }
    return false;

  merge_index:
    {
        const Index here = (Index) slots[inx];
        for (++inx; inx < count; ++inx) {
            if (!slots[inx]) break;
            slots[inx-1] = slots[inx];
        }
        slots[inx - 1] = 0;

        if (!type_Any(reference,here->slot, &join)) return false;
        if (!type_Index(index, join, &type)) return false;

        return insertAny_Ordered(type, count, slots);
    }

  label_loop:
    for (inx = 0; inx < count; ++inx) {
        const Base here = slots[inx];
        if (!here) {
            slots[inx] = type;
            return true;
        }

        if (type == here) return true;

        if (tc_label == here->code) {
            const Symbol at = ((Label) here)->label;
            if (at == label) goto merge_label;
        }

        if (entry_Greater(type, here)) continue;
        slots[inx] = type;
        type = here;
    }
    return false;

  merge_label:
    {
        const Label here = (Label) slots[inx];
        for (++inx; inx < count; ++inx) {
            if (!slots[inx]) break;
            slots[inx-1] = slots[inx];
        }
        slots[inx - 1] = 0;

        if (!type_Any(reference,here->slot, &join)) return false;
        if (!type_Label(label, join, &type)) return false;

        return insertAny_Ordered(type, count, slots);
    }

  general_loop:
    for (inx = 0; inx < count; ++inx) {
        const Base here = slots[inx];
        if (!here) {
            slots[inx] = type;
            return true;
        }

        if (type == here) return true;

        if (entry_Greater(type, here)) continue;
        slots[inx] = type;
        type = here;
    }

    return false;
}

// true = (added|found|merged), false = full
static bool insertAll_Ordered(Base type, const unsigned count, Base *slots) {
    if (!type) return true;

    int      inx;
    unsigned index;
    Symbol   label;
    Base     reference;
    Base     join;

    if (tc_index == type->code) {
        index     = ((Index) type)->index;
        reference = ((Index) type)->slot;
        goto index_loop;
    }
    if (tc_label == type->code) {
        label     = ((Label) type)->label;
        reference = ((Label) type)->slot;
        goto label_loop;
    }
    goto general_loop;

  index_loop:
    for (inx = 0; inx < count; ++inx) {
        const Base here = slots[inx];
        if (!here) {
            slots[inx] = type;
            return true;
        }

        if (type == here) return true;

        if (tc_index == here->code) {
            const unsigned at = ((Index) here)->index;
            if (at == index) goto merge_index;
        }

        if (entry_Greater(type, here)) continue;
        slots[inx] = type;
        type = here;
    }
    return false;

  merge_index:
    {
        const Index here = (Index) slots[inx];
        for (++inx; inx < count; ++inx) {
            if (!slots[inx]) break;
            slots[inx-1] = slots[inx];
        }
        slots[inx - 1] = 0;

        if (!type_All(reference,here->slot, &join)) return false;
        if (void_s == (Constant)join) {
            type = (Base)void_s;
        } else {
            if (!type_Index(index, join, &type)) return false;
        }

        return insertAll_Ordered(type, count, slots);
    }

  label_loop:
    for (inx = 0; inx < count; ++inx) {
        const Base here = slots[inx];
        if (!here) {
            slots[inx] = type;
            return true;
        }

        if (type == here) return true;

        if (tc_label == here->code) {
            const Symbol at = ((Label) here)->label;
            if (at == label) goto merge_label;
        }

        if (entry_Greater(type, here)) continue;
        slots[inx] = type;
        type = here;
    }
    return false;

  merge_label:
    {
        const Label here = (Label) slots[inx];
        for (++inx; inx < count; ++inx) {
            if (!slots[inx]) break;
            slots[inx-1] = slots[inx];
        }
        slots[inx - 1] = 0;

        if (!type_All(reference,here->slot, &join)) return false;
        if (void_s == (Constant)join) {
            type = (Base)void_s;
        } else {
            if (!type_Label(label, join, &type)) return false;
        }

        return insertAll_Ordered(type, count, slots);
    }

  general_loop:
    for (inx = 0; inx < count; ++inx) {
        const Base here = slots[inx];
        if (!here) {
            slots[inx] = type;
            return true;
        }

        if (type == here) return true;

        if (entry_Greater(type, here)) continue;
        slots[inx] = type;
        type = here;
    }

    return false;
}

static bool filter_Ordered(const Base type, const unsigned count, Base *buffer) {
    unsigned target;
    unsigned inx = 0;

    for (; inx < count; ++inx) {
        if (type == buffer[inx]) goto shift;
    }
    return false;

 shift:
    target = inx;
    for ( ; inx < count ; ++inx) {
        if (type == buffer[inx]) continue;
        buffer[target] = buffer[inx];
        ++target;
    }

    for ( ; target < count ; ++target) {
        buffer[target] = 0;
    }

    return true;
}

static unsigned count_Ordered(const unsigned count, Base *buffer) {
    unsigned fullcount = 0;
    unsigned inx;

    for (inx = 0; inx < count; ++inx) {
        if (!buffer[inx]) break;
    }

    fullcount = inx;

    for (; inx < count; ++inx) {
        if (!buffer[inx]) continue;
        buffer[fullcount] = buffer[inx];
        buffer[inx] = 0;
        ++fullcount;
    }

    return fullcount;
}

static bool branch_IsMember(const Branch set, const Base type) {
    if (!set) return false;

    const unsigned count = set->count;
    const Base    *slots = set->slots;

    int inx;
    for (inx = 0; inx < count; ++inx) {
        const Base here = slots[inx];
        if (here != type) continue;
        if (entry_Greater(here, type)) break;
        return true;
    }

    return false;
}

static bool branch_Cons(const type_code kind,
                        const unsigned count,
                        const Base *slots,
                        Base* result)
{
    HashCode temp = 0;
    {
        int inx;
        for (inx = 0; inx < count; ++inx) {
            const Base here = slots[inx];
            if (here) {
                temp = hash_merge(temp, here->hashcode);
            }
        }
    }

    const HashCode hashcode = temp;

    const int row   = hashcode % _global_typetable->size;
    Header    group = _global_typetable->row[row].first;

    for ( ; group; group = group->after) {
        if (!isIdentical(group->kind.constructor, s_branch)) continue;

        Branch test = (Branch) asReference(group);

        if (test->code  != kind)  continue;
        if (test->count != count) continue;

        int inx;
        for (inx = 0; inx < count; ++inx) {
            const Base here  = slots[inx];
            const Base there = test->slots[inx];
            if (here != there) goto next;
        }

        ASSIGN(result, test);
        return true;

      next: continue;
    }

    Header entry = fresh_atom(0, sizeof(struct type_branch) + (sizeof(Base) * count));

    if (!entry) return false;

    entry->kind.type        = (Node) s_branch;
    entry->kind.constructor = (Node) s_branch;
    entry->kind.constant    = 1;

    Branch branch = (Branch) asReference(entry);

    branch->hashcode = hashcode;
    branch->code     = kind;
    branch->count    = count;

    {
        int inx;
        for (inx = 0; inx < count; ++inx) {
            branch->slots[inx] = slots[inx];
        }
    }

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(result, branch);
    return true;
}

static bool branchAny_Merge(const Branch left,
                            const unsigned count,
                            const Base *slots,
                            Base* result)
{
    if (!left)  return false;
    if (!slots) return false;

    type_code         kind = left->code;
    unsigned     fullcount = left->count + count;
    unsigned long fullsize = sizeof(Base) * (fullcount + 1);

    if (1 > count) return false;

    Base *buffer = (Base *)malloc(fullsize);

    memset(buffer, 0, fullsize);

    unsigned inx;

    for (inx = 0; inx < count; ++inx) {
        if (!insertAny_Ordered(slots[inx], fullcount, buffer)) goto error;
    }

    for (inx = 0; inx < left->count; ++inx) {
        if (!insertAny_Ordered(left->slots[inx], fullcount, buffer)) goto error;
    }

    filter_Ordered((Base)void_s, fullcount, buffer);
    fullcount = count_Ordered(fullcount, buffer);

    if (!branch_Cons(kind, fullcount, buffer, result)) goto error;

    free(buffer);
    return true;

  error:
    free(buffer);
    return false;
}

static bool branchAll_Merge(const Branch left,
                            const unsigned count,
                            const Base *slots,
                            Base* result)
{
    if (!left)  return false;
    if (!slots) return false;

    type_code         kind = left->code;
    unsigned     fullcount = left->count + count;
    unsigned long fullsize = sizeof(Base) * (fullcount + 1);

    if (1 > count) return false;

    Base *buffer = (Base *)malloc(fullsize);

    memset(buffer, 0, fullsize);

    unsigned inx;

    for (inx = 0; inx < count; ++inx) {
        if (!insertAll_Ordered(slots[inx], fullcount, buffer)) goto error;
    }

    for (inx = 0; inx < left->count; ++inx) {
        if (!insertAll_Ordered(left->slots[inx], fullcount, buffer)) goto error;
    }

    if (filter_Ordered((Base)void_s, fullcount, buffer)) {
        ASSIGN(result, void_s);
    } else {
        fullcount = count_Ordered(fullcount, buffer);

        if (!branch_Cons(kind, fullcount, buffer, result)) goto error;
    }
    free(buffer);
    return true;

  error:
    free(buffer);
    return false;
}

/*
 * Any
 *
 * any(void,a) == any(a,void) == a
 *
 * any(a,a)   == a
 * any(a,b)   == any(b,a)
 * any(a,b,c) == any(a,any(b,c))
 */
extern bool type_Any(const Base left, const Base right, Base* result) {
    if (!left)  return false;
    if (!right) return false;

    /* forall(A) any(A,A) == A */
    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    if (left == (Base)void_s) {
        ASSIGN(result, right);
        return true;
    }

    if (right == (Base)void_s) {
        ASSIGN(result, left);
        return true;
    }

    unsigned option = 0;

    option += (left->code  == tc_any) ? 1 : 0;
    option += (right->code == tc_any) ? 2 : 0;

    switch (option) {
    case 0: { /* left != tc_any, right != tc_any */
        Base array[3] = { 0, 0, 0 };
        if (!insertAny_Ordered(left, 3, array)) return false;
        if (!insertAny_Ordered(right, 3, array)) return false;

        unsigned count = count_Ordered(3, array);

        if (1 > count) return false;
        if (1 < count) return branch_Cons(tc_any, count, array, result);

        ASSIGN(result, array[0]);

        return true;
    }

    case 1: { /* left == tc_any, right != tc_any */
        const Base array[3] = { right, 0, 0};
        return branchAny_Merge((Branch)left, 1, array, result);
    }

    case 2: { /* left != tc_any, right == tc_any */
        const Base array[3] = { left, 0, 0};
        return branchAny_Merge((Branch)right, 1, array, result);
    }

    case 3: {
        unsigned    count = ((Branch) right)->count;
        const Base *slots = ((Branch) right)->slots;
        return branchAny_Merge((Branch) left, count, slots, result);
    }}

    return false;
}

/*
 * Tuples := index(i,x) | tuple(a,b) where a,b in Tuple
 *
 * tuple(a,a) == a
 * tuple(a,b) == tuple(b,a)
 * tuple(a,tuple(b,c)) == tuple(tuple(a,b),c)
 */
extern bool type_Tuple(const Base left, const Base right, Base* result) {
    if (!left)  return false;
    if (!right) return false;

    if (left->code != tc_index) {
        if (left->code != tc_tuple) {
            ASSIGN(result, void_s);
            return true;
        }
    }

    if (right->code != tc_index) {
        if (right->code != tc_tuple) {
            ASSIGN(result, void_s);
            return true;
        }
    }

    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    unsigned option = 0;

    option += (left->code  == tc_tuple) ? 1 : 0;
    option += (right->code == tc_tuple) ? 2 : 0;

    switch (option) {
    case 0: { /* left != tc_tuple, right != tc_tuple */
        Base array[3] = { 0, 0, 0 };
        if (!insertAny_Ordered(left, 3, array)) return false;
        if (!insertAny_Ordered(right, 3, array)) return false;

        unsigned count = count_Ordered(3, array);

        if (1 > count) return false;
        if (1 < count) return branch_Cons(tc_tuple, count, array, result);

        ASSIGN(result, array[0]);

        return true;
    }

    case 1: { /* left == tc_tuple, right != tc_tuple */
        const Base array[3] = { right, 0, 0};
        return branchAny_Merge((Branch)left, 1, array, result);
    }

    case 2: { /* left != tc_tuple, right == tc_tuple */
        const Base array[3] = { left, 0, 0};
        return branchAny_Merge((Branch)right, 1, array, result);
    }

    case 3: {
        unsigned    count = ((Branch) right)->count;
        const Base *slots = ((Branch) right)->slots;
        return branchAny_Merge((Branch) left, count, slots, result);
    }}

    return false;
}

/*
 * Records := label(i,a) | record(a,b) where a,b in Record
 *
 * record(a,a) == a
 * record(a,b) == record(b,a)
 * record(a,record(b,c)) == record(record(a,b),c)
 */
extern bool type_Record(const Base left, const Base right, Base* result) {
    if (!left)  return false;
    if (!right) return false;

    if (left->code != tc_label) {
        if (left->code != tc_record) {
            ASSIGN(result, void_s);
            return true;
        }
    }

    if (right->code != tc_label) {
        if (right->code != tc_record) {
            ASSIGN(result, void_s);
            return true;
        }
    }

    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    unsigned option = 0;

    option += (left->code  == tc_record) ? 1 : 0;
    option += (right->code == tc_record) ? 2 : 0;

    switch (option) {
    case 0: { /* left != tc_record, right != tc_record */
        Base array[3] = { 0, 0, 0};
        if (!insertAny_Ordered(left, 3, array)) return false;
        if (!insertAny_Ordered(right, 3, array)) return false;

        unsigned count = count_Ordered(3, array);

        if (1 > count) return false;
        if (1 < count) return branch_Cons(tc_record, count, array, result);

        ASSIGN(result, array[0]);

        return true;
    }

    case 1: { /* left == tc_record, right != tc_record */
        const Base array[3] = { right, 0, 0};
        return branchAny_Merge((Branch)left, 1, array, result);
    }

    case 2: { /* left != tc_record, right == tc_record */
        const Base array[3] = { left, 0, 0};
        return branchAny_Merge((Branch)right, 1, array, result);
    }

    case 3: {
        unsigned    count = ((Branch) right)->count;
        const Base *slots = ((Branch) right)->slots;
        return branchAny_Merge((Branch) left, count, slots, result);
    }}

    return false;
}

/*
 * All
 *
 * all(void,a)  == all(a,void)  == void
 * all(const,a) == all(a,const) == void
 *
 * all(a,a)   == a
 * all(a,b)   == all(b,a)
 * all(a,b,c) == all(a,all(b,c))
 */
extern bool type_All(const Base left, const Base right, Base* result) {
    if (!left)  return false;
    if (!right) return false;

    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    if (isIdentical(left, void_s)) {
        ASSIGN(result, void_s);
        return true;
    }

    if (isIdentical(right, void_s)) {
        ASSIGN(result, void_s);
        return true;
    }

    if (left->code == tc_constant) {
        ASSIGN(result, void_s);
        return true;
    }

    if (right->code == tc_constant) {
        ASSIGN(result, void_s);
        return true;
    }

    unsigned option = 0;

    option += (left->code  == tc_all) ? 1 : 0;
    option += (right->code == tc_all) ? 2 : 0;

    switch (option) {
    case 0: { /* left != tc_any, right != tc_any */
        Base array[3] = { 0, 0, 0};
        if (!insertAll_Ordered(left, 3, array)) return false;
        if (!insertAll_Ordered(right, 3, array)) return false;

        unsigned count = count_Ordered(3, array);

        if (1 > count) return false;
        if (1 < count) return branch_Cons(tc_any, count, array, result);

        ASSIGN(result, array[0]);

        return true;
    }

    case 1: { /* left == tc_any, right != tc_any */
        const Base array[3] = { right, 0, 0};
        return branchAll_Merge((Branch)left, 1, array, result);
    }

    case 2: { /* left != tc_any, right == tc_any */
        const Base array[3] = { left, 0, 0};
        return branchAll_Merge((Branch)right, 1, array, result);
    }

    case 3: {
        unsigned    count = ((Branch) right)->count;
        const Base *slots = ((Branch) right)->slots;
        return branchAll_Merge((Branch) left, count, slots, result);
    }}

    return false;
}

static bool axiom_Map(Operator func, const Axiom node, const Node env, Target target) {
   GC_Begin(4);

   Node element;
   Node class;
   Pair tail;

   GC_Protect(element);
   GC_Protect(class);
   GC_Protect(tail);

   func(node->element, env, &element);
   func(node->class, env, &class);

   if (!pair_Create(class, NIL, &tail)) goto error;
   if (!pair_Create(element, tail, target.pair)) goto error;

   GC_End();
   return true;

 error:
    GC_End();
    return false;
}

static bool rule_Map(Operator  func, const Rule node, const Node env, Target target) {
   GC_Begin(8);

   Node functor;
   Node xxx;
   Node yyy;
   Node zzz;
   Pair tail;
   Pair hold;

   GC_Protect(functor);
   GC_Protect(xxx);
   GC_Protect(yyy);
   GC_Protect(zzz);
   GC_Protect(hold);

   func(node->functor, env, &functor);
   func(node->xxx, env, &xxx);
   func(node->yyy, env, &yyy);
   func(node->zzz, env, &zzz);

   if (!pair_Create(zzz, NIL,  &hold)) goto error;
   if (!pair_Create(yyy, hold, &tail)) goto error;
   if (!pair_Create(xxx, tail, &hold)) goto error;
   if (!pair_Create(functor, hold, target.pair)) goto error;

   GC_End();
   return true;

 error:
    GC_End();
    return false;
}

static bool index_Map(Operator func, const Index node, const Node env, Target target) {
   GC_Begin(5);

   Integer hold;
   Node    index;
   Node    slot;
   Pair    tail;

   GC_Protect(hold);
   GC_Protect(index);
   GC_Protect(slot);
   GC_Protect(tail);

   if (!integer_Create(node->index, &hold)) goto error;

   func(hold, env, &index);
   func(node->slot, env, &slot);

   if (!pair_Create(slot, NIL, &tail)) goto error;
   if (!pair_Create(index, tail, target.pair)) goto error;

   GC_End();
    return true;

 error:
    GC_End();
    return false;
}

static bool label_Map(Operator func, const Label node, const Node env, Target target) {
   GC_Begin(4);

   Node label;
   Node slot;
   Pair tail;

   GC_Protect(label);
   GC_Protect(slot);
   GC_Protect(tail);

   func(node->label, env, &label);
   func(node->slot, env, &slot);

   if (!pair_Create(slot, NIL, &tail)) goto error;
   if (!pair_Create(label, tail, target.pair)) goto error;

   GC_End();
    return true;

 error:
    GC_End();
    return false;
}

static bool branch_Map(Operator func, const Branch branch, const Node env, Target target)
{
    GC_Begin(7);

    Pair first;
    Pair last;
    Base input;
    Node output;
    Pair hold;

    GC_Protect(first);
    GC_Protect(last);
    GC_Protect(input);
    GC_Protect(output);
    GC_Protect(hold);

    const unsigned count = branch->count;
    const Base    *slots = branch->slots;

    if (0 < count) {
        unsigned inx = 0;

        input = slots[inx];

        func(input, env, &output);

        if (!pair_Create(output,NIL, &first)) goto error;

        last = first;

        for (++inx; inx < count; ++inx) {
            hold   = 0;
            input  = slots[inx];
            output = NIL;

            func(input, env, &output);

            if (!pair_Create(output,NIL, &hold)) goto error;
            if (!pair_SetCdr(last, hold))        goto error;

            last = hold;
        }
    }


    ASSIGN(target, first);
    GC_End();
    return true;

 error:
    GC_End();
    return false;
}

extern bool type_Map(Operator func, const Node node, const Node env, Target target) {
    if (isNil(node)) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!func) {
        fatal("\nerror: type_Map applied to a null function");
        return false;
    }

    Node ctor = getConstructor(node);

    if (isIdentical(ctor, s_sort)) {
        func(node, env, target);
        return true;
    }

    if (isIdentical(ctor, s_axiom)) {
        return axiom_Map(func, node.axiom, env, target);
    }

    if (isIdentical(ctor, s_rule)) {
        return rule_Map(func, node.rule, env, target);
    }

    if (isIdentical(ctor, s_base)) {
        func(node, env, target);
        return true;
    }

    if (isIdentical(ctor, s_index)) {
        return index_Map(func, node.index, env, target);
    }

    if (isIdentical(ctor, s_label)) {
        return label_Map(func, node.label, env, target);
    }

    if (isIdentical(ctor, s_branch)) {
        return branch_Map(func, node.branch, env, target);
    }

    fatal("\nerror: type_Map applied to a non type obj");
    return false;
}


extern bool type_Pi(Base* target, ...) {
    return false;
}

extern bool type_Sigma(Base* target, ...) {
    return false;
}

extern bool type_Mu(Base* target, ...) {
    return false;
}

extern bool type_Delta(Base* target, ...) {
    return false;
}


extern bool type_Contains(const Base type, const Node value) {
    if (!type) return false;

    Kind kind = asKind(type);

    if (!kind) return false;

    if (!isAType(kind->type)) return false;

    Base vtype = kind->type.type;

    if (type == vtype) return true;
}

static bool collect_Any(const Base hold,
                        const Base next,
                        const unsigned fullcount,
                        Base *slots,
                        Base* result)
{
    Base temp = (Base)0;

    filter_Ordered((Base)void_s, fullcount, slots);

    unsigned current = count_Ordered(fullcount, slots);

    if (1 == current) {
        temp = slots[0];
    }

    if (1 < current) {
        branch_Cons(tc_any, current, slots, &temp);
    }

    if (hold) {
        if (!temp) {
            temp = hold;
        } else {
            type_Any(hold, temp, &temp);
        }
    }

    if (next) {
        if (!temp) {
            temp = hold;
        } else {
            type_Any(hold, temp, &temp);
        }
    }

    result[0] = temp;
}

static bool collect_Sorts(Constant element, Target result) {
    Base buffer[100];
    Base hold = 0;

    const unsigned fullcount = 99;

    memset(buffer, 0, sizeof(Base) * 100);

    unsigned row = 0;
    unsigned max = _global_typetable->size;
    for (; row < max; ++row) {
        Header group = _global_typetable->row[row].first;
        for ( ; group; group = group->after) {
            if (!isIdentical(group->kind.constructor, s_axiom)) continue;

            Axiom test = (Axiom) asReference(group);

            if (test->element != element) continue;

            Base next = (Base)(test->class);

            if (!insertAny_Ordered(next, fullcount, buffer)) {
                collect_Any(hold, next, fullcount, buffer, &hold);
                memset(buffer, 0, sizeof(Base) * 100);
            }
        }
    }

    return collect_Any(hold, (Base)0, fullcount, buffer, result.type);
}

extern bool compute_Sort(Base value, Target result) {
    if (!value) {
        ASSIGN(result, undefined_s);
        return true;
    }

    switch (value->code) {
    case tc_index:
        return compute_Sort(((Index)value)->slot, result);

    case tc_label:
        return compute_Sort(((Label)value)->slot, result);

    case tc_constant:
        return collect_Sorts((Constant)value, result);

    case tc_sort:
        return collect_Sorts((Constant)value, result);

    case tc_tuple:
    case tc_record:
    case tc_any:
    case tc_all:
        ASSIGN(result, undefined_s);
        return false;
    }
}

extern bool compute_Type(Node value, Target result) {
    if (isNil(value)) {
        ASSIGN(result, t_nil);
        return true;
    }

    if (isAType(value)) {
        return compute_Sort(value.type, result);
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

