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

extern bool opaque_Create(Node type, const Symbol ctor, long size, Reference* target);

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
        if (!isPair(list) && !isQuote(list)) {
            ASSIGN(result, list);
            goto done;
        }

        head = list.pair->car;
        tail = list.pair->cdr;

        // first expand the head of the list
        expand(head, env, &head);

        // deal with quoted values
        if (isIdentical(s_quote, head)) goto list_done;
        if (isIdentical(s_type, head))  goto list_begin;

        // check if the head is a reference
        if (!isSymbol(head)) goto list_begin;

        Node value = NIL;

        // check if the enviroment
        if (!alist_Get(env.pair, head.symbol, &value)) {
            alist_Get(enki_globals.pair, head.symbol, &value);
        }

        // check if the reference is a form
        if (!isForm(value)) goto list_begin;

        VM_ON_DEBUG(9, {
                fprintf(stderr, "expand ");
                prettyPrint(stderr, head);
                fprintf(stderr, " as ");
                prettyPrint(stderr, value);
                fprintf(stderr, "\n");
            });

        // apply the form function to the rest of the list
        apply(value, tail, env, &list);
    }

 list_begin:
    // first expand all user defined forms
    list_Map(tail.pair, expand, env, &(tail.pair));

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

    if (!isPair(list) && !isQuote(list)) {
        ASSIGN(result, list);
        goto done;
    }

    head = list.pair->car;
    tail = list.pair->cdr;

    encode(head, env, &head);

    Node value = NIL;

    if (!isSymbol(head)) {
        goto list_begin;
    } else {
        // check if the enviroment
        if (!alist_Get(env.pair, head.symbol, &value)) {
            alist_Get(enki_globals.pair, head.symbol, &value);
        }

        if (isPrimitive(value)) {
            head = value;
            goto list_begin;
        }

        if (!isFixed(value)) goto list_begin;
    }

    Node encode_action = NIL;

    tuple_GetItem(value.tuple, fxd_encode, &encode_action);

    if (isNil(encode_action)) {
        fprintf(stderr, "no encode for fixed: ");
        prettyPrint(stderr, head);
        fprintf(stderr, "\n");
        fatal(0);
    }

    apply(encode_action, tail, env, &(tail.pair));

    pair_Create(value, tail, result.pair);
    goto done;

 list_begin:
    list_Map(tail.pair, encode, env, &(tail.pair));
    pair_Create(head, tail, result.pair);

 done:
    VM_ON_DEBUG(9, {
            fprintf(stderr, "encode => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    GC_End();
}

static void eval_pair(const Node args, const Node env, Target result)
{
    GC_Begin(5);
    Node obj, head, tail, tmp;

    GC_Protect(obj);
    GC_Protect(head);
    GC_Protect(tail);
    GC_Protect(tmp);

    pair_GetCar(args.pair, &head);
    pair_GetCdr(args.pair, &tail);

    pushTrace(args);

    // first eval the head
    eval(head, env, &head);

    if (isBoxed(head)) {
        pair_GetCar(head.pair, &head);
    }

    if (isDelayed(head)) {
        Node dexpr, denv;
        tuple_GetItem(head.tuple, 1, &dexpr);
        tuple_GetItem(head.tuple, 2, &denv);
        eval(dexpr, denv, &tmp);
        tuple_SetItem(head.tuple, 0, tmp);
        setConstructor(head.tuple, s_forced);
        head = tmp;
    }

    if (isFixed(head)) {
        // apply Fixed to un-evaluated arguments
        Node func = NIL;
        tuple_GetItem(head.tuple, fxd_eval, &func);
        apply(func, tail, env, result);
        goto done;
    }

    // evaluate the arguments
    list_Map(tail.pair, eval, env, &tail.pair);

    // now apply the head to the evaluated arguments
    apply(head, tail, env, result);

 done:
    popTrace();

    GC_End();
}

static void eval_symbol(const Node args, const Node env, Target result)
{
    GC_Begin(2);

    Symbol   symbol;
    Node     tmp;
    Variable entry;

    GC_Protect(tmp);

    symbol = args.symbol;

    // lookup symbol in the current enviroment
    if (!alist_Entry(env.pair, symbol, &entry)) {
        if (!alist_Entry(enki_globals.pair, symbol, &entry)) goto error;
    }

    tmp = entry->value;

    if (isForced(tmp)) {
        tuple_GetItem(tmp.tuple, 0, &(entry->value));
        tmp = entry->value;
    }

    ASSIGN(result, tmp);

    GC_End();
    return;

 error:
    GC_End();
    if (!isSymbol(symbol)) {
        fatal("undefined variable: <non-symbol>");
    } else {
        fatal("undefined variable: %s", symbol_Text(symbol));
    }
}

static void eval_tuple(const Node args, const Node env, Target result)
{
    GC_Begin(5);

    Node head;
    Node func;
    Variable entry;

    GC_Protect(head);
    GC_Protect(func);
    GC_Protect(entry);

    tuple_GetItem(args.tuple, 0, &head);

    if (!isSymbol(head)) goto no_op;

    if (!alist_Entry(env.pair, head.symbol, &entry)) {
        if (!alist_Entry(enki_globals.pair,  head.symbol, &entry)) goto no_op;
    }

    head = entry->value;

    if (isBoxed(head)) {
        pair_GetCar(head.pair, &head);
    }

    if (!isFixed(head)) goto no_op;

    // apply Fixed to un-evaluated arguments

    tuple_GetItem(head.tuple, fxd_eval, &func);

    apply(func, args, env, result);
    goto done;

  no_op:
    tuple_Map(args.tuple, eval, env, result.tuple);

  done:
    popTrace();

    GC_End();
}


extern void eval(const Node expr, const Node env, Target result)
{
    pushTrace(expr);

    VM_ON_DEBUG(2, {
            fprintf(stderr, "eval: ");
            prettyPrint(stderr, expr);
            fprintf(stderr, "\n");
        });

    if (isForced(expr)) {
        tuple_GetItem(expr.tuple, 0, result);
        goto done;
    }

    if (isSymbol(expr)) {
        eval_symbol(expr, env, result);
        goto done;
    }

    if (isQuote(expr)) {
        eval_pair(expr, env, result);
        goto done;
    }

    if (isPair(expr)) {
        eval_pair(expr, env, result);
        goto done;
    }

    if (isTuple(expr)) {
        eval_tuple(expr, env, result);
        goto done;
    }

    ASSIGN(result, expr);

 done:
    VM_ON_DEBUG(9, {
            fprintf(stderr, "eval => ");
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    popTrace();
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
    if (isPrimitive(fun)) {
      Operator function = fun.primitive->function;
      function(args, env, result);
      goto done;
    }

    // lambda -> p_apply_lambda
    if (isLambda(fun)) {
      if (!pair_Create(fun, args, &(args.pair))) goto error;
      Operator function = p_apply_lambda->function;
      function(args, env, result);
      goto done;
    }

    // Form -> p_apply_form
    if (isForm(fun)) {
      if (!pair_Create(fun, args, &(args.pair))) goto error;
      Operator function = p_apply_form->function;
      function(args, env, result);
      goto done;
    }

 error:
    fprintf(stderr, "\nerror: cannot apply: ");
    dump(stderr, fun);
    fprintf(stderr,"\n  to: ");
    dump(stderr, args);
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

extern void eval_begin(Node body, Node env, Target last)
{
    GC_Begin(2);

    Node expr, value;

    GC_Protect(expr);
    GC_Protect(value);

    while (isPair(body)) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &value);
    }

    ASSIGN(last, value);
    GC_End();
}

struct raw_frame {
    Symbol escape;
    Node body;
    Pair env;
};

struct raw_label {
    void *label;
};

static void eval_closure(void *label, struct raw_frame *context, Target last)
{
    GC_Begin(3);

    Reference escape;
    Pair entry;
    Pair env2;

    GC_Protect(escape);
    GC_Protect(entry);
    GC_Protect(env2);

    if (!opaque_Create(t_continuation, s_escape, sizeof(struct raw_label), &escape)) {
        fatal("failed to allocate opaque object");
    }

    ((struct raw_label*)(escape))->label = label;

    Symbol symbol = context->escape;

    if (!alist_Add(context->env, symbol, escape, getType(escape).constant, &env2)) {
        fatal("ASSERT unable to add variable: %s", symbol_Text(symbol));
    }

    eval_begin(context->body, env2, last);

    setType(escape, t_closed);

    GC_End();
}

extern void eval_escape(Node node, Node result)
{
    Reference escape = node.reference;

    if (!inType(node, t_continuation)) {
        fatal("eval_escape: not a continuation");
    }

    if (!isEscape(node)) {
        fatal("eval_escape: not a escape");
    }

    setType(escape, t_closed);

    void *label = ((struct raw_label*)(escape))->label;

    clink_Goto(label, result);
}

extern void eval_block(const Symbol escape, const Constant type, Node body, Node env, Target last)
{
    GC_Begin(2);

    struct raw_frame frame;

    frame.escape = escape;
    frame.body   = body;
    frame.env    = env.pair;

    clink_Label((Operator)eval_closure, &frame, last);

    GC_End();
}

/*****************
 ** end of file **
 *****************/

