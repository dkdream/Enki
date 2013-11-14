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

/**
 * find
 *   [with <declare>...]
 *   declare = name expr
 *
 * target <- [(name . expr)...]
 */
static bool find_with(Tuple frame, const Node env, Tuple* target) {
    if (!frame) return false;

    Kind    kind = asKind(frame);
    unsigned max = kind->count;
    unsigned inx = 0;
    Tuple clause = (Tuple) 0;
    Node   symbol;

    ASSIGN(target, NIL);

    for (; ; ++inx) {
        if (inx >= max) return false;

        Node value = frame->item[inx];

        if (!isTuple(value)) return false;

        tuple_GetItem(value.tuple, 0, &symbol);

        if (isIdentical(s_with, symbol)) {
            clause = value.tuple;
            break;
        }
    }

    Tuple locals;
    Node  name;
    Node  type;
    Pair  var;

    max = getCount(clause);

    if (1 >= max) return false;

    GC_Begin(5);

    GC_Protect(locals);
    GC_Protect(name);
    GC_Protect(type);
    GC_Protect(var);

    unsigned jnx = 0;
    unsigned vcount = max >> 1;

    tuple_Create(vcount, &locals);

    inx = 0;
    for(;; ++jnx) {
        if (++inx > max) break;

        name = clause->item[inx];

        if (++inx > max) {
            pair_Create(name, NIL, &var);
            tuple_SetItem(locals, jnx, var);
            break;
        }

        encode(clause->item[inx], env, &type);

        pair_Create(name, type, &var);

        tuple_SetItem(locals, jnx, var);
    }

    ASSIGN(target, locals);
    GC_End();
    return true;
}


static bool find_as(Tuple frame, const Node env, Pair* target) {
    if (!frame) return false;

    Kind    kind = asKind(frame);
    unsigned max = kind->count;
    unsigned inx = 0;
    Tuple clause = (Tuple) 0;
    Node   symbol;

    for (; ; ++inx) {
        if (inx >= max) return false;

        Node value = frame->item[inx];

        if (!isTuple(value)) return false;

        tuple_GetItem(value.tuple, 0, &symbol);

        if (isIdentical(s_as, symbol)) {
            clause = value.tuple;
            break;
        }
    }

    Pair  returns;
    Node  name;
    Node  type;
    Tuple results;

    max = getCount(clause);

    // [with] ?
    if (1 >= max) return false;

    GC_Begin(5);

    GC_Protect(returns);
    GC_Protect(name);
    GC_Protect(type);
    GC_Protect(results);

    switch (max) {
    case 0:
    case 1: //[with]
        break;

    case 2: // [with name]
        pair_Create(name, NIL, &returns);
        break;

    case 3:// [with name type]
        encode(clause->item[2], env, &type);
        pair_Create(name, type, &returns);
        break;

    default: // [with name type0..typen]
        tuple_Section(clause, 2, 0, &results);
        tuple_Update(results, encode, env, 0);
        pair_Create(name, results, &returns);
        break;
    }

    ASSIGN(target, returns);
    GC_End();
    return true;
}

static bool find_initialize(Tuple frame, const Node env, Tuple* target) {

    if (!frame) return false;

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

        VM_DEBUG(2, "checking %s", symbol_Text(symbol.symbol));

        // [bind n1 .. ni expr]
        if (isIdentical(s_bind, symbol)) {
            ++count;
            bits_set(&array, inx, true);
            continue;
        }

        // [set n1 .. ni expr]
        if (isIdentical(s_set, symbol)) {
            ++count;
            bits_set(&array, inx, true);
            continue;
        }
    }

    if (0 == count) return false;

    Tuple result;

    tuple_Select(frame, count, &array, &result);

    bits_free(&array);

    tuple_Update(result, encode, env, 0);

    ASSIGN(target, result);

    return true;
}

static bool find_body(Tuple frame, Tuple* target) {

    if (!frame) return false;

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

    return tuple_Section(frame, inx, 0, target);
}

static bool var_declare(unsigned index, Node declare, Node context, Node env, Target result) {
    Symbol   name;
    Constant type;

    VM_ON_DEBUG(2, {
            fprintf(stderr, "declare var %u ", index);
            prettyPrint(stderr, declare);
            fprintf(stderr,"\n context ");
            prettyPrint(stderr, context);
            fprintf(stderr,"\n");
            fflush(stderr);
        });

    if (!isPair(declare)) {
        ASSIGN(result, context);
        return true;
    }

    name = declare.pair->car.symbol;
    type = declare.pair->cdr.constant;

    return alist_Declare(context.pair, name, type, result.pair);
}

static bool encode_body(Tuple with, Pair as, Node env, Tuple body)
{
    GC_Begin(3);

    Node context;

    GC_Protect(context);

    tuple_FoldRight(with, env, var_declare, NIL, &context);

    if (as) {
        alist_Declare(context.pair, as->car.symbol, as->cdr.constant, &(context.pair));
    }

    tuple_Update(body, encode, context, 0);

    GC_End();
    return true;
}

