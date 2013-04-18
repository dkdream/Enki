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

Space     _zero_space;
unsigned __alloc_cycle;
unsigned __scan_cycle;

Node        enki_globals = NIL; // nt_pair(nil, alist)
Node              true_v = NIL;
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

extern bool opaque_Create(Node type, long size, Reference* target) {
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

extern void forceArgs(Node args, ...)
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

extern void fetchArgs(Node args, ...)
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

extern void eval_binding(Node binding, Node env, Target entry)
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

extern void eval_begin(Node body, Node env, Target last)
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

    for (; isType(body, t_pair) ;) {
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

    for (; isType(body, t_pair) ;) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &ans);
        if (!isNil(ans)) break;
    }

    ASSIGN(result, ans);
    GC_End();
}

#if 0
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
#endif

extern SUBR(set)
{ //Fixed
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;

    fetchArgs(args, &symbol, &expr, 0);

    if (!isType(symbol, s_symbol)) {
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

    eval(expr, env, &value);

    pair_SetCdr(entry, value);

    ASSIGN(result,value);
}

extern SUBR(define)
{ //Fixed
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;

    fetchArgs(args, &symbol, &expr, 0);

    if (!isType(symbol, s_symbol)) {
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

extern SUBR(type)
{ //Fixed
    Node symbol;
    pair_GetCar(args.pair, &symbol);

    if (!isType(symbol, s_symbol)) {
        ASSIGN(result, symbol);
        return;
    }

    type_Create(symbol.symbol, zero_s, result.type);
}

extern void environ_Let(Node local, Node env, Target result)
{
    Node symbol;
    pair_GetCar(local.pair, &symbol);
    pair_Create(symbol, NIL, result.pair);
}

extern SUBR(encode_let) {
    Node locals; Node lenv;

    pair_GetCar(args.pair, &locals);

    if (!isType(locals, t_pair)) {
        encode(args, env, result);
        return;
    }

    /*
    ** given args=(locals . body)
    **
    ** (set locals (car body))
    ** (set lenv (map (lambda (binding) (box (car binding))) locals))
    ** (set-end lenv env)
    **
    ** result=(encode args lenv)
    */
    list_Map(environ_Let, locals.pair, env, &lenv);
    list_SetEnd(lenv.pair, env);

    //list_Map(encode, args.pair, lenv, result);

    encode(args, lenv, result);
}

extern SUBR(let)
{ //Fixed
    Node env2     = NIL;
    Node bindings = NIL;
    Node body     = NIL;

    pair_GetCar(args.pair, &bindings);
    pair_GetCdr(args.pair, &body);

    if (!isType(bindings, t_pair)) {
        eval_begin(body, env, result);
    } else {
        list_Map(eval_binding, bindings.pair, env, &(env2.pair));

        list_SetEnd(env2.pair, env);

        eval_begin(body, env2, result);
    }
}

extern void environ_Lambda(Node symbol, Node env, Target result)
{
    pair_Create(symbol, NIL, result.pair);
}

extern SUBR(encode_lambda) {
    Node formals; Node body; Node lenv;

    pair_GetCar(args.pair, &formals);
    pair_GetCdr(args.pair, &body);

    /*
    ** given args=(formal . body)
    **
    ** (set lenv (map box formal))
    ** (set-end lenv env)
    **
    ** result=(cons formals (encode body lenv))
    */

    list_Map(environ_Lambda, formals.pair, env, &lenv);
    list_SetEnd(lenv.pair, env);
    encode(body, lenv, &(body.pair));

    pair_Create(formals, body, result.pair);
}

extern SUBR(lambda)
{
    pair_Create(args, env, result.pair);
    setType(*result.reference, t_lambda);
}

extern SUBR(delay)
{
    Tuple tuple, expr;

    pair_GetCar(args.pair, &expr);

    tuple_Create(3, &tuple);
    tuple_SetItem(tuple, 0, NIL);
    tuple_SetItem(tuple, 1, expr);
    tuple_SetItem(tuple, 2, env);
    setType(tuple, t_delay);
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

    while (isType(lst, t_pair)) {
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
    Node tst = NIL;
    Node lst = NIL;

    forceArgs(args, &tst, &lst, 0);

    while (isType(lst, t_pair)) {
        Node elm   = NIL;
        Node check = NIL;

        pair_GetCar(lst.pair, &elm);
        pair_Create(elm, NIL, &(args.pair));

        apply(tst, args, env, &check);

        if (!isNil(check)) {
            ASSIGN(result,elm);
            break;
        }

        pair_GetCdr(lst.pair, &lst);
    }
}

extern void call_with(const Node function, const Node value, const Node env, Target target) {
    Pair args;
    pair_Create(value, NIL, &args);
    apply(function, args, env, target);
}

extern SUBR(map) {
    Node function;
    Node list;

    checkArgs(args, "map", 1, NIL, t_pair);
    forceArgs(args, &function, &list, 0);

    if (isNil(function)) {
        ASSIGN(result, list);
        return;
    }

    if (!isType(list, t_pair)) {
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

    for (; isType(pair->cdr.pair, t_pair) ;) {
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

    if (isType(tmp, t_forced)) {
        tuple_GetItem(tmp.tuple, 0, &tmp);
        pair_SetCdr(entry, tmp);
    }

    ASSIGN(result, tmp);

    GC_End();
    return;

 error:
    GC_End();
    if (!isType(symbol, s_symbol)) {
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

    if (isType(head, t_delay)) {
        Node dexpr, denv;
        tuple_GetItem(head.tuple, 1, &dexpr);
        tuple_GetItem(head.tuple, 2, &denv);
        eval(dexpr, denv, &tmp);
        tuple_SetItem(head.tuple, 0, tmp);
        setType(head.tuple, t_forced);
        head = tmp;
    }

    if (isType(head, t_fixed)) {
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

extern SUBR(apply_lambda)
{
    GC_Begin(3);
    Node cenv, tmp;

    GC_Protect(cenv);
    GC_Protect(tmp);

    Node fun, arguments;

    pair_GetCar(args.pair, &fun); //(fun . arguments)
    pair_GetCdr(args.pair, &arguments);

    Node defn;

    pair_GetCar(fun.pair, &defn); //((formals) . body)
    pair_GetCdr(fun.pair, &cenv); // retreve the closure enviroment

    Node formals, body;

    pair_GetCar(defn.pair, &formals);
    pair_GetCdr(defn.pair, &body);

    Node vars  = formals;
    Node vlist = arguments;

    // bind parameters to values
    // extending the closure enviroment
    while (isType(formals, t_pair)) {
        Node var = NIL;
        Node val = NIL;

        if (!isType(vlist, t_pair)) {
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

    // bind (rest) parameter to remaining values
    // extending the closure enviroment
    if (isType(formals, s_symbol)) {
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
  setType(tuple, t_form);

  ASSIGN(result, (Node)tuple);
}

extern SUBR(fixed)
{
  Tuple tuple;
  Node  func = NIL;
  Node  enc = NIL;

  int count = checkArgs(args, "fixed", 1, NIL);

  ASSIGN(result,NIL);

  if (1 < count) {
      forceArgs(args, &func, &enc, 0);
      tuple_Create(2, &tuple);
      tuple_SetItem(tuple, 0, func);
      tuple_SetItem(tuple, 1, enc);
  } else {
      forceArgs(args, &func, 0);
      tuple_Create(1, &tuple);
      tuple_SetItem(tuple, 0, func);
  }

  setType(tuple, t_fixed);

  ASSIGN(result, (Node)tuple);
}

extern SUBR(type_of)
{
    Node value, type;
    int count = checkArgs(args, "type-of", 1, NIL);

    ASSIGN(result,NIL);

    if (1 < count) {
        forceArgs(args, &value, &type, 0);
        setType(value, type);
    } else {
        pair_GetCar(args.pair, &value);
        node_TypeOf(value, result);
    }
}

extern SUBR(com) {
    Node val = NIL;
    checkArgs(args, "~", 1, t_integer);
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
    checkArgs(args, #OP, 2, t_integer, t_integer); \
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
    checkArgs(args, #OP, 2, t_integer, t_integer); \
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
    checkArgs(args, "iso", 3, t_integer, NIL, NIL);
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

    if (isType(value, t_integer)) {
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

    if (isType(args, t_pair)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        prettyPrint(stdout, value);
    }

    while (isType(args, t_pair)) {
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

    if (isType(args, t_pair)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        prettyPrint(stdout, value);
    }

    while (isType(args, t_pair)) {
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

    while (isType(args, t_pair)) {
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

    while (isType(args, t_pair)) {
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
    if (isType(args, t_pair)) {
        pair_GetCar(args.pair, &value);
        if (isType(value, t_integer)) {
            level = value.integer->value;
            pair_GetCdr(args.pair, &args);
        }
    }
    if (ea_global_debug <= level) {
        while (isType(args, t_pair)) {
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

    if (isType(args, t_pair)) {
        Node nvalue;
        pair_GetCar(args.pair, &nvalue);
        if (isType(nvalue, t_integer)) {
            ea_global_debug = nvalue.integer->value;
        }
    }

    ASSIGN(result,value);
}

extern SUBR(element) {
    Node tuple, index, value;
    int count = checkArgs(args, "element", 2, NIL, t_integer);

    ASSIGN(result,NIL);

    if (2 < count) {
        forceArgs(args, &tuple, &index, &value, 0);
        tuple_SetItem(tuple.tuple, index.integer->value, value);
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

extern SUBR(allocate) {
    Node type; Node size;
    checkArgs(args, "allocate", 2, s_symbol, t_integer);

    ASSIGN(result,NIL);

    forceArgs(args, &type, &size, 0);

    long slots = size.integer->value;

    if (1 > slots) return;

    Tuple value;

    tuple_Create(slots, &value);

    setType(value, type.symbol);

    ASSIGN(result,value);
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

    checkArgs(args, "concat-text", 2, t_text, t_text);

    while (isType(args, t_pair)) {
        Node text;

        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (isType(text, t_text)) {
            buffer_add(&buffer, text_Text(text.text));
            continue;
        }

        if (isType(text, s_symbol)) {
            buffer_add(&buffer, symbol_Text(text.symbol));
            continue;
        }

        if (isType(text, t_integer)) {
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

    while (isType(args, t_pair)) {
        Node text;

        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (isType(text, t_text)) {
            buffer_add(&buffer, text_Text(text.text));
            continue;
        }

        if (isType(text, s_symbol)) {
            buffer_add(&buffer, symbol_Text(text.symbol));
            continue;
        }

        if (isType(text, t_integer)) {
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

    while (isType(args, t_pair)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        if (isType(value, s_symbol)) {
            print(stderr, value);
            prefix = true;
            continue;
        }
        if (isType(value, t_text)) {
            print(stderr, value);
            prefix = true;
            continue;
        }
        if (isType(value, t_integer)) {
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
    checkArgs(args, "system", 1, t_text);
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

    checkArgs(args, "error", 1, s_symbol, t_text);
    forceArgs(args, &kind, &message, 0);

    print(stderr, kind);
    fatal("%s", text_Text(message.text));
}

extern void formatAppendTo(TextBuffer *buffer, Node value) {
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

extern SUBR(format) {
    static TextBuffer buffer = BUFFER_INITIALISER;

    buffer_reset(&buffer);

    const char* format;

    checkArgs(args, "format", 1, t_text);

    if (!isType(args, t_pair)) {
        fatal("missing first argument to format\n");
    } else {
        Node form = NIL;
        pair_GetCar(args.pair, &form);
        pair_GetCdr(args.pair, &args);
        if (!isType(form, t_text)) {
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

                if (!isType(args, t_pair)) {
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

                    if ('p' == code) {
                        char data[20];
                        sprintf(data, "%p", value.reference);
                        buffer_add(&buffer, data);
                        continue;
                    }

                    if ('c' == code) {
                        char data[20];
                        char chr;
                        if (!isType(value, t_integer)) {
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

    checkArgs(args, "require", 1, t_text);
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
    checkArgs(args, "open-in", 1, t_text);
    forceArgs(args, &path, 0);

    FILE* file = fopen(text_Text(path.text), "r");

    if (!file) {
        fatal("failed to open \'%s\' for reading", text_Text(path.text));
        return;
    }

    Reference infile = 0;

    if (!opaque_Create(t_infile, sizeof(struct os_file), &infile)) {
        fatal("failed to allocate opaque object");
    }

    ((OSFile)(infile))->file = file;

    ASSIGN(result, infile);
}

extern SUBR(open_out) {
    Node path;
    checkArgs(args, "open-out", 1, t_text);
    forceArgs(args, &path, 0);

    FILE* file = fopen(text_Text(path.text), "w");

    if (!file) {
        fatal("failed to open \'%s\' for writing", text_Text(path.text));
        return;
    }

    Reference outfile = 0;

    if (!opaque_Create(t_outfile, sizeof(struct os_file), &outfile)) {
        fatal("failed to allocate opaque object");
    }

    ((OSFile)(outfile))->file = file;

    ASSIGN(result, outfile);
}

extern SUBR(close_in) {
    Node file;
    checkArgs(args, "close-in", 1, t_infile);
    forceArgs(args, &file, 0);

    if (!isType(file, t_infile)) {
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

    if (!isType(file, t_outfile)) {
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

    if (!isType(args, t_pair)) {
         fatal("missing first argument to fprint\n");
    } else {
        Node outfile = NIL;

        pair_GetCar(args.pair, &outfile);
        pair_GetCdr(args.pair, &args);

        if (!isType(outfile, t_outfile)) {
            fatal("first argument to fprint is not an outfile\n");
        }

        out = ((OSFile)(outfile.reference))->file;
    }

    while (isType(args, t_pair)) {
        Node text;
        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (!isType(text, t_text)) {
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

    if (!isType(file, t_infile)) {
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

    if (!isType(file, t_infile)) {
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

    if (!isType(file, t_infile)) {
        fatal("read-line: not an infile");
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

    checkArgs(args, "inode", 1, t_text);
    forceArgs(args, &fname, 0);

    if (!isType(fname, t_text)) {
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

    if (isIdentical(kind, s_header)) {
        integer_Create(sizeof(struct gc_header), result.integer);
    }

    if (isIdentical(kind, s_kind)) {
        integer_Create(sizeof(struct gc_kind), result.integer);
    }

    if (isIdentical(kind, s_node)) {
        integer_Create(sizeof(Node), result.integer);
    }
}

extern SUBR(car) {
    Pair pair;
    checkArgs(args, "car", 1, t_pair);
    forceArgs(args, &pair, 0);

    pair_GetCar(pair, result);
}

extern SUBR(cdr) {
    Pair pair;

    checkArgs(args, "car", 1, t_pair);
    forceArgs(args, &pair, 0);

    pair_GetCdr(pair, result);
}

extern SUBR(pair_q) {
    Node value;
    checkArgs(args, "pair?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isType(value, t_pair)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(integer_q) {
    Node value;
    checkArgs(args, "integer?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isType(value, t_integer)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(gc_scan) {
    Node value;

    checkArgs(args, "gc-scan", 1, t_integer);
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

extern SUBR(box) {
    Node value;
    checkArgs(args, "box", 1, NIL);
    fetchArgs(args, &value, 0);
    pair_Create(value, NIL, result.pair);
}

extern SUBR(set_end) {
    Node head;
    Node tail;
    checkArgs(args, "set-end", 1, t_pair, NIL);
    forceArgs(args, &head, &tail, 0);

    if (!isType(head, t_pair)) {
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

    if (!isType(value, type)) {
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

    if (!opaque_Create(t_buffer, sizeof(struct text_buffer), &buffer)) {
        fatal("failed to allocate opaque object");
    }

    buffer_init((TextBuffer *)buffer);

    ASSIGN(result, buffer);
}

extern SUBR(bprint) {
    TextBuffer *out = 0;

    checkArgs(args, "bprint", 1, t_buffer);

    if (!isType(args, t_pair)) {
        fatal("missing first argument to bprint\n");
    } else {
        Node buffer = NIL;

        pair_GetCar(args.pair, &buffer);
        pair_GetCdr(args.pair, &args);

        if (!isType(buffer, t_buffer)) {
            fatal("first argument to bprint is not an buffer\n");
        }

        out = ((TextBuffer *)(buffer.reference));
    }

    while (isType(args, t_pair)) {
        Node text;
        pair_GetCar(args.pair, &text);
        pair_GetCdr(args.pair, &args);

        if (!isType(text, t_text)) {
            fatal("invalid argument to bprint\n");
        }

        buffer_add(out, text_Text(text.text));
    }
}

extern SUBR(close_buffer) {
    Node buffer;
    checkArgs(args, "close-buffer", 1, t_buffer);
    forceArgs(args, &buffer, 0);

    if (!isType(buffer, t_buffer)) {
        fatal("close-buffer: not an buffer");
    }

    TextBuffer *buff = ((TextBuffer *)(buffer.reference));

    text_Create(*(buff), result.text);

    buffer_free(buff);

    setType(buffer, s_opaque);
}

extern SUBR(text_q) {
    Node value;
    checkArgs(args, "text?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isType(value, t_text)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

extern SUBR(symbol_q) {
    Node value;
    checkArgs(args, "symbol?", 1, NIL);
    pair_GetCar(args.pair, &value);

    if (isType(value, s_symbol)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
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

static Node defineFixed(const char* name, Operator eval)
{
    GC_Begin(8);

    Tuple fixed; Symbol label; Primitive prim;

    GC_Protect(fixed);
    GC_Protect(label);
    GC_Protect(prim);

    symbol_Convert(name, &label);
    primitive_Create(label, eval, &prim);

    tuple_Create(1, &fixed);
    setType(fixed, t_fixed);

    tuple_SetItem(fixed, fxd_eval, prim);

    defineValue(label, fixed);

    GC_End();

    return (Node) fixed;
}

static Node defineEFixed(const char* neval,  Operator oeval,
                         const char* nencode, Operator oencode)
{
    GC_Begin(8);

    Tuple fixed; Symbol label; Primitive prim;

    GC_Protect(fixed);
    GC_Protect(label);
    GC_Protect(prim);

    tuple_Create(2, &fixed);
    setType(fixed, t_fixed);

    if (nencode) {
        symbol_Convert(nencode, &label);
        primitive_Create(label, oencode, &prim);
        tuple_SetItem(fixed, fxd_encode, prim);
    }

    symbol_Convert(neval, &label);
    primitive_Create(label, oeval, &prim);
    tuple_SetItem(fixed, fxd_eval, prim);

    defineValue(label, fixed);

    GC_End();

    return (Node) fixed;
}

#define MK_CONST(x,y) defineConstant(#x, y)
#define MK_BTYPE(x)   defineConstant(#x, t_ ##x)
#define MK_PRM(x)     definePrimitive(#x, opr_ ## x)
#define MK_FXD(x)     defineFixed(#x, opr_ ## x)
#define MK_FXD(x)     defineFixed(#x, opr_ ## x)
#define MK_EFXD(x,y)  defineEFixed(#x, opr_ ## x, #y, opr_ ## y)
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

    clink_Manage(&(enki_zero_space.start_clinks), &enki_globals);

    VM_DEBUG(1, "startEnkiLibrary init symbol table");

    init_global_symboltable();

    VM_DEBUG(1, "startEnkiLibrary init type table");
    init_global_typetable();

    true_v = (Node)init_atom(&enki_true, 0);

    setType(true_v, t_true);

    pair_Create(NIL,NIL, &enki_globals.pair);

    MK_CONST(true,true_v);
    MK_CONST(nil,NIL);

    MK_CONST(Zero,zero_s);

    VM_DEBUG(1, "startEnkiLibrary FXD");

    MK_PRM(system_check);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_FXD(define);
    MK_EFXD(quote,list);
    MK_EFXD(type,list);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_FXD(if);
    MK_FXD(and);
    MK_FXD(or);
    MK_FXD(set);
    MK_FXD(delay);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_EFXD(let,encode_let);
    MK_EFXD(lambda,encode_lambda);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_OPR(%if,if);
    MK_OPR(%and,and);
    MK_OPR(%or,or);
    MK_OPR(%set,set);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_OPR(%let,let);
    MK_OPR(%lambda,lambda);
    MK_OPR(%delay,delay);

    MK_OPR(%encode-let,encode_let);
    MK_OPR(%encode-lambda,encode_lambda);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_PRM(gensym);
    MK_PRM(member);
    MK_PRM(find);
    MK_PRM(map);

    VM_DEBUG(1, "startEnkiLibrary ---");

    p_eval_symbol  = MK_OPR(%eval-symbol,eval_symbol);
    p_eval_pair    = MK_OPR(%eval-pair,eval_pair);

    p_apply_lambda = MK_OPR(%apply-lambda,apply_lambda);
    p_apply_form   = MK_OPR(%apply-form,apply_form);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_PRM(expand);
    MK_PRM(encode);
    MK_PRM(eval);
    MK_PRM(reduce);
    MK_PRM(apply);
    MK_PRM(form);
    MK_PRM(fixed);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_OPR(type-of, type_of);

    MK_OPR(~,com);

    VM_DEBUG(1, "startEnkiLibrary ---");

#define _do(NAME, OP) definePrimitive(#OP, opr_ ## NAME);

    _do_binary();
    _do_relation();

#undef _do

    VM_DEBUG(1, "startEnkiLibrary ---");

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
    MK_PRM(list);
    MK_PRM(tuple);
    MK_PRM(allocate);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_PRM(force);
    MK_OPR(concat-text,concat_text);
    MK_OPR(concat-symbol,concat_symbol);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_OPR(start-time,start_time);
    MK_OPR(mark-time,mark_time);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_PRM(system);
    MK_PRM(error);
    MK_PRM(format);
    MK_PRM(require);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_OPR(open-in,open_in);
    MK_OPR(open-out,open_out);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_OPR(close-in,close_in);
    MK_OPR(close-out,close_out);

    VM_DEBUG(1, "startEnkiLibrary ---");

    MK_PRM(fprint);
    MK_OPR(read-line,read_line);
    MK_OPR(eof-in,eof_in);
    MK_OPR(read-sexpr,read_sexpr);
    MK_PRM(inode);
    MK_PRM(sizeof);
    MK_PRM(car);
    MK_PRM(cdr);
    MK_OPR(pair?,pair_q);
    MK_OPR(integer?,integer_q);
    MK_OPR(gc-scan,gc_scan);
    MK_PRM(box);
    MK_OPR(set-end,set_end);
    MK_PRM(the);
    MK_OPR(nil?,nil_q);
    MK_PRM(not);
    MK_OPR(open-buffer,open_buffer);
    MK_PRM(bprint);
    MK_OPR(close-buffer,close_buffer);
    MK_OPR(text?,text_q);
    MK_OPR(symbol?,symbol_q);

    clock_t cend = clock();

    fprintf(stderr,
            "started (%.3f cpu sec)\n",
            ((double)cend - (double)cbegin) * 1.0e-6);
    fflush(stderr);

    csegment = clock();

    __alloc_cycle = 100;
    __scan_cycle  = 10;

    VM_DEBUG(1, "startEnkiLibrary end");
}

void stopEnkiLibrary() {
    if (!__initialized) return;
}
