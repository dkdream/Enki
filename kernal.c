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

static bool __initialized = false;

//
unsigned int ea_global_debug = 5;

//
struct gc_treadmill enki_zero_space;

Space _zero_space;

// pair(nil, alist)
Node enki_globals = NIL;

Node      f_quote = NIL;       // quote a syntax tree
Primitive p_eval_symbol = 0;
Primitive p_eval_pair = 0;
Primitive p_apply_expr = 0;
Primitive p_apply_form = 0;

Symbol s_dot = 0;
Symbol s_quasiquote = 0;
Symbol s_quote = 0;
Symbol s_unquote = 0;
Symbol s_unquote_splicing = 0;
Symbol s_delay = 0;
Symbol s_t = 0;
Symbol s_nil = 0;
Symbol s_lambda = 0;
Symbol s_set = 0;

extern void defineValue(Node symbol, const Node value) {
    Node globals;
    pair_GetCdr(enki_globals.pair, &globals);
    alist_Add(globals.pair, symbol, value, &globals.pair);
    pair_SetCdr(enki_globals.pair, globals);
}

#define GC_PROTECT(NODE)
#define GC_UNPROTECT(NODE)

#define SUBR(NAME) bool opr_##NAME(Node args, Node env, Target result)

static bool eval_binding(Node binding, Node env, Target entry)
{
    Node symbol = NIL;
    Node expr   = NIL;
    Node value  = NIL;

    pair_GetCar(binding.pair, &symbol);
    pair_GetCdr(binding.pair, &expr);

    eval(expr, env, &value);

    return pair_Create(symbol, value, entry.pair);
}

static bool eval_begin(Node body, Node env, Target last)
{
    Node expr  = NIL;
    Node value = NIL;

    while (isKind(body, nt_pair)) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &value);
    }

    ASSIGN(last, value);
    return true;
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
        return eval_begin(e_body, env, result);
    } else {
        return eval(t_expr, env, result);
    }
}

static SUBR(and)
{ //Fixed
    Node body = args;
    Node expr = NIL;
    Node ans  = (Node) s_t;

    for (; isKind(body, nt_pair) ;) {
        pair_GetCar(body.pair, &expr);
        pair_GetCdr(body.pair, &body);
        eval(expr, env, &ans);
        if (isNil(ans)) break;
    }

    ASSIGN(result, ans);
    return true;;
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
    return true;
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

    if (alist_Entry(env.pair, symbol, &entry)) {
        fprintf(stderr, "\nerror: cannot set undefined variable: ");
        dump(stderr, symbol);
        fprintf(stderr, "\n");
        fatal(0);
    }

    eval(expr, env, &value);

    pair_SetCdr(entry, value);

    ASSIGN(result,value);

    return true;;
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

    GC_PROTECT(value);

    defineValue(symbol, value);

    GC_UNPROTECT(value);

    ASSIGN(result, value);

    return true;
}

static SUBR(let)
{ //Fixed
    Node env2     = NIL;
    Node bindings = NIL;
    Node body     = NIL;

    pair_GetCar(args.pair, &bindings);
    pair_GetCdr(args.pair, &body);

    GC_PROTECT(env2);

    list_Map(eval_binding, bindings.pair, env, &(env2.pair));
    list_SetEnd(env2.pair, env);
    eval_begin(body, env2, result);

    GC_UNPROTECT(env2);

    return true;
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

    return true;
}

static SUBR(gensym)
{
    static unsigned long counter = 0;
    static char data[20];

    unsigned long current = counter;
    counter += 1;

    sprintf(data, "0x%.10lx", current);

    return symbol_Convert(data, result.symbol);
}

static SUBR(find)
{
    Node tst = NIL;
    Node lst = NIL;

    list_GetItem(args.pair, 0, &tst);
    list_GetItem(args.pair, 0, &lst);

    GC_PROTECT(tst);
    GC_PROTECT(lst);

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

    GC_UNPROTECT(lst);
    GC_UNPROTECT(tst);

    return true;
}

static SUBR(quote)
{
    return pair_GetCar(args.pair, result);
}

