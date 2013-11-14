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

/**
 * Expand expression
 */
static unsigned expand_level = 0;
extern void expand(const Node expr, const Node env, Target result)
{
    GC_Begin(8);

    Node list; Node head; Node tail;

    GC_Protect(list);
    GC_Protect(head);
    GC_Protect(tail);

    list = expr;

    VM_ON_DEBUG(9, {
            fprintf(stderr, "expand(%u) <= ", expand_level++);
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
                fprintf(stderr, "expand(%u) ", expand_level);
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
            fprintf(stderr, "expand(%d) => ", --expand_level);
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    GC_End();
}

/**
 * Encode expanded expression
 *
 * translate symbol in head position to known
 * * primitive function
 *   then encode arguments
 * * composite function
 *   then call encoder with arguments
 */
static unsigned enode_level = 0;
extern void encode(const Node expr, const Node env, Target result)
{
    if (!isPair(expr)) {
        ASSIGN(result, expr);
        return;
    }

    GC_Begin(8);

    Node list; Node head; Node tail;

    GC_Protect(list);
    GC_Protect(head);
    GC_Protect(tail);

    list = expr;

    VM_ON_DEBUG(3, {
            fprintf(stderr,"encode(%u) <= ", enode_level++);
            prettyPrint(stderr, list);
            fprintf(stderr, "\n");
        });

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

        if (!isComposite(value)) goto list_begin;
    }

    Operator function = value.composite->encoder;

    if (!function) {
        fprintf(stderr, "no encode for composite: ");
        prettyPrint(stderr, head);
        fprintf(stderr, "\n");
        fatal(0);
    }

    function(tail, env, &(tail.pair));

    pair_Create(value, tail, result.pair);
    goto done;

 list_begin:
    list_Map(tail.pair, encode, env, &(tail.pair));
    pair_Create(head, tail, result.pair);

 done:
    VM_ON_DEBUG(3, {
            fprintf(stderr, "encode(%u) => ", --enode_level);
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    GC_End();
}

/**
 * Analysis encoded expression
 */
static unsigned analysis_level = 0;
extern void analysis(const Node expr, const Node env, Target result)
{
    ASSIGN(result, expr);
    return;

    if (!isPair(expr)) {
        ASSIGN(result, expr);
        return;
    }

    GC_Begin(8);

    Node list; Node head; Node tail;

    GC_Protect(list);
    GC_Protect(head);
    GC_Protect(tail);

    list = expr;

    VM_ON_DEBUG(4, {
            fprintf(stderr,"analysis(%u) <= ", analysis_level++);
            prettyPrint(stderr, list);
            fprintf(stderr, "\n");
        });

    head = list.pair->car;
    tail = list.pair->cdr;

    analysis(head, env, &head);

    Node     value = NIL;
    Analyser function = 0;

    if (!isSymbol(head)) {
        goto list_begin;
    } else {
        // check if the enviroment
        if (!alist_Get(env.pair, head.symbol, &value)) {
            alist_Get(enki_globals.pair, head.symbol, &value);
        }

        if (isPrimitive(value)) {
            head     = value;
            function = value.primitive->analyser;
        }

        if (isComposite(value)) {
            head     = value;
            function = value.composite->analyser;
        }
    }

    if (!function) {
        fprintf(stderr, "no analysis for: ");
        prettyPrint(stderr, head);
        fprintf(stderr, "\n");
        fatal(0);
    }

    function(tail, env, &(tail.pair));

    pair_Create(value, tail, result.pair);
    goto done;

 list_begin:
    list_Map(tail.pair, analysis, env, &(tail.pair));
    pair_Create(head, tail, result.pair);

 done:
    VM_ON_DEBUG(4, {
            fprintf(stderr, "analysis(%u) => ", --analysis_level);
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    GC_End();
}
/**
 * Evaluate analysised expression
 */
static void eval_symbol(const Symbol symbol, const Node env, Target result)
{
    GC_Begin(2);

    Node     tmp;
    Variable entry;

    GC_Protect(tmp);

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

static void eval_pair(const Pair args, const Node env, Target result)
{
    GC_Begin(5);

    Node obj, head, tail, tmp;

    GC_Protect(obj);
    GC_Protect(head);
    GC_Protect(tail);
    GC_Protect(tmp);

    pair_GetCar(args, &head);
    pair_GetCdr(args, &tail);

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

    if (isComposite(head)) {
        apply(head, tail, env, result);
        goto done;
    }

    // evaluate the arguments
    list_Map(tail.pair, eval, env, &tail.pair);

    // now apply the head to the evaluated arguments
    apply(head, tail, env, result);

 done:
    GC_End();
}

static void eval_tuple(const Tuple args, const Node env, Target result)
{
    tuple_Map(args, eval, env, result.tuple);
}

static unsigned eval_level = 0;
extern void eval(const Node expr, const Node env, Target result)
{
    if (isForced(expr)) {
        tuple_GetItem(expr.tuple, 0, result);
        return;
    }

    if (isSymbol(expr)) {
        eval_symbol(expr.symbol, env, result);
        return;
    }

    unsigned action = 0;

    if (isQuote(expr)) action = 1;
    if (isPair(expr))  action = 2;
    if (isTuple(expr)) action = 3;

    if (0 == action) {
        ASSIGN(result, expr);
        return;
    }

    pushTrace(expr);

    VM_ON_DEBUG(5, {
            fprintf(stderr, "eval(%u) <= ", eval_level++);
            prettyPrint(stderr, expr);
            fprintf(stderr, "\n");
        });

    switch (action) {
    case 1:
        eval_pair(expr.pair, env, result);
        break;
    case 2:
        eval_pair(expr.pair, env, result);
        break;
    case 3:
        eval_tuple(expr.tuple, env, result);
        break;
    default:
        fatal("coding error %u", action);
    }

    VM_ON_DEBUG(5, {
            fprintf(stderr, "eval(%u) => ", --eval_level);
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    popTrace();
}

static unsigned apply_level = 0;
extern void apply(Node fun, Node args, const Node env, Target result)
{
    GC_Begin(3);

    GC_Add(fun);
    GC_Add(args);

    VM_ON_DEBUG(9, {
            fprintf(stderr, "apply(%u) <= ", apply_level++);
            prettyPrint(stderr, fun);
            fprintf(stderr, " to: ");
            prettyPrint(stderr, args);
            fprintf(stderr,"\n");
        });

    // primitive -> Operator
    if (isPrimitive(fun)) {
      Operator function = fun.primitive->evaluator;
      function(args, env, result);
      goto done;
    }

    // composite -> Operator
    if (isComposite(fun)) {
      Operator function = fun.composite->evaluator;
      function(args, env, result);
      goto done;
    }

    // lambda -> p_apply_lambda
    if (isLambda(fun)) {
      if (!pair_Create(fun, args, &(args.pair))) goto error;
      Operator function = p_apply_lambda->evaluator;
      function(args, env, result);
      goto done;
    }

    // Form -> p_apply_form
    if (isForm(fun)) {
      if (!pair_Create(fun, args, &(args.pair))) goto error;
      Operator function = p_apply_form->evaluator;
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
            fprintf(stderr, "apply(%d) => ", --apply_level);
            prettyPrint(stderr, *result.reference);
            fprintf(stderr, "\n");
        });

    GC_End();

    return;
}

// infer type of expr (if possible)
extern bool analysis_infer(const Node expr, const Node env, Constant *type) {
    return false;
}

// check type of expr (if possible)
extern bool analysis_check(const Node expr, const Node env, const Constant type) {
    return false;
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

