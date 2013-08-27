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

struct sort {
    HashCode hashcode;
    Symbol   name;
};

enum type_code {
    tc_constant,
    tc_index,
    tc_label,
    tc_union,  // type_branch
    tc_tuple,  // type_branch
    tc_record, // type_branch
};

struct type {
    HashCode hashcode; Sort sort; enum type_code code;
    HashCode marker[0];
};

struct type_constant {
    HashCode hashcode; Sort sort; enum type_code code;
    Symbol name;
};

struct type_index {
    HashCode hashcode; Sort sort; enum type_code code;
    unsigned index;
    Type     slot;
};

struct type_label {
    HashCode hashcode; Sort sort; enum type_code code;
    Symbol label;
    Type   slot;
};

struct type_branch {
    HashCode hashcode; Sort sort; enum type_code code;
    Type left;
    Type right;
};

extern Sort void_s;   // the sort with NO types
extern Sort zero_s;   // the sort of values
extern Sort symbol_s; // the sort of symbols
extern Sort opaque_s; // the sort of opaque types

extern Type t_integer; // the type of integer values
extern Type t_pair;    // the union of all pairs types
extern Type t_symbol;  // the union of all symbol types
extern Type t_text;    // the union of all text types
extern Type t_tuple;   // the union of all tuple types

extern Type t_any;     // the union of all type of sort Zero
extern Type t_buffer;  // the type of a c-text-buffer
extern Type t_false;   // the type of the value void
extern Type t_infile;  // the type of a c-os-infile
extern Type t_nil;     // the type of the value nil
extern Type t_opaque;  // all raw collections are this type
extern Type t_outfile; // the type of a c-os-infile
extern Type t_true;    // the type of the value true
extern Type t_void;    // the intersection of all type of sort Zero

extern bool sort_Create(Symbol,Sort*);      /* each sort has a unique name */
extern bool type_Create(Symbol,Sort,Type*); /* each type constant in a sort has a unique name */

extern bool make_Axiom(Sort, Sort);
extern bool make_Rule(Symbol, Sort, Sort, Sort);

/* Forall S, a, b    where a:S and b:S         then union(a,b):S */
/* Forall S, a, b    where a:S and b:S         then union(a,b) == union(b,a) */
/* Forall S, a, b, c where a:S and b:S and c:S then union(union(a,b),c) == union(a,union(b,c)) */
/* currently unions are mono-sorted */
extern bool type_Union(const Type left, const Type right, Type*);

/* Tuples := index(i,a) | tuple(a,b) where a,b in Tuple */
/* tuple(a,b) == tuple(b,a) */
/* tuple(a,tuple(b,c)) == tuple(tuple(a,b),c) */
extern bool type_Index(const unsigned index, const Type at, Type*);
extern bool type_Tuple(const Type left, const Type right, Type*);

/* Record := label(i,a) | record(a,b) where a,b in Record */
/* record(a,b) == record(b,a) */
/* record(a,record(b,c)) == record(record(a,b),c) */
extern bool type_Label(const Symbol label, const Type at, Type*);
extern bool type_Record(const Type left, const Type right, Type*);


extern bool type_Pi(Type*, ...);             /* Pi(var:type...):type      dependent-functions*/
extern bool type_Sigma(Type*, ...);          /* Sigma(var:type...):type   dependent-tuples*/
extern bool type_Mu(Type*, ...);             /* Mu(var:type):type         recursive-functions*/
extern bool type_Delta(Type*, ...);          /* Delta(var:type,predicate) subtypes */

extern bool type_Contains(const Type type, const Node value); /* */

extern void init_global_typetable();
extern void final_global_typetable();
extern void check_TypeTable__(const char* filename, unsigned line);

/******************
  inline functions
 ******************/

extern inline bool sort_Contains(const Type type, const Sort sort) __attribute__((always_inline));
extern inline bool sort_Contains(const Type type, const Sort sort) {
    if (!type) return false;
    if (!sort) return false;
    return (type->sort == sort);
}

extern inline bool isAType(const Node type) __attribute__((always_inline));
extern inline bool isAType(const Node type) {
    Kind kind = asKind(type);
    if (!kind)   return false;
    if (!s_base) return false; // symbols are constructed before types
    if (!s_type) return false; // symbols are constructed before types
    if (kind->type.symbol == s_base) return true;
    if (kind->type.symbol == s_type) return true;
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

extern inline const char* type_SortName(Type type) __attribute__((always_inline));
extern inline const char* type_SortName(Type type) {
    if (!type) return "";

    return (const char*)(type->sort->name->value);
}

extern inline const char* type_ConstantName(Type type) __attribute__((always_inline));
extern inline const char* type_ConstantName(Type type) {
    if (!type) return "";
    if (type->code != tc_constant) return "";

    struct type_constant* tconst = (struct type_constant*) type;

    return (const char*)(tconst->name->value);
}

extern inline const char* type_LabelName(Type type) __attribute__((always_inline));
extern inline const char* type_LabelName(Type type) {
    if (!type) return "";
    if (type->code != tc_label) return "";

    struct type_label* tlabel = (struct type_label*) type;

    return (const char*)(tlabel->label->value);
}

extern inline const Type type_LabelSlot(Type type) __attribute__((always_inline));
extern inline const Type type_LabelSlot(Type type) {
    if (!type) return 0;
    if (type->code != tc_label) return 0;

    struct type_label* tlabel = (struct type_label*) type;

    return tlabel->slot;
}

extern inline const unsigned type_IndexOffset(Type type) __attribute__((always_inline));
extern inline const unsigned type_IndexOffset(Type type) {
    if (!type) return 0;
    if (type->code != tc_index) return 0;

    struct type_index* tindex = (struct type_index*) type;

    return (tindex->index);
}

extern inline const Type type_IndexSlot(Type type) __attribute__((always_inline));
extern inline const Type type_IndexSlot(Type type) {
    if (!type) return 0;
    if (type->code != tc_index) return 0;

    struct type_index* tindex = (struct type_index*) type;

    return tindex->slot;
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
