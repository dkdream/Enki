/*-*- mode: c;-*-*/
#if !defined(_ea_type_h_)
#define _ea_type_h_
/***************************
 **
 ** Purpose
 **   Allow fast comparision of types
 **
 ** Properties
 **   o. a type that is never collected (gc)
 **   o. if two types have the same content (iso A B)
 **      then they have the same address (A == B)
 **
 **/
#include "reference.h"
#include "symbol.h"
#include "treadmill.h"
#include "hashcode.h"
#include "primitive.h"

typedef enum {
    tc_undefined,
    tc_index,
    tc_label,
    tc_constant,
    tc_sort,
    tc_tuple,    // type_branch (indexed collection of types )
    tc_record,   // type_branch (labeled collection of types )
    tc_any,      // type_branch (union of types )
    tc_all,      // type_branch (intersection of types)
} type_code;

struct type_base {
    HashCode hashcode; type_code code;
};

// s_sort is a sort (sort constant)
struct sort {
    HashCode hashcode; type_code code;
    Symbol   name;
};

// s_axiom is a axiom pair (c:s)
struct axiom {
    HashCode hashcode;
    Sort element;
    Sort class;
};

// s_rule is a rule  triple (in=s1,out=s2,kind=s3)
struct rule {
    HashCode hashcode;
    Symbol   functor;
    Sort xxx, yyy, zzz;
};


// s_base is a base type
struct type_constant {
    HashCode hashcode; type_code code;
    Sort sort;
    Symbol name;
};

// s_index is an indexed type
struct type_index {
    HashCode hashcode; type_code code;
    unsigned index;
    Base     slot;
};

// s_label is an labeled type
struct type_label {
    HashCode hashcode; type_code code;
    Symbol label;
    Base   slot;
};

// s_branch is a branch type (any,tuple,record,all)
struct type_branch {
    HashCode hashcode; type_code code;
    unsigned count;
    Base slots[1];
};

// s_name is a variable reference used in a formula
struct name {
    Sort sort;
};

extern Sort opaque_s;    // the sort of opaque types
extern Sort symbol_s;    // the sort of symbols
extern Sort zero_s;      // the sort of values

extern Sort boolean_s;   // the sort with true and false values
extern Sort undefined_s; // the sort with the void value
extern Sort unit_s;      // the sort with the unit value
extern Sort void_s;      // the sort with no values

extern Base t_integer; // the type of integer values
extern Base t_pair;    // the union of all pairs types
extern Base t_symbol;  // the union of all symbol types
extern Base t_text;    // the union of all text types
extern Base t_tuple;   // the union of all tuple types

extern Base t_buffer;    // the type of a c-text-buffer
extern Base t_infile;    // the type of a c-os-infile
extern Base t_nil;       // the type of the value nil
extern Base t_outfile;   // the type of a c-os-infile

extern bool sort_Create(Symbol,Sort*);      /* each sort has a unique name */
extern bool type_Create(Symbol,Sort,Base*); /* each type constant in a sort has a unique name */

extern bool find_Axiom(const Sort, const Sort);
extern bool make_Axiom(const Sort, const Sort);
extern bool find_Rule(const Symbol, const Sort, const Sort, const Sort);
extern bool make_Rule(const Symbol, const Sort, const Sort, const Sort);

extern bool compute_Sort(Base value, Target result);

/* Any := type | any(a,b) where a,b in Any
 * any(a,void) == any(void,a) == a
 * any(index(i,a),index(i,b)) == index(i,any(a,b))
 * any(label(i,a),label(i,b)) == label(i,any(a,b))
 * any(a,a) == a
 * any(a,b) == any(b,a)
 * any(any(a,b),c) == any(a,any(b,c))
 */
extern bool type_Any(const Base left, const Base right, Base*);

/* Tuples := index(i,a) | tuple(a,b) where a,b in Tuple
 *
 * tuple(index(i,a),index(i,b)) == index(i,any(a,b))
 * tuple(index(i,a),index(i,a)) == index(i,a)
 * tuple(a,b) == tuple(b,a)
 * tuple(a,tuple(b,c)) == tuple(tuple(a,b),c)
 */