static bool bind_vars(unsigned index, Node binding, Node context, Node env, Target result) {
    ASSIGN(result, context);

    VM_ON_DEBUG(2, {
         fprintf(stderr, "binding var %u ", index);
         prettyPrint(stderr, binding);
         fprintf(stderr,"\n context ");
         prettyPrint(stderr, context);
         fprintf(stderr,"\n");
         fflush(stderr);
        });

    if (!isTuple(binding)) return true;

    unsigned max = getCount(binding);

    if (1 >= max) return true;

    Tuple  entry = binding.tuple;

    if (2 == max) {
        alist_Set(context.pair, entry->item[1].symbol, NIL);
        return true;
    }

    Node expr = entry->item[max-1];
    Node  value;

    GC_Begin(3);

    GC_Protect(value);

    eval(expr, env, &value);

    switch (max) {
    case 0: //[]
    case 1: //[bind],     [set]
    case 2: //[bind name],[set name]
        break;

    case 3: //[bind name expr],[set name expr]
        VM_ON_DEBUG(2, {
                Symbol symbol = entry->item[1].symbol;
                fprintf(stderr, "binding %s ", symbol_Text(symbol));
                prettyPrint(stderr, value);
                fprintf(stderr,"\n");
                fflush(stderr);
            });

        alist_Set(context.pair, entry->item[1].symbol, value);
        break;

    default: //[bind n1..ni expr],[set n1..ni expr]
        if (!isTuple(value)) {
            VM_ON_DEBUG(2, {
                    Symbol symbol = entry->item[1].symbol;
                    fprintf(stderr, "binding %s ", symbol_Text(symbol));
                    prettyPrint(stderr, value);
                    fprintf(stderr,"\n");
                    fflush(stderr);
                });
            alist_Set(context.pair, entry->item[1].symbol, value);
        } else {
            Tuple group = value.tuple;
            --max;
            unsigned count = getCount(value);
            unsigned inx   = 1;
            unsigned jnx   = 0;
            for(;; ++inx,++jnx) {
                if (inx >= max) break;
                if (jnx >= count) break;
                VM_ON_DEBUG(2, {
                        Symbol symbol = entry->item[inx].symbol;
                        Node   value  = group->item[jnx];
                        fprintf(stderr, "binding %s ", symbol_Text(symbol));
                        prettyPrint(stderr, value);
                        fprintf(stderr,"\n");
                        fflush(stderr);
                    });

                alist_Set(context.pair,
                          entry->item[inx].symbol,
                          group->item[jnx]);
            }
        }
    }

    GC_End();
    return true;
}

extern SUBR(encode_elet)
{
    /*
    ** given args    = ([with <declare>...]? [as name expr...]? <initialize>... body)
    **       declare = name expr
    **    initialize = [bind name... expr]
    **               | [set  name... expr]
    */

    GC_Begin(10);

    Tuple frame;
    Tuple with;
    Pair  as;
    Tuple initialize;
    Tuple block;
    Pair  body;
    Node  context;

    GC_Protect(frame);
    GC_Protect(with);
    GC_Protect(as);
    GC_Protect(initialize);
    GC_Protect(block);
    GC_Protect(body);
    GC_Protect(context);

    tuple_Convert(args.pair, &frame);

    find_with(frame, env, &with);
    find_as(frame, env, &as);
    find_initialize(frame, env, &initialize);
    find_body(frame, &block);

    VM_ON_DEBUG(2, {
            fprintf(stderr,"encode elet <begin>\n with ");
            prettyPrint(stderr, with);
            fprintf(stderr,"\n as ");
            prettyPrint(stderr, as);
            fprintf(stderr,"\n initialize ");
            prettyPrint(stderr, initialize);
            fprintf(stderr,"\n body");
            prettyPrint(stderr, block);
            fprintf(stderr,"\n");
            fprintf(stderr,"<end>\n");
            fflush(stderr);
        });

    if (with) {
        tuple_FoldRight(with, env, var_declare, NIL, &context);
    }
    if (as) {
        alist_Declare(context.pair, as->car.symbol, as->cdr.constant, &(context.pair));
    }
    tuple_Update(block, encode, context, 0);
    list_Convert(block, &body);

    tuple_Create(4, &frame);
    tuple_SetItem(frame, 0, with);
    tuple_SetItem(frame, 1, as);
    tuple_SetItem(frame, 2, initialize);
    tuple_SetItem(frame, 3, body);

    ASSIGN(result, frame);

    GC_End();
}

extern SUBR(elet)
{
    GC_Begin(7);

    Tuple frame;
    Node  context;
    Node  with;
    Node  as;
    Node  initialize;
    Node  body;
    Node  tmp;

    GC_Protect(frame);
    GC_Protect(context);
    GC_Protect(with);
    GC_Protect(as);
    GC_Protect(initialize);
    GC_Protect(body);

    frame = args.tuple;

    tuple_GetItem(frame, 0, &with);
    tuple_GetItem(frame, 1, &as);
    tuple_GetItem(frame, 2, &initialize);
    tuple_GetItem(frame, 3, &body);

    VM_ON_DEBUG(2, {
            fprintf(stderr,"eval elet <begin>\n with ");
            prettyPrint(stderr, with);
            fprintf(stderr,"\n as ");
            prettyPrint(stderr, as);
            fprintf(stderr,"\n initialize ");
            prettyPrint(stderr, initialize);
            fprintf(stderr,"\n body");
            prettyPrint(stderr, body);
            fprintf(stderr,"\n");
            fprintf(stderr,"<end>\n");
            fflush(stderr);
        });

    if (!isTuple(with)) {
        context = env;
    } else {
        tuple_FoldRight(with.tuple, env, var_declare, env, &context);
    }

    if (isTuple(initialize)) {
        tuple_FoldRight(initialize.tuple, context, bind_vars, env, &context);
    }

    if (!isPair(as)) {
        eval_begin(body, context, result);
    } else {
        // fetch name [as name t1..]
        eval_block(as.pair->car.symbol,
                   as.pair->cdr.constant,
                   body, context, result);
    }

 exit:
    GC_End();
}

