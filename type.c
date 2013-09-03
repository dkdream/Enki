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

Sort zero_s   = 0;
Sort symbol_s = 0;
Sort opaque_s = 0;

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

    make_sort("Symbol", &symbol_s);
    make_basetype("symbol", symbol_s, &t_symbol);

    make_sort("Opaque", &opaque_s);
    make_basetype("opaque", opaque_s, &t_opaque);

    make_sort("Zero", &zero_s);

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
//   Pi    - (xxx -> yyy):zzz (dependent function types)
//   Sigma - (xxx, yyy):zzz   (dependent tuple types)
extern bool make_Rule(Symbol functor, Sort xxx, Sort yyy, Sort zzz) {
    if (!functor) return false;
    if (!xxx)     return false;
    if (!yyy)     return false;
    if (!zzz)    return false;

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

        if (((Constant)test)->name != symbol)      continue;

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
    result->sort     = sort;
    result->code     = tc_constant;
    result->name     = symbol;

    entry->after = _global_typetable->row[row].first;
    _global_typetable->row[row].first = entry;

    ASSIGN(target, result);

    return true;
}

/*
 * Index
 *
 * index(x, void) == void
 */
extern bool type_Index(const unsigned index, const Type at, Type* result) {
    if (!at) return false;

    if (isIdentical(at, t_void)) {
        ASSIGN(result, t_void);
    }

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
    inx->sort     = sort;
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
extern bool type_Label(const Symbol label, const Type at, Type* result) {
    if (!label) return false;
    if (!at)    return false;

    if (isIdentical(at, t_void)) {
        ASSIGN(result, t_void);
    }

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
    field->sort     = sort;
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
static compare_code entry_Compare(const Type left, const Type right) {
    if (left == right) return cc_equal;

    if (!right) return cc_greater;
    if (!left)  return cc_lesser;

    if (left->code != right->code) {
        return (left->code < right->code) ? cc_lesser : cc_greater;
    }

    switch (left->code) {
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
                const Type llinx = lleft->slots[inx];
                const Type rrinx = rright->slots[inx];
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
static bool entry_Lesser(const Type left, const Type right) {
    if (!right) return false;
    if (!left)  return true;

    if (left->code != right->code) {
        return left->code < right->code;
    }

    switch (left->code) {
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
                const Type llinx = lleft->slots[inx];
                const Type rrinx = rright->slots[inx];
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
static bool entry_Greater(const Type left, const Type right) {
    if (!left)  return false;
    if (!right) return true;

    if (left->code != right->code) {
        return left->code > right->code;
    }

    switch (left->code) {
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
                const Type llinx = lleft->slots[inx];
                const Type rrinx = rright->slots[inx];
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
static bool insertAny_Ordered(Type type, const unsigned count, Type *slots) {
    if (!type) return true;

    int      inx;
    unsigned index;
    Symbol   label;
    Type     reference;
    Type     join;

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
        const Type here = slots[inx];
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
        const Type here = slots[inx];
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
        const Type here = slots[inx];
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
static bool insertAll_Ordered(Type type, const unsigned count, Type *slots) {
    if (!type) return true;

    int      inx;
    unsigned index;
    Symbol   label;
    Type     reference;
    Type     join;

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
        const Type here = slots[inx];
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
        if (t_void == join) {
            type = t_void;
        } else {
            if (!type_Index(index, join, &type)) return false;
        }

        return insertAll_Ordered(type, count, slots);
    }

  label_loop:
    for (inx = 0; inx < count; ++inx) {
        const Type here = slots[inx];
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
        if (t_void == join) {
            type = t_void;
        } else {
            if (!type_Label(label, join, &type)) return false;
        }

        return insertAll_Ordered(type, count, slots);
    }

  general_loop:
    for (inx = 0; inx < count; ++inx) {
        const Type here = slots[inx];
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

static bool filter_Ordered(const Type type, const unsigned count, Type *buffer) {
    unsigned target = 0;
    unsigned inx;

    for (inx = 0; inx < count; ++inx) {
        if (type == buffer[inx]) {
            target = inx;
            goto shift;
        }
    }
    return false;

 shift:
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

static unsigned count_Ordered(const unsigned count, Type *buffer) {
    unsigned fullcount = 0;
    unsigned inx;

    for (inx = 0; inx < count; ++inx) {
        if (!buffer[inx]) {
            fullcount = inx;
            break;
        }
    }

    for (; inx < count; ++inx) {
        if (!buffer[inx]) continue;
        buffer[fullcount] = buffer[inx];
        buffer[inx] = 0;
        ++fullcount;
    }

    return fullcount;
}

static bool branch_IsMember(const Branch set, const Type type) {
    if (!set) return false;

    const unsigned count = set->count;
    const Type    *slots = set->slots;

    int inx;
    for (inx = 0; inx < count; ++inx) {
        const Type here = slots[inx];
        if (here != type) continue;
        if (entry_Greater(here, type)) break;
        return true;
    }

    return false;
}

static bool branch_Cons(const type_code kind,
                        const Sort sort,
                        const unsigned count,
                        const Type *slots,
                        Type* result)
{
    HashCode temp = sort->hashcode;

    {
        int inx;
        for (inx = 0; inx < count; ++inx) {
            const Type here = slots[inx];
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

        if (test->sort  != sort)  continue;
        if (test->code  != kind)  continue;
        if (test->count != count) continue;

        int inx;
        for (inx = 0; inx < count; ++inx) {
            const Type here  = slots[inx];
            const Type there = test->slots[inx];
            if (here != there) goto next;
        }

        ASSIGN(result, test);
        return true;

      next: continue;
    }

    Header entry = fresh_atom(0, sizeof(struct type_branch) + (sizeof(Type) * count));

    if (!entry) return false;

    entry->kind.type        = (Node) s_branch;
    entry->kind.constructor = (Node) s_branch;
    entry->kind.constant    = 1;

    Branch branch = (Branch) asReference(entry);

    branch->hashcode = hashcode;
    branch->sort     = sort;
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

static bool branchAny_Merge(const Sort sort,
                            const Branch left,
                            const unsigned count,
                            const Type *slots,
                            Type* result)
{
    if (!sort)  return false;
    if (!left)  return false;
    if (!slots) return false;

    type_code         kind = left->code;
    unsigned     fullcount = left->count + count;
    unsigned long fullsize = sizeof(Type) * (fullcount + 1);

    if (1 > count) return false;

    Type *buffer = (Type *)malloc(fullsize);

    memset(buffer, 0, fullsize);

    unsigned inx;

    for (inx = 0; inx < count; ++inx) {
        if (!insertAny_Ordered(slots[inx], fullcount, buffer)) goto error;
    }

    for (inx = 0; inx < left->count; ++inx) {
        if (!insertAny_Ordered(left->slots[inx], fullcount, buffer)) goto error;
    }

    filter_Ordered(t_void, fullcount, buffer);
    fullcount = count_Ordered(fullcount, buffer);

    if (!branch_Cons(kind, sort, fullcount, buffer, result)) goto error;

    free(buffer);
    return true;

  error:
    free(buffer);
    return false;
}

static bool branchAll_Merge(const Sort sort,
                            const Branch left,
                            const unsigned count,
                            const Type *slots,
                            Type* result)
{
    if (!sort)  return false;
    if (!left)  return false;
    if (!slots) return false;

    type_code         kind = left->code;
    unsigned     fullcount = left->count + count;
    unsigned long fullsize = sizeof(Type) * (fullcount + 1);

    if (1 > count) return false;

    Type *buffer = (Type *)malloc(fullsize);

    memset(buffer, 0, fullsize);

    unsigned inx;

    for (inx = 0; inx < count; ++inx) {
        if (!insertAll_Ordered(slots[inx], fullcount, buffer)) goto error;
    }

    for (inx = 0; inx < left->count; ++inx) {
        if (!insertAll_Ordered(left->slots[inx], fullcount, buffer)) goto error;
    }

    if (filter_Ordered(t_void, fullcount, buffer)) {
        ASSIGN(result, t_void);
    } else {
        fullcount = count_Ordered(fullcount, buffer);

        if (!branch_Cons(kind, sort, fullcount, buffer, result)) goto error;
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
extern bool type_Any(const Type left, const Type right, Type* result) {
    if (!left)  return false;
    if (!right) return false;

    /* forall(A) any(A,A) == A */
    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    if (left == t_void) {
        ASSIGN(result, right);
        return true;
    }

    if (right == t_void) {
        ASSIGN(result, left);
        return true;
    }

    unsigned option = 0;

    option += (left->code  == tc_any) ? 1 : 0;
    option += (right->code == tc_any) ? 2 : 0;

    switch (option) {
    case 0: { /* left != tc_any, right != tc_any */
        Type array[3] = { 0, 0, 0 };
        if (!insertAny_Ordered(left, 3, array)) return false;
        if (!insertAny_Ordered(right, 3, array)) return false;

        unsigned count = count_Ordered(3, array);

        if (1 > count) return false;
        if (1 < count) return branch_Cons(tc_any, left->sort, count, array, result);

        ASSIGN(result, array[0]);

        return true;
    }

    case 1: { /* left == tc_any, right != tc_any */
        const Type array[3] = { right, 0, 0};
        return branchAny_Merge(left->sort, (Branch)left, 1, array, result);
    }

    case 2: { /* left != tc_any, right == tc_any */
        const Type array[3] = { left, 0, 0};
        return branchAny_Merge(right->sort, (Branch)right, 1, array, result);
    }

    case 3: {
        unsigned    count = ((Branch) right)->count;
        const Type *slots = ((Branch) right)->slots;
        return branchAny_Merge(left->sort, (Branch) left, count, slots, result);
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
extern bool type_Tuple(const Type left, const Type right, Type* result) {
    if (!left)  return false;
    if (!right) return false;

    if (left->code != tc_index) {
        if (left->code != tc_tuple) {
            ASSIGN(result, t_void);
            return true;
        }
    }

    if (right->code != tc_index) {
        if (right->code != tc_tuple) {
            ASSIGN(result, t_void);
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
        Type array[3] = { 0, 0, 0 };
        if (!insertAny_Ordered(left, 3, array)) return false;
        if (!insertAny_Ordered(right, 3, array)) return false;

        unsigned count = count_Ordered(3, array);

        if (1 > count) return false;
        if (1 < count) return branch_Cons(tc_tuple, left->sort, count, array, result);

        ASSIGN(result, array[0]);

        return true;
    }

    case 1: { /* left == tc_tuple, right != tc_tuple */
        const Type array[3] = { right, 0, 0};
        return branchAny_Merge(left->sort, (Branch)left, 1, array, result);
    }

    case 2: { /* left != tc_tuple, right == tc_tuple */
        const Type array[3] = { left, 0, 0};
        return branchAny_Merge(right->sort, (Branch)right, 1, array, result);
    }

    case 3: {
        unsigned    count = ((Branch) right)->count;
        const Type *slots = ((Branch) right)->slots;
        return branchAny_Merge(left->sort, (Branch) left, count, slots, result);
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
extern bool type_Record(const Type left, const Type right, Type* result) {
    if (!left)  return false;
    if (!right) return false;

    if (left->code != tc_label) {
        if (left->code != tc_record) {
            ASSIGN(result, t_void);
            return true;
        }
    }

    if (right->code != tc_label) {
        if (right->code != tc_record) {
            ASSIGN(result, t_void);
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
        Type array[3] = { 0, 0, 0};
        if (!insertAny_Ordered(left, 3, array)) return false;
        if (!insertAny_Ordered(right, 3, array)) return false;

        unsigned count = count_Ordered(3, array);

        if (1 > count) return false;
        if (1 < count) return branch_Cons(tc_record, left->sort, count, array, result);

        ASSIGN(result, array[0]);

        return true;
    }

    case 1: { /* left == tc_record, right != tc_record */
        const Type array[3] = { right, 0, 0};
        return branchAny_Merge(left->sort, (Branch)left, 1, array, result);
    }

    case 2: { /* left != tc_record, right == tc_record */
        const Type array[3] = { left, 0, 0};
        return branchAny_Merge(right->sort, (Branch)right, 1, array, result);
    }

    case 3: {
        unsigned    count = ((Branch) right)->count;
        const Type *slots = ((Branch) right)->slots;
        return branchAny_Merge(left->sort, (Branch) left, count, slots, result);
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
extern bool type_All(const Type left, const Type right, Type* result) {
    if (!left)  return false;
    if (!right) return false;

    if (isIdentical(left,right)) {
        ASSIGN(result, left);
        return true;
    }

    if (isIdentical(left, t_void)) {
        ASSIGN(result, t_void);
        return true;
    }

    if (isIdentical(right, t_void)) {
        ASSIGN(result, t_void);
        return true;
    }

    if (left->code == tc_constant) {
        ASSIGN(result, t_void);
        return true;
    }

    if (right->code == tc_constant) {
        ASSIGN(result, t_void);
        return true;
    }

    unsigned option = 0;

    option += (left->code  == tc_all) ? 1 : 0;
    option += (right->code == tc_all) ? 2 : 0;

    switch (option) {
    case 0: { /* left != tc_any, right != tc_any */
        Type array[3] = { 0, 0, 0};
        if (!insertAll_Ordered(left, 3, array)) return false;
        if (!insertAll_Ordered(right, 3, array)) return false;

        unsigned count = count_Ordered(3, array);

        if (1 > count) return false;
        if (1 < count) return branch_Cons(tc_any, left->sort, count, array, result);

        ASSIGN(result, array[0]);

        return true;
    }

    case 1: { /* left == tc_any, right != tc_any */
        const Type array[3] = { right, 0, 0};
        return branchAll_Merge(left->sort, (Branch)left, 1, array, result);
    }

    case 2: { /* left != tc_any, right == tc_any */
        const Type array[3] = { left, 0, 0};
        return branchAll_Merge(right->sort, (Branch)right, 1, array, result);
    }

    case 3: {
        unsigned    count = ((Branch) right)->count;
        const Type *slots = ((Branch) right)->slots;
        return branchAll_Merge(left->sort, (Branch) left, count, slots, result);
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
    Type input;
    Node output;
    Pair hold;

    GC_Protect(first);
    GC_Protect(last);
    GC_Protect(input);
    GC_Protect(output);
    GC_Protect(hold);

    const unsigned count = branch->count;
    const Type    *slots = branch->slots;

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

