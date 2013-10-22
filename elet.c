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

#define SUBR(NAME) void opr_##NAME(Node args, Node env, Target result)
#define APPLY(NAME,ARGS,ENV,RESULT) opr_##NAME(ARGS,ENV,RESULT)


static bool with_types(unsigned index, Node value, Node env) {
    if (0 == index) return false;

    unsigned test = (index % 2);

    if (1 == test) return false;
    return true;
}
static bool with_names(unsigned index, Node value, Node env) {
    if (0 == index) return false;

    unsigned test = (index % 2);

    if (1 == test) return false;
    return true;
}

static bool as_types(unsigned index, Node value, Node env) {
    return (2 < index);
}
static bool as_name(unsigned index, Node value, Node env) {
    return (1 == index);
}

Tuple find_with(Tuple frame, const Node env) {
    if (!frame) return (Tuple) 0;

    Kind    kind = asKind(frame);
    unsigned max = kind->count;
    unsigned inx = 0;

    Node symbol;

    for (; inx < max ;++inx) {
        Node value = frame->item[inx];
        if (!isTuple(value)) return (Tuple) 0;
        tuple_GetItem(value.tuple, 0, &symbol);
        if (isIdentical(s_with, symbol)) {
            return value.tuple;
        }
    }

    return (Tuple) 0;
}


Tuple find_as(Tuple frame, const Node env) {
    if (!frame) return (Tuple) 0;

    Kind    kind = asKind(frame);
    unsigned max = kind->count;
    unsigned inx = 0;

    BitArray array = BITS_INITIALISER;
    Node     symbol;

    for (; inx < max ;++inx) {
        Node value = frame->item[inx];
        if (!isTuple(value)) return (Tuple) 0;
        tuple_GetItem(value.tuple, 0, &symbol);
        if (isIdentical(s_as, symbol)) {
            bits_reset(&array);
            tuple_Find(value.tuple, as_types, 0, &array);
            tuple_Update(value.tuple, encode, env, &array);
            bits_free(&array);
            return value.tuple;
        }
    }

    return (Tuple) 0;
}

Tuple find_initialize(Tuple frame, const Node env) {

    if (!frame) return (Tuple) 0;

    Kind      kind = asKind(frame);
    unsigned   max = kind->count;
    unsigned   inx = 0;
    unsigned count = 0;

    BitArray array;

    bits_init(&array);

    Node symbol;

    for (; inx < max ;++inx) {
        Node value = frame->item[inx];
        if (!isTuple(value)) break;
        tuple_GetItem(value.tuple, 0, &symbol);
        if (isIdentical(s_bind, symbol)) {
            ++count;
            bits_set(&array, inx, true);
            continue;
        }
        if (isIdentical(s_set, symbol)) {
            ++count;
            bits_set(&array, inx, true);
            continue;
        }
    }

    if (0 == count) return (Tuple) 0;

    Tuple result;

    tuple_Select(frame, count, &array, &result);

    bits_free(&array);
    return result;
}

Tuple find_body(Tuple frame) {

    if (!frame) return (Tuple) 0;

    Kind      kind = asKind(frame);
    unsigned   max = kind->count;
    unsigned   inx = 0;

    Node symbol;

    for (; inx < max ;++inx) {
        Node value = frame->item[inx];

        if (!isTuple(value)) break;

        tuple_GetItem(value.tuple, 0, &symbol);

        if (isIdentical(s_with, symbol)) continue;
        if (isIdentical(s_as,   symbol)) continue;
        if (isIdentical(s_bind, symbol)) continue;
        if (isIdentical(s_set,  symbol)) continue;

        break;
    }

    Tuple result;

    tuple_Section(frame, inx, 0, &result);

    return result;
}

extern SUBR(encode_elet)
{
    /*
    ** given args    = ([with <declare>...]? [as name expr...]? <initialize>... body)
    **       declare = name expr
    **    initialize = [bind name... expr]
    **               | [set  name... expr]
    */

    GC_Begin(7);

    Tuple frame;
    Tuple with;
    Tuple as;
    Tuple initialize;
    Tuple body;

    GC_Protect(frame);
    GC_Protect(with);
    GC_Protect(as);
    GC_Protect(initialize);
    GC_Protect(body);

    tuple_Convert(args.pair, &frame);

    with       = find_with(frame, env);
    as         = find_as(frame, env);
    initialize = find_initialize(frame, env);
    body       = find_body(frame);

body:
    encode(body, env, &body);

    tuple_Create(3, &frame);
    //    tuple_SetItem(frame, 0, bindings);
    //    tuple_SetItem(frame, 1, marker);
    //    tuple_SetItem(frame, 2, body);

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
    pair_Create(symbol, value, result.pair);

    GC_End();
}

extern SUBR(elet)
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
        eval_block(marker.symbol, body, env2, result);
    } else {
        eval_begin(body, env2, result);
    }

 exit:
    GC_End();
}

