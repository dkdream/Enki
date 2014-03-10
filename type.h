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
    tc_type,  // type
    tc_sort,  // sort
    tc_axiom, // axiom
    tc_rule,  // rule
} type_code;

// s_type is a type constant
// s_sort is a sort constant
struct constant {
    HashCode hashcode; type_code code;
    Symbol name;
};

// s_axiom is a axiom pair (c:s)
struct axiom {
    HashCode hashcode; type_code code;
    Constant element; // type or sort
    Constant class;   // sort
};

// s_rule is a rule  triple (in=s1,out=s2,kind=s3)
struct rule {
    HashCode hashcode; type_code code;
    Symbol   functor;
    Constant xxx; // sort
    Constant yyy; // sort
    Constant zzz; // sort
};

extern Constant zero_s;   // the sort of all base types (including t_nil)

// zero_s
extern Constant t_ASTree;  // the type of an Abstract Syntax Tree
extern Constant t_nil;     // the type of the value nil
extern Constant t_assert;  // the type of the value true
extern Constant t_integer; // the type of integer values
extern Constant t_pair;    // the union of all pairs types
extern Constant t_symbol;  // the union of all symbol types
extern Constant t_text;    // the union of all text types
extern Constant t_tuple;   // the union of all tuple types
extern Constant t_arrow;   // the union of all arrow types
extern Constant t_unknown; // the type of the void value

extern Constant t_buffer;       // the type of a c-text-buffer
extern Constant t_closed;       // the type of a closed object
extern Constant t_continuation; // the type of an escape
extern Constant t_infile;       // the type of a c-os-infile
extern Constant t_outfile;      // the type of a c-os-infile

extern bool sort_Create(const Symbol, Constant*); /* each sort has a unique name */
extern bool type_Create(const Symbol, const Constant, Constant*); /* each type has a unique name and is in a unique sort */

extern bool find_Axiom(const Constant, const Constant);
extern bool make_Axiom(const Constant, const Constant);

extern bool find_Rule(const Symbol, const Constant, const Constant, const Constant);
extern bool make_Rule(const Symbol, const Constant, const Constant, const Constant);

extern bool type_Contains(const Constant type, const Node value); /* */

extern bool compute_Type(Node value, Target result);

/* Any := type | any(a,b) where a,b in Any
 * any(a,void) == any(void,a) == a
 * any(index(i,a),index(i,b)) == index(i,any(a,b))
 * any(label(i,a),label(i,b)) == label(i,any(a,b))
 * any(a,a) == a
 * any(a,b) == any(b,a)
 * any(any(a,b),c) == any(a,any(b,c))
 */

/* Tuples := index(i,a) where i in Count and a in Type
 *        |  tuple(a,b) where a,b in Tuple
 *
 * tuple(index(i,a),index(i,b)) == index(i,any(a,b))
 * tuple(index(i,a),index(i,a)) == index(i,a)
 * tuple(a,b) == tuple(b,a)
 * tuple(a,tuple(b,c)) == tuple(tuple(a,b),c)
 */

/* Record := label(i,a)  where i in Symbol and a in Type
 *        |  record(a,b) where a,b in Record
 *
 * record(label(i,a),label(i,b)) == label(i,any(a,b))
 * record(label(i,a),label(i,a)) == label(i,a)
 * record(a,b) == record(b,a)
 * record(a,record(b,c)) == record(record(a,b),c)
 */

/* All := type | all(a,b) where a,b in All
 *
 * all(a,a) == a
 * all(a,b) == all(b,a)
 * all(all(a,b),c) == all(a,all(b,c))
 */

/*
 * self(a,b)
 */

/*
 * walk a type tree and call func for each node
 */

extern void init_global_typetable(Clink *roots);
extern void final_global_typetable();
extern void check_TypeTable__(const char* filename, unsigned line);

/******************
  inline functions
 ******************/

