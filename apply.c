/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "apply.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>


// add a stack slot to the root set (with markings __FILE__, __LINE__)
#define GC_PROTECT(V)
// remove a stack slot from the root set
#define GC_UNPROTECT(V)

union Object;
typedef union Object *oop;

enum { oop_Undefined,
       oop_Long,
       oop_String,
       oop_Symbol,
       oop_Pair,
       _oop_Array,
       oop_Array,
       oop_Expr,
       oop_Form,
       oop_Fixed,
       oop_Subr
};

typedef oop (*imp_t)(oop args, oop env);
#define oop_nil ((oop)0)
#define oop_eof ((oop)-1)

static int   getType(oop);
static bool  is(int,oop);
static oop   car(oop);
static oop   cdr(oop);
static oop   cdr(oop);
static oop   assq(oop,oop);
static oop   map(imp_t,oop,oop);
static void  fdump(FILE*, oop);
static void  fdumpln(FILE*, oop);
static oop   arrayAt(oop, int);
static oop   arrayAtPut(oop, int, oop);
static oop   newPair(oop,oop);
static oop   newLong(int);


static imp_t getImp(oop);
static oop   getFunc(oop);

static oop oop_read(FILE*);
static oop oop_apply(oop fun, oop args, oop env);
static oop oop_expand(oop expr, oop env);
static oop oop_encode(oop expr, oop env);
static oop oop_eval(oop obj, oop env);
static void oop_fatal(char *reason, ...);
static void oop_replFile(FILE *stream);


static oop oop_applicators;

static oop oop_s_quote;
static oop oop_p_apply_expr;
static oop oop_p_eval_symbol;
static oop oop_p_eval_pair;
static oop oop_f_quote;
static oop oop_globals;

static int opt_v;

static oop traceStack = oop_nil;
static int traceDepth = 0;

static oop oop_apply(oop fun, oop args, oop env)
{
    oop result = oop_nil;

    GC_PROTECT(result);
    GC_PROTECT(args);

    for (;;) {
        if (opt_v > 1) {
            fprintf(stderr, "APPLY ");
            fdump(stderr, fun);
            fprintf(stderr, " TO ");
            fdump(stderr, args);
            fprintf(stderr, " IN ");
            fdumpln(stderr, env);
        }

        if (oop_Subr == getType(fun)) {
            imp_t function = getImp(fun);
            result = function(args, env);
            break;
        }

        // oop_Expr       -> oop_Subr(subr_apply_expr)
        // <selector> -> oop_Expr
        // <generic>  -> oop_Expr
        oop ap = oop_nil;

        switch (getType(fun)) {
        case oop_Expr:
            ap = oop_p_apply_expr;
            break;

        default:
            ap = arrayAt(cdr(oop_applicators), getType(fun));
            if (oop_nil == ap) goto error;
        }

        args = newPair(fun, args);
        fun  = ap;
    }

    GC_UNPROTECT(args);
    GC_UNPROTECT(result);

    return result;

 error:
    fprintf(stderr, "\nerror: cannot apply: ");
    fdumpln(stderr, fun);
    oop_fatal(0);
    return oop_nil;
}

static oop oop_expand(oop expr, oop env)
{
    oop list = expr;
    oop head = oop_nil;
    oop tail = oop_nil;

    GC_PROTECT(list);
    GC_PROTECT(head);
    GC_PROTECT(tail);

    for (;;) {
        if (opt_v > 1) {
            fprintf(stderr, "EXPAND ");
            fdumpln(stderr, list);
            fprintf(stderr, " IN ");
            fdumpln(stderr, env);
        }

        if (!is(oop_Pair, list)) goto done;

        // first expand the head of the list
        head = oop_expand(car(list), env);
        tail = cdr(list);

        // check if the head is a reference
        if (!is(oop_Symbol, head)) goto list_begin;

        if (oop_s_quote == head) goto list_done;

        oop reference = cdr(assq(head, env));

        // check if the reference is a form
        if (!is(oop_Form, reference)) goto list_begin;

        // get the form function
        oop func = getFunc(reference);

        // apply the form function to the rest of the list
        list = oop_apply(func, tail, env);
    }

 list_begin:
    // first expand all user defined forms
    tail = map(oop_expand, tail, env);

 list_done:
    list = newPair(head, tail);

 done:
    GC_UNPROTECT(tail);
    GC_UNPROTECT(head);
    GC_UNPROTECT(list);

    return list;
}

