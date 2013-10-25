/*-*- mode: c;-*-*/
#if !defined(_ea_reference_h_)
#define _ea_reference_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **
 ** Coding Invariant:
 **
 ** the caller is responsible for making sure that ALL parameters
 ** passed to a functions are seen by the garbage collector.
 ** (i.e. in a live root, or list)
 **
 ** No allocate node is passed back as a return value. it MUST
 ** be passed back thur a Target parameters.
 **
 ** the caller is responsible for making sure that the address
 ** in a Target parameter is not an alias to a live slot that
 ** is holding another parameter.
 **
 ** All functions must return at least a bool if there is something
 ** that can go wrong to inform the virtual machine when something
 ** went wrong. (true == ok, false == didnt work)
 **
 ** Currently there is one gc space (_empty_space)
 **/

#include <stdbool.h>
#include <error.h>


typedef unsigned long      Size;

typedef void *Reference;
/*    */
typedef struct integer       *Integer;
typedef struct pair          *Pair;
typedef struct primitive     *Primitive;
typedef struct symbol        *Symbol;
typedef struct text          *Text;
typedef struct tuple         *Tuple;
typedef struct type_base     *Base;
typedef struct type_constant *Constant;
typedef struct type_index    *Index;
typedef struct type_label    *Label;
typedef struct type_branch   *Branch;
typedef struct axiom         *Axiom;
typedef struct rule          *Rule;
typedef struct name          *Name;
typedef struct variable      *Variable;
/*    */

union  __attribute__ ((__transparent_union__ __packed__))
node {
    long      value;
    /**/
    Reference reference;
    /**/
    Integer   integer;
    Pair      pair;
    Primitive primitive;
    Symbol    symbol;
    Text      text;
    Tuple     tuple;
    /**/
    Base      type;
    /**/
    Constant  constant;
    Index     index;
    Label     label;
    Branch    branch;
    /**/
    Axiom     axiom;
    Rule      rule;
    Name      name;
    /**/
    Variable  variable;
    /**/
};

typedef union node Node;

union __attribute__ ((__transparent_union__ __packed__))
node_target {
    long      *value;
    /**/
    Reference *reference;
    /**/
    Integer   *integer;
    Pair      *pair;
    Primitive *primitive;
    Symbol    *symbol;
    Text      *text;
    Tuple     *tuple;
    /**/
    Base      *type;
    /**/
    Constant  *constant;
    Index     *index;
    Label     *label;
    Branch    *branch;
    /**/
    Axiom     *axiom;
    Rule      *rule;
    Name      *name;
    /**/
    Variable  *variable;
    /**/
    Node      *node;
    /**/
};

typedef union node_target Target;

#define NIL  ((Node)((Reference)0))
#define VOID ((Node)((Reference)-1))

extern Node void_v;
extern Node true_v;
extern Node false_v;
extern Node unit_v;

#define ASSIGN(target, node) enki_assign(target, node)

extern inline void enki_assign(Target var, const Node value)  __attribute__((always_inline));
extern inline void enki_assign(Target var, const Node value) {
  var.reference[0] = value.reference;
}

extern inline void enki_noop() __attribute__((always_inline));
extern inline void enki_noop() { }

extern inline bool isNil(Node node) __attribute__((always_inline));
extern inline bool isNil(Node node) {
    return 0 == node.reference;
}

extern inline bool isIdentical(const Node left, const Node right)  __attribute__((always_inline));
extern inline bool isIdentical(const Node left, const Node right) {
    return (left.reference == right.reference);
}

/* */
extern Node enki_globals;
extern void startEnkiLibrary();
extern void stopEnkiLibrary();

/***************************
 ** end of file
 **************************/
#endif
