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
extern SUBR(encode_define);
/* */

static void environ_Pattern(Node local, Node env, Target result)
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
        list_GetItem(local.pair, 1, &expr);
        if (!isNil(expr)) {
            Node nexpr = NIL;
            encode(expr, env, &nexpr);
            list_SetItem(local.pair, 1, nexpr);
        }
    }

    pair_Create(symbol, NIL, result.pair);
}

static void encode_Pattern(Node pattern, Node env, Target result)
{
    /*
    ** pattern = (ctor (<binding>...) . body)
    **         | (ctor name . body)
    **         | (_ (<binding>...) . body)
    **         | (_ name . body)
    **         | (. body)
    */
    GC_Begin(6);

    Node ctor;
    Node bindings;
    Pair bound;
    Node body;
    Node lenv;

    GC_Protect(ctor);
    GC_Protect(bindings);
    GC_Protect(bound);
    GC_Protect(body);
    GC_Protect(lenv);

    VM_ON_DEBUG(2, {
            fprintf(stderr,"encoding pattern ");
            prettyPrint(stderr, pattern);
            fprintf(stderr,"\n");
            fflush(stderr);
        });

    pair_GetCar(pattern.pair, &ctor);

    // (. body)
    if (isIdentical(ctor, s_dot)) {
        pair_GetCdr(pattern.pair, &body);

        encode(body, env, &(body.pair));

        pair_Create(ctor, body, result.pair);
        goto done;
    }

    if (!isSymbol(ctor)) {
        fprintf(stderr, "\ncase error: invalid constructor name: ");
        dump(stderr, ctor);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    // (ctor (<binding>...) . body)
    // (ctor <name> . body)
    // (_ (<binding>...) . body)
    // (_ <name> . body)
    list_GetItem(pattern.pair, 1, &bindings);
    list_GetTail(pattern.pair, 1, &body);

    if (isSymbol(bindings)) {
        pair_Create(bindings, NIL, &bound);
        pair_Create(bound, env, &(lenv.pair));

        encode(body, lenv, &(body.pair));

        pair_Create(bindings, body, &(body.pair));
        pair_Create(ctor, body, result.pair);
        goto done;
    }

    if (isPair(bindings)) {
        list_Map(bindings.pair, environ_Pattern, env, &(lenv.pair));
        list_SetEnd(lenv.pair, env);

        encode(body, lenv, &(body.pair));

        pair_Create(bindings, body, &(body.pair));
        pair_Create(ctor, body, result.pair);
        goto done;
    }

    fprintf(stderr, "\ncase error: invalid binding list: ");
    dump(stderr, bindings);
    fprintf(stderr, "\n");
    fflush(stderr);
    fatal(0);

 done:
    GC_End();
}

extern SUBR(encode_case)
{
    /*
    ** arg     = (expr <pattern>...)
    ** pattern = (ctor (<binding>...) . body)
    **         | (ctor name . body)
    **         | (_ (<binding>...) . body)
    **         | (_ name . body)
    **         | (. body)
    */
    GC_Begin(4);

    Node expr;
    Node cases;
    Node hold;

    GC_Protect(expr);
    GC_Protect(cases);
    GC_Protect(hold);

    pair_GetCar(args.pair, &expr);
    pair_GetCdr(args.pair, &cases);

    encode(expr, env, &(expr.pair));

    list_Map(cases.pair, encode_Pattern, env, &(cases.pair));

    pair_Create(expr, cases, result.pair);

    GC_End();
}

static void binding_Pattern(Node names, Node object, Node env, Pair* result)
{
    /*
    ** names = <binding>...
    **       | name
    */
    GC_Begin(10);

    Node name;
    Node value;
    Pair bound;
    Pair first;
    Pair next;
    Pair last;

    GC_Protect(first);
    GC_Protect(bound);
    GC_Protect(next);

    VM_ON_DEBUG(2, {
            fprintf(stderr,"binding names ");
            prettyPrint(stderr, names);
            fprintf(stderr,"\n");
            fflush(stderr);
        });

    if (!isPair(names)) {
        bindValue(env.pair, names.symbol, object, &last);
        goto done;
    }

    if (isAtomic(object)) {
        fprintf(stderr, "\ncase error: invalid compound value: ");
        dump(stderr, object);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    unsigned index = 0;
    for (;;) { // skip leading s_uscore
        pair_GetCar(names.pair, &name);
        if (!isIdentical(name, s_uscore)) break;
        pair_GetCdr(names.pair, &names);
        index += 1;
    }

    tuple_GetItem(object.tuple, index, &value);
    bindValue(env.pair, name.symbol, value, &first);

    last = first;

    for (;;) {
        pair_GetCdr(names.pair, &names);

        if (!isPair(names)) goto done;

        index += 1;
        pair_GetCar(names.pair, &name);

        if (isIdentical(name, s_uscore)) continue;

        tuple_GetItem(object.tuple, index, &value);

        bindValue(last, name.symbol, value, &next);

        last = next;
    }

 done:
    ASSIGN(result, last);
    GC_End();
}

extern SUBR(case)
{
    /*
    ** arg     = (expr <pattern>...)
    ** pattern = (ctor (<binding>...) . body)
    **         | (ctor name . body)
    **         | (_ (<binding>...) . body)
    **         | (_ name . body)
    **         | (. body)
    */
    GC_Begin(10);

    Node expr;
    Node cases;
    Node value;
    Node env2;
    Node pattern;
    Node bindings;
    Node body;
    Node elsePtn;

    GC_Protect(expr);
    GC_Protect(cases);
    GC_Protect(value);
    GC_Protect(env2);
    GC_Protect(pattern);
    GC_Protect(bindings);
    GC_Protect(body);
    GC_Protect(elsePtn);

    pair_GetCar(args.pair, &expr);
    pair_GetCdr(args.pair, &cases);

    eval(expr, env, &value);
    forceArg(value, &value);

    const Symbol ctor = getConstructor(value);

    bool          atomic = isAtomic(value);
    unsigned long length = 0;

    if (!atomic) {
        length = getCount(value);
    }

    while (isPair(cases)) {
        pair_GetCar(cases.pair, &pattern);
        pair_GetCdr(cases.pair, &cases);

        if (isSymbol(pattern.pair->car)) {
            if (isIdentical(pattern.pair->car, s_dot)) {
                elsePtn = pattern;
                continue;
            }
            if (!isIdentical(pattern.pair->car, s_uscore)) {
                if (!isIdentical(pattern.pair->car, ctor)) continue;
            }
            pair_GetCdr(pattern.pair, &pattern);
        }

        if (isSymbol(pattern.pair->car)) {
            pair_GetCar(pattern.pair, &bindings);
            pair_GetCdr(pattern.pair, &body);
            goto found_pattern;
        } else {
            if (atomic) continue;
            if (isPair(pattern.pair->car)) {
                unsigned count  = 0;
                bool     dotted = false;

                pair_GetCar(pattern.pair, &bindings);
                list_State(bindings.pair, &count, &dotted);

                if (!dotted) {
                    if (length != count) continue;
                } else {
                    if (length < (count - 1)) continue;
                }

                pair_GetCdr(pattern.pair, &body);
                goto found_pattern;
            }
        }
    }

    if (isNil(elsePtn)) {
        ASSIGN(result, NIL);
        GC_End();
        return;

    }

    pair_GetCdr(elsePtn.pair, &body);

 found_else:  // (. body)
    VM_ON_DEBUG(2, {
            fprintf(stderr,"found else \n");
            fflush(stderr);
        });

    eval_begin(body, env, result);
    GC_End();
    return;

 found_pattern:
    VM_ON_DEBUG(2, {
            fprintf(stderr,"found match\n");
            fflush(stderr);
        });

    binding_Pattern(bindings, value, env, &(env2.pair));
    eval_begin(body, env2, result);
    GC_End();
}
