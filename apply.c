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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static Pair traceStack = 0;

static void environ_Lambda(Node symbol, Node env, Target result)
{
    pair_Create(symbol, NIL, result.pair);
}

static void environ_Let(Node local, Node env, Target result)
{
    Node symbol;
    pair_GetCar(local.pair, &symbol);
    pair_Create(symbol, NIL, result.pair);
}

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

    VM_ON_DEBUG(1, {
            fprintf(stderr, "expand: ");
            prettyPrint(stderr, list);
            fprintf(stderr, "\n");
        });

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

    VM_ON_DEBUG(1, {
            fprintf(stderr,"encode: ");
            prettyPrint(stderr, list);
            fprintf(stderr, "\n");
        });

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

    /* one way to fixed this problem is
       to give a Fixed value two slots
       one for the encode (encode pair) phase and
       one for the apply  (apply pair) phase */

    if (isIdentical(f_quote, head))  goto list_done;

    if (isIdentical(f_lambda, head)) {
        Node args; Node body; Node lenv;

        pair_GetCar(tail.pair, &args);
        pair_GetCdr(tail.pair, &body);

        VM_ON_DEBUG(1, {
                fprintf(stderr,"params: ");
                prettyPrint(stderr, args);
                fprintf(stderr, "\n");
            });

        list_Map(environ_Lambda, args.pair, env, &lenv);

        VM_ON_DEBUG(1, {
                fprintf(stderr,"environ: ");
                prettyPrint(stderr, lenv);
                fprintf(stderr, "\n");
            });

        list_SetEnd(lenv.pair, env);

        list_Map(encode, body.pair, lenv, &(body.pair));
        pair_Create(args, body, &tail.pair);
        goto list_done;
    }


    if (isIdentical(f_let, head)) {
        Node locals; Node lenv;

        pair_GetCar(tail.pair, &locals);

        VM_ON_DEBUG(1, {
                fprintf(stderr,"locals: ");
                prettyPrint(stderr, locals);
                fprintf(stderr, "\n");
            });

        list_Map(environ_Let, locals.pair, env, &lenv);

        VM_ON_DEBUG(1, {
                fprintf(stderr,"environ: ");
                prettyPrint(stderr, lenv);
                fprintf(stderr, "\n");
            });
        list_SetEnd(lenv.pair, env);

        list_Map(encode, tail.pair, lenv, &(tail.pair));
        goto list_done;
    }


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

    VM_ON_DEBUG(1, {
            fprintf(stderr, "eval: ");
            prettyPrint(stderr, expr);
            fprintf(stderr, "\n");
        });

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

    VM_ON_DEBUG(1, {
            fprintf(stderr, "eval => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    popTrace();
}

extern void apply(Node fun, Node args, const Node env, Target result)
{
    VM_ON_DEBUG(1, {
            fprintf(stderr, "apply: ");
            prettyPrint(stderr, fun);
            fprintf(stderr, " to: ");
            prettyPrint(stderr, args);
            fprintf(stderr,"\n");
        });

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

