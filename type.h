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
    tc_index,    //
    tc_label,    //
    tc_constant, //
    tc_sort,     //
    tc_self,     // singleton type
    tc_refine,   //
    tc_tuple,    // type_branch (indexed collection of types )
    tc_record,   // type_branch (labeled collection of types )
    tc_any,      // type_branch (union of types )
    tc_all,      // type_branch (intersection of types)
} type_code;

struct type_base {
    HashCode hashcode; type_code code;
};

// s_axiom is a axiom pair (c:s)
struct axiom {
    HashCode hashcode;
    Constant element;
    Constant class;
};

// s_rule is a rule  triple (in=s1,out=s2,kind=s3)
struct rule {
    HashCode hashcode;
    Symbol   functor;
    Constant xxx, yyy, zzz;
};

// s_base is a type constant
// s_sort is a sort constant
struct type_constant {
    HashCode hashcode; type_code code;
    Symbol name;
};

// s_self
enum self_kind {
    sk_undefined,
    sk_integer,
    sk_symbol,
    sk_text,
};

struct type_self {
    HashCode hashcode; type_code code;
    Base base;
    enum self_kind kind;
    long long buffer[1]; //hold aleast an integer
};

struct type_refine {
    HashCode hashcode; type_code code;
    Base slot;
    Node predicate; // must be in the external_references
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
    Base sort;
};

extern Constant opaque_s;    // the sort of opaque types
extern Constant zero_s;      // the sort of values

extern Constant boolean_s;   // the sort with true and false values
extern Constant undefined_s; // the sort with all possible values (including the void and nil)
extern Constant unit_s;      // the sort with the unit value
extern Constant void_s;      // the sort with no values

extern Constant t_ASTree; // the type of an Abstract Syntax Tree

extern Constant t_nil;  // the type of the value nil

extern Constant t_integer; // the type of integer values
extern Constant t_pair;    // the union of all pairs types
extern Constant t_symbol;  // the union of all symbol types
extern Constant t_text;    // the union of all text types
extern Constant t_tuple;   // the union of all tuple types
extern Constant t_arrow;   // the union of all arrow types

extern Constant t_buffer;       // the type of a c-text-buffer
extern Constant t_closed;       // the type of a closed object
extern Constant t_continuation; // the type of an escape
extern Constant t_infile;       // the type of a c-os-infile
extern Constant t_outfile;      // the type of a c-os-infile

extern bool sort_Create(const Symbol,Constant*); /* each sort has a unique name */
extern bool type_Create(const Symbol, const Constant, Constant*); /* each type constant in a sort has a unique name */

extern bool find_Axiom(const Constant, const Constant);
extern bool make_Axiom(const Constant, const Constant);

extern bool find_Rule(const Symbol, const Constant, const Constant, const Constant);
extern bool make_Rule(const Symbol, const Constant, const Constant, const Constant);

extern bool compute_Sort(Base value, Target result);
extern bool compute_Type(Node value, Target result);

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
 * self(a,b)
 */
extern bool type_Self(const Base left, const Node right, Base*);


/*
 * walk a type tree and call func for each node
 */
extern bool type_Map(Operator func, const Node type, const Node env, Target target);


extern bool type_Pi(Base*, ...);             /* Pi(var:type...):type      dependent-functions*/
extern bool type_Sigma(Base*, ...);          /* Sigma(var:type...):type   dependent-tuples*/
extern bool type_Mu(Base*, ...);             /* Mu(var:type):type         recursive-functions*/
extern bool type_Delta(Base*, ...);          /* Delta(var:type,predicate) subtypes */

extern bool type_Contains(const Base type, const Node value); /* */

extern void init_global_typetable(Clink *roots);
extern void final_global_typetable();
extern void check_TypeTable__(const char* filename, unsigned line);

/******************
  inline functions
 ******************/

extern inline bool sort_Contains(const Constant sort, const Base type) __attribute__((always_inline));
extern inline bool sort_Contains(const Constant sort, const Base type) {
    if (!type) return false;
    if (!sort) return false;
    if (type->code == tc_constant) return find_Axiom((Constant)type, sort);
    if (type->code == tc_sort)     return find_Axiom((Constant)type, sort);
    return false;
}

extern inline bool isATypeObj(const Node node) __attribute__((always_inline));
extern inline bool isATypeObj(const Node node) {
    if (isNil(node)) return false;
    const Symbol ctor = getConstructor(node);
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
    if (kind->constructor == s_sort)   return true;
    if (kind->constructor == s_base)   return true;
    if (kind->constructor == s_branch) return true;
    if (kind->constructor == s_index)  return true;
    if (kind->constructor == s_label)  return true;

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
    return type_Contains(type.type, value);
}

extern inline bool inSort(const Node value, const Node sort) __attribute__((always_inline));
extern inline bool inSort(const Node value, const Node sort) {
    Kind kind = asKind(value);
    if (!kind) return false;
    if (!isASort(sort)) return false;
    return sort_Contains(sort.constant, kind->type.type);
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
