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

extern bool pair_Create(const Node car, const Node cdr, Pair* target) {
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct pair),
                       0,
                       target))
        return false;

    Pair result = (*target);

    if (!darken_Node(car)) return false;
    if (!darken_Node(cdr)) return false;

    setKind(result, nt_pair);
    result->car = car;
    result->cdr = cdr;

    return true;
}

extern bool pair_SetCar(Pair pair, const Node car) {
    if (!pair) return false;
    if (!darken_Node(car)) return false;

    pair->car = car;

    return true;
}

extern bool pair_SetCdr(Pair pair, const Node cdr) {
    if (!pair) return false;
    if (!darken_Node(cdr)) return false;

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
        if (!isKind(pair, nt_pair)) {
            *count  = at;
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
        if (isKind(pair->cdr, nt_pair)) {
            pair = pair->cdr.pair;
            continue;
        }

        return pair_Create(pair->cdr, NIL, &(pair->cdr.pair));
    }

    return true;
}


extern bool list_SetItem(Pair pair, unsigned index, const Node value) {
    if (!pair) return false;

     for (; isKind(pair, nt_pair) ; --index) {
         if (0 < index) {
             pair = pair->cdr.pair;
             continue;
         }

         if (!darken_Node(value)) return false;

         pair->car = value;
         return true;
     }

     return false;
}

extern bool list_GetItem(Pair pair, unsigned index, Target value) {
     if (!pair) return false;

     for (; isKind(pair, nt_pair) ; --index) {
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

     for (; isKind(pair, nt_pair) ; --index) {
         if (0 < index) {
             pair = pair->cdr.pair;
             continue;
         }

         if (!darken_Node(value)) return false;

         pair->cdr = value;
         return true;
     }

     return false;
}

extern bool list_GetTail(Pair pair, unsigned index, Target value) {
     if (!pair) return false;

     for (; isKind(pair, nt_pair) ; --index) {
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

     for (; isKind(pair, nt_pair) ;) {
         if (isKind(pair->cdr, nt_pair)) {
             pair = pair->cdr.pair;
            continue;
         }

         if (!darken_Node(value)) return false;

         pair->cdr = value;
         return true;
     }

     return false;
}

extern bool list_GetEnd(Pair pair, Target value) {
     if (!pair) return false;

     for (; isKind(pair, nt_pair) ;) {
         if (isKind(pair->cdr, nt_pair)) {
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

    for (; isKind(pair, nt_pair) ;) {
        if (!isKind(pair->car, nt_pair)) {
            pair = pair->cdr.pair;
            continue;
        }

        Pair entry = pair->car.pair;

        if (!isIdentical(label, entry->car)) {
            pair = pair->cdr.pair;
            continue;
        }

        *value = entry;
        return true;
    }

    return false;

}

extern bool alist_Get(Pair pair, const Node label, Target value) {
    if (!pair) return false;

    for (; isKind(pair, nt_pair) ;) {
        if (!isKind(pair->car, nt_pair)) {
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

    for (; isKind(pair, nt_pair) ;) {
        if (!isKind(pair->car, nt_pair)) {
            pair = pair->cdr.pair;
            continue;
        }

        Pair entry = pair->car.pair;

        if (!isIdentical(label, entry->car)) {
            pair = pair->cdr.pair;
            continue;
        }

        if (!darken_Node(value)) return false;

        entry->cdr = value;
        return true;
    }

    return false;
}

extern bool alist_Add(Pair pair, const Node label, const Node value, Pair* target) {
    Pair entry = 0;

    if (!pair_Create(label, value, &entry)) return false;
    if (!pair_Create(entry, pair, target)) return false;

    return true;
}

extern bool list_Map(Operator func, Pair pair, const Node env, Pair* target) {
    if (!pair) {
        *target = 0;
        return true;
    }

    if (!func) {
        fatal("\nerror: list_Map applied to a null function");
        return false;
    }

#if 0
    printf("list_Map: %p", func);
    printf(" to ");
    prettyPrint(stdout, pair);
    printf("\n");
#endif

    if (!isKind(pair, nt_pair)) {
        func(pair, env, target);
        return true;
    }

    Pair first  = 0;
    Pair last   = 0;
    Node input  = NIL;
    Node output = NIL;

    input  = pair->car;
    output = NIL;

    func(input, env, &output);

    if (!pair_Create(output,NIL, &first)) return false;
    if (!pair_SetCar(first, output)) return false;

    last = first;

    for (; isKind(pair->cdr.pair, nt_pair) ;) {
        Pair hold = 0;

        pair   = pair->cdr.pair;
        input  = pair->car;
        output = NIL;

        func(input, env, &output);

        if (!pair_Create(output,NIL, &hold)) return false;
        if (!pair_SetCdr(last, hold))        return false;

        last = hold;
    }

    if (!isNil(pair->cdr.pair)) {
        input  = pair->cdr;
        output = NIL;
        func(input, env, &output);
        if (!pair_SetCdr(last, output)) return false;
    }

#if 0
    printf("list_Map => ");
    prettyPrint(stdout, first);
    printf("\n");
#endif

    *target = first;

    return true;
}
