/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "pair.h"
#include "treadmill.h"
#include "debug.h"
#include "apply.h"
#include "type.h"
#include "tuple.h"
#include <stdarg.h>

extern bool pair_Create(const Node car, const Node cdr, Target target) {
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct pair),
                       target))
        return false;

    Pair result = *(target.pair);

    darken_Node(car);
    darken_Node(cdr);

    setType(result, t_pair);
    setConstructor(result, s_pair);

    result->car = car;
    result->cdr = cdr;

    return true;
}

extern bool variable_Create(const Symbol label,
                            const Node value, const Constant type,
                            Variable* target)
{
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct variable),
                       target))
        return false;

    Variable result = (*target);

    darken_Node(label);
    darken_Node(value);
    darken_Node(type);

    setType(result, type);
    setConstructor(result, s_variable);

    result->label = label;
    result->value = value;
    result->type  = type;

    return true;
}

extern bool pair_SetCar(Pair pair, const Node car) {
    if (!pair) return false;

    darken_Node(car);

    pair->car = car;

    return true;
}

extern bool pair_SetCdr(Pair pair, const Node cdr) {
    if (!pair) return false;

    darken_Node(cdr);

    pair->cdr = cdr;

    return true;
}

extern bool pair_GetCar(Pair pair, Target car) {
    if (!pair) return false;

    ASSIGN(car, pair->car);

    return true;
}

extern bool pair_GetCdr(Pair pair, Target cdr) {
    if (!pair) return false;

    ASSIGN(cdr, pair->cdr);

    return true;
}

extern bool list_Make(Pair* target, ...) {
    va_list ap;
    va_start(ap, target);

    Node value = va_arg(ap, Node);

    if (isNil(value)) {
        fatal("\nerror: list_Make applied to no values");
        return false;
    }

    GC_Begin(6);

    Pair result;
    Pair at;
    Pair next;
    Node tail;

    GC_Protect(result);
    GC_Protect(at);
    GC_Protect(next);
    GC_Protect(tail);

    if (!pair_Create(value, NIL, &result)) goto error;

    for (;;) {
        value = va_arg(ap, Node);

        if (isNil(value)) break;

        tail.pair = result;

        if (!pair_Create(value, tail, &result)) goto error;
    }

    if (isNil(result->cdr)) goto done;

    at          = result->cdr.pair;
    tail.pair   = result;
    result->cdr = NIL;

    for (;;) {
        if (isNil(at->cdr)) {
            at->cdr = tail;
            result  = at;
            break;
        }

        next      = at->cdr.pair;
        at->cdr   = tail;
        tail.pair = at;
        at        = next;
    }

  done:
    ASSIGN(target, result);
    GC_End();
    va_end(ap);
    return true;

  error:
    GC_End();
    va_end(ap);
    return false;
}

extern bool list_State(Pair pair, unsigned *count, bool *dotted) {
    if (!pair) return false;

    unsigned at = 0;
    for (; pair ; ++at) {
        if (!isPair(pair)) {
            *count  = ++at;
            *dotted = true;
            return true;
        }
        pair = pair->cdr.pair;
    }

    *count  = at;
    *dotted = false;
    return true;
}

extern bool list_UnDot(Pair pair) {
    if (!pair) return false;

    for (; pair ;) {
        if (isPair(pair->cdr)) {
            pair = pair->cdr.pair;
            continue;
        }

        return pair_Create(pair->cdr, NIL, &(pair->cdr.pair));
    }

    return true;
}


extern bool list_SetItem(Pair pair, unsigned index, const Node value) {
    if (!pair) return false;

     for (; isPair(pair) ; --index) {
         if (0 < index) {
             pair = pair->cdr.pair;
             continue;
         }

         darken_Node(value);

         pair->car = value;
         return true;
     }

     return false;
}

extern bool list_GetItem(Pair pair, unsigned index, Target value) {
     if (!pair) return false;

     darken_Node(pair);

     for (; isPair(pair) ; --index) {
         if (0 < index) {
             pair = pair->cdr.pair;
             continue;
         }

         ASSIGN(value, pair->car);
         return true;
     }

     return false;
}

extern bool list_SetTail(Pair pair, unsigned index, const Node value) {
    if (!pair) return false;

     for (; isPair(pair) ; --index) {
         if (0 < index) {
             pair = pair->cdr.pair;
             continue;
         }

         darken_Node(value);

         pair->cdr = value;
         return true;
     }

     return false;
}

extern bool list_GetTail(Pair pair, unsigned index, Target value) {
     if (!pair) return false;

    darken_Node(pair);

     for (; isPair(pair) ; --index) {
         if (0 < index) {
             pair = pair->cdr.pair;
             continue;
         }

         ASSIGN(value, pair->cdr);
         return true;
     }

     return false;
}

