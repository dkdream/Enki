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
/* */

static void environ_Let(Node local, Node env, Target result)
{
    /*
    ** local = symbol
    **       | (symbol expr)
    */
    Node symbol = NIL;

    if (isSymbol(local)) {
        symbol = local;
    } else if (isPair(local)) {
        Node expr = NIL;
        Node nexpr = NIL;

        pair_GetCar(local.pair, &symbol);
        pair_GetCdr(local.pair, &expr);

        encode(expr, env, &nexpr);

        pair_SetCdr(local.pair, nexpr);
    } else if (isTuple(local)) {
        Node expr = NIL;
        Node nexpr = NIL;

        tuple_GetItem(local.tuple, 0, &symbol);

        tuple_GetItem(local.tuple, 1, &expr);

        encode(expr, env, &nexpr);

        tuple_SetItem(local.tuple, 1, nexpr);
    }


    variable_Create(symbol.symbol, NIL, 0, result.variable);
}

extern SUBR(encode_let)
{
    /*
    ** given args    = (name (<binding>...) . body)
    **               | ((<binding>...) . body)
    **               | (name _ . body)
    **               | (_ . body)
    **       binding = name
    **               | (name expr)
    **               | [name type expr]
    **
    */

    /* notes:
    **   when using '(' ')'
    **   - the expr's need to be evaluated in the current context
    **   when using '[' ']'
    **   - the expr.i, expr.r, expr.b's  needs to be encoded in the current context
    */

    GC_Begin(7);

    Tuple  frame;
    Node   bindings;
    Symbol marker;
    Node   body;
    Node   lenv;
    Node   hold;

    GC_Protect(frame);
    GC_Protect(bindings);
    GC_Protect(marker);
    GC_Protect(body);
    GC_Protect(lenv);
    GC_Protect(hold);

    pair_GetCar(args.pair, &bindings);
    pair_GetCdr(args.pair, &hold);

    if (!isSymbol(bindings)) { // (let (binding...) ...)
        body = hold;
    } else if (isIdentical(bindings, s_uscore)) { // (let _ ...)
        bindings = NIL;
        body     = hold;
    } else { // (let name (binding...) ...)
        marker = bindings.symbol;
        pair_GetCar(hold.pair, &bindings);
        pair_GetCdr(hold.pair, &body);
    }

    if (!isPair(bindings)) {
        lenv = env;
    } else { // (binding...)
        list_Map(bindings.pair, environ_Let, env, &(lenv.pair));
        list_SetEnd(lenv.pair, env);
    }

    if (isSymbol(marker)) {
        alist_Add(lenv.pair, marker, NIL, opaque_s, &(lenv.pair));
    }

body:
    encode(body, lenv, &body);

    tuple_Create(3, &frame);
    tuple_SetItem(frame, 0, bindings);
    tuple_SetItem(frame, 1, marker);
    tuple_SetItem(frame, 2, body);

    ASSIGN(result, frame);

    GC_End();
}

static void binding_Let(Node local, Node env, Target result)
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
    variable_Create(symbol.symbol, value, getType(value).constant,  result.variable);

    GC_End();
}

extern SUBR(let)
{
    GC_Begin(7);

    Tuple frame;
    Node  env2;
    Node  bindings;
    Node  marker;
    Node  body;

    GC_Protect(frame);
    GC_Protect(env2);
    GC_Protect(bindings);
    GC_Protect(marker);
    GC_Protect(body);

    frame = args.tuple;

    tuple_GetItem(frame, 0, &bindings);
    tuple_GetItem(frame, 1, &marker);
    tuple_GetItem(frame, 2, &body);

    if (!isPair(bindings)) {
        env2 = env;
    } else {
        list_Map(bindings.pair, binding_Let, env, &(env2.pair));
        list_SetEnd(env2.pair, env);
    }

    if (isSymbol(marker)) {
        eval_block(marker.symbol, (Constant)0, body, env2, result);
    } else {
        eval_begin(body, env2, result);
    }

 exit:
    GC_End();
}
