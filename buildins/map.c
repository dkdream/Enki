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

static void call_with(Node function, Node value, Node env, Target target) {
    GC_Begin(5);
    GC_Add(function);
    GC_Add(value);
    GC_Add(env);

    Pair args;

    GC_Protect(args);

    pair_Create(value, NIL, &args);
    apply(function, args, env, target);

    GC_End();
}

extern SUBR(map)
{
    Node function;
    Node list;

    checkArgs(args, "map", 1, NIL, s_pair);
    forceArgs(args, &function, &list, 0);

    if (isNil(function)) {
        ASSIGN(result, list);
        return;
    }

    if (!isPair(list)) {
        call_with(function, list, env, result);
        return;
    }

    Pair pair = list.pair;

    GC_Begin(4);

    Node input;
    Node output;
    Node first;

    GC_Protect(input);
    GC_Protect(output);
    GC_Protect(first);

    call_with(function, pair->car, env, &output);

    pair_Create(output,NIL, &first.pair);

    Pair last = first.pair;

    for (; isPair(pair->cdr.pair) ;) {
        pair   = pair->cdr.pair;
        input  = pair->car;
        output = NIL;

        call_with(function, input, env, &output);

        pair_Create(output,NIL, &(last->cdr.pair));

        last = last->cdr.pair;
    }

    if (!isNil(pair->cdr.pair)) {
        input  = pair->cdr;
        output = NIL;

        call_with(function, input, env, &output);
        pair_SetCdr(last, output);
    }

    ASSIGN(result, first);

    GC_End();
}
