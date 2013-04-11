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

struct sort {
    HashCode hashcode;
    Symbol   name;
};

struct type {
    HashCode hashcode;
    Sort sort;
    union {
        Symbol name;
        Type   item[1];
    };
};

extern Sort zero_s;

extern Type t_any;
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
extern Type t_primitive;
extern Type t_text;
extern Type t_true;
extern Type t_tuple;
extern Type t_buffer;

extern bool sort_Create(Symbol,Sort*);                /* each sort has a unique name */
extern bool type_Create(Symbol,Sort,Type*);           /* each basetype in a sort has a unique name */
extern bool rule_Create(Symbol,Sort,Sort,Sort,Rule*); /* add a new rule for symbol (pi,sigma) */
extern bool name_Create(Sort,Name*);                  /* construct a new type variable */

extern bool type_Pi(Type*, ...);             /* Pi(var:type...):type      dependent-functions*/
extern bool type_Sigma(Type*, ...);          /* Sigman(var:type...):type  dependent-tuples*/
extern bool type_Mu(Type*, ...);             /* Mu(var:type):type         recursive-functions*/
extern bool type_Delta(Type*, ...);          /* Delta(var:type,predicate) subtypes */

/* to construct a tuple type use type_Sigma without variables */
/* to construct a lambda type use type_Pi without variables */
/* all base-types are symbols. */

/* is the value a member of type
   example:   type_Member(integer, 1);
   example:   type_Member(symbol, 'symbol);
 */
extern bool type_Member(Type type, Node value);

/* match values with type
   example:   type_Match([integer,symbol,string], 3, 1, 'foo, "bar");
   example:   type_Match([integer], 1, 1);
 */
extern bool type_Match(Type type, unsigned size, ...);


extern void init_global_typetable();
extern void final_global_typetable();
extern void check_TypeTable__(const char* filename, unsigned line);

/***************************
 ** end of file
 **************************/
#endif
