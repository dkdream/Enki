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

extern Sort void_s;
extern Sort zero_s;

extern Type t_any;
extern Type t_block;
extern Type t_buffer;
extern Type t_comma;
extern Type t_delay;
extern Type t_fixed;
extern Type t_forced;
extern Type t_form;
extern Type t_infile;
extern Type t_integer;
extern Type t_lambda;
extern Type t_opaque;
extern Type t_outfile;
extern Type t_pair;
extern Type t_path;
extern Type t_primitive;
extern Type t_semi;
extern Type t_text;
extern Type t_true;
extern Type t_tuple;
extern Type t_void;

extern bool sort_Create(Symbol,Sort*);                /* each sort has a unique name */
extern bool rule_Create(Symbol,Sort,Sort,Sort,Rule*); /* add a new rule for symbol (pi,sigma) */
extern bool name_Create(Sort,Name*);                  /* construct a new type variable */

extern bool type_Create(Symbol,Sort,Type*); /* each type constant in a sort has a unique name */

/* Forall S, a, b    where a:S and b:S         then union(a,b):S */
/* Forall S, a, b    where a:S and b:S         then union(a,b) == union(b,a) */
/* Forall S, a, b, c where a:S and b:S and c:S then union(union(a,b),c) == union(a,union(b,c)) */
extern bool type_Union(const Type left, const Type right, Type*);

/* Forall S, i, a       where a:S and i>0          then tuple(i:a):S */
/* Forall S, i, j, a, b where i!=j and a:S and b:s then tuple(tuple(i:a),tuple(j:b)):S */
/* Forall S, i, j, a, b where i!=j and a:S and b:s then tuple(tuple(i:a),tuple(j:b)) == tuple(tuple(j:b),tuple(i:a)) */
extern bool type_Tuple(const unsigned index, const Type at, Type*);
extern bool type_TupleExtend(const Type left, const Type right, Type*);


/* Forall S, i, a       where a:S and i:Symbol     then record(i:a):S */
/* Forall S, i, j, a, b where i!=j and a:S and b:s then record(record(i:a),record(j:b)):S */
/* Forall S, i, j, a, b where i!=j and a:S and b:s then record(record(i:a),record(j:b)) == record(record(j:b),record(i:a)) */
extern bool type_Record(const Symbol label, const Type at, Type*);
extern bool type_RecordExtend(const Type left, const Type right, Type*);


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
    if (!kind) return false;
    if (!s_type) return false; // symbols are constructed before types
    return (kind->type.symbol == s_type);
}

extern inline bool isASort(const Node sort) __attribute__((always_inline));
extern inline bool isASort(const Node sort) {
    Kind kind = asKind(sort);
    if (!kind) return false;
    if (!s_type) return false; // symbols are constructed before sorts
    return (kind->type.symbol == s_type);
}

extern inline bool isType(const Node value, const Node type) __attribute__((always_inline));
extern inline bool isType(const Node value, const Node type) {
    Kind kind = asKind(value);
    if (!kind) return false;
    if (kind->type.reference == type.reference) return true;
    if (!isAType(type)) return false;
    return type_Contains(type.type, value);
}

extern inline const char* type_ConstantName(Type type) __attribute__((always_inline));
extern inline const char* type_ConstantName(Type type) {
    if (!type) return "";
    if (type->code != tc_constant) return "";

    struct type_constant* tconst = (struct type_constant*) type;

    return (const char*)(tconst->name->value);
}

/***************************
 ** end of file
 **************************/
#endif
