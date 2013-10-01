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

static bool __initialized = false;
//
unsigned int ea_global_debug = 0;
unsigned int ea_global_trace = 0;

//
struct gc_treadmill enki_zero_space;
struct gc_header    enki_true;
struct gc_header    enki_false;
struct gc_header    enki_unit;
struct gc_header    enki_void;


Space     _zero_space;
unsigned __alloc_cycle;
unsigned __scan_cycle;

Node        enki_globals = NIL; // nt_pair(nil, alist)
Node              true_v = NIL;
Node             false_v = NIL;
Node              unit_v = NIL;
Node              void_v = NIL;
Primitive  p_encode_args = 0;
Primitive  p_eval_symbol = 0;
Primitive    p_eval_pair = 0;
Primitive p_apply_lambda = 0;
Primitive  p_apply_delay = 0;
Primitive p_apply_forced = 0;
Primitive   p_apply_form = 0;

#define SUBR(NAME) void opr_##NAME(Node args, Node env, Target result)

extern void defineValue(Node symbol, const Node value) {
    GC_Begin(2);
    Node globals;

    GC_Protect(globals);

    pair_GetCdr(enki_globals.pair, &globals);
    alist_Add(globals.pair, symbol, value, &globals.pair);
    pair_SetCdr(enki_globals.pair, globals);

    GC_End();
}

extern bool opaque_Create(Node type, const Symbol ctor, long size, Reference* target) {
    if (!node_Allocate(_zero_space,
                       true,
                       size,
                       target))
        return 0;

    Reference result = *target;

    memset(result, 0, size);

    //    if (isIdentical(type, t_symbol)) return true;

    setType(result, type);
    setConstructor(result, ctor);

    return true;
}

// args := (type | constructor | nil)*
extern unsigned checkArgs(Node args, const char* name, unsigned min, ...)
{
    unsigned count  = 0;
    bool     dotted = false;

    list_State(args.pair, &count, &dotted);

    if (count < min) {
        fatal("too few arguments (%i) to: %s\n", count, name);
    }

    return count;
}

extern void forceArg(Node arg, Target result) {
    GC_Begin(2);
    Node tmp;

    GC_Protect(tmp);

    if (fromCtor(arg, s_delay)) {
        Node dexpr, denv;
        tuple_GetItem(arg.tuple, 1, &dexpr);
        tuple_GetItem(arg.tuple, 2, &denv);
        eval(dexpr, denv, &tmp);
        tuple_SetItem(arg.tuple, 0, tmp);
        setConstructor(arg.tuple, s_forced);
        ASSIGN(result, tmp);
        goto done;
    }

    if (fromCtor(arg, s_forced)) {
        tuple_GetItem(arg.tuple, 0, result);
        goto done;
    }

    ASSIGN(result, arg);

  done:
    GC_End();
}

extern int forceArgs(Node args, ...)
{
    int count = 0;

    GC_Begin(2);
    Node tmp;

    GC_Protect(tmp);

    va_list ap;
    va_start(ap, args);

    Node *location = va_arg(ap, Node*);

    darken_Node(args);

    while (isPair(args)) {
        ++count;

        if (location) {
            Node holding   = NIL;

            pair_GetCar(args.pair, &holding);

            forceArg(holding, location);

            location = va_arg(ap, Node*);
        }

        pair_GetCdr(args.pair, &args);

        darken_Node(args);
    }

    for(;;) {
        if (!location) goto done;
        *location = NIL;
        location = va_arg(ap, Node*);
    }

 done:
    va_end(ap);
    GC_End();

    return count;
}

extern int fetchArgs(Node args, ...)
{
    int count = 0;

    va_list ap;
    va_start(ap, args);

    Node *location = va_arg(ap, Node*);

    darken_Node(args);

    while (isPair(args)) {
        ++count;

        if (location) {
            pair_GetCar(args.pair, location);
            location = va_arg(ap, Node*);
        }

        pair_GetCdr(args.pair, &args);

        darken_Node(args);
    }

    for(;;) {
        if (!location) goto done;
        *location = NIL;
        location = va_arg(ap, Node*);
    }

 done:
    va_end(ap);

    return count;
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

extern void call_begin(Node variables,
                       Node body,
                       Symbol exit,
                       Node env,
                       Node values,
                       Target result)
{
}

extern SUBR(system_check)
{
    check_SymbolTable__(__FILE__, __LINE__);
    ASSIGN(result, NIL);
}

extern SUBR(if)
{ //Fixed
    GC_Begin(2);

    Node tst, val, t_expr, e_body;

    GC_Protect(val);

    list_GetItem(args.pair, 0, &tst);
    list_GetItem(args.pair, 1, &t_expr);
    list_GetTail(args.pair, 1, &e_body);

    eval(tst, env, &val);

    if (isNil(val)) {
        eval_begin(e_body, env, result);
    } else {
        eval(t_expr, env, result);
    }
    GC_End();
}

extern SUBR(and)
{ //Fixed
    GC_Begin(2);
    Node body, expr, ans;

    GC_Protect(ans);

    body = args;
    expr = NIL;
    ans  = true_v;

    for (; isPair(body) ;) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &ans);
        if (isNil(ans)) break;
    }

    ASSIGN(result, ans);
    GC_End();
}

extern SUBR(or)
{ //Fixed
    GC_Begin(2);
    Node body, expr, ans;

    GC_Protect(ans);

    body = args;
    expr = NIL;
    ans  = NIL;

    for (; isPair(body) ;) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &ans);
        if (!isNil(ans)) break;
    }

    ASSIGN(result, ans);
    GC_End();
}

extern SUBR(while)
{ //Fixed
    Node tst  = NIL;
    Node body = NIL;
    Node val  = NIL;

    pair_GetCar(args.pair, &tst);
    pair_GetCdr(args.pair, &body);

    for (;;) {
        eval(tst, env, &val);

        if (isNil(val)) break;

        eval_begin(body, env, result);
    }
}

extern SUBR(begin)
{ //Fixed
    eval_begin(args, env, result);
}

