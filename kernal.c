/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/

#include "all_types.inc"
#include "treadmill.h"

/* */
#include <string.h>
#include <stdbool.h>
#include <error.h>
#include <stdarg.h>
#include <stdio.h>

static bool __initialized = false;

//
unsigned int ea_global_debug = 0;

//
struct gc_treadmill enki_zero_space;
struct gc_header    enki_true;

Space _zero_space;

Node       enki_globals = NIL; // nt_pair(nil, alist)
Node            true_v  = NIL; // nt_unknown
Node            f_quote = NIL; // nt_fixed
Primitive p_eval_symbol = 0;
Primitive   p_eval_pair = 0;
Primitive  p_apply_expr = 0;
Primitive  p_apply_form = 0;

Symbol s_dot = 0;

Symbol s_global = 0;
Symbol s_current = 0;
Symbol s_delay = 0;
Symbol s_lambda = 0;
Symbol s_nil = 0;
Symbol s_quasiquote = 0;
Symbol s_quote = 0;
Symbol s_set = 0;
Symbol s_t = 0;
Symbol s_unquote = 0;
Symbol s_unquote_splicing = 0;

#define SUBR(NAME) void opr_##NAME(Node args, Node env, Target result)

extern void defineValue(Node symbol, const Node value) {
    Node globals;
    pair_GetCdr(enki_globals.pair, &globals);
    alist_Add(globals.pair, symbol, value, &globals.pair);
    pair_SetCdr(enki_globals.pair, globals);
}

static unsigned checkArgs(Node args, const char* name, unsigned min, ...)
{
    unsigned count  = 0;
    bool     dotted = false;

    list_State(args.pair, &count, &dotted);

    if (count < min) {
        fatal("wrong number of arguments (%i) in: %s\n", count, name);
    }

    return count;
}

static void fetchArgs(Node args, ...)
{
    va_list ap;
    va_start(ap, args);

    while (isKind(args, nt_pair)) {
        Node *location = va_arg(ap, Node*);

        if (!location) goto done;

        pair_GetCar(args.pair, location);
        pair_GetCdr(args.pair, &args);
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
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;

    list_GetItem(binding.pair, 0, &symbol);
    list_GetItem(binding.pair, 1, &expr);

    eval(expr, env, &value);

    pair_Create(symbol, value, entry.pair);
}

static void eval_begin(Node body, Node env, Target last)
{
    Node expr  = NIL;
    Node value = NIL;

    while (isKind(body, nt_pair)) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &value);
    }

    ASSIGN(last, value);
}

static SUBR(if)
{ //Fixed
    Node tst    = NIL;
    Node val    = NIL;
    Node t_expr = NIL;
    Node e_body = NIL;

    list_GetItem(args.pair, 0, &tst);
    list_GetItem(args.pair, 1, &t_expr);
    list_GetTail(args.pair, 1, &e_body);

    eval(tst, env, &val);

    if (isNil(val)) {
        eval_begin(e_body, env, result);
    } else {
        eval(t_expr, env, result);
    }
}

static SUBR(and)
{ //Fixed
    Node body = args;
    Node expr = NIL;
    Node ans  = true_v;

    for (; isKind(body, nt_pair) ;) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &ans);
        if (isNil(ans)) break;
    }

    ASSIGN(result, ans);
}

static SUBR(or)
{ //Fixed
    Node body = args;
    Node expr = NIL;
    Node ans  = NIL;

    for (; isKind(body, nt_pair) ;) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &ans);
        if (!isNil(ans)) break;
    }

    ASSIGN(result, ans);
}

