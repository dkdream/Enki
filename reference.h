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

#define GCC_VERSION (__GNUC__ * 10000                 \
                     + __GNUC_MINOR__ * 100           \
                     + __GNUC_PATCHLEVEL__)

/* Test for GCC > 4.3.0 */
#if GCC_VERSION > 40300
#define HOT __attribute__ ((hot))
#else
#define HOT
#endif

typedef unsigned long      Size;

typedef void *Reference;
/*    */
typedef struct atomic        *Atomic; /* operator */
typedef struct axiom         *Axiom;
typedef struct composite     *Composite; /* operator */
typedef struct integer       *Integer;
typedef struct name          *Name;
typedef struct pair          *Pair;
typedef struct primitive     *Primitive; /* operator */
typedef struct rule          *Rule;
typedef struct symbol        *Symbol;
typedef struct text          *Text;
typedef struct tuple         *Tuple;
typedef struct constant      *Constant;
typedef struct variable      *Variable;
/*    */

union
node {
    long      value;
    /**/
    Reference reference;
    /**/
    Atomic    atomic;
    Composite composite;
    Primitive primitive;
    /**/
    Integer   integer;
    Pair      pair;
    Symbol    symbol;
    Text      text;
    Tuple     tuple;
    /**/
    Constant  constant;
    /**/
    Axiom     axiom;
    Rule      rule;
    /**/
    Variable  variable;
    /**/
}   __attribute__ ((__transparent_union__, __packed__));

typedef union node Node;

union
node_target {
    long      *value;
    /**/
    Reference *reference;
    /**/
    Atomic    *atomic;
    Composite *composite;
    Primitive *primitive;
    /**/
    Integer   *integer;
    Pair      *pair;
    Symbol    *symbol;
    Text      *text;
    Tuple     *tuple;
    /**/
    Constant  *constant;
    /**/
    Axiom     *axiom;
    Rule      *rule;
    Name      *name;
    /**/
    Variable  *variable;
    /**/
    Node      *node;
    /**/
}  __attribute__ ((__transparent_union__, __packed__));

typedef union node_target Target;

#define NIL  ((Node)((Reference)0))
#define VOID ((Node)((Reference)-1))

extern Node void_v;
extern Node true_v;

#define ASSIGN(target, node) enki_assign(target, node)

static inline void enki_assign(Target var, const Node value)  __attribute__((always_inline));
static inline void enki_assign(Target var, const Node value) {
  var.reference[0] = value.reference;
}

static inline void enki_noop() __attribute__((always_inline));
static inline void enki_noop() { }

static inline bool isNil(Node node) __attribute__((always_inline));
static inline bool isNil(Node node) {
    return 0 == node.reference;
}

static inline bool isIdentical(const Node left, const Node right)  __attribute__((always_inline));
static inline bool isIdentical(const Node left, const Node right) {
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
