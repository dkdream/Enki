/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#define _GNU_SOURCE
#define debug_THIS
#include "all_types.inc"
#include "treadmill.h"
#include "debug.h"
#include "text_buffer.h"
#include "id_set.h"

/* */
#include <error.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
/* */

#define SUBR(NAME) void opr_##NAME(Node args, Node env, Target result)
#define APPLY(NAME,ARGS,ENV,RESULT) opr_##NAME(ARGS,ENV,RESULT)

/* */
extern Node fixed_bind;
extern SUBR(encode_define);
/* */

static void variable_Fix(Node local, Node env, Target result)
{
    /*
    ** local = symbol
    **       | (symbol expr)
    */
    Node symbol;

    if (isSymbol(local)) {
        ASSIGN(result, local);
    } else if (isPair(local)) {
        list_GetItem(local.pair, 0, result);
    }
}

static void frame_Fix(Node local, Node env, Target result)
{
    /*
    ** local = symbol
    **       | (symbol expr)
    */
    Node symbol;

    if (isSymbol(local)) {
        symbol = local;
    } else if (isPair(local)) {
        Node expr = NIL;
        list_GetItem(local.pair, 0, &symbol);
    }

    variable_Create(symbol.symbol, NIL, 0, result.variable);
}

static void initialize_Fix(Node local, Node env, Target result)
{
    /*
    ** local = symbol
    **       | (symbol expr)
    */
    GC_Begin(2);

    Pair temp;

    GC_Protect(temp);

    if (isSymbol(local)) {
        pair_Create(local, NIL, &temp);
    } else if (isPair(local)) {
        APPLY(encode_define, local, env, &temp);
    }

    pair_Create(fixed_bind, temp, result.pair);

    GC_End();
}

extern SUBR(encode_fix)
{
    /*
    ** given args    = ((<binding>...) . body)
    **       binding = name
    **               | (name expr)
    ** to-do
    **       args    = (name (<binding>...) . body)
    **               | ([name.0 expr.0 ... name.n expr.n] [as name expr.r] initialize . body)
    **               | ([name.0 expr.0 ... name.n expr.n] initialize . body)
    **    initialize = bound...
    **         bound = [bind name expr.b]
    **         bound = [set  name expr.b]
    **
    */

    /* notes:
    **   when using '(' ')'
    **   - the expr's need to be evaluated in the inner context
    **   when using '[' ']'
    **   - the expr.i, expr.r, expr.b's  needs to be encoded in the current context
    */

    GC_Begin(6);

    Node bindings;
    Node variables;
    Node initializers;
    Node body;
    Node lenv;

    GC_Protect(bindings);
    GC_Protect(variables);
    GC_Protect(initializers);
    GC_Protect(body);
    GC_Protect(lenv);

    pair_GetCar(args.pair, &bindings);

    if (isPair(bindings)) {
        pair_GetCdr(args.pair, &body);
    } else {
        encode(args, env, result);
        GC_End();
        return;
    }

    list_Map(bindings.pair, variable_Fix, env, &(variables.pair));

    list_Map(bindings.pair, frame_Fix, env, &(lenv.pair));
    list_SetEnd(lenv.pair, env);

    list_Map(bindings.pair, initialize_Fix, lenv, &(initializers.pair));

    encode(body, lenv, &body);

    list_SetEnd(initializers.pair, body);
    pair_Create(variables, initializers, result.pair);

    GC_End();
}

static void binding_Fix(Node local, Node env, Target result)
{
    /*
    ** local = name
    **       | (name expr)
    */
    GC_Begin(4);

    Node symbol;
    Node expr;
    Node value;

    GC_Protect(expr);
    GC_Protect(value);

    if (isSymbol(local)) { // name
        symbol = local;
        value  = void_v;
        goto done;
    }

    if (isPair(local)) { // name expr
        list_GetItem(local.pair, 0, &symbol);
        list_GetItem(local.pair, 1, &expr);
        goto do_eval;
    }

  do_eval:
    if (!isNil(expr)) {
        eval(expr, env, &value);
    } else {
        value = NIL;
    }

  done:
    pair_Create(symbol, value, result.pair);

    GC_End();
}

extern SUBR(fix)
{
    /*
    ** given args    = ((binding...) . body)
    **       binding = name
    **               | (name expr) -- removed by encode_fix
    ** to-do
    **       args    = (name (<binding>...) . body)
    **               | ([name.0 expr.0 ... name.n expr.n] [as name expr.r] initialize . body)
    **               | ([name.0 expr.0 ... name.n expr.n] initialize . body)
    **    initialize = bound...
    **         bound = [bind name expr.b]
    **         bound = [set  name expr.b]
    */

    /* notes:
    **   when using '(' ')'
    **   - the expr's need to be evaluated in the inner context
    **   when using '[' ']'
    **   - the expr.i and expr.r needs to be evaluated in the current context
    **   - the expr.b's needs to be evaluated in the inner context
    */

    GC_Begin(4);

    Node env2;
    Node bindings;
    Node body;

    GC_Protect(env2);
    GC_Protect(bindings);
    GC_Protect(body);

    pair_GetCar(args.pair, &bindings);
    pair_GetCdr(args.pair, &body);

    if (!isPair(bindings)) {
        eval_begin(body, env, result);
    } else {
        list_Map(bindings.pair, binding_Fix, env, &(env2.pair));

        list_SetEnd(env2.pair, env);

        eval_begin(body, env2, result);
    }

    GC_End();
}