static oop oop_encode(oop expr, oop env)
{
    oop list = expr;
    oop head = oop_nil;
    oop tail = oop_nil;

    GC_PROTECT(list);
    GC_PROTECT(head);
    GC_PROTECT(tail);
    GC_PROTECT(env);
    GC_PROTECT(tmp);

    if (opt_v > 1) {
        fprintf(stderr, "ENCODE ");
        fdumpln(stderr, list);
        fprintf(stderr, " IN ");
        fdumpln(stderr, env);
    }

    if (!is(oop_Pair, list)) goto done;

    head = oop_encode(car(list), env);
    tail = cdr(list);

    if (is(oop_Symbol, head)) {
        oop val = cdr(assq(head, env));
        if (is(oop_Fixed, val)
            || is(oop_Subr, val))
            head = val;
    }

    if (oop_f_quote == head) goto list_done;

    /*
      this short cut will NOT work for
      (let ((let (lambda (let) let)))
          (let 1))

      because it will be encoded as
      (Fixed<let> ((Fixed<let> (Fixed<lambda> (Fixed<let>) let)))
          (Fixed<let> 1))

      instead of
      (Fixed<let> ((let (Fixed<lambda> (let) let)))
          (let 1))
     */

    /* one way to fixed this problem is
       to give a Fixed value two slots
       one for the encode (encode pair) phase and
       one for the apply  (apply pair) phase */

    tail = map(oop_encode, tail, env);

 list_done:
    list = newPair(head, tail);

 done:
    GC_UNPROTECT(tmp);
    GC_UNPROTECT(env);
    GC_UNPROTECT(tail);
    GC_UNPROTECT(head);
    GC_UNPROTECT(list);

    return list;
}

static oop oop_eval(oop obj, oop env)
{
    if (opt_v > 1) {
        fprintf(stderr, "EVAL ");
        fdumpln(stderr, obj);
        fprintf(stderr, " IN ");
        fdumpln(stderr, env);
    }

    arrayAtPut(traceStack, traceDepth++, obj);

    oop ev = oop_nil;

    switch (getType(obj)) {
    case oop_Symbol: // Symbol -> subr_eval_symbol
        ev = oop_p_eval_symbol;
        break;

    case oop_Pair: // Pair   -> subr_eval_pair
        ev =  oop_p_eval_pair;
        break;
    default:
        goto done;
    }

    oop args = oop_nil;

    GC_PROTECT(args);

    args = newPair(obj, oop_nil);
    obj  = oop_apply(ev, args, env);

    GC_UNPROTECT(args);

 done:
    --traceDepth;
    return obj;
}

// print error message followed be oop stacktrace
static void oop_fatal(char *reason, ...)
{
    if (reason) {
        va_list ap;
        va_start(ap, reason);
        fprintf(stderr, "\nerror: ");
        vfprintf(stderr, reason, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    }

    int i= traceDepth;
    while (i--) {
        printf("%3d: ", i);
        fdumpln(stdout, arrayAt(traceStack, i));
    }

    exit(1);
}

static void oop_replFile(FILE *stream)
{
    for (;;) {
        if (stream == stdin) {
            printf(".");
            fflush(stdout);
        }

        oop obj = oop_read(stream);

        if (obj == oop_eof) break;

        GC_PROTECT(obj);

        if (opt_v) {
            fdumpln(stdout, obj);
            fflush(stdout);
        }

        obj = oop_expand(obj, oop_globals);
        obj = oop_encode(obj, oop_globals);
        obj = oop_eval(obj, oop_globals);

        if (stream == stdin) {
            printf(" => ");
            fflush(stdout);
            fdumpln(stdout, obj);
            fflush(stdout);
        }

        GC_UNPROTECT(obj);

#if 0
        if (opt_v) {
            GC_gcollect();
            printf("%ld collections, %ld objects, %ld bytes, %4.1f%% fragmentation\n",
                   (long)GC_collections, (long)GC_count_objects(), (long)GC_count_bytes(),
                   GC_count_fragments() * 100.0);
        }
#endif
    }

    int c = getc(stream);

    if (EOF != c)
        oop_fatal("unexpected character 0x%02x '%c'\n", c, c);
}

static void oop_sigint(int signo)
{
    oop_fatal("\nInterrupt(%d)",signo);
}

static oop_setup() {
    signal(SIGINT, oop_sigint);
}

/*****************
 ** end of file **
 *****************/