extern bool list_SetEnd(Pair pair, const Node value) {
     if (!pair) return false;

    darken_Node(pair);

     for (; isPair(pair) ;) {
         if (isPair(pair->cdr)) {
             pair = pair->cdr.pair;
             continue;
         }

         darken_Node(value);

         pair->cdr = value;
         return true;
     }

     return false;
}

extern bool list_GetEnd(Pair pair, Target value) {
     if (!pair) return false;

    darken_Node(pair);

     for (; isPair(pair) ;) {
         if (isPair(pair->cdr)) {
             pair = pair->cdr.pair;
             continue;
         }

         ASSIGN(value, pair->cdr);
         return true;
     }

     return false;
}

extern bool list_Convert(Tuple tuple, Pair* target) {
    if (!tuple) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!isTuple(tuple)) {
        fatal("\nerror: list_Convert applied to a non-tuple");
        return false;
    }

    Kind kind = asKind(tuple);

    const unsigned max = kind->count;

    if (1 > max) {
        ASSIGN(target, NIL);
        return true;
    }

    GC_Begin(7);

    Pair first;
    Pair last;
    Node value;
    Pair hold;

    GC_Protect(first);
    GC_Protect(last);
    GC_Protect(value);
    GC_Protect(hold);

    if (!pair_Create(tuple->item[0],NIL, &first)) goto error;

    last = first;

    unsigned inx = 1;
    for (; inx < max ;++inx) {

        if (!pair_Create(tuple->item[inx],NIL, &hold)) goto error;
        if (!pair_SetCdr(last, hold)) goto error;

        last = hold;
    }

  done:
    ASSIGN(target, first);

    GC_End();
    return true;

 error:
    GC_End();
    return false;


}

extern bool alist_Entry(Pair pair, const Symbol label, Variable* value) {
    if (!pair)  return false;

    for (; isPair(pair) ;) {
        if (!isVariable(pair->car)) {
            pair = pair->cdr.pair;
            continue;
        }

        Variable entry = pair->car.variable;

        if (!isIdentical(label, entry->label)) {
            pair = pair->cdr.pair;
            continue;
        }

        ASSIGN(value, entry);
        return true;
    }

    return false;

}

extern bool alist_Add(Pair pair, const Symbol label, const Node value, const Constant type, Pair* target) {
    GC_Begin(2);

    Variable entry;

    GC_Protect(entry);

    if (!variable_Create(label, value, type, &entry)) {
        fatal("ASSERT unable to create variable: %s", symbol_Text(label));
        goto error;
    }
    if (!pair_Create(entry, pair, target)) {
        fatal("ASSERT unable to add variable: %s", symbol_Text(label));
        goto error;
    }

    GC_End();
    return true;

 error:
    GC_End();
    return false;
}

extern bool list_Map(Pair pair, Operator func, const Node env, Target target) {
    if (!pair) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!func) {
        fatal("\nerror: list_Map applied to a null function");
        return false;
    }

    if (!isPair(pair)) {
        func(pair, env, target);
        return true;
    }

    GC_Begin(7);

    Node first;
    Pair last;
    Node input;
    Node output;
    Pair hold;

    GC_Protect(first);
    GC_Protect(last);
    GC_Protect(input);
    GC_Protect(output);
    GC_Protect(hold);

    input = pair->car;

    func(input, env, &output);

    if (!pair_Create(output,NIL, &first.pair)) goto error;

    last = first.pair;

    for (; isPair(pair->cdr.pair) ;) {
        hold   = 0;
        pair   = pair->cdr.pair;
        input  = pair->car;
        output = NIL;

        func(input, env, &output);

        if (!pair_Create(output,NIL, &hold)) goto error;
        if (!pair_SetCdr(last, hold))        goto error;

        last = hold;
    }

    if (!isNil(pair->cdr.pair)) {
        input  = pair->cdr;
        output = NIL;

        func(input, env, &output);

        if (!pair_SetCdr(last, output)) goto error;
    }

    ASSIGN(target, first);

    GC_End();
    return true;

 error:
    GC_End();
    return false;
}

