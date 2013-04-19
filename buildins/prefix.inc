/*-*- mode: c;-*-*/
/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#define _GNU_SOURCE
#include "../all_types.inc"
#include "../treadmill.h"
#include "../debug.h"
#include "../text_buffer.h"
#include "../id_set.h"

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

#define SUBR(NAME) opr_##NAME(Node args, Node env, Target result)
#define checkArgs(node, name, min, ...) countArgs(node)

#define int_do(NAME, OP) \
void SUBR(NAME) \
{ \
    Node left; Node right; \
    checkArgs(args, #OP, 2, t_integer, t_integer); \
    forceArgs(args, &left, &right, 0); \
    integer_Create((left.integer->value) OP (right.integer->value), result.integer); \
}

static unsigned countArgs(Node args) {
    unsigned count  = 0;
    bool     dotted = false;

    list_State(args.pair, &count, &dotted);

    return count;
}

static bool opaque_Create(Node type, long size, Reference* target) {
    if (!node_Allocate(_zero_space,
                       true,
                       size,
                       target))
        return 0;

    Reference result = *target;

    memset(result, 0, size);

    setType(result, type);

    return result;
}

#if 0
static unsigned checkArgs(Node args, const char* name, unsigned min, ...)
{
    unsigned count  = 0;
    bool     dotted = false;

    list_State(args.pair, &count, &dotted);

    if (count < min) {
        fatal("too few arguments (%i) to: %s\n", count, name);
    }

    return count;
}
#endif

static void forceArg(Node arg, Target result) {
    GC_Begin(2);
    Node tmp;

    GC_Protect(tmp);

    if (isType(arg, t_delay)) {
        Node dexpr, denv;
        tuple_GetItem(arg.tuple, 1, &dexpr);
        tuple_GetItem(arg.tuple, 2, &denv);
        eval(dexpr, denv, &tmp);
        tuple_SetItem(arg.tuple, 0, tmp);
        setType(arg.tuple, t_forced);
        ASSIGN(result, tmp);
        goto done;
    }

    if (isType(arg, t_forced)) {
        tuple_GetItem(arg.tuple, 0, result);
        goto done;
    }

    ASSIGN(result, arg);

  done:
    GC_End();
}

static void forceArgs(Node args, ...)
{
    GC_Begin(2);
    Node tmp;

    GC_Protect(tmp);

    va_list ap;
    va_start(ap, args);

    darken_Node(args);

    while (isType(args, t_pair)) {
        Node holding   = NIL;
        Node *location = va_arg(ap, Node*);

        if (!location) goto done;

        pair_GetCar(args.pair, &holding);

        forceArg(holding, location);

        pair_GetCdr(args.pair, &args);

        darken_Node(args);
    }

    for(;;) {
        Node *location = va_arg(ap, Node*);
        if (!location) goto done;
        *location = NIL;
    }

 done:
    va_end(ap);
    GC_End();
}

static void fetchArgs(Node args, ...)
{
    va_list ap;
    va_start(ap, args);

    darken_Node(args);

    while (isType(args, t_pair)) {
        Node *location = va_arg(ap, Node*);

        if (!location) goto done;

        pair_GetCar(args.pair, location);
        pair_GetCdr(args.pair, &args);

        darken_Node(args);
    }

    for(;;) {
        Node *location = va_arg(ap, Node*);
        if (!location) goto done;
        *location = NIL;
    }

 done:
    va_end(ap);
}


static void eval_binding(Node binding, Node env, Target entry)
{
    GC_Begin(2);
    Node symbol, expr, value;

    GC_Protect(expr);
    GC_Protect(value);

    list_GetItem(binding.pair, 0, &symbol);
    list_GetItem(binding.pair, 1, &expr);

    eval(expr, env, &value);

    pair_Create(symbol, value, entry.pair);

    GC_End();
}

static void eval_begin(Node body, Node env, Target last)
{
    GC_Begin(2);

    Node expr, value;

    GC_Protect(expr);
    GC_Protect(value);

    while (isType(body, t_pair)) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &value);
    }

    ASSIGN(last, value);
    GC_End();
}

static void environ_Let(Node local, Node env, Target result)
{
    Node symbol;
    pair_GetCar(local.pair, &symbol);
    pair_Create(symbol, NIL, result.pair);
}

static void environ_Lambda(Node symbol, Node env, Target result)
{
    pair_Create(symbol, NIL, result.pair);
}

static void call_with(const Node function, const Node value, const Node env, Target target) {
    Pair args;
    pair_Create(value, NIL, &args);
    apply(function, args, env, target);
}

static void formatAppendTo(TextBuffer *buffer, Node value) {
    char data[20];

    if (isNil(value)) return;

    Node type = getType(value);

    if (isNil(type)) return;

    if (isIdentical(type, s_symbol)) {
        buffer_add(buffer, symbol_Text(value.symbol));
        return;
    }

    if (isIdentical(type, t_text)) {
        buffer_add(buffer, text_Text(value.text));
        return;
    }

    if (isIdentical(type, t_integer)) {
        sprintf(data, "%lld", value.integer->value);
        buffer_add(buffer, data);
        return;
    }
}

struct os_file {
    FILE* file;
};

typedef struct os_file* OSFile;