extern inline bool sort_Contains(const Constant sort, const Constant type) __attribute__((always_inline));
extern inline bool sort_Contains(const Constant sort, const Constant type) {
    if (!type) return false;
    if (!sort) return false;
    if (type->code == tc_type) return find_Axiom((Constant)type, sort);
    if (type->code == tc_sort) return find_Axiom((Constant)type, sort);
    return false;
}

extern inline bool isATypeObj(const Node node) __attribute__((always_inline));
extern inline bool isATypeObj(const Node node) {
    if (isNil(node)) return false;
    const Symbol ctor = getConstructor(node);
    if (!s_sort) return false; // symbols are constructed before types
    if (isIdentical(ctor, s_type))   return true;
    if (isIdentical(ctor, s_sort))   return true;
    if (isIdentical(ctor, s_axiom))  return true;
    if (isIdentical(ctor, s_rule))   return true;
    return false;
}

extern inline bool isAType(const Node type) __attribute__((always_inline));
extern inline bool isAType(const Node type) {
    Kind kind = asKind(type);
    if (!kind)   return false;
    if (!s_type) return false; // symbols are constructed before types
    if (kind->constructor == s_type)   return true;
    if (kind->constructor == s_sort)   return true;
    return false;
}

extern inline bool isASort(const Node sort) __attribute__((always_inline));
extern inline bool isASort(const Node sort) {
    Kind kind = asKind(sort);
    if (!kind)   return false;
    if (!s_sort) return false; // symbols are constructed before sorts
    return (kind->type.symbol == s_sort);
}

extern inline bool inType(const Node value, const Node type) __attribute__((always_inline));
extern inline bool inType(const Node value, const Node type) {
    Kind kind = asKind(value);
    if (!kind) return false;
    if (kind->type.reference == type.reference) return true;
    if (!isAType(type)) return false;
    return type_Contains(type.constant, value);
}

extern inline bool inSort(const Node value, const Node sort) __attribute__((always_inline));
extern inline bool inSort(const Node value, const Node sort) {
    Kind kind = asKind(value);
    if (!kind) return false;
    if (!isASort(sort)) return false;
    return sort_Contains(sort.constant, kind->type.constant);
}

extern inline bool fromCtor(const Node value, const Node ctor) __attribute__((always_inline));
extern inline bool fromCtor(const Node value, const Node ctor) {
    if (isNil(ctor)) return false;
    Kind kind = asKind(value);
    if (!kind) return false;
    return (kind->constructor == ctor.symbol);
}

extern inline const char* sort_Name(Constant sort) __attribute__((always_inline));
extern inline const char* sort_Name(Constant sort) {
    if (!sort) return "";
    return (const char*)(sort->name->value);
}

extern inline const char* type_ConstantName(Constant type) __attribute__((always_inline));
extern inline const char* type_ConstantName(Constant type) {
    if (!type) return "";
    if (type->code != tc_type) return "";
    return (const char*)(type->name->value);
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

extern inline bool isDelayed(const Node value) __attribute__((always_inline));
extern inline bool isDelayed(const Node value) {
    return fromCtor(value, s_delay);
}

extern inline bool isBoxed(const Node value) __attribute__((always_inline));
extern inline bool isBoxed(const Node value) {
    return fromCtor(value, s_box);
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

extern inline bool isQuote(const Node value) __attribute__((always_inline));
extern inline bool isQuote(const Node value) {
    return fromCtor(value, s_quote);
}

extern inline bool isVariable(const Node value) __attribute__((always_inline));
extern inline bool isVariable(const Node value) {
    return fromCtor(value, s_variable);
}

extern inline bool isEscape(const Node value) __attribute__((always_inline));
extern inline bool isEscape(const Node value) {
    return fromCtor(value, s_escape);
}

extern inline bool isComposite(const Node value) __attribute__((always_inline));
extern inline bool isComposite(const Node value) {
    return fromCtor(value, s_composite);
}



/***************************
 ** end of file
 **************************/
#endif
