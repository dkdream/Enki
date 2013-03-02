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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>


// add a stack slot to the root set (with markings __FILE__, __LINE__)
#define GC_PROTECT(V)
// remove a stack slot from the root set
#define GC_UNPROTECT(V)

static Pair traceStack = 0;

// print error message followed be oop stacktrace
extern void fatal(const char *reason, ...)
{
    if (reason) {
        va_list ap;
        va_start(ap, reason);
        fprintf(stderr, "\nerror: ");
        vfprintf(stderr, reason, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    }

    if (!traceStack) exit(1);

    for(; traceStack ;) {
        Node value;
        if (pair_GetCar(traceStack, &value)) {
            //printf("%3d: ", inx);
            dump(stdout, value);
            printf("\n");
        }
        if (!pair_GetCdr(traceStack, &traceStack)) break;
    }

    exit(1);
}

extern void pushTrace(const Node expr) {
    pair_Create(expr, traceStack, &traceStack);
}

extern void popTrace() {
    pair_GetCdr(traceStack, &traceStack);
}

extern bool apply(Node fun, Node args, const Node env, Target result)
{
    GC_PROTECT(args);

    for (;;) {
        // Primitive -> Operator
        if (isKind(fun, nt_primitive)) {
            Operator function = fun.primitive->function;
            if (!function(args, env, result)) goto error;
            GC_UNPROTECT(args);
            return true;
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
    //    fatal(0);

    GC_UNPROTECT(args);
    return false;
}

// expand all forms
extern bool expand(const Node expr, const Node env, Target result)
{
    Node list = expr;
    Node head = NIL;
    Node tail = NIL;

    GC_PROTECT(list);
    GC_PROTECT(head);
    GC_PROTECT(tail);

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
        if (!apply(value, tail, env, &list)) goto error;
    }

 list_begin:
    // first expand all user defined forms
    list_Map(expand, tail.pair, env, &(tail.pair));

 list_done:
    pair_Create(head, tail, result.pair);

 done:
    GC_UNPROTECT(tail);
    GC_UNPROTECT(head);
    GC_UNPROTECT(list);

    return true;

 error:
    fprintf(stderr, "\nexpand error");
    //   fatal(0)

    GC_UNPROTECT(tail);
    GC_UNPROTECT(head);
    GC_UNPROTECT(list);

    return false;
}

// translate know symbols to known values
extern bool encode(const Node expr, const Node env, Target result)
{
    Node list = expr;
    Node head = NIL;
    Node tail = NIL;

    GC_PROTECT(list);
    GC_PROTECT(head);
    GC_PROTECT(tail);

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
    GC_UNPROTECT(tail);
    GC_UNPROTECT(head);
    GC_UNPROTECT(list);

    return true;
}

extern bool eval(const Node expr, const Node env, Target result)
{
    pushTrace(expr);

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

    GC_PROTECT(args);

    pair_Create(expr, NIL, &(args.pair));

    apply(evaluator, args, env, result);

    GC_UNPROTECT(args);

 done:
    popTrace();
    return true;
}

/*****************
 ** end of file **
 *****************/

