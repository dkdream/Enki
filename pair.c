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

extern bool pair_Create(const Node car, const Node cdr, Pair* target) {
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct pair),
                       target))
        return false;

    Pair result = (*target);

    darken_Node(car);
    darken_Node(cdr);

    setConstructor(result, s_pair);

    result->car = car;
    result->cdr = cdr;

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

extern bool alist_Entry(Pair pair, const Node label, Pair* value) {
    if (!pair) return false;

    for (; isPair(pair) ;) {
        if (!isPair(pair->car)) {
            pair = pair->cdr.pair;
            continue;
        }

        Pair entry = pair->car.pair;

        if (!isIdentical(label, entry->car)) {
            pair = pair->cdr.pair;
            continue;
        }

        ASSIGN(value, entry);
        return true;
    }

    return false;

}

extern bool alist_Get(Pair pair, const Node label, Target value) {
    if (!pair) return false;

    for (; isPair(pair) ;) {
        if (!isPair(pair->car)) {
            pair = pair->cdr.pair;
            continue;
        }

        Pair entry = pair->car.pair;

        if (!isIdentical(label, entry->car)) {
            pair = pair->cdr.pair;
            continue;
        }

        ASSIGN(value, entry->cdr);
        return true;
    }

    return false;
}

extern bool alist_Set(Pair pair, const Node label, const Node value) {
    if (!pair) return false;

    for (; isPair(pair) ;) {
        if (!isPair(pair->car)) {
            pair = pair->cdr.pair;
            continue;
        }

        Pair entry = pair->car.pair;

        if (!isIdentical(label, entry->car)) {
            pair = pair->cdr.pair;
            continue;
        }

        darken_Node(value);

        entry->cdr = value;
        return true;
    }

    return false;
}

extern bool alist_Add(Pair pair, const Node label, const Node value, Pair* target) {
    GC_Begin(2);

    Pair entry;

    GC_Protect(entry);

    if (!pair_Create(label, value, &entry)) goto error;
    if (!pair_Create(entry, pair, target)) goto error;

    GC_End();
    return true;

 error:
    GC_End();
    return false;
}

extern bool list_Map(Operator func, Pair pair, const Node env, Target target) {
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
