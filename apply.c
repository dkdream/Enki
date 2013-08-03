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
#include "type.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static Pair traceStack = 0;

extern void dump_enki_stack() {
    if (!traceStack) return;

    int inx = 0;

    for(; traceStack ; ++inx) {
        if (8 < inx) {
            fprintf(stderr, "%3d:", inx);
            fprintf(stderr, " ... ...\n");
            break;
        }
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
    GC_Begin(8);

    Node list; Node head; Node tail;

    GC_Protect(list);
    GC_Protect(head);
    GC_Protect(tail);

    list = expr;

    VM_ON_DEBUG(9, {
            fprintf(stderr, "expand: ");
            prettyPrint(stderr, list);
            fprintf(stderr, "\n");
        });

    for (;;) {
        if (!isPair(list)) {
            ASSIGN(result, list);
            goto done;
        }

        head = list.pair->car;
        tail = list.pair->cdr;

        // first expand the head of the list
        expand(head, env, &head);

        // deal with quoted values
        if (isIdentical(s_quote, head)) goto list_done;
        if (isIdentical(s_type, head))  goto list_done;

        // check if the head is a reference
        if (!isType(head, t_symbol)) goto list_begin;

        Node value = NIL;

        // check if the enviroment
        if (!alist_Get(env.pair, head, &value)) {
            alist_Get(enki_globals.pair, head, &value);
        }

        VM_ON_DEBUG(9, {
                fprintf(stderr, "expand ");
                prettyPrint(stderr, head);
                fprintf(stderr, " as ");
                prettyPrint(stderr, value);
                fprintf(stderr, "\n");
            });

        // check if the reference is a form
        if (!fromCtor(value, s_form)) goto list_begin;

        // apply the form function to the rest of the list
        apply(value, tail, env, &list);
    }

 list_begin:
    // first expand all user defined forms
    list_Map(expand, tail.pair, env, &(tail.pair));

 list_done:
    pair_Create(head, tail, result.pair);

 done:
    VM_ON_DEBUG(9, {
            fprintf(stderr, "expand => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    GC_End();
}

// translate symbol in head position to known primitive/fixed functions
extern void encode(const Node expr, const Node env, Target result)
{
    GC_Begin(8);

    Node list; Node head; Node tail;

    GC_Protect(list);
    GC_Protect(head);
    GC_Protect(tail);

    list = expr;

    VM_ON_DEBUG(9, {
            fprintf(stderr,"encode: ");
            prettyPrint(stderr, list);
            fprintf(stderr, "\n");
        });

    if (!isPair(list)) {
        ASSIGN(result, list);
        goto done;
    }

    head = list.pair->car;
    tail = list.pair->cdr;

    encode(head, env, &head);

    if (isType(head, t_symbol)) {
        Node value = NIL;
        // check if the enviroment
        if (!alist_Get(env.pair, head, &value)) {
            alist_Get(enki_globals.pair, head, &value);
        }

        if (fromCtor(value, s_primitive)) {
            head = value;
        } else if (fromCtor(value, s_fixed)) {
            head = value;
        }
    }

    if (!fromCtor(head, s_fixed)) goto list_begin;

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
    VM_ON_DEBUG(9, {
            fprintf(stderr, "encode => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    GC_End();
}

extern void eval(const Node expr, const Node env, Target result)
{
    GC_Begin(3);

    Primitive evaluator; Node args;

    GC_Protect(evaluator);
    GC_Protect(args);

    pushTrace(expr);

    VM_ON_DEBUG(2, {
            fprintf(stderr, "eval: ");
            prettyPrint(stderr, expr);
            fprintf(stderr, "\n");
        });

    if (isPair(expr)) {
        evaluator = p_eval_pair;
    }

    if (isSymbol(expr)) {
        evaluator = p_eval_symbol;
    }

    if (fromCtor(expr, s_forced)) {
        tuple_GetItem(expr.tuple, 0, result);
        goto done;
    }

    if (!evaluator) {
        ASSIGN(result, expr);
        goto done;
    }

    pair_Create(expr, NIL, &(args.pair));

    ASSIGN(result, NIL);
    apply(evaluator, args, env, result);

 done:

    VM_ON_DEBUG(9, {
            fprintf(stderr, "eval => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    popTrace();

    GC_End();
}

extern void apply(Node fun, Node args, const Node env, Target result)
{
    GC_Begin(3);

    GC_Add(fun);
    GC_Add(args);

    VM_ON_DEBUG(9, {
            fprintf(stderr, "apply: ");
            prettyPrint(stderr, fun);
            fprintf(stderr, " to: ");
            prettyPrint(stderr, args);
            fprintf(stderr,"\n");
        });

    // primitive -> Operator
    if (fromCtor(fun, s_primitive)) {
      Operator function = fun.primitive->function;
      function(args, env, result);
      goto done;
    }

    // lambda -> p_apply_lambda
    if (fromCtor(fun, s_lambda)) {
      if (!pair_Create(fun, args, &(args.pair))) goto error;
      Operator function = p_apply_lambda->function;
      function(args, env, result);
      goto done;
    }

    // Form -> p_apply_form
    if (fromCtor(fun, s_form)) {
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

    VM_ON_DEBUG(9, {
            fprintf(stderr, "apply => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    GC_End();

    return;
}

/*****************
 ** end of file **
 *****************/