static SUBR(set)
{ //Fixed
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;

    list_GetItem(args.pair, 0, &symbol);
    list_GetItem(args.pair, 1, &expr);

    if (!isKind(symbol, nt_symbol)) {
        fprintf(stderr, "\nerror: non-symbol identifier in set: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fatal(0);
    }

    Pair entry;

    if (!alist_Entry(env.pair, symbol, &entry)) {
        fprintf(stderr, "\nerror: cannot set undefined variable: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fatal(0);
    }

    eval(expr, env, &value);
    pair_SetCdr(entry, value);
    ASSIGN(result,value);
}

static SUBR(define)
{ //Fixed
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;

    list_GetItem(args.pair, 0, &symbol);
    list_GetItem(args.pair, 1, &expr);

    if (!isKind(symbol, nt_symbol)) {
        fprintf(stderr, "\nerror: non-symbol identifier in define: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fatal(0);
    }

    eval(expr, env, &value);

    defineValue(symbol, value);

    fprintf(stderr, "defined: ");
    print(stderr, symbol);
    fprintf(stderr, "\n");

    ASSIGN(result, value);
}

static SUBR(let)
{ //Fixed
    Node env2     = NIL;
    Node bindings = NIL;
    Node body     = NIL;

    pair_GetCar(args.pair, &bindings);
    pair_GetCdr(args.pair, &body);

    list_Map(eval_binding, bindings.pair, env, &(env2.pair));


    list_SetEnd(env2.pair, env);

    eval_begin(body, env2, result);
}

static SUBR(while)
{ //Fixed
    Node tst  = NIL;
    Node body = NIL;
    Node val  = NIL;

    list_GetItem(args.pair, 0, &tst);
    list_GetItem(args.pair, 1, &body);

    for (;;) {
        eval(tst, env, &val);

        if (isNil(val)) break;

        eval_begin(body, env, result);
    }
}

static SUBR(gensym)
{
    static unsigned long counter = 0;
    static char data[20];

    unsigned long current = counter;
    counter += 1;

    sprintf(data, "0x%.10lx", current);

    symbol_Convert(data, result.symbol);
}

static SUBR(find)
{
    Node tst = NIL;
    Node lst = NIL;

    list_GetItem(args.pair, 0, &tst);
    list_GetItem(args.pair, 0, &lst);

    while (isKind(lst, nt_pair)) {
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

static SUBR(quote)
{
    pair_GetCar(args.pair, result);
}

static SUBR(lambda)
{
    pair_Create(args, env, result.pair);
    fprintf(stderr, "lambda:");
    prettyPrint(stderr, args);
    fprintf(stderr, "\n");
    setKind(*result.reference, nt_expression);
}

static SUBR(eval_symbol)
{
    Node symbol = NIL;

    pair_GetCar(args.pair, &symbol);

    // lookup symbol in the current enviroment
    if (!alist_Get(env.pair, symbol, result)) {
        if (!isKind(symbol, nt_symbol)) {
            fatal("undefined variable: <non-symbol>");
        } else {
            fatal("undefined variable: %s", symbol_Text(symbol.symbol));
        }
    }
}

static SUBR(eval_pair)
{
    Node obj   = NIL;
    Node head  = NIL;
    Node tail  = NIL;

    // (subr_eval_pair obj)
    pair_GetCar(args.pair, &obj);
    pair_GetCar(obj.pair, &head);
    pair_GetCdr(obj.pair, &tail);

#if 0
    printf("eval_pair: ");
    prettyPrint(stdout, obj);
    printf("\n");
#endif

    pushTrace(obj);

    // first eval the head
    eval(head, env, &head);

    if (isKind(head, nt_fixed)) {
        // apply Fixed to un-evaluated arguments
        Node func = NIL;
        pair_GetCar(head.pair, &func);
        apply(func, tail, env, result);
        goto done;
    }

    // evaluate the arguments
    list_Map(eval, tail.pair, env, &tail.pair);

    // now apply the head to the evaluated arguments
    apply(head, tail, env, result);

 done:

#if 0
    printf("eval_pair => ");
    prettyPrint(stdout, *result.reference);
    printf("\n");
#endif

    popTrace();
}

static SUBR(eval)
{
    Node expr;
    Node cenv;

    // (eval expr env)
    // (eval expr)
    list_GetItem(args.pair, 0, &expr);
    list_GetItem(args.pair, 1, &cenv);

    if (isNil(cenv)) cenv = env;

    expand(expr, cenv, &expr);
    encode(expr, cenv, &expr);
    eval(expr, cenv, result);
}

static SUBR(apply_expr)
{
    Node fun       = NIL;
    Node arguments = NIL;
    Node defn      = NIL;
    Node cenv      = NIL;
    Node formals   = NIL;
    Node body      = NIL;
    Node tmp       = NIL;

    pair_GetCar(args.pair, &fun); //(fun . arguments)
    pair_GetCdr(args.pair, &arguments);

    pair_GetCar(fun.pair, &defn); //((formals) . body)
    pair_GetCdr(fun.pair, &cenv); // retreve the closure enviroment

    pair_GetCar(defn.pair, &formals);
    pair_GetCdr(defn.pair, &body);

    Node vars  = formals;
    Node vlist = arguments;

    // bind parameters to values
    // extending the closure enviroment
    while (isKind(formals, nt_pair)) {
        Node var = NIL;
        Node val = NIL;

        if (!isKind(vlist, nt_pair)) {
            fprintf(stderr, "\nerror: too few arguments params: ");
            prettyPrint(stderr, vars);
            fprintf(stderr, " args: ");
            prettyPrint(stderr, arguments);
            fprintf(stderr, "\n");
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
    if (isKind(formals, nt_symbol)) {
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
        fatal(0);
    }

    // process the body of the lambda
    // and return the last value
    eval_begin(body, cenv, result);
}

static SUBR(apply_form)
{
    Node form;
    Node cargs;
    Node func;

    pair_GetCar(args.pair, &form);
    pair_GetCdr(args.pair, &cargs);
    pair_GetCar(form.pair, &func);

    apply(func, cargs, env, result);
}

static SUBR(apply)
{
    Node func  = NIL;
    Node cargs = NIL;
    Node cenv  = NIL;

    list_GetItem(args.pair, 0, &func);
    list_GetItem(args.pair, 1, &cargs);
    list_GetItem(args.pair, 2, &cargs);

    if (isNil(cenv)) cenv = env;

    apply(func, cargs, cenv, result);
}

static SUBR(form)
{
    Node func = NIL;
    pair_GetCar(args.pair, &func);

    pair_Create(func, NIL, result.pair);

    setKind(*(result.pair), nt_form);
}

static SUBR(type_of)
{
    Node val = NIL;
    checkArgs(args, "type-of", 1, nt_unknown);
    pair_GetCar(args.pair, &val);
    node_TypeOf(val, result);

#if 0
    printf("type-of: ");
    dump(stdout, val);
    printf(" is ");
    dump(stdout, *result.reference);
    printf("\n");
#endif
}

static SUBR(com) {
    Node val = NIL;
    checkArgs(args, "~", 1, nt_integer);

    pair_GetCar(args.pair, &val);

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
static SUBR(NAME) \
{ \
    Node left; Node right; \
    checkArgs(args, #OP, 2, nt_integer, nt_integer); \
    fetchArgs(args, &left, &right, 0); \
    integer_Create((left.integer->value) OP (right.integer->value), result.integer); \
}

_do_binary()

#undef _do

#define _do_relation() \
    _do(lt, <) _do(le, <=) _do(ge, >=) _do(gt, >)

#define _do(NAME, OP) \
static SUBR(NAME) \
{ \
    Node left; Node right; \
    checkArgs(args, #OP, 2, nt_integer, nt_integer); \
    fetchArgs(args, &left, &right, 0); \
    if ((left.integer->value) OP (right.integer->value)) { \
        ASSIGN(result, true_v);                            \
    } else { \
        ASSIGN(result, NIL); \
    } \
}

_do_relation()

#undef _do

static SUBR(eq)
{
    Node left; Node right;
    checkArgs(args, "=", 2, nt_unknown, nt_unknown);
    fetchArgs(args, &left, &right, 0);

    if (node_Match(left,right)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
static SUBR(neq)
{
    Node left; Node right;
    checkArgs(args, "!=", 2, nt_unknown, nt_unknown);
    fetchArgs(args, &left, &right, 0);

    if (node_Match(left,right)) {
        ASSIGN(result, NIL);
    } else {
        ASSIGN(result, true_v);
    }
}

static SUBR(iso)
{
    Node depth; Node left; Node right;
    checkArgs(args, "iso", 3, nt_integer, nt_unknown, nt_unknown);
    fetchArgs(args, &depth, &left, &right, 0);

    if (node_Iso(depth.integer->value, left,right)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}

static SUBR(assert)
{
    Node left; Node right;
    checkArgs(args, "assert", 2, nt_unknown, nt_unknown);
    fetchArgs(args, &left, &right, 0);

    if (node_Iso(20, left,right)) {
        ASSIGN(result, true_v);
    } else {
        fprintf(stderr, "not iso: ");
        print(stderr, left);
        fprintf(stderr, " to: ");
        print(stderr, right);
        fprintf(stderr, "\n");
        fatal(0);
    }
}

static SUBR(exit)
{
    Node value = NIL;

    pair_GetCar(args.pair, &value);

    if (isKind(value, nt_integer)) {
        exit(value.integer->value);
    } else {
        exit(0);
    }
}

static SUBR(abort)
{
    fatal("aborted");
}

static SUBR(environment)
{
    Node value = NIL;
    fetchArgs(args, &value, 0);

    if (isIdentical(value, s_current)) {
         ASSIGN(result, env);
    } else {
        pair_GetCdr(enki_globals.pair, result);
    }
}

static SUBR(dump)
{
    Node value = NIL;
    pair_GetCar(args.pair, &value);
    prettyPrint(stdout, value);
    ASSIGN(result,NIL);
}

static SUBR(dumpln)
{
    Node value = NIL;
    pair_GetCar(args.pair, &value);
    prettyPrint(stdout, value);
    printf("\n");
    ASSIGN(result,NIL);
}

static SUBR(print)
{
    Node value = NIL;
    while (isKind(args, nt_pair)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        print(stdout, value);
    }
    ASSIGN(result,NIL);
}

static SUBR(println)
{
    Node value = NIL;
    while (isKind(args, nt_pair)) {
        pair_GetCar(args.pair, &value);
        pair_GetCdr(args.pair, &args);
        print(stdout, value);
    }
    printf("\n");
    ASSIGN(result,NIL);
}

static SUBR(debug)
{
    Node value = NIL;
    long level = 1;
    if (isKind(args, nt_pair)) {
        pair_GetCar(args.pair, &value);
        if (isKind(value, nt_integer)) {
            level = value.integer->value;
            pair_GetCdr(args.pair, &args);
        }
    }
    if (ea_global_debug <= level) {
        while (isKind(args, nt_pair)) {
            pair_GetCar(args.pair, &value);
            pair_GetCdr(args.pair, &args);
            print(stderr, value);
        }
        fprintf(stderr, "\n");
    }
    ASSIGN(result,NIL);
}

static SUBR(level)
{
    Node value = NIL;

    integer_Create(ea_global_debug, &value.integer);

    if (isKind(args, nt_pair)) {
        pair_GetCar(args.pair, &value);
        if (isKind(value, nt_integer)) {
            ea_global_debug = value.integer->value;
        }
    }

    ASSIGN(result,value);
}

static SUBR(element) {
    Node tuple; Node index; Node value;
    int count = checkArgs(args, "element", 2, nt_unknown, nt_integer);

    ASSIGN(result,NIL);

    if (2 < count) {
        fetchArgs(args, &tuple, &index, &value, 0);
        tuple_SetItem(tuple.tuple, index.integer->value, value);
    } else {
        fetchArgs(args, &tuple, &index, 0);
        tuple_GetItem(tuple.tuple, index.integer->value, result);
    }
}

static void defineConstant(const char* name, const Node value) {
    Symbol label = 0;

    symbol_Convert(name, &label);

    defineValue(label, value);
}

static Primitive definePrimitive(const char* name, Operator func) {
    Symbol    label = 0;
    Primitive prim  = 0;

    symbol_Convert(name, &label);
    primitive_Create(label, func, &prim);

    defineValue(label, prim);

    return prim;
}

static Node defineFixed(const char* name, Operator func) {
    Node      fixed = NIL;
    Symbol    label = 0;
    Primitive prim  = 0;

    symbol_Convert(name, &label);
    primitive_Create(label, func, &prim);

    pair_Create(prim, NIL, &(fixed.pair));

    setKind(fixed, nt_fixed);

    defineValue(label, fixed);

    return fixed;
}

#define MK_SYM(x)     symbol_Convert(#x, &s_ ##x)
#define MK_CONST(x,y) defineConstant(#x, y);
#define MK_PRM(x)     definePrimitive(#x, opr_ ## x)
#define MK_FXD(x)     defineFixed(#x, opr_ ## x)
#define MK_OPR(x,y)   definePrimitive(#x, opr_ ## y)

void startEnkiLibrary() {
    if (__initialized) return;

    true_v = (Node)init_atom(&enki_true, 0);

    space_Init(&enki_zero_space);

    _zero_space = &enki_zero_space;

    clink_Manage(&(enki_zero_space.start_clinks), &enki_globals);

    init_global_symboltable();

    pair_Create(NIL,NIL, &enki_globals.pair);

    symbol_Convert(".", &s_dot);

    MK_SYM(current);
    MK_SYM(delay);
    MK_SYM(global);
    MK_SYM(lambda);
    MK_SYM(nil);
    MK_SYM(quasiquote);
    MK_SYM(quote);
    MK_SYM(set);
    MK_SYM(t);
    MK_SYM(unquote);
    MK_SYM(unquote_splicing);


    f_quote = MK_FXD(quote);

    MK_FXD(and);
    MK_FXD(define);
    MK_FXD(if);
    MK_FXD(lambda);
    MK_FXD(let);
    MK_FXD(or);
    MK_FXD(set);
    MK_FXD(while);

    MK_PRM(gensym);
    MK_PRM(find);

    p_eval_symbol = MK_OPR(eval-symbol,eval_symbol);
    p_eval_pair   = MK_OPR(eval-pair,eval_pair);
    p_apply_expr  = MK_OPR(apply-expr,apply_expr);
    p_apply_form  = MK_OPR(apply-form,apply_form);

    MK_PRM(form);
    MK_PRM(eval);
    MK_PRM(apply);
    MK_OPR(type-of, type_of);

    MK_OPR(~,com);

#define _do(NAME, OP) definePrimitive(#OP, opr_ ## NAME);

    _do_binary();
    _do_relation();

#undef _do

    MK_OPR(=,eq);
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
    MK_OPR(%element,element);

    MK_CONST(t,true_v);
    MK_CONST(nil,NIL);
}

void stopEnkiLibrary() {
    if (!__initialized) return;
 }