extern bool type_Index(const unsigned index, const Base at, Base*);
extern bool type_Tuple(const Base left, const Base right, Base*);

/* Record := label(i,a) | record(a,b) where a,b in Record
 *
 * record(label(i,a),label(i,b)) == label(i,any(a,b))
 * record(label(i,a),label(i,a)) == label(i,a)
 * record(a,b) == record(b,a)
 * record(a,record(b,c)) == record(record(a,b),c)
 */
extern bool type_Label(const Symbol label, const Base at, Base*);
extern bool type_Record(const Base left, const Base right, Base*);

/* All := type | all(a,b) where a,b in All
 *
 * all(a,a) == a
 * all(a,b) == all(b,a)
 * all(all(a,b),c) == all(a,all(b,c))
 */
extern bool type_All(const Base left, const Base right, Base*);

/*
 * walk a type tree and call func for each node
 */
extern bool type_Map(Operator func, const Node type, const Node env, Target target);


extern bool type_Pi(Base*, ...);             /* Pi(var:type...):type      dependent-functions*/
extern bool type_Sigma(Base*, ...);          /* Sigma(var:type...):type   dependent-tuples*/
extern bool type_Mu(Base*, ...);             /* Mu(var:type):type         recursive-functions*/
extern bool type_Delta(Base*, ...);          /* Delta(var:type,predicate) subtypes */

extern bool type_Contains(const Base type, const Node value); /* */

extern void init_global_typetable();
extern void final_global_typetable();
extern void check_TypeTable__(const char* filename, unsigned line);

/******************
  inline functions
 ******************/

extern inline bool sort_Contains(const Base type, const Sort sort) __attribute__((always_inline));
extern inline bool sort_Contains(const Base type, const Sort sort) {
    if (!type) return false;
    if (!sort) return false;
    if (type->code == tc_constant) {
        (((Constant)type)->sort == sort);
    }
    if (type->code == tc_sort) return find_Axiom((Sort)type, sort);
    return false;
}

extern inline bool isATypeObj(const Node node) __attribute__((always_inline));
extern inline bool isATypeObj(const Node node) {
    if (isNil(node)) return false;
    Node ctor = getConstructor(node);
    if (!s_sort) return false; // symbols are constructed before types
    if (isIdentical(ctor, s_sort))   return true;
    if (isIdentical(ctor, s_axiom))  return true;
    if (isIdentical(ctor, s_rule))   return true;
    if (isIdentical(ctor, s_base))   return true;
    if (isIdentical(ctor, s_branch)) return true;
    if (isIdentical(ctor, s_index))  return true;
    if (isIdentical(ctor, s_label))  return true;
    return false;
}

extern inline bool isAType(const Node type) __attribute__((always_inline));
extern inline bool isAType(const Node type) {
    Kind kind = asKind(type);
    if (!kind)   return false;
    if (!s_base) return false; // symbols are constructed before types
    if (kind->constructor.symbol == s_sort)   return true;
    if (kind->constructor.symbol == s_base)   return true;
    if (kind->constructor.symbol == s_branch) return true;
    if (kind->constructor.symbol == s_index)  return true;
    if (kind->constructor.symbol == s_label)  return true;

    return false;
}

extern inline bool isASort(const Node sort) __attribute__((always_inline));
extern inline bool isASort(const Node sort) {
    Kind kind = asKind(sort);
    if (!kind)   return false;
    if (!s_sort) return false; // symbols are constructed before sorts
    return (kind->type.symbol == s_sort);
}

extern inline bool isType(const Node value, const Node type) __attribute__((always_inline));
extern inline bool isType(const Node value, const Node type) {
    Kind kind = asKind(value);
    if (!kind) return false;
    if (kind->type.reference == type.reference) return true;
    if (!isAType(type)) return false;
    return type_Contains(type.type, value);
}

