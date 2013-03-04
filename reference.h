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

typedef enum node_type {
    nt_unknown,
    nt_integer,
    nt_pair,
    nt_primitive,  // like maru Subr
    nt_symbol,
    nt_text,
    nt_tuple,      // fixed size tuple (like maru _Array)

    // these use pairs
    nt_expression, // to simulate maru Expr  (expr env)
    nt_form,       // to simulate maru Form  (expr nil)
    nt_fixed,      // to simulate maru Fixed (expr nil)
} EA_Type;

typedef unsigned long long HashCode;
typedef unsigned long      Size;

typedef void *Reference;
/*    */
typedef struct integer        *Integer;
typedef struct pair           *Pair;
typedef struct primitive      *Primitive;
typedef struct symbol         *Symbol;
typedef struct text           *Text;
typedef struct tuple          *Tuple;
/*    */

union  __attribute__ ((__transparent_union__ __packed__))
node {
    Reference     reference;
    /**/
    Integer       integer;
    Pair          pair;
    Primitive     primitive;
    Symbol        symbol;
    Text          text;
    Tuple         tuple;
    /**/
};

typedef union node Node;

union __attribute__ ((__transparent_union__ __packed__))
node_target {
    Reference     *reference;
    /**/
    Integer       *integer;
    Pair          *pair;
    Primitive     *primitive;
    Symbol        *symbol;
    Text          *text;
    Tuple         *tuple;
    /**/
    Node          *node;
    /**/
};

typedef union node_target Target;

#define NIL       ((Node)((Reference)0))
#define NIL_CODE  ((Code)((Reference)0))

#define ASSIGN(target, node) (target.reference[0] = node.reference)

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
extern void startEnkiLibrary();
extern void stopEnkiLibrary();

/***************************
 ** end of file
 **************************/
#endif
