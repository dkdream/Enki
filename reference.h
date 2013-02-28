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
    nt_count,
    nt_input,
    nt_integer,
    nt_output,
    nt_pair,
    nt_primitive,
    nt_symbol,
    nt_text,
    nt_tuple,
#if 0
    nt_hash,
    nt_hash_block,
    nt_hash_entry,
    nt_set,
    nt_set_block,
    nt_set_cell,
#endif
} EA_Type;

typedef enum result_code {
    rc_continue,
    rc_done,
    rc_signal,
    rc_throw,
    rc_error,
} ResultCode;

typedef void *Reference;
/*    */
typedef struct count          *Count;
typedef struct input          *Input;
typedef struct integer        *Integer;
typedef struct output         *Output;
typedef struct pair           *Pair;
typedef struct primitive      *Primitive;
typedef struct symbol         *Symbol;
typedef struct text           *Text;
typedef struct tuple          *Tuple;

#ifdef USE_HASH
typedef struct hash           *Hash;
typedef struct hash_block     *Hash_block;
typedef struct hash_entry     *Hash_entry;
#endif

#ifdef USE_SET
typedef struct set            *Set;
typedef struct set_block      *Set_block;
typedef struct set_cell       *Set_cell;
#endif
/*    */

union node {
    Reference     reference;
    /**/
    Count         count;
    Input         input;
    Integer       integer;
    Output        output;
    Pair          pair;
    Primitive     primitive;
    Symbol        symbol;
    Text          text;
    Tuple         tuple;
#if 0
    Hash          hash;
    Hash_block    hash_block;
    Hash_entry    hash_entry;
    Set           set;
    Set_block     set_block;
    Set_cell      set_cell;
#endif
    /**/
}  __attribute__ ((__transparent_union__));

union node_target {
    Reference     *reference;
    /**/
    Count         *count;
    Input         *input;
    Integer       *integer;
    Output        *output;
    Pair          *pair;
    Primitive     *primitive;
    Symbol        *symbol;
    Text          *text;
    Tuple         *tuple;
#if 0
    Hash          *hash;
    Hash_block    *hash_block;
    Hash_entry    *hash_entry;
    Set           *set;
    Set_block     *set_block;
    Set_cell      *set_cell;
#endif
    /**/
}  __attribute__ ((__transparent_union__));

typedef union node        Node;
typedef union node_target Target;

typedef unsigned long long HashCode;
typedef unsigned long      Size;

#define NIL       ((Node)((Reference)0))
#define NIL_CODE  ((Code)((Reference)0))

extern unsigned int ea_global_debug;
extern struct gc_treadmill* _zero_space;
extern struct gc_header*    _fooness;

extern void debug_Message(const char *filename, unsigned int linenum, bool newline, const char *format, ...);

/* macros */

#define VM_ERROR(args...) error_at_line(1, 0,  __FILE__,  __LINE__, args)

#if 1
#define VM_DEBUG(level, args...) ({ typeof (level) hold__ = (level); if (hold__ <= ea_global_debug) debug_Message(__FILE__,  __LINE__, true, args); })
#else
#define VM_DEBUG(level, args...) vm_noop()
#endif

#if 1
#define VM_ON_DEBUG(level, arg) ({ typeof (level) hold__ = (level); if (hold__ <= ea_global_debug) arg; })
#else
#define VM_ON_DEBUG(level, args...) vm_noop()
#endif

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