extern inline bool fromCtor(const Node value, const Node ctor) __attribute__((always_inline));
extern inline bool fromCtor(const Node value, const Node ctor) {
    if (isNil(ctor)) return false;
    Kind kind = asKind(value);
    if (!kind) return false;
    return (kind->constructor.reference == ctor.reference);
}

extern inline const char* sort_Name(Sort sort) __attribute__((always_inline));
extern inline const char* sort_Name(Sort sort) {
    if (!sort) return "";
    return (const char*)(sort->name->value);
}

extern inline const char* type_SortName(Base type) __attribute__((always_inline));
extern inline const char* type_SortName(Base type) {
    if (!type) return "";
    if (type->code != tc_constant) return "";

    Constant type_const = (Constant)type;

    return (const char*)(type_const->sort->name->value);
}

extern inline const char* type_ConstantName(Base type) __attribute__((always_inline));
extern inline const char* type_ConstantName(Base type) {
    if (!type) return "";
    if (type->code != tc_constant) return "";

    struct type_constant* tconst = (Constant)type;

    return (const char*)(tconst->name->value);
}

extern inline const char* type_LabelName(Base type) __attribute__((always_inline));
extern inline const char* type_LabelName(Base type) {
    if (!type) return "";
    if (type->code != tc_label) return "";

    struct type_label* tlabel = (Label)type;

    return (const char*)(tlabel->label->value);
}

extern inline const Node type_LabelSlot(Base type) __attribute__((always_inline));
extern inline const Node type_LabelSlot(Base type) {
    if (!type) return NIL;
    if (type->code != tc_label) return NIL;

    struct type_label* tlabel = (Label)type;

    return (Node)tlabel->slot;
}

extern inline const unsigned type_IndexOffset(Base type) __attribute__((always_inline));
extern inline const unsigned type_IndexOffset(Base type) {
    if (!type) return 0;
    if (type->code != tc_index) return 0;

    struct type_index* tindex = (Index)type;

    return (tindex->index);
}

extern inline const Node type_IndexSlot(Base type) __attribute__((always_inline));
extern inline const Node type_IndexSlot(Base type) {
    if (!type) return NIL;
    if (type->code != tc_index) return NIL;

    struct type_index* tindex = (Index)type;

    return (Node)tindex->slot;
}

extern inline bool isPair(const Node value) __attribute__((always_inline));
extern inline bool isPair(const Node value) {
    return fromCtor(value, s_pair);
}

extern inline bool isTuple(const Node value) __attribute__((always_inline));
extern inline bool isTuple(const Node value) {
    return fromCtor(value, s_tuple);
}

extern inline bool isText(const Node value) __attribute__((always_inline));
extern inline bool isText(const Node value) {
    return fromCtor(value, s_text);
}

extern inline bool isInteger(const Node value) __attribute__((always_inline));
extern inline bool isInteger(const Node value) {
    return fromCtor(value, s_integer);
}

extern inline bool isForm(const Node value) __attribute__((always_inline));
extern inline bool isForm(const Node value) {
    return fromCtor(value, s_form);
}

extern inline bool isLambda(const Node value) __attribute__((always_inline));
extern inline bool isLambda(const Node value) {
    return fromCtor(value, s_lambda);
}

extern inline bool isPrimitive(const Node value) __attribute__((always_inline));
extern inline bool isPrimitive(const Node value) {
    return fromCtor(value, s_primitive);
}

extern inline bool isForced(const Node value) __attribute__((always_inline));
extern inline bool isForced(const Node value) {
    return fromCtor(value, s_forced);
}

extern inline bool isFixed(const Node value) __attribute__((always_inline));
extern inline bool isFixed(const Node value) {
    return fromCtor(value, s_fixed);
}

extern inline bool isSymbol(const Node value) __attribute__((always_inline));
extern inline bool isSymbol(const Node value) {
    return fromCtor(value, s_symbol);
}

extern inline bool isBlock(const Node value) __attribute__((always_inline));
extern inline bool isBlock(const Node value) {
    return fromCtor(value, s_block);
}

/***************************
 ** end of file
 **************************/
#endif
