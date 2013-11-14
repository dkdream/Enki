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

static void environ_Lambda(Node parameter, Node env, Target result)
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
        list_Map(formals.pair, environ_Lambda, env, &(lenv.pair));
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

        bindValue(cenv.pair, var.symbol, val, &cenv.pair);
    }

    // formals = name
    //         | . name
    //
    // bind (rest) parameter to remaining values
    // extending the closure enviroment
    if (isSymbol(formals)) {
        bindValue(cenv.pair, formals.symbol, vlist, &cenv.pair);
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