extern bool list_Filter(Pair pair, Predicate func, const Node env, Pair* target) {
    if (!pair) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!func) {
        fatal("\nerror: list_Filter applied to a null predicate");
        return false;
    }

    if (!isPair(pair)) {
        fatal("\nerror: list_Filter applied to a non-list");
        return false;
    }

    GC_Begin(7);

    Node first;
    Pair last;
    Node input;
    Node output;
    Pair hold;

    GC_Protect(first);
    GC_Protect(last);
    GC_Protect(input);
    GC_Protect(hold);

    input = pair->car;

    for (; !func(input, env) ;) {
        pair = pair->cdr.pair;
        if (isPair(pair)) {
            input = pair->car;
            continue;
        }
        if (isNil(pair)) {
            ASSIGN(target, NIL);
        } else {
            if (func(pair, env)) {
                ASSIGN(target, pair);
            } else {
                ASSIGN(target, NIL);
            }
        }
        goto done;
    }

    if (!pair_Create(input,NIL, &first.pair)) goto error;

    last = first.pair;

    for (; isPair(pair->cdr.pair) ;) {
        hold   = 0;
        pair   = pair->cdr.pair;
        input  = pair->car;

        if (!func(input, env)) continue;

        if (!pair_Create(input,NIL, &hold)) goto error;
        if (!pair_SetCdr(last, hold))       goto error;

        last = hold;
    }

    if (!isNil(pair->cdr.pair)) {
        input = pair->cdr;
        if (func(input, env)) {
            if (!pair_SetCdr(last, output)) goto error;
        }
    }

    ASSIGN(target, first);

 done:
    GC_End();
    return true;

 error:
    GC_End();
    return false;
}

extern bool list_FoldLeft(Pair pair, const Node init, Folder func, const Node env, Target target) {
    if (!pair) {
        ASSIGN(target, init);
        return true;
    }

    GC_Begin(4);

    Node result;
    Node left;
    Node right;

    GC_Protect(result);
    GC_Protect(left);
    GC_Protect(right);

    left  = init;
    right = pair->car;

    if (!func(left, right, env, &result)) goto error;

    for (; isPair(pair->cdr.pair) ;) {
        pair  = pair->cdr.pair;
        left  = result;
        right = pair->car;
        if (!func(left, right, env, &result)) goto error;
    }

    ASSIGN(target, result);
    GC_End();
    return true;

  error:
    GC_End();
    return false;
}

extern bool list_Reverse(Pair pair, Pair* target) {
    if (!pair) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!isPair(pair)) {
        fatal("\nerror: list_Find applied to a non-list");
        return false;
    }

    GC_Begin(4);

    Pair result;
    Node left;
    Node right;

    GC_Protect(result);
    GC_Protect(left);
    GC_Protect(right);

    left = pair->car;

    if (!pair_Create(left, NIL, &result)) goto error;

    for (; isPair(pair->cdr) ;) {
        pair       = pair->cdr.pair;
        left       = pair->car;
        right.pair = result;

        if (!pair_Create(left, right, &result)) goto error;
    }

    if (!isNil(pair->cdr)) {
        fatal("\nerror: list_Reverse applied to a non-proper-list");
        return false;
    }

    ASSIGN(target, result);

    GC_End();
    return true;

  error:
    GC_End();
    return false;
}

extern bool list_Find(Pair pair, Predicate func, const Node env, BitArray *array) {
    if (!pair) return true;

    if (!array) {
        fatal("\nerror: list_Find applied to a null bit array");
        return false;
    }

    if (!func)  {
        fatal("\nerror: list_Find applied to a null predicate");
        return false;
    }

    if (!isPair(pair)) {
        fatal("\nerror: list_Find applied to a non-list");
        return false;
    }

    Node head = pair->car;

    if (func(head,env)) {
        bits_set(array, 0, true);
    }

    unsigned count = 1;

    for (; isPair(pair->cdr) ; ++count) {
        pair = pair->cdr.pair;
        head = pair->car;

        if (func(head,env)) {
            bits_set(array, count, true);
        }
    }

    if (!isNil(pair->cdr)) {
        fatal("\nerror: list_Find applied to a non-proper-list");
        return false;
    }

    return true;
}

extern bool list_Select(Pair pair, BitArray *array, Pair* target) {
    if (!pair) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!array) {
        fatal("\nerror: list_Select applied to a null bit array");
        return false;
    }

    if (!isPair(pair)) {
        fatal("\nerror: list_Select applied to a non-list");
        return false;
    }

    unsigned count = 0;
    int at = bits_ascend(array, -1);

    if (0 > at) {
        ASSIGN(target, NIL);
        return true;
    }

    GC_Begin(7);

    Pair first;
    Pair last;
    Node value;
    Pair hold;

    GC_Protect(first);
    GC_Protect(last);
    GC_Protect(value);
    GC_Protect(hold);

    for (; isPair(pair) ; ++count) {
        if (count < at) {
            pair = pair->cdr.pair;
            continue;
        }

        value = pair->car;

        if (!pair_Create(value,NIL, &first)) goto error;

        break;
    }

    if (!first) {
        ASSIGN(target, NIL);
        goto done;
    }

    last = first;
    at   = bits_ascend(array, at);

    if (0 > at) {
        ASSIGN(target, first);
        goto done;
    }

    for (; isPair(pair) ; ++count) {
        if (count < at) {
            pair = pair->cdr.pair;
            continue;
        }

        value = pair->car;

        if (!pair_Create(value,NIL, &hold)) goto error;
        if (!pair_SetCdr(last, hold))       goto error;

        at = bits_ascend(array, at);

        if (0 > at) break;

        pair = pair->cdr.pair;
    }

    ASSIGN(target, first);

 done:
    GC_End();
    return true;

 error:
    GC_End();
    return false;
}