static SUBR(lambda)
{
    pair_Create(args, env, result.pair);
    return setKind(*result.reference, nt_expression);
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

    return true;
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
    popTrace();
    return true;
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

    return true;
}

static SUBR(apply_expr)
{
    return true;
#if 0
    oop fun       = car(args);
    oop arguments = cdr(args);
    oop defn      = get(fun, Expr,defn);
    oop formals   = car(defn);
    oop body      = cdr(defn);
    oop tmp       = nil;
    oop ans       = nil;

    GC_PROTECT(defn);
    GC_PROTECT(env);
    GC_PROTECT(tmp);

    // retreve the closure enviroment
    env = get(fun, Expr,env);

    // bind parameters to values
    // extending the closure enviroment
    oop argl = arguments;
    while (is(Pair, formals)) {
        if (!is(Pair, argl)) {
            fprintf(stderr, "\nerror: too few arguments applying ");
            fdump(stderr, fun);
            fprintf(stderr, " to ");
            fdumpln(stderr, arguments);
            fatal(0);
        }
        tmp     = newPair(car(formals), car(argl));
        env     = newPair(tmp, env);
        formals = cdr(formals);
        argl    = cdr(argl);
    }

    // bind (rest) parameter to remaining values
    // extending the closure enviroment
    if (is(Symbol, formals)) {
        tmp  = newPair(formals, argl);
        env  = newPair(tmp, env);
        argl = nil;
    }

    // check --
    if (nil != argl) {
        fprintf(stderr, "\nerror: too many arguments applying ");
        fdump(stderr, fun);
        fprintf(stderr, " to ");
        fdumpln(stderr, arguments);
        fatal(0);
    }

    // process the body of the lambda
    // and return the last value
    while (is(Pair, body)) {
        ans  = eval(car(body), env);
        body = cdr(body);
    }

    GC_UNPROTECT(tmp);
    GC_UNPROTECT(env);
    GC_UNPROTECT(defn);

    return ans;
#endif
}

static SUBR(apply_form)
{
    Node form;
    Node cargs;
    Node func;

    pair_GetCar(args.pair, &form);
    pair_GetCdr(args.pair, &cargs);
    pair_GetCar(form.pair, &func);

    return apply(func, cargs, env, result);
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

    return apply(func, cargs, cenv, result);
}

static SUBR(form)
{
    Node func = NIL;
    pair_GetCar(args.pair, &func);

    bool rtn = pair_Create(func, NIL, result.pair);
    setKind(*(result.pair), nt_form);

    return rtn;
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

#define MK_SYM(x) symbol_Convert(#x, &s_ ##x)
#define MK_PRM(x) definePrimitive(#x, opr_ ## x);
#define MK_FXD(x) defineFixed(#x, opr_ ## x);

void startEnkiLibrary() {
    if (__initialized) return;

    space_Init(&enki_zero_space);

    _zero_space = &enki_zero_space;

    clink_Manage(&(enki_zero_space.start_clinks), &enki_globals);

    init_global_symboltable();

    pair_Create(NIL,NIL, &enki_globals.pair);

    symbol_Convert(".", &s_dot);

    MK_SYM(quasiquote);
    MK_SYM(quote);
    MK_SYM(unquote);
    MK_SYM(unquote_splicing);
    MK_SYM(delay);
    MK_SYM(t);
    MK_SYM(nil);
    MK_SYM(lambda);
    MK_SYM(set);

    f_quote = MK_FXD(quote);

    MK_FXD(if);
    MK_FXD(and);
    MK_FXD(or);
    MK_FXD(set);
    MK_FXD(define);
    MK_FXD(let);
    MK_FXD(while);
    MK_FXD(lambda);

    MK_PRM(gensym);
    MK_PRM(find);

    p_eval_symbol = MK_PRM(eval_symbol);
    p_eval_pair   = MK_PRM(eval_pair);
    p_apply_expr  = MK_PRM(apply_expr);
    p_apply_form  = MK_PRM(apply_form);

    MK_PRM(form);
    MK_PRM(eval);
    MK_PRM(apply);
}

void stopEnkiLibrary() {
    if (!__initialized) return;
 }
