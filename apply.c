/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#define debug_THIS
#include "apply.h"
#include "primitive.h"
#include "reader.h"
#include "dump.h"
#include "treadmill.h"
#include "pair.h"
#include "symbol.h"
#include "dump.h"
#include "debug.h"
#include "tuple.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static Pair traceStack = 0;

extern void dump_enki_stack() {
    if (!traceStack) return;

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
}

extern void pushTrace(const Node expr) {
    pair_Create(expr, traceStack, &traceStack);
}

extern void popTrace() {
    pair_GetCdr(traceStack, &traceStack);
}

// expand all forms
extern void expand(const Node expr, const Node env, Target result)
{
    Node list = expr;
    Node head = NIL;
    Node tail = NIL;

    darken_Node(expr);
    darken_Node(env);

    VM_ON_DEBUG(1, {
            fprintf(stderr, "expand: ");
            prettyPrint(stderr, list);
            fprintf(stderr, "\n");
        });

    for (;;) {
        if (!isType(list, s_pair)) {
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
        if (!isType(head, s_symbol)) goto list_begin;

        Node value = NIL;

        // check if the enviroment
        alist_Get(env.pair, head, &value);

        // check if the reference is a form
        if (!isType(value, s_form)) goto list_begin;

        // apply the form function to the rest of the list
        apply(value, tail, env, &list);
    }

 list_begin:
    // first expand all user defined forms
    list_Map(expand, tail.pair, env, &(tail.pair));

 list_done:
    pair_Create(head, tail, result.pair);

 done:
    VM_ON_DEBUG(1, {
            fprintf(stderr, "expand => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });
    return;
}

// translate symbol in head position to known primitive/fixed functions
extern void encode(const Node expr, const Node env, Target result)
{
    Node list = expr;
    Node head = NIL;
    Node tail = NIL;

    darken_Node(expr);
    darken_Node(env);

    VM_ON_DEBUG(1, {
            fprintf(stderr,"encode: ");
            prettyPrint(stderr, list);
            fprintf(stderr, "\n");
        });

    if (!isType(list, s_pair)) {
        ASSIGN(result, list);
        goto done;
    }

    head = list.pair->car;
    tail = list.pair->cdr;

    encode(head, env, &head);

    if (isType(head, s_symbol)) {
        Node value = NIL;
        // check if the enviroment
        alist_Get(env.pair, head, &value);

        if (isType(value, s_primitive)) {
            head = value;
        } else if (isType(value, s_fixed)) {
            head = value;
        }
    }

    if (!isType(head, s_fixed)) goto list_begin;

    Node action = NIL;

    tuple_GetItem(head.tuple, fxd_encode, &action);

    if (isNil(action)) goto list_begin;

    apply(action, tail, env, &(tail.pair));
    goto list_done;

 list_begin:
    list_Map(encode, tail.pair, env, &(tail.pair));

 list_done:
    pair_Create(head, tail, result.pair);

 done:
    VM_ON_DEBUG(1, {
            fprintf(stderr, "encode => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    return;
}

extern void eval(const Node expr, const Node env, Target result)
{
    pushTrace(expr);

    darken_Node(expr);
    darken_Node(env);

    VM_ON_DEBUG(1, {
            fprintf(stderr, "eval: ");
            prettyPrint(stderr, expr);
            fprintf(stderr, "\n");
        });

    Primitive evaluator = 0;

    if (isType(expr, s_pair)) {
        evaluator = p_eval_pair;
    }

    if (isType(expr, s_symbol)) {
        evaluator = p_eval_symbol;
    }

    if (!evaluator) {
        ASSIGN(result, expr);
        goto done;
    }

    Node args = NIL;

    pair_Create(expr, NIL, &(args.pair));

    apply(evaluator, args, env, result);

 done:

    VM_ON_DEBUG(1, {
            fprintf(stderr, "eval => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    popTrace();
}

extern void apply(Node fun, Node args, const Node env, Target result)
{
    darken_Node(fun);
    darken_Node(args);
    darken_Node(env);

    VM_ON_DEBUG(1, {
            fprintf(stderr, "apply: ");
            prettyPrint(stderr, fun);
            fprintf(stderr, " to: ");
            prettyPrint(stderr, args);
            fprintf(stderr,"\n");
        });

    // Primitive -> Operator
    if (isType(fun, s_primitive)) {
      Operator function = fun.primitive->function;
      function(args, env, result);
      goto done;
    }

    // Expression -> p_apply_expr
    if (isType(fun, s_expression)) {
      if (!pair_Create(fun, args, &(args.pair))) goto error;
      Operator function = p_apply_expr->function;
      function(args, env, result);
      goto done;
    }

    // Form -> p_apply_form
    if (isType(fun, s_form)) {
      if (!pair_Create(fun, args, &(args.pair))) goto error;
      Operator function = p_apply_form->function;
      function(args, env, result);
      goto done;
    }

 error:
    fprintf(stderr, "\nerror: cannot apply: ");
    dump(stderr, fun);
    fatal(0);

 done:

    VM_ON_DEBUG(1, {
            fprintf(stderr, "apply => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    return;
}

/*****************
 ** end of file **
 *****************/

