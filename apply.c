/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "apply.h"
#include "primitive.h"
#include "reader.h"
#include "dump.h"
#include "treadmill.h"
#include "pair.h"
#include "symbol.h"
#include "dump.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <execinfo.h>

static Pair traceStack = 0;

static void dump_c_stack()
{
    void *array[100];
    size_t size;
    char **strings;
    size_t i;

    size    = backtrace (array, 100);
    strings = backtrace_symbols (array, size);

    fprintf(stderr, "Obtained %zd stack frames.\n", size);

    for (i = 0; i < size; i++)
        fprintf(stderr, "%s\n", strings[i]);

    free (strings);
}

// print error message followed be oop stacktrace
extern void fatal(const char *reason, ...)
{
    fflush(stdout);

    if (reason) {
        va_list ap;
        va_start(ap, reason);
        fprintf(stderr, "\nerror: ");
        vfprintf(stderr, reason, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    }

    if (!traceStack) {
        dump_c_stack();
        exit(1);
    }

    int inx = 0;
    for(; traceStack ; ++inx) {
        Node value;
        if (pair_GetCar(traceStack, &value)) {
            fprintf(stderr, "%3d: ", inx);
            prettyPrint(stderr, value);
            fprintf(stderr, "\n");
        }
        if (!pair_GetCdr(traceStack, &traceStack)) break;
    }
    fprintf(stderr, "\n");

    dump_c_stack();
    exit(1);
}

extern void pushTrace(const Node expr) {
    pair_Create(expr, traceStack, &traceStack);
}

extern void popTrace() {
    pair_GetCdr(traceStack, &traceStack);
}

extern void apply(Node fun, Node args, const Node env, Target result)
{
#if 0
        printf("apply: ");
        prettyPrint(stdout, fun);
        printf(" to: ");
        prettyPrint(stdout, args);
        printf("\n");
#endif

    for (;;) {
        // Primitive -> Operator
        if (isKind(fun, nt_primitive)) {
            Operator function = fun.primitive->function;
            function(args, env, result);
            goto done;
        }

        // Expression -> p_apply_expr
        if (isKind(fun, nt_expression)) {
            if (!pair_Create(fun, args, &(args.pair))) goto error;
            fun.primitive = p_apply_expr;
            continue;
        }

        // Form -> p_apply_form
        if (isKind(fun, nt_form)) {
            if (!pair_Create(fun, args, &(args.pair))) goto error;
            fun.primitive = p_apply_form;
            continue;
        }

        goto error;
    }

 error:
    fprintf(stderr, "\nerror: cannot apply: ");
    dump(stderr, fun);
    fatal(0);

 done:

#if 0
    printf("apply => ");
    prettyPrint(stdout, *result.reference);
    printf("\n");
#endif
    return;
}

// expand all forms
extern void expand(const Node expr, const Node env, Target result)
{
    Node list = expr;
    Node head = NIL;
    Node tail = NIL;

#if 0
    printf("expand: ");
    prettyPrint(stdout, list);
    printf("\n");
#endif

    for (;;) {
        if (!isKind(list, nt_pair)) {
            ASSIGN(result, list);
            goto done;
        }

        head = list.pair->car;
        tail = list.pair->cdr;

        // first expand the head of the list
        expand(head, env, &head);

        // deal with quoted values
        if (isIdentical(s_quote, head)) goto list_done;

        // check if the head is a reference
        if (!isKind(head, nt_symbol)) goto list_begin;

        Node value = NIL;

        // check if the enviroment
        alist_Get(env.pair, head, &value);

        // check if the reference is a form
        if (!isKind(value, nt_form)) goto list_begin;

        // apply the form function to the rest of the list
        apply(value, tail, env, &list);
    }

 list_begin:
    // first expand all user defined forms
    list_Map(expand, tail.pair, env, &(tail.pair));

 list_done:
    pair_Create(head, tail, result.pair);

 done:

#if 0
    printf("expand => ");
    prettyPrint(stdout, *result.reference);
    printf("\n");
#endif
    return;
}

// translate know symbols to known values
extern void encode(const Node expr, const Node env, Target result)
{
    Node list = expr;
    Node head = NIL;
    Node tail = NIL;

#if 0
    printf("encode: ");
    prettyPrint(stdout, list);
    printf("\n");
#endif

    if (!isKind(list, nt_pair)) {
        ASSIGN(result, list);
        goto done;
    }

    head = list.pair->car;
    tail = list.pair->cdr;

    encode(head, env, &head);

    if (isKind(head, nt_symbol)) {
        Node value = NIL;
        // check if the enviroment
        alist_Get(env.pair, head, &value);
        switch (getKind(value)) {
        case nt_fixed:
        case nt_primitive:
            head = value;
        default:
            break;
        }
    }

    if (isIdentical(f_quote, head)) goto list_done;

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

    list_Map(encode, tail.pair, env, &(tail.pair));

 list_done:
    pair_Create(head, tail, result.pair);

 done:

#if 0
    printf("encode => ");
    prettyPrint(stdout, *result.reference);
    printf("\n");
#endif

    return;
}

extern void eval(const Node expr, const Node env, Target result)
{
    pushTrace(expr);

#if 0
    printf("eval: ");
    prettyPrint(stdout, expr);
    printf("\n");
#endif

    Primitive evaluator = 0;

    switch (getKind(expr)) {
    case nt_symbol:
        evaluator = p_eval_symbol;
        break;

    case nt_pair:
        evaluator = p_eval_pair;
        break;

    default:
        ASSIGN(result, expr);
        goto done;
    }

    Node args = NIL;

    pair_Create(expr, NIL, &(args.pair));

    apply(evaluator, args, env, result);

 done:

#if 0
    printf("eval => ");
    prettyPrint(stdout, *result.reference);
    printf("\n");
#endif

    popTrace();
}

/*****************
 ** end of file **
 *****************/