extern SUBR(bind)
{ //Fixed
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;
    Node temp   = NIL;

    fetchArgs(args, &symbol, &expr, 0);

    if (!isSymbol(symbol)) {
        fprintf(stderr, "\nerror: non-symbol identifier in set: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    Pair entry;

    if (!alist_Entry(env.pair, symbol, &entry)) {
        fprintf(stderr, "\nerror: cannot set undefined variable: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    pair_GetCdr(entry, &temp);

    if (!isIdentical(temp, void_v)) {
        fprintf(stderr, "\nerror: cannot bind a bound local value: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    eval(expr, env, &value);

    pair_SetCdr(entry, value);

    ASSIGN(result,value);
}

extern SUBR(set)
{ //Fixed
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;
    Node temp   = NIL;

    fetchArgs(args, &symbol, &expr, 0);

    if (!isSymbol(symbol)) {
        fprintf(stderr, "\nerror: non-symbol identifier in set: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    Pair entry;

    if (!alist_Entry(env.pair, symbol, &entry)) {
        fprintf(stderr, "\nerror: cannot set undefined variable: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    pair_GetCdr(entry, &temp);

    if (isIdentical(temp, void_v)) {
        fprintf(stderr, "\nerror: cannot set an unbound local value: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    eval(expr, env, &value);

    pair_SetCdr(entry, value);

    ASSIGN(result,value);
}

extern SUBR(encode_args)
{
    list_Map(encode, args.pair, env, result);
}

extern SUBR(encode_define)
{
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;

    fetchArgs(args, &symbol, &expr, 0);

    if (!isSymbol(symbol)) {
        fprintf(stderr, "\nerror: non-symbol identifier in define: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    encode(expr, env, &value);

    pair_Create(value, NIL, &(value.pair));
    pair_Create(symbol, value, result.pair);
}

extern SUBR(define)
{ //Fixed
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;

    fetchArgs(args, &symbol, &expr, 0);

    if (!isSymbol(symbol)) {
        fprintf(stderr, "\nerror: non-symbol identifier in define: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    eval(expr, env, &value);

    VM_DEBUG(1, "defining %s", symbol_Text(symbol.symbol));

    defineValue(symbol, value);

    ASSIGN(result, value);
}

extern SUBR(quote)
{ //Fixed
    pair_GetCar(args.pair, result);
}

extern SUBR(encode_type) {
    Node value;

    pair_GetCar(args.pair, &value);

    if (isSymbol(value)) {
        ASSIGN(result, value);
        return;
    }

    if (isBlock(value)) {
        Symbol type;
        Symbol sort;

        switch (getCount(value)) {
        case 0:
            fatal("invalid block to type: size==0");
            return;

        case 1:
            tuple_GetItem(value.tuple, 0, &sort);
            if (isSymbol(sort)) {
                sort_Create(sort, result.constant);
            } else {
                fatal("named sorts must be symbols");
            }
            return;

        case 2:
            tuple_GetItem(value.tuple, 0, &type);
            tuple_GetItem(value.tuple, 1, &sort);
            if (!isSymbol(sort)) {
                fatal("named sorts must use symbols");
            }
            if (!isSymbol(type)) {
                fatal("named types must use symbols");
            }
            {
                Constant set;
                sort_Create(sort, &set);
                type_Create(type, set, result.constant);
            }
            return;

        default:
            fatal("invalid block to type: size>2");
        }
        return;
    }

    if (isPair(value)) {
        encode(value, env, result);
        return;
    }

    ASSIGN(result, args);
}

extern SUBR(type)
{ //Fixed
    Node value;

    if (isSymbol(args)) {
        Pair entry;
        // lookup symbol in the current enviroment
        if (!alist_Entry(env.pair, args.symbol, &entry)) {
            // lookup symbol in the global enviroment
            if (!alist_Entry(enki_globals.pair,  args.symbol, &entry)) {
                fatal("undefined variable: %s", symbol_Text(args.symbol));
            }
        }
        pair_GetCdr(entry, result);
        return;
    }

    if (isAType(args)) {
        ASSIGN(result, args);
        return;
    }

    if (isASort(args)) {
        ASSIGN(result, args);
        return;
    }

    if (isPair(args)) {
        eval(args, env, result);
        return;
    }

    // this needs to handle complex types
    // right now its is just like quote
    ASSIGN(result, args);
    return;
}

extern void environ_Let(Node local, Node env, Target result)
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

extern SUBR(encode_let)
{
    /*
    ** given args    = ((<binding>...) . body)
    **       args    = (_ . body)
    **       binding = name
    **               | (name expr)
    **               | (name type ... expr)
    ** to-do
    **       args    = ((binding...) [as name expr.r] . body)
    **               | ([as name expr.r] . body)
    **               | ([name.0 expr.0 ... name.n expr.n] [as name expr.r] initialize . body)
    **               | ([name.0 expr.0 ... name.n expr.n] initialize . body)
    **    initialize = bound...
    **         bound = [bind name expr.b]
    **         bound = [set  name expr.b]
    **
    */

    /* notes:
    **   when using '(' ')'
    **   - the expr's need to be evaluated in the current context
    **   when using '[' ']'
    **   - the expr.i, expr.r, expr.b's  needs to be encoded in the current context
    */

    GC_Begin(4);

    Node bindings;
    Node body;
    Node lenv;

    GC_Protect(bindings);
    GC_Protect(body);
    GC_Protect(lenv);

    pair_GetCar(args.pair, &bindings);
    pair_GetCdr(args.pair, &body);

    if (!isPair(bindings)) {
        encode(args, env, result);
        GC_End();
        return;
    }

    list_Map(environ_Let, bindings.pair, env, &lenv);
    list_SetEnd(lenv.pair, env);

    encode(body, lenv, &body);

    pair_Create(bindings, body, result.pair);

    GC_End();
}

extern void binding_Let(Node local, Node env, Target result)
{
    /*
    ** local = name
    **       | (name expr)
    */
    GC_Begin(4);

    Node symbol;
    Node expr;
    Node type;
    Node value;

    GC_Protect(expr);
    GC_Protect(type);
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

    if (!isNil(type)) {
        eval(type, env, &type);
        if (!inType(value, type)) {
            value = void_v;
        }
    }

  done:
    pair_Create(symbol, value, result.pair);

    GC_End();
}

extern SUBR(let)
{
    /*
    ** given args    = ((binding...) . body)
    **       args    = (_ . body)
    **       binding = name
    **               | (name expr)
    ** to-do
    **       args    = ((binding...) [as name expr.r] . body)
    **               | ([as name expr.r] . body)
    **               | ([name.0 expr.0 ... name.n expr.n] [as name expr.r] initialize . body)
    **               | ([name.0 expr.0 ... name.n expr.n] initialize . body)
    **    initialize = bound...
    **         bound = [bind name expr.b]
    **         bound = [set  name expr.b]
    */

    /* notes:
    **   when using '(' ')'
    **   - the expr's need to be evaluated in the current context
    **   when using '[' ']'
    **   - the expr.i, expr.r, expr.b's need to be evaluated in the current context
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
        list_Map(binding_Let, bindings.pair, env, &(env2.pair));

        list_SetEnd(env2.pair, env);

        eval_begin(body, env2, result);
    }

    GC_End();
}

extern SUBR(encode_fix)
{
    /*
    ** given args    = ((<binding>...) . body)
    **       args    = (_ . body)
    **       binding = name
    **               | (name expr)
    **               | (name type ... expr)
    ** to-do
    **       args    = ((binding...) [as name expr.r] . body)
    **               | ([as name expr.r] . body)
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

    GC_Begin(4);

    Node bindings;
    Node lenv;

    GC_Protect(bindings);
    GC_Protect(lenv);

    pair_GetCar(args.pair, &bindings);

    if (!isPair(bindings)) {
        encode(args, env, result);
        GC_End();
        return;
    }

    list_Map(environ_Let, bindings.pair, env, &lenv);
    list_SetEnd(lenv.pair, env);

    encode(args, lenv, result);

    GC_End();
}

extern SUBR(fix)
{
    /*
    ** given args    = ((binding...) . body)
    **       args    = (_ . body)
    **       binding = name
    **               | (name expr)
    ** to-do
    **       args    = ((binding...) [as name expr.r] . body)
    **               | ([as name expr.r] . body)
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
        list_Map(binding_Let, bindings.pair, env, &(env2.pair));

        list_SetEnd(env2.pair, env);

        eval_begin(body, env2, result);
    }

    GC_End();
}

extern void encode_Pattern(Node pattern, Node env, Target result)
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
        list_Map(environ_Let, bindings.pair, env, &lenv);
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

    list_Map(encode_Pattern, cases.pair, env, &(cases.pair));

    pair_Create(expr, cases, result.pair);

    GC_End();
}

extern void binding_Pattern(Node names, Node object, Node env, Pair* result)
{
    /*
    ** pattern = (ctor (<binding>...) . body)
    **         | (ctor name . body)
    **         | (_ (<binding>...) . body)
    **         | (_ name . body)
    **         | (. body)
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
        pair_Create(names, object, &bound);
        pair_Create(bound, NIL, &first);
        last = first;
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
    for (;;) {
        pair_GetCar(names.pair, &name);
        if (!isIdentical(name, s_uscore)) break;
        pair_GetCdr(names.pair, &names);
        index += 1;
    }

    tuple_GetItem(object.tuple, index, &value);
    pair_Create(name, value, &bound);
    pair_Create(bound, NIL, &first);

    last = first;

    for (;;) {
        pair_GetCdr(names.pair, &names);
        if (!isPair(names)) goto done;
        index += 1;
        pair_GetCar(names.pair, &name);
        if (isIdentical(name, s_uscore)) continue;
        tuple_GetItem(object.tuple, index, &value);
        pair_Create(name, value, &bound);
        pair_Create(bound, NIL, &next);
        pair_SetCdr(last, next);
        last = next;
    }

 done:
    VM_ON_DEBUG(2, {
            fprintf(stderr,"bound names ");
            prettyPrint(stderr, first);
            fprintf(stderr,"\n");
            fflush(stderr);
        });

    pair_SetCdr(last, env);
    ASSIGN(result, first);
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

extern void environ_Lambda(Node parameter, Node env, Target result)
{
    Node symbol = NIL;

    if (isSymbol(parameter)) {
        pair_Create(parameter, NIL, result.pair);
        return;
    }

    pair_Create(parameter, NIL, result.pair);
}

extern SUBR(encode_lambda)
{
    /*
    ** args    = (formals . body)
    ** formals = name
    **         | ()
    **         | (name ...)
    **         | (name ... . name)
    ** to-do
    **         | [name.0 expr.0 ... name.n expr.n] [as name expr.r]
    **         | [name.0 expr.0 ... name.n expr.n] [as name]
    **         | [name.0 expr.0 ... name.n expr.n]
    */

    /* notes:
    **    when using '[' ']'
    **    the expr.i needs to be encoded in the current context
    */

    GC_Begin(4);

    Node formals;
    Node body;
    Node lenv;

    GC_Protect(formals);
    GC_Protect(body);
    GC_Protect(lenv);

    pair_GetCar(args.pair, &formals);
    pair_GetCdr(args.pair, &body);

    if (!isPair(formals)) {
        environ_Lambda(formals, env, &lenv);
    } else {
        list_Map(environ_Lambda, formals.pair, env, &lenv);
    }
    list_SetEnd(lenv.pair, env);

    encode(body, lenv, &(body.pair));

    pair_Create(formals, body, result.pair);

    GC_End();
}

extern SUBR(lambda)
{
    /*
    ** args    = (formals . body)
    ** formals = name
    **         | ()
    **         | (name ...)
    **         | (name ... . name)
    ** to-do
    **         | [name.0 expr.0 ... name.n expr.n] [as name expr.r]
    **         | [name.0 expr.0 ... name.n expr.n] [as name]
    **         | [name.0 expr.0 ... name.n expr.n]
    */

    /* notes:
    **    when using '[' ']'
    **    the expr.i needs to be evaluated in the current context to get the type (constraints)
    */

    pair_Create(args, env, result.pair);
    setConstructor(*result.reference, s_lambda);
}

extern SUBR(apply_lambda)
{
    /*
    ** args   = (<lambda> value...)
    ** lambda = (formals . body)
    ** formals = name
    **         | ()
    **         | (name ...)
    **         | (name ... . name)
    ** to-do
    **         | [name.0 type.0 ... name.n type.n] [as name type.r]
    **         | [name.0 type.0 ... name.n type.n] [as name]
    **         | [name.0 type.0 ... name.n type.n]
    */

    /* notes:
    **    when using '[' ']'
    **    the value of each expr.i is the type of the argument
    */

    GC_Begin(3);
    Node cenv, tmp;

    GC_Protect(cenv);
    GC_Protect(tmp);

    Node fun, arguments;

    pair_GetCar(args.pair, &fun); //(fun . arguments)
    pair_GetCdr(args.pair, &arguments);

    Node defn;

    pair_GetCar(fun.pair, &defn); // (formals . body)
    pair_GetCdr(fun.pair, &cenv); // retreve the closure enviroment

    Node formals, body;

    pair_GetCar(defn.pair, &formals);
    pair_GetCdr(defn.pair, &body);

    Node vars  = formals;
    Node vlist = arguments;

    // formals = ()
    //         | (name ...)
    //
    // bind parameters to values
    // extending the closure enviroment
    while (isPair(formals)) {
        Node var = NIL;
        Node val = NIL;

        if (!isPair(vlist)) {
            fprintf(stderr, "\nerror: too few arguments params: ");
            prettyPrint(stderr, vars);
            fprintf(stderr, " args: ");
            prettyPrint(stderr, arguments);
            fprintf(stderr, "\n");
            fflush(stderr);
            GC_End();
            fatal(0);
        }

        pair_GetCar(formals.pair, &var);
        pair_GetCar(vlist.pair,   &val);

        pair_GetCdr(formals.pair, &formals);
        pair_GetCdr(vlist.pair,   &vlist);

        pair_Create(var, val, &tmp.pair);
        pair_Create(tmp, cenv, &cenv.pair);
    }

    // formals = name
    //         | . name)
    //
    // bind (rest) parameter to remaining values
    // extending the closure enviroment
    if (isSymbol(formals)) {
        pair_Create(formals, vlist, &tmp.pair);
        pair_Create(tmp, cenv, &cenv.pair);
        vlist = NIL;
    }

    // check --
    if (!isNil(vlist)) {
        fprintf(stderr, "\nerror: too many arguments params: ");
        prettyPrint(stderr, vars);
        fprintf(stderr, " args: ");
        prettyPrint(stderr, arguments);
        fprintf(stderr, "\n");
        fflush(stderr);
        GC_End();
        fatal(0);
    }

    // process the body of the lambda
    // and return the last value
    eval_begin(body, cenv, result);

    GC_End();
}

extern SUBR(delay)
{
    Tuple tuple, expr;

    pair_GetCar(args.pair, &expr);

    tuple_Create(3, &tuple);
    tuple_SetItem(tuple, 0, NIL);
    tuple_SetItem(tuple, 1, expr);
    tuple_SetItem(tuple, 2, env);
    setConstructor(tuple, s_delay);

    ASSIGN(result, tuple);
}

extern SUBR(gensym)
{
    static unsigned long counter = 0;
    static char data[20];

    unsigned long current = counter;
    counter += 1;

    sprintf(data, "0x%.10lx", current);

    symbol_Convert(data, result.symbol);
}

extern SUBR(member)
{
    Node tst = NIL;
    Node lst = NIL;

    forceArgs(args, &tst, &lst, 0);

    while (isPair(lst)) {
        Node elm = NIL;

        pair_GetCar(lst.pair, &elm);

        if (node_Match(tst, elm)) {
            ASSIGN(result, true_v);
            break;
        }

        pair_GetCdr(lst.pair, &lst);
    }
}

extern SUBR(find)
{
    GC_Begin(5);
    GC_Add(args);

    Node tst;
    Node lst;
    Node check;

    GC_Protect(tst);
    GC_Protect(lst);
    GC_Protect(check);

    forceArgs(args, &tst, &lst, 0);

    while (isPair(lst)) {
        Node elm = NIL;

        pair_GetCar(lst.pair, &elm);
        pair_Create(elm, NIL, &(args.pair));

        check = NIL;
        apply(tst, args, env, &check);

        if (!isNil(check)) {
            ASSIGN(result,elm);
            break;
        }

        pair_GetCdr(lst.pair, &lst);
    }

    GC_End();
}

extern void call_with(Node function, Node value, Node env, Target target) {
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

extern SUBR(map) {
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

extern SUBR(eval_symbol)
{
    GC_Begin(2);
    Node symbol, tmp;
    Pair entry;

    GC_Protect(tmp);

    pair_GetCar(args.pair, &symbol);

    // lookup symbol in the current enviroment
    if (!alist_Entry(env.pair, symbol, &entry)) {
        if (!alist_Entry(enki_globals.pair,  symbol, &entry)) goto error;
    }

    pair_GetCdr(entry, &tmp);

    if (fromCtor(tmp, s_forced)) {
        tuple_GetItem(tmp.tuple, 0, &tmp);
        pair_SetCdr(entry, tmp);
    }

    ASSIGN(result, tmp);

    GC_End();
    return;

 error:
    GC_End();
    if (!isSymbol(symbol)) {
        fatal("undefined variable: <non-symbol>");
    } else {
        fatal("undefined variable: %s", symbol_Text(symbol.symbol));
    }
}

extern SUBR(eval_pair)
{
    GC_Begin(5);
    Node obj, head, tail, tmp;

    GC_Protect(obj);
    GC_Protect(head);
    GC_Protect(tail);
    GC_Protect(tmp);

    // (subr_eval_pair obj)
    pair_GetCar(args.pair, &obj);
    pair_GetCar(obj.pair, &head);
    pair_GetCdr(obj.pair, &tail);

    pushTrace(obj);

    // first eval the head
    eval(head, env, &head);

    if (fromCtor(head, s_box)) {
        pair_GetCar(head.pair, &head);
    }

    if (fromCtor(head, s_delay)) {
        Node dexpr, denv;
        tuple_GetItem(head.tuple, 1, &dexpr);
        tuple_GetItem(head.tuple, 2, &denv);
        eval(dexpr, denv, &tmp);
        tuple_SetItem(head.tuple, 0, tmp);
        setConstructor(head.tuple, s_forced);
        head = tmp;
    }

    if (fromCtor(head, s_fixed)) {
        // apply Fixed to un-evaluated arguments
        Node func = NIL;
        tuple_GetItem(head.tuple, fxd_eval, &func);
        apply(func, tail, env, result);
        goto done;
    }

    // evaluate the arguments
    list_Map(eval, tail.pair, env, &tail.pair);

    // now apply the head to the evaluated arguments
    apply(head, tail, env, result);

 done:
    popTrace();

    GC_End();
}

extern SUBR(apply_form)
{
    Node form, cargs, func;

    pair_GetCar(args.pair, &form);
    pair_GetCdr(args.pair, &cargs);

    pair_GetCar(form.pair, &func);

    apply(func, cargs, env, result);
}

extern SUBR(expand)
{
    GC_Begin(3);

    Node expr, cenv;

    GC_Protect(expr);
    GC_Protect(cenv);

    // (expand expr env)
    // (expand expr)
    list_GetItem(args.pair, 0, &expr);
    list_GetItem(args.pair, 1, &cenv);

    if (isNil(cenv)) cenv = env;

    expand(expr, cenv, result);

    GC_End();
}

extern SUBR(encode)
{
    GC_Begin(3);

    Node expr, cenv;

    GC_Protect(expr);
    GC_Protect(cenv);

    // (encode expr env)
    // (encode expr)
    list_GetItem(args.pair, 0, &expr);
    list_GetItem(args.pair, 1, &cenv);

    if (isNil(cenv)) cenv = env;

    encode(expr, cenv, result);

    GC_End();
}

extern SUBR(eval)
{
    GC_Begin(3);

    Node expr, cenv;

    GC_Protect(expr);
    GC_Protect(cenv);

    // (eval expr env)
    // (eval expr)
    list_GetItem(args.pair, 0, &expr);
    list_GetItem(args.pair, 1, &cenv);

    if (isNil(cenv)) cenv = env;

    eval(expr, cenv, result);

    GC_End();
}

extern SUBR(reduce)
{
    GC_Begin(3);

    Node expr, cenv;

    GC_Protect(expr);
    GC_Protect(cenv);

    // (reduce expr env)
    // (reduce expr)
    list_GetItem(args.pair, 0, &expr);
    list_GetItem(args.pair, 1, &cenv);

    if (isNil(cenv)) cenv = env;

    expand(expr, cenv, &expr);
    encode(expr, cenv, &expr);
    eval(expr, cenv, result);

    GC_End();
}

extern SUBR(apply)
{
    Node func  = NIL;
    Node cargs = NIL;
    Node cenv  = NIL;

    list_GetItem(args.pair, 0, &func);
    list_GetItem(args.pair, 1, &cargs);
    list_GetItem(args.pair, 2, &cenv);

    if (isNil(cenv)) cenv = env;

    apply(func, cargs, cenv, result);
}

extern SUBR(form)
{
  Tuple tuple; Node func = NIL;

  checkArgs(args, "form", 1, NIL);
  forceArgs(args, &func, 0);

  tuple_Create(1, &tuple);
  tuple_SetItem(tuple, 0, func);
  setConstructor(tuple, s_form);

  ASSIGN(result, (Node)tuple);
}

extern SUBR(fixed)
{
  Tuple tuple;
  Node  name = NIL;
  Node  func = NIL;
  Node  enc = NIL;

  int count = checkArgs(args, "fixed", 1, NIL);

  ASSIGN(result,NIL);

  if (2 < count) {
      forceArgs(args, &name, &func, &enc, 0);
      tuple_Create(3, &tuple);
      tuple_SetItem(tuple, fxd_name, name);
      tuple_SetItem(tuple, fxd_eval, func);
      tuple_SetItem(tuple, fxd_encode, enc);
  } else if (1 < count) {
      forceArgs(args, &name, &func, 0);
      tuple_Create(3, &tuple);
      tuple_SetItem(tuple, fxd_name, name);
      tuple_SetItem(tuple, fxd_eval, func);
      tuple_SetItem(tuple, fxd_encode, p_encode_args);
  } else {
      ASSIGN(result, NIL);
      return;
  }

  setConstructor(tuple, s_fixed);

  ASSIGN(result, (Node)tuple);
}

extern SUBR(ctor_of)
{
    Node value;

    checkArgs(args, "ctor-of", 1, NIL);
    fetchArgs(args, &value, 0);

    ASSIGN(result, getConstructor(value));
}

extern SUBR(type_of)
{
    Node value;

    checkArgs(args, "type-of", 1, NIL);
    fetchArgs(args, &value, 0);

    node_TypeOf(value, result);
}

extern SUBR(sort_of)
{
    Node value, type;

    checkArgs(args, "kind-of", 1, NIL);
    fetchArgs(args, &value, 0);

    node_TypeOf(value, &type);
    node_TypeOf(type, result);
}

extern SUBR(ctor_q)
{
    Node value, ctor;

    checkArgs(args, "ctor?", 2, NIL, NIL);
    fetchArgs(args, &value, &ctor, 0);

    ASSIGN(result,NIL);

    if (fromCtor(value, ctor)) {
        ASSIGN(result, true_v);
    }
}

extern SUBR(type_q)
{
    Node value, type;
    checkArgs(args, "type?", 2, NIL, NIL);
    fetchArgs(args, &value, &type, 0);

    ASSIGN(result,NIL);

    if (inType(value, type)) {
        ASSIGN(result, true_v);
    }
}

extern SUBR(sort_q)
{
    Node value, type, sort;

    checkArgs(args, "sort?", 2, NIL, NIL);
    fetchArgs(args, &value, &sort, 0);

    node_TypeOf(value, &type);

    ASSIGN(result,NIL);

    if (inSort(type, sort)) {
        ASSIGN(result, true_v);
    }
}

extern SUBR(com) {
    Node val = NIL;
    checkArgs(args, "~", 1, s_integer);
    forceArgs(args, &val, 0);
    integer_Create(~(val.integer->value), result.integer);
}

/*
 macro magic
   1. define a defining macro (_do_binary) terms of an action macro (_do)
   2. define the action macro
   3. call the defining macro (to do the action)
   4. un-define the action macro
 later we can re-use the defining macro to add definitions to Enki
*/
#define _do_binary() \
    _do(sub,  -) _do(add, +) _do(mul, *)  _do(div, /)  _do(mod,  %) \
    _do(bitand, &) _do(bitor, |) _do(bitxor, ^) _do(shl, <<)  _do(shr, >>)

#define _do(NAME, OP) \
extern SUBR(NAME) \
{ \
    Node left; Node right; \
    checkArgs(args, #OP, 2, s_integer, s_integer); \
    forceArgs(args, &left, &right, 0); \
    integer_Create((left.integer->value) OP (right.integer->value), result.integer); \
}

_do_binary()

#undef _do

#define _do_relation() \
    _do(lt, <) _do(le, <=) _do(ge, >=) _do(gt, >)

#define _do(NAME, OP) \
extern SUBR(NAME) \
{ \
    Node left; Node right; \
    checkArgs(args, #OP, 2, s_integer, s_integer); \
    forceArgs(args, &left, &right, 0); \
    if ((left.integer->value) OP (right.integer->value)) { \
        ASSIGN(result, true_v);                            \
    } else { \
        ASSIGN(result, NIL); \
    } \
}

_do_relation()

#undef _do

extern SUBR(eq)
{
    Node left; Node right;
    checkArgs(args, "==", 2, NIL, NIL);
    forceArgs(args, &left, &right, 0);

    if (node_Match(left,right)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
extern SUBR(neq)
{
    Node left; Node right;
    checkArgs(args, "!=", 2, NIL, NIL);
    forceArgs(args, &left, &right, 0);

    if (node_Match(left,right)) {
        ASSIGN(result, NIL);
    } else {
        ASSIGN(result, true_v);
    }
}

extern SUBR(iso)
{
    Node depth; Node left; Node right;
    checkArgs(args, "iso", 3, s_integer, NIL, NIL);
    forceArgs(args, &depth, &left, &right, 0);

    if (node_Iso(depth.integer->value, left,right)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(assert)
{
    Node test; Node message;

    unsigned count = checkArgs(args, "assert", 1, NIL);

    fetchArgs(args, &test, &message, 0);

    if (!isNil(test)) {
        ASSIGN(result, true_v);
        return;
    }

    if (1 < count) {
        fprintf(stderr, "assert: ");
        print(stderr, message);
    } else {
        fprintf(stderr, "assert falure");
    }
    fprintf(stderr, "\n");
    fflush(stderr);
    fatal(0);
}

extern SUBR(exit)
{
    Node value = NIL;
    forceArgs(args, &value, 0);

    if (isInteger(value)) {
        exit(value.integer->value);
    } else {
        exit(0);
    }
}

extern SUBR(abort)
{
    fatal("aborted");
}

extern SUBR(environment)
{
    Node value = NIL;
    forceArgs(args, &value, 0);

    if (isIdentical(value, s_current)) {
         ASSIGN(result, env);
    } else {
        pair_GetCdr(enki_globals.pair, result);
    }
}

extern SUBR(dump)
{
    Node value = NIL;

    if (isPair(args)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        prettyPrint(stdout, value);
    }

    while (isPair(args)) {
        printf(" ");
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        prettyPrint(stdout, value);
    }

    fflush(stdout);
    ASSIGN(result,NIL);
}

extern SUBR(dumpln)
{
    Node value = NIL;

    if (isPair(args)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        prettyPrint(stdout, value);
    }

    while (isPair(args)) {
        printf(" ");
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        prettyPrint(stdout, value);
    }

    printf("\n");
    fflush(stdout);
    ASSIGN(result,NIL);
}

extern SUBR(print)
{
    Node value = NIL;

    while (isPair(args)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        print(stdout, value);
    }

    fflush(stdout);
    ASSIGN(result,NIL);
}

extern SUBR(println)
{
    Node value = NIL;

    while (isPair(args)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        print(stdout, value);
    }

    printf("\n");
    fflush(stdout);
    ASSIGN(result,NIL);
}

extern SUBR(debug)
{
    Node value = NIL;
    long level = 1;
    if (isPair(args)) {
        pair_GetCar(args.pair, &value);
        if (isInteger(value)) {
            level = value.integer->value;
            pair_GetCdr(args.pair, &args);
        }
    }
    if (ea_global_debug <= level) {
        while (isPair(args)) {
            pair_GetCar(args.pair, &value);
            pair_GetCdr(args.pair, &args);
            print(stderr, value);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    ASSIGN(result,NIL);
}

extern SUBR(level)
{
    Node value = NIL;

    integer_Create(ea_global_debug, &value.integer);

    if (isPair(args)) {
        Node nvalue;
        pair_GetCar(args.pair, &nvalue);
        if (isInteger(nvalue)) {
            ea_global_debug = nvalue.integer->value;
        }
    }

    ASSIGN(result,value);
}

extern SUBR(element) {
    Node tuple, index, value;
    int count = checkArgs(args, "element", 2, NIL, s_integer);

    ASSIGN(result,NIL);

    if (2 < count) {
        forceArgs(args, &tuple, &index, &value, 0);
        tuple_SetItem(tuple.tuple, index.integer->value, value);
        ASSIGN(result,tuple);
    } else {
        forceArgs(args, &tuple, &index, 0);
        tuple_GetItem(tuple.tuple, index.integer->value, result);
    }
}

extern SUBR(cons) {
    Node car; Node cdr;

    checkArgs(args, "cons", 2, NIL, NIL);
    fetchArgs(args, &car, &cdr, 0);

    pair_Create(car, cdr, result.pair);
}

extern SUBR(list) {
    ASSIGN(result, args);
}

extern SUBR(tuple) {
    unsigned size = 0;
    bool     dotted = false;

    list_State(args.pair, &size, &dotted);

    if (dotted) fatal("dotted list in: tuple");

    if (1 > size) {
        ASSIGN(result, NIL);
        return;
    }

    GC_Begin(2);
    Node tuple;
    GC_Protect(tuple);

    tuple_Create(size, &tuple.tuple);
    tuple_Fill(tuple.tuple, args.pair);
    ASSIGN(result, tuple);

    GC_End();
}

extern SUBR(ctor) {
    unsigned size = 0;
    bool     dotted = false;

    list_State(args.pair, &size, &dotted);

    if (dotted)   fatal("ctor: with a dotted list");
    if (1 > size) fatal("ctor: no label");
    if (2 > size) fatal("ctor: no type");
    if (3 > size) fatal("ctor: no initial values");

    Node label;
    Node type;

    pair_GetCar(args.pair, &label);

    if (!isSymbol(label)) fatal("ctor: label must be a symbol");

    pair_GetCdr(args.pair, &args);

    pair_GetCar(args.pair, &type);

    if (!isAType(type)) fatal("ctor: type must be a type");

    pair_GetCdr(args.pair, &args);

    GC_Begin(2);

    Node tuple;

    GC_Protect(tuple);

    tuple_Create(size - 2, &tuple.tuple);
    tuple_Fill(tuple.tuple, args.pair);

    setType(tuple, type);
    setConstructor(tuple, label.symbol);

    ASSIGN(result, tuple);

    GC_End();
}

extern SUBR(force) {
    Node value = NIL;
    checkArgs(args, "force", 1, NIL);
    forceArgs(args, &value, 0);
    ASSIGN(result, value);
}

extern SUBR(concat_text) {
    static TextBuffer buffer = BUFFER_INITIALISER;
    static char data[20];
    buffer_reset(&buffer);

    checkArgs(args, "concat-text", 2, s_text, s_text);

    while (isPair(args)) {
        Node text;

        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (isText(text)) {
            buffer_add(&buffer, text_Text(text.text));
            continue;
        }

        if (isSymbol(text)) {
            buffer_add(&buffer, symbol_Text(text.symbol));
            continue;
        }

        if (isInteger(text)) {
            long value = text.integer->value;
            sprintf(data, "%ld", value);
            buffer_add(&buffer, data);
            continue;
        }

        fatal("wrong type of argument to: %s", "concat-text");
    }

    text_Create(buffer, result.text);
}

extern SUBR(concat_symbol) {
    static TextBuffer buffer = BUFFER_INITIALISER;
    static char data[20];
    buffer_reset(&buffer);

    checkArgs(args, "concat-symbol", 2, NIL, NIL);

    while (isPair(args)) {
        Node text;

        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (isText(text)) {
            buffer_add(&buffer, text_Text(text.text));
            continue;
        }

        if (isSymbol(text)) {
            buffer_add(&buffer, symbol_Text(text.symbol));
            continue;
        }

        if (isInteger(text)) {
            long value = text.integer->value;
            sprintf(data, "%ld", value);
            buffer_add(&buffer, data);
            continue;
        }

        fatal("wrong type of argument to: %s", "concat-symbol");
    }

    symbol_Create(buffer, result.symbol);
}

static clock_t cstart;
static clock_t csegment;
extern SUBR(start_time) {
    clock_t current = clock();
    cstart   = current;
    csegment = current;
}

extern SUBR(mark_time) {
    clock_t current = clock();

    Node value = NIL;
    bool prefix = false;

    while (isPair(args)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        if (isSymbol(value)) {
            print(stderr, value);
            prefix = true;
            continue;
        }
        if (isText(value)) {
            print(stderr, value);
            prefix = true;
            continue;
        }
        if (isInteger(value)) {
            print(stderr, value);
            prefix = true;
            continue;
        }
    }

    if (prefix) {
        fprintf(stderr, " ");
    }

    fprintf(stderr, "(%.3f %.3f) cpu sec\n",
            ((double)current - (double)csegment) * 1.0e-6,
            ((double)current - (double)cstart) * 1.0e-6);
    fflush(stderr);

    csegment = clock();
}

extern SUBR(system) {
    Node command = NIL;
    checkArgs(args, "system", 1, s_text);
    forceArgs(args, &command, 0);

    int state = system(text_Text(command.text));

    if (WIFEXITED(state)) {
        int status = WEXITSTATUS(state);
        integer_Create(status, result.integer);
    } else {
        if (!WIFSIGNALED(state)) {
            ASSIGN(result, NIL);
        } else {
            int signo = (WTERMSIG(state) * -1);
            integer_Create(signo, result.integer);
        }
    }
}

extern SUBR(error) {
    Node kind    = NIL;
    Node message = NIL;

    checkArgs(args, "error", 1, s_symbol, s_text);
    forceArgs(args, &kind, &message, 0);

    print(stderr, kind);
    fatal("%s", text_Text(message.text));
}

extern void formatAppendTo(TextBuffer *buffer, Node value) {
    char data[20];

    if (isNil(value)) return;

    if (isSymbol(value)) {
        buffer_add(buffer, symbol_Text(value.symbol));
        return;
    }

    if (isText(value)) {
        buffer_add(buffer, text_Text(value.text));
        return;
    }

    if (isInteger(value)) {
        sprintf(data, "%lld", value.integer->value);
        buffer_add(buffer, data);
        return;
    }
}

extern SUBR(format) {
    static TextBuffer buffer = BUFFER_INITIALISER;

    buffer_reset(&buffer);

    const char* format;

    checkArgs(args, "format", 1, s_text);

    if (!isPair(args)) {
        fatal("missing first argument to format\n");
    } else {
        Node form = NIL;
        pair_GetCar(args.pair, &form);
        pair_GetCdr(args.pair, &args);
        if (!isText(form)) {
            fatal("first argument to format is not text\n");
        }
        format = text_Text(form.text);
    }

    unsigned count = 0;

    for (; *format ; ++format) {
        char at = format[0];

        if ('@' == at) {
            if (('{' == format[1]) && ('}' == format[3])) {
                char code = format[2];
                format += 3;

                ++count;

                if (!isPair(args)) {
                    fatal("missing argument %d to format code \'%c\' \n", count, code);
                } else {
                    Node value = NIL;

                    pair_GetCar(args.pair, &value);
                    pair_GetCdr(args.pair, &args);

                    if ('q' == code) {
                        buffer_append(&buffer, '"');
                        formatAppendTo(&buffer, value);
                        buffer_append(&buffer, '"');
                        continue;
                    }

                    if ('s' == code) {
                        formatAppendTo(&buffer, value);
                        continue;
                    }

                    if ('o' == code) {
                        buffer_print(&buffer, value);
                    }

                    if ('d' == code) {
                        buffer_dump(&buffer, value);
                    }

                    if ('f' == code) {
                        buffer_prettyPrint(&buffer, value);
                    }

                    if ('p' == code) {
                        char data[20];
                        sprintf(data, "%p", value.reference);
                        buffer_add(&buffer, data);
                        continue;
                    }

                    if ('c' == code) {
                        char data[20];
                        char chr;
                        if (!isInteger(value)) {
                            fatal("argument to format code \'c\' is not integer\n");
                        }
                        chr = (char)(0xff & value.integer->value);
                        sprintf(data, "%c", chr);
                        buffer_add(&buffer, data);
                        continue;
                    }
                }
                continue;
            }
        }

        buffer_append(&buffer, at);
    }

    text_Create(buffer, result.text);
}

static ID_set loaded_inodes = SET_INITIALISER;

extern SUBR(require) {
    Node path;

    unsigned xxx = __alloc_cycle;
    unsigned yyy = __scan_cycle;

    __alloc_cycle = 10000;
    __scan_cycle  = 1;

    checkArgs(args, "require", 1, s_text);
    forceArgs(args, &path, 0);

    FILE* file = fopen(text_Text(path.text), "r");

    if (!file) {
        fatal("failed to open \'%s\' for require", text_Text(path.text));
        return;
    }

    struct stat stbuf;
    int filedes = fileno(file);

    if (fstat(filedes, &stbuf) == -1) {
        fatal("failed retrieve fstat of \'%s\' for require", text_Text(path.text));
        return;
    }

    long long id = stbuf.st_ino;

#if 0

    readFile(file);
    fclose(file);

#else

    switch (set_add(&loaded_inodes, id)) {
    case -1:
        fatal("loaded_inodes not init-ed for require");
        break;

    case 1: // added
        readFile(file);
        fclose(file);

    default: // found
        break;
    }

#endif

    integer_Create(id, result.integer);

    __alloc_cycle = xxx;
    __scan_cycle  = yyy;
}

struct os_file {
    FILE* file;
};

typedef struct os_file* OSFile;

extern SUBR(open_in) {
    Node path;
    checkArgs(args, "open-in", 1, s_text);
    forceArgs(args, &path, 0);

    FILE* file = fopen(text_Text(path.text), "r");

    if (!file) {
        fatal("failed to open \'%s\' for reading", text_Text(path.text));
        return;
    }

    Reference infile = 0;

    if (!opaque_Create(t_infile, s_opaque, sizeof(struct os_file), &infile)) {
        fatal("failed to allocate opaque object");
    }

    ((OSFile)(infile))->file = file;

    ASSIGN(result, infile);
}

extern SUBR(open_out) {
    Node path;
    checkArgs(args, "open-out", 1, s_text);
    forceArgs(args, &path, 0);

    FILE* file = fopen(text_Text(path.text), "w");

    if (!file) {
        fatal("failed to open \'%s\' for writing", text_Text(path.text));
        return;
    }

    Reference outfile = 0;

    if (!opaque_Create(t_outfile, s_opaque, sizeof(struct os_file), &outfile)) {
        fatal("failed to allocate opaque object");
    }

    ((OSFile)(outfile))->file = file;

    ASSIGN(result, outfile);
}

extern SUBR(close_in) {
    Node file;
    checkArgs(args, "close-in", 1, t_infile);
    forceArgs(args, &file, 0);

    if (!inType(file, t_infile)) {
        fatal("close-in: not an infile");
    }

    FILE* in = ((OSFile)(file.reference))->file;

    if (fclose(in)) {
        fatal("close-in: error closing os-file");
    }

    ((OSFile)(file.reference))->file = 0;

    setType(file, s_opaque);
}

extern SUBR(close_out) {
    Node file;
    checkArgs(args, "close-out", 1, t_outfile);
    forceArgs(args, &file, 0);

    if (!inType(file, t_outfile)) {
        fatal("close-out: not an outfile");
    }

    FILE* out = ((OSFile)(file.reference))->file;

    if (fclose(out)) {
        fatal("close-out: error closing os-file");
    }

    ((OSFile)(file.reference))->file = 0;

    setType(file, s_opaque);
}

extern SUBR(fprint) {
    FILE* out = 0;

    checkArgs(args, "fprint", 1, t_outfile);

    if (!isPair(args)) {
         fatal("missing first argument to fprint\n");
    } else {
        Node outfile = NIL;

        pair_GetCar(args.pair, &outfile);
        pair_GetCdr(args.pair, &args);

        if (!inType(outfile, t_outfile)) {
            fatal("first argument to fprint is not an outfile\n");
        }

        out = ((OSFile)(outfile.reference))->file;
    }

    while (isPair(args)) {
        Node text;
        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (!isText(text)) {
            fatal("invalid argument to fprint\n");
        }

        fprintf(out, "%s", text_Text(text.text));
    }
}

extern SUBR(read_line) {
    static TextBuffer buffer = BUFFER_INITIALISER;

    Node file;

    checkArgs(args, "read-line", 1, t_infile);
    forceArgs(args, &file, 0);

    if (!inType(file, t_infile)) {
        fatal("read-line: not an infile");
    }

    FILE* in = ((OSFile)(file.reference))->file;

    if (feof(in)) {
        ASSIGN(result, NIL);
        return;
    }

    buffer_reset(&buffer);

    size_t  len  = buffer.size;
    ssize_t read = getdelim(&(buffer.buffer), &len, '\n', in);

    buffer.size = len;

    if (0 > read) {
        ASSIGN(result, NIL);
        return;
    }

    buffer.position = read;

    text_Create(buffer, result.text);
}

extern SUBR(eof_in) {
    Node file;

    checkArgs(args, "eof_in", 1, t_infile);
    forceArgs(args, &file, 0);

    if (!inType(file, t_infile)) {
        fatal("read-line: not an infile");
    }

    FILE* in = ((OSFile)(file.reference))->file;

    if (feof(in)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(read_sexpr) {
    static TextBuffer buffer = BUFFER_INITIALISER;
    buffer_reset(&buffer);

    Node file;

    checkArgs(args, "read-sexpr", 1, t_infile);
    forceArgs(args, &file, 0);

    if (!inType(file, t_infile)) {
        fatal("read-sexpr: not an infile");
    }

    FILE* in = ((OSFile)(file.reference))->file;

    if (feof(in)) {
        ASSIGN(result, NIL);
        return;
    }

    readExpr(in, result);
}

extern SUBR(inode) {
    struct stat stbuf;
    Node fname;

    checkArgs(args, "inode", 1, s_text);
    forceArgs(args, &fname, 0);

    if (!isText(fname)) {
        fatal("inode: not text");
    }

    const char *name = text_Text(fname.text);

    int filedes = open(name, O_RDONLY|O_DIRECT);

    if (-1 == filedes) {
        ASSIGN(result, NIL);
    }

    if (fstat(filedes, &stbuf) == -1) {
        ASSIGN(result, NIL);
    }

    close(filedes);

    integer_Create(stbuf.st_ino, result.integer);
}

// size of pointer (in bytes)
extern SUBR(sizeof) {
    Node kind;
    checkArgs(args, "sizeof", 1, s_symbol);
    forceArgs(args, &kind, 0);

    ASSIGN(result, NIL);

    if (isIdentical(kind, s_pointer)) {
        integer_Create(POINTER_SIZE, result.integer);
    }

    if (isIdentical(kind, s_word)) {
        integer_Create(WORD_SIZE, result.integer);
    }

    if (isIdentical(kind, s_clink)) {
        integer_Create(sizeof(struct gc_clink), result.integer);
    }

    if (isIdentical(kind, s_header)) {
        integer_Create(sizeof(struct gc_header), result.integer);
    }

    if (isIdentical(kind, s_kind)) {
        integer_Create(sizeof(struct gc_kind), result.integer);
    }

    if (isIdentical(kind, s_node)) {
        integer_Create(sizeof(Node), result.integer);
    }

    if (isIdentical(kind, s_target)) {
        integer_Create(sizeof(Target), result.integer);
    }

    if (isIdentical(kind, s_unsigned)) {
        integer_Create(sizeof(unsigned), result.integer);
    }

    if (isIdentical(kind, s_integer)) {
        integer_Create(sizeof(struct integer), result.integer);
    }

    if (isIdentical(kind, s_pair)) {
        integer_Create(sizeof(struct pair), result.integer);
    }

    if (isIdentical(kind, s_hashcode)) {
        integer_Create(sizeof(HashCode), result.integer);
    }

    if (isIdentical(kind, s_text)) {
        integer_Create(sizeof(struct text), result.integer);
    }

    if (isIdentical(kind, s_tuple)) {
        integer_Create(sizeof(struct tuple), result.integer);
    }
}

extern SUBR(car) {
    Pair pair;
    unsigned count = checkArgs(args, "car", 1, s_pair);

    ASSIGN(result,NIL);

    if (1 < count) {
        Node value;
        forceArgs(args, &pair, &value, 0);
        pair_SetCar(pair, value);
        ASSIGN(result,pair);
    } else {
        forceArgs(args, &pair, 0);
        pair_GetCar(pair, result);
    }
}

extern SUBR(cdr) {
    Pair pair;
    unsigned count = checkArgs(args, "cdr", 1, s_pair);

    ASSIGN(result,NIL);

    if (1 < count) {
        Node value;
        forceArgs(args, &pair, &value, 0);
        pair_SetCdr(pair, value);
        ASSIGN(result,pair);
    } else {
        forceArgs(args, &pair, 0);
        pair_GetCdr(pair, result);
    }
}

extern SUBR(pair_q) {
    Node value;
    checkArgs(args, "pair?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isPair(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(tuple_q) {
    Node value;
    checkArgs(args, "tuple?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isTuple(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(text_q) {
    Node value;
    checkArgs(args, "text?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isText(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(integer_q) {
    Node value;
    checkArgs(args, "integer?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isInteger(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(quote_q) {
    Node value;
    checkArgs(args, "quote?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isQuote(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(block_q) {
    Node value;
    checkArgs(args, "block?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isBlock(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(gc_scan) {
    Node value;

    checkArgs(args, "gc-scan", 1, s_integer);
    forceArgs(args, &value, 0);

    unsigned int count = value.integer->value;

    space_Scan(_zero_space, count);

    if (!space_CanFlip(_zero_space)) {
        ASSIGN(result, NIL);
    } else {
        space_Flip(_zero_space);
        ASSIGN(result, true_v);
    }
}

extern SUBR(set_end) {
    Node head;
    Node tail;
    checkArgs(args, "set-end", 1, s_pair, NIL);
    forceArgs(args, &head, &tail, 0);

    if (!isPair(head)) {
        ASSIGN(result, NIL);
    }

    list_SetEnd(head.pair, tail);
    ASSIGN(result, true_v);
}

extern SUBR(the) {
    Node type;
    Node value;

    checkArgs(args, "the", 2, NIL, NIL);
    fetchArgs(args, &type, &value, 0);

    if (!inType(value, type)) {
        fprintf(stderr, "the : ");
        print(stderr, value);
        fprintf(stderr, " :is not a: ");
        print(stderr, type);
        fprintf(stderr, "\n");
        fflush(stderr);
        fatal(0);
    }

    ASSIGN(result, value);
}

extern SUBR(nil_q) {
    Node value;

    checkArgs(args, "nil?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isNil(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(not) {
    Node value;

    checkArgs(args, "not", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isNil(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(open_buffer) {
    Reference buffer = 0;

    if (!opaque_Create(t_buffer, s_opaque, sizeof(struct text_buffer), &buffer)) {
        fatal("failed to allocate opaque object");
    }

    buffer_init((TextBuffer *)buffer);

    ASSIGN(result, buffer);
}

extern SUBR(bprint) {
    TextBuffer *out = 0;

    checkArgs(args, "bprint", 1, t_buffer);

    if (!isPair(args)) {
        fatal("missing first argument to bprint\n");
    } else {
        Node buffer = NIL;

        pair_GetCar(args.pair, &buffer);
        pair_GetCdr(args.pair, &args);

        if (!inType(buffer, t_buffer)) {
            fatal("first argument to bprint is not an buffer\n");
        }

        out = ((TextBuffer *)(buffer.reference));
    }

    while (isPair(args)) {
        Node text;
        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (!isText(text)) {
            fatal("invalid argument to bprint\n");
        }

        buffer_add(out, text_Text(text.text));
    }
}

extern SUBR(close_buffer) {
    Node buffer;
    checkArgs(args, "close-buffer", 1, t_buffer);
    forceArgs(args, &buffer, 0);

    if (!inType(buffer, t_buffer)) {
        fatal("close-buffer: not an buffer");
    }

    TextBuffer *buff = ((TextBuffer *)(buffer.reference));

    text_Create(*(buff), result.text);

    buffer_free(buff);

    setType(buffer, s_opaque);
}

extern SUBR(forced_q) {
    Node value;
    checkArgs(args, "forced?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (fromCtor(value, s_forced)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(symbol_q) {
    Node value;
    checkArgs(args, "symbol?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isSymbol(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(alloc_cycle) {
    Node value;
    unsigned count = checkArgs(args, "alloc-cycle", 0, NIL);

    integer_Create(__alloc_cycle, result.integer);

    if (0 >= count) return;

    pair_GetCar(args.pair, &value);

    if (!isInteger(value)) return;

    if (0 > value.integer->value) return;

    __alloc_cycle = value.integer->value;
}

extern SUBR(scan_cycle) {
    Node value;
    unsigned count = checkArgs(args, "scan-cycle", 0, NIL);

    integer_Create(__scan_cycle, result.integer);

    if (0 >= count) return;

    pair_GetCar(args.pair, &value);

    if (!isInteger(value)) return;

    if (0 > value.integer->value) return;

    __scan_cycle = value.integer->value;
}

extern SUBR(length) {
    Node list;

    forceArgs(args, &list, 0);

    ASSIGN(result, NIL);

    if (isNil(list)) {
        integer_Create(0, result.integer);
        return;
    }

    if (isPair(list)) {
        unsigned count  = 0;
        bool     dotted = false;
        list_State(list.pair, &count, &dotted);
        integer_Create(count, result.integer);
        return;
    }
}

extern SUBR(fixed_encoder) {
    Tuple fixed;

    checkArgs(args, "fixed-encoder", 1, s_fixed);
    fetchArgs(args, &fixed, 0);

    if (!tuple_GetItem(fixed, fxd_encode, result)) {
        ASSIGN(result, NIL);
    }
}

extern SUBR(fixed_evaluator) {
    Tuple fixed;

    checkArgs(args, "fixed-evaluator", 1, s_fixed);
    fetchArgs(args, &fixed, 0);

    if (!tuple_GetItem(fixed, fxd_eval, result)) {
        ASSIGN(result, NIL);
    }
}

extern SUBR(form_action) {
    Tuple form;

    checkArgs(args, "form-action", 1, s_form);
    fetchArgs(args, &form, 0);

    if (!tuple_GetItem(form, 0, result)) {
        ASSIGN(result, NIL);
    }
}

extern SUBR(forced_value) {
    Tuple forced;

    checkArgs(args, "forced-value", 1, s_forced);
    fetchArgs(args, &forced, 0);

    if (!tuple_GetItem(forced, 0, result)) {
        ASSIGN(result, NIL);
    }
}

extern SUBR(fixed_q) {
    Node value;
    checkArgs(args, "fixed?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (fromCtor(value, s_fixed)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(primitive_q) {
    Node value;
    checkArgs(args, "primitive?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (fromCtor(value, s_primitive)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(delay_q) {
    Node value;
    checkArgs(args, "delay?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (fromCtor(value, s_delay)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(lambda_q) {
    Node value;
    checkArgs(args, "lambda?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (fromCtor(value, s_lambda)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(form_q) {
    Node value;
    checkArgs(args, "form?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isForm(value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(all_q) {
    Node list;
    Node type;

    checkArgs(args, "all?", 1, s_pair, NIL);
    forceArgs(args, &list, &type, 0);

    while (isPair(list)) {
        Node value = NIL;

        pair_GetCar(list.pair, &value);
        pair_GetCdr(list.pair, &list);

        if (inType(value, type)) continue;

        ASSIGN(result, NIL);
        return;
    }

    ASSIGN(result, true_v);
}

extern SUBR(dot_q) {
    Node list;

    checkArgs(args, "dot?", 1, s_pair, NIL);
    forceArgs(args, &list, 0);

    ASSIGN(result, NIL);

    if (isPair(list)) {
        unsigned count  = 0;
        bool     dotted = false;
        list_State(list.pair, &count, &dotted);
        if (dotted) {
            ASSIGN(result, true_v);
        }
    }
}

extern SUBR(void_q) {
    Node value;

    checkArgs(args, "void?", 1, NIL);
    forceArgs(args, &value, 0);

    if (!isIdentical(value, void_v)) {
        ASSIGN(result, NIL);
    } else {
        ASSIGN(result, true_v);
    }
}

extern SUBR(true_q) {
    Node value;

    checkArgs(args, "true?", 1, NIL);
    forceArgs(args, &value, 0);

    if (!isIdentical(value, true_v)) {
        ASSIGN(result, NIL);
    } else {
        ASSIGN(result, true_v);
    }
}

extern SUBR(Any) {
    Node left;
    Node right;

    checkArgs(args, "Any", 2, NIL, NIL);
    fetchArgs(args, &left, &right, 0);

    if (!isAType(left))  fatal("Any: left is not a type");
    if (!isAType(right)) fatal("Any: right is not a type");

    type_Any(left.type, right.type, result.type);
}

extern SUBR(Field) {
    Node label;
    Node type;

    checkArgs(args, "Field", 2, NIL, NIL);
    fetchArgs(args, &label, &type, 0);

    if (!isSymbol(label)) fatal("Field: no label");
    if (!isAType(type)) fatal("Field: no type");

    type_Label(label.symbol, type.type, result.type);
}

extern SUBR(Index) {
    Node offset;
    Node type;

    checkArgs(args, "Index", 2, NIL, NIL);
    fetchArgs(args, &offset, &type, 0);


    if (!isInteger(offset)) fatal("Index: no offset");
    if (!isAType(type)) fatal("Index: no type");

    if (offset.integer->value < 0) fatal("Index: offset is lessthen zero");

    type_Index(offset.integer->value, type.type, result.type);
}

extern SUBR(Tuple) {
    Node left;
    Node right;

    checkArgs(args, "Tuple", 2, NIL, NIL);
    fetchArgs(args, &left, &right, 0);

    if (!isAType(left))  fatal("Tuple: left is not a type");
    if (!isAType(right)) fatal("Tuple: right is not a type");

    type_Tuple(left.type, right.type, result.type);
}

extern SUBR(Record) {
    Node left;
    Node right;

    checkArgs(args, "Record", 2, NIL, NIL);
    fetchArgs(args, &left, &right, 0);

    if (!isAType(left))  fatal("Record: left is not a type");
    if (!isAType(right)) fatal("Record: right is not a type");

    type_Record(left.type, right.type, result.type);
}

extern SUBR(All) {
    Node left;
    Node right;

    checkArgs(args, "All", 2, NIL, NIL);
    fetchArgs(args, &left, &right, 0);

    if (!isAType(left))  fatal("All: left is not a type");
    if (!isAType(right)) fatal("All: right is not a type");

    type_All(left.type, right.type, result.type);
}

static void mirror(Node local, Node env, Target result) {
    ASSIGN(result, local);
}

extern SUBR(Unfold) {
    Node type;

    checkArgs(args, "Unfold", 1, NIL);
    fetchArgs(args, &type, 0);

    type_Map(mirror, type, env, result);
}

extern SUBR(cast) {
    Node type;
    Node left;
    Node right;

    checkArgs(args, "cast", 2, NIL, NIL);
    fetchArgs(args, &left, &right, 0);

    node_TypeOf(right, &type);

    if (node_Iso(5, left, type)) {
        ASSIGN(result,right);
    } else {
        fprintf(stderr, "cast failure:\n target=");
        print(stderr, left);
        fprintf(stderr, "\n value=");
        prettyPrint(stderr, right);
        fprintf(stderr, "\n typeof=");
        print(stderr, type);
        fatal("\n");
    }
}

/***************************************************************
 ***************************************************************
 ***************************************************************
 ***************************************************************/

static void defineConstant(const char* name, const Node value) {
    GC_Begin(8);

    Symbol label;

    GC_Protect(label);

    symbol_Convert(name, &label);

    defineValue(label, value);

    GC_End();
}

static Primitive definePrimitive(const char* name, Operator func) {
    GC_Begin(8);

    Symbol label; Primitive prim;

    GC_Protect(label);
    GC_Protect(prim);

    symbol_Convert(name, &label);
    primitive_Create(label, func, &prim);

    defineValue(label, prim);

    GC_End();

    return prim;
}

static Node defineFixed(const char* neval,  Operator oeval,
                        const char* nencode, Operator oencode)
{
    GC_Begin(8);

    Tuple fixed; Symbol label; Primitive prim;

    GC_Protect(fixed);
    GC_Protect(label);
    GC_Protect(prim);

    tuple_Create(3, &fixed);
    setConstructor(fixed, s_fixed);

    if (nencode) {
        symbol_Convert(nencode, &label);
        primitive_Create(label, oencode, &prim);
        tuple_SetItem(fixed, fxd_encode, prim);
    }

    symbol_Convert(neval, &label);
    tuple_SetItem(fixed, fxd_name, label);

    primitive_Create(label, oeval, &prim);
    tuple_SetItem(fixed, fxd_eval, prim);

    defineValue(label, fixed);

    GC_End();

    return (Node) fixed;
}

#define MK_CONST(x,y) defineConstant(#x, y)
#define MK_BTYPE(x)   defineConstant(#x, t_ ##x)
#define MK_PRM(x)     definePrimitive(#x, opr_ ## x)
#define MK_FXD(x,y)   defineFixed(#x, opr_ ## x, #y, opr_ ## y)
#define MK_OPR(x,y)   definePrimitive(#x, opr_ ## y)


void startEnkiLibrary() {
    if (__initialized) return;

    if (sizeof(void*) != sizeof(Node)) {
        fatal("sizeof(void*) != sizeof(Node)");
    }

    if (sizeof(void*) != sizeof(long)) {
        fatal("sizeof(void*) != sizeof(long)");
    }

    VM_DEBUG(1, "startEnkiLibrary begin");

    clock_t cbegin = clock();

    cstart = cbegin;

    set_init(&loaded_inodes, SET_COLUMNS);

    _zero_space   = &enki_zero_space;
    __alloc_cycle = 1000;
    __scan_cycle  = 10;

    space_Init(_zero_space);

    VM_DEBUG(1, "startEnkiLibrary enki_globals %p", &enki_globals);

    clink_Manage(&(enki_zero_space.start_clinks), &enki_globals, true);

    VM_DEBUG(1, "startEnkiLibrary init symbol table");

    init_global_symboltable();

    VM_DEBUG(1, "startEnkiLibrary init type table");
    init_global_typetable();

    true_v  = (Node)init_atom(&enki_true, 0);
    false_v = (Node)init_atom(&enki_false, 0);
    unit_v  = (Node)init_atom(&enki_unit, 0);
    void_v  = (Node)init_atom(&enki_void, 0);

    setType(true_v,  boolean_s);
    setType(false_v, boolean_s);
    setType(unit_v,  unit_s);
    setType(void_v,  undefined_s);

    pair_Create(NIL,NIL, &enki_globals.pair);

    VM_DEBUG(1, "startEnkiLibrary init globals");

    MK_CONST(nil,NIL);
    MK_CONST(void,void_v);
    MK_CONST(unit,unit_v);
    MK_CONST(false,false_v);
    MK_CONST(true,true_v);


    MK_CONST(integer,t_integer);
    MK_CONST(pair,t_pair);
    MK_CONST(symbol,t_symbol);
    MK_CONST(text,t_text);
    MK_CONST(tuple,t_tuple);

    MK_CONST(Zero,zero_s);
    MK_CONST(Opaque,opaque_s);
    MK_CONST(Boolean,boolean_s);
    MK_CONST(Unit,unit_s);
    MK_CONST(Undefined,undefined_s);

    MK_PRM(system_check);

    MK_FXD(define,encode_define);
    MK_FXD(quote,list);
    MK_FXD(type,encode_type);

    MK_FXD(if,encode_args);
    MK_FXD(and,encode_args);
    MK_FXD(or,encode_args);
    MK_FXD(bind,encode_define);
    MK_FXD(set,encode_define);
    MK_FXD(delay,encode_args);

    MK_FXD(while,encode_args);
    MK_FXD(begin,encode_args);

    MK_FXD(let,encode_let);
    MK_FXD(lambda,encode_lambda);
    MK_FXD(case,encode_case);

    MK_OPR(%type,type);
    MK_OPR(%if,if);
    MK_OPR(%and,and);
    MK_OPR(%or,or);
    MK_OPR(%set,set);

    MK_OPR(%while,while);
    MK_OPR(%begin,begin);

    MK_OPR(%let,let);
    MK_OPR(%lambda,lambda);
    MK_OPR(%delay,delay);
    MK_OPR(%case,case);

    MK_OPR(%encode-let,encode_let);
    MK_OPR(%encode-lambda,encode_lambda);
    MK_OPR(%encode-case,encode_case);

    MK_PRM(gensym);
    MK_PRM(member);
    MK_PRM(find);
    MK_PRM(map);

    p_encode_args  = MK_OPR(%encode_args, encode_args);

    p_eval_symbol  = MK_OPR(%eval-symbol,eval_symbol);
    p_eval_pair    = MK_OPR(%eval-pair,eval_pair);

    p_apply_lambda = MK_OPR(%apply-lambda,apply_lambda);
    p_apply_form   = MK_OPR(%apply-form,apply_form);

    MK_PRM(expand);
    MK_PRM(encode);
    MK_PRM(eval);
    MK_PRM(reduce);
    MK_PRM(apply);
    MK_PRM(form);
    MK_PRM(fixed);

    MK_OPR(ctor-of, ctor_of);
    MK_OPR(type-of, type_of);
    MK_OPR(sort-of, sort_of);

    MK_OPR(ctor?, ctor_q);
    MK_OPR(type?, type_q);
    MK_OPR(sort?, sort_q);

    MK_OPR(isA?, type_q);

    MK_OPR(~,com);

#define _do(NAME, OP) definePrimitive(#OP, opr_ ## NAME);

    _do_binary();
    _do_relation();

#undef _do

    MK_OPR(==,eq);
    MK_OPR(!=,neq);
    MK_PRM(iso);
    MK_PRM(assert);
    MK_PRM(exit);
    MK_PRM(abort);
    MK_PRM(environment);
    MK_PRM(dump);
    MK_PRM(dumpln);
    MK_PRM(print);
    MK_PRM(println);
    MK_PRM(debug);
    MK_PRM(level);
    MK_PRM(element);
    MK_PRM(cons);
    MK_OPR(new-list,list);
    MK_OPR(new-tuple,tuple);

    MK_PRM(force);
    MK_OPR(concat-text,concat_text);
    MK_OPR(concat-symbol,concat_symbol);

    MK_OPR(start-time,start_time);
    MK_OPR(mark-time,mark_time);

    MK_PRM(system);
    MK_PRM(error);
    MK_PRM(format);
    MK_PRM(require);

    MK_OPR(open-in,open_in);
    MK_OPR(open-out,open_out);

    MK_OPR(close-in,close_in);
    MK_OPR(close-out,close_out);

    MK_PRM(fprint);
    MK_OPR(read-line,read_line);
    MK_OPR(read-sexpr,read_sexpr);
    MK_PRM(inode);
    MK_PRM(sizeof);
    MK_PRM(car);
    MK_PRM(cdr);
    MK_OPR(gc-scan,gc_scan);
    MK_OPR(set-end,set_end);
    MK_PRM(the);
    MK_PRM(not);
    MK_OPR(open-buffer,open_buffer);
    MK_PRM(bprint);
    MK_OPR(close-buffer,close_buffer);
    MK_OPR(alloc-cycle,alloc_cycle);
    MK_OPR(scan-cycle,scan_cycle);
    MK_PRM(length);
    MK_PRM(ctor);

    MK_OPR(eof-in,eof_in);

    MK_OPR(all?,all_q);
    MK_OPR(block?,block_q);
    MK_OPR(delay?,delay_q);
    MK_OPR(fixed?,fixed_q);
    MK_OPR(forced?,forced_q);
    MK_OPR(form?,form_q);
    MK_OPR(integer?,integer_q);
    MK_OPR(lambda?,lambda_q);
    MK_OPR(nil?,nil_q);
    MK_OPR(pair?,pair_q);
    MK_OPR(primitive?,primitive_q);
    MK_OPR(quote?,quote_q);
    MK_OPR(symbol?,symbol_q);
    MK_OPR(text?,text_q);
    MK_OPR(tuple?,tuple_q);

    MK_OPR(dot?,dot_q);
    MK_OPR(void?,void_q);
    MK_OPR(true?,true_q);

    MK_OPR(form-action,form_action);
    MK_OPR(fixed-encoder,fixed_encoder);
    MK_OPR(fixed-evaluator,fixed_evaluator);
    MK_OPR(forced-value,forced_value);

    MK_PRM(Any);
    MK_PRM(Field);
    MK_PRM(Index);
    MK_PRM(Tuple);
    MK_PRM(Record);
    MK_PRM(All);
    MK_PRM(Unfold);

    MK_PRM(cast);

    Reference std_in  = 0;
    Reference std_out = 0;
    Reference std_err = 0;

    if (!opaque_Create(t_infile, s_opaque, sizeof(struct os_file), &std_in)) {
        fatal("failed to allocate opaque object: stdin");
    }
    ((OSFile)(std_in))->file = stdin;
    MK_CONST(stdin,std_in);


    if (!opaque_Create(t_outfile, s_opaque, sizeof(struct os_file), &std_out)) {
        fatal("failed to allocate opaque object: stdout");
    }
    ((OSFile)(std_out))->file = stdout;
    MK_CONST(stdout,std_out);

    if (!opaque_Create(t_outfile, s_opaque, sizeof(struct os_file), &std_err)) {
        fatal("failed to allocate opaque object: stderr");
    }
    ((OSFile)(std_err))->file = stderr;
    MK_CONST(stderr,std_err);

    clock_t cend = clock();

    fprintf(stderr,
            "started (%.3f cpu sec)\n",
            ((double)cend - (double)cbegin) * 1.0e-6);
    fflush(stderr);

    csegment = clock();

    __alloc_cycle = 1000;
    __scan_cycle  = 10;

    while (!space_CanFlip(_zero_space)) {
        space_Scan(_zero_space, 1000);
    }

    space_Flip(_zero_space);

    while (!space_CanFlip(_zero_space)) {
        space_Scan(_zero_space, 1000);
    }

    space_Flip(_zero_space);

    VM_DEBUG(1, "startEnkiLibrary end");
}

void stopEnkiLibrary() {
    if (!__initialized) return;
}