extern bool list_Update(Pair pair, Operator func, const Node env, BitArray *array) {
    if (!pair) return true;

    if (!func) {
        fatal("\nerror: list_Update applied to a null function");
        return false;
    }

    if (!isPair(pair)) {
        fatal("\nerror: list_Update applied to a non-list");
        return false;
    }

    GC_Begin(3);

    Node input;
    Node output;

    GC_Protect(input);
    GC_Protect(output);

    if (!array) {
        for (; isPair(pair) ;) {
            input = pair->car;

            func(input, env, &output);

            pair->car = output;

            pair = pair->cdr.pair;
        }
    } else {
        unsigned count = 0;
        int at = bits_ascend(array, -1);

        for (; isPair(pair) ; ++count) {
            if (count < at) {
                pair = pair->cdr.pair;
                continue;
            }

            input = pair->car;
            func(input, env, &output);
            pair->car = output;

            at = bits_ascend(array, at);

            if (0 > at) goto done;

            pair = pair->cdr.pair;
        }
    }

 done:
    GC_End();
    return true;

 error:
    GC_End();
    return false;
}

extern bool list_Curry(Tuple tuple, const Operator func, const Node env, BitArray *array, Pair* target) {
    if (!tuple) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!func) {
        fatal("\nerror: list_Curry applied to a null operator");
        return false;
    }

    if (!isTuple(tuple)) {
        fatal("\nerror: list_Curry applied to a non-tuple");
        return false;
    }

    Kind kind = asKind(tuple);

    const unsigned max = kind->count;

    if (1 > max) {
        ASSIGN(target, NIL);
        return true;
    }

    GC_Begin(7);

    Pair first;
    Pair last;
    Node input;
    Node output;
    Pair hold;

    GC_Protect(first);
    GC_Protect(last);
    GC_Protect(input);
    GC_Protect(output);
    GC_Protect(hold);

    if (!array) {
        input = tuple->item[0];

        func(input, env, &output);

        if (!pair_Create(output,NIL, &first)) goto error;

        last = first;

        unsigned inx = 1;
        for (; inx < max ;++inx) {
            input = tuple->item[inx];

            func(input, env, &output);

            if (!pair_Create(output,NIL, &hold)) goto error;
            if (!pair_SetCdr(last, hold))        goto error;

            last = hold;
        }
    } else {
        int at = bits_ascend(array, -1);

        if (0 > at)   goto done;
        if (max < at) goto done;

        input = tuple->item[at];

        func(input, env, &output);

        if (!pair_Create(output,NIL, &first)) goto error;

        last = first;
        at   = bits_ascend(array, at);

        if (0 > at) goto done;

        for (; at < max ;) {
            input = tuple->item[at];

            func(input, env, &output);

            if (!pair_Create(output,NIL, &hold)) goto error;
            if (!pair_SetCdr(last, hold))        goto error;

            last = hold;
            at   = bits_ascend(array, at);

            if (0 > at) goto done;
        }
    }

 done:
    ASSIGN(target, first);

    GC_End();
    return true;

 error:
    GC_End();
    return false;
}

extern bool list_SplitFirst(Pair pair, Predicate func, const Node env, Pair* target) {
    if (!pair) return false;

    darken_Node(pair);

    if (!isPair(pair)) return false;

    for (;;) {
        if (!isPair(pair->cdr)) return false;

        Pair next = pair->cdr.pair;

        if (!func(next->car, env)) {
            pair = next;
            continue;
        }

        pair->cdr = NIL;
        ASSIGN(target, next);
        return true;
    }

    return false;
}

extern bool list_SplitLast(Pair pair, Predicate func, const Node env, Pair* target) {
    if (!pair) return false;

    darken_Node(pair);

    Pair last = ((Pair)0);

    if (!isPair(pair)) return false;

    for (;;) {
        if (!isPair(pair->cdr)) break;

        Pair next = pair->cdr.pair;

        if (func(next->car, env)) {
            last = pair;
        }

        pair = next;
    }

    if (!last) return false;

    {
        Pair next = last->cdr.pair;
        last->cdr = NIL;
        ASSIGN(target, next);
    }

    return true;
}
