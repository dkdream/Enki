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

    setType(result, t_pair);
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

    Node collector;
    Node left;
    Node right;

    GC_Protect(collector);
    GC_Protect(left);
    GC_Protect(right);

    left  = pair->car;
    right = init;

    if (!func(left, right, env, &collector)) goto error;

    for (; isPair(pair->cdr.pair) ;) {
        pair  = pair->cdr.pair;
        left  = pair->car;
        right = collector;
        if (!func(left, right, env, &collector)) goto error;
    }

    ASSIGN(target, collector);
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

    Pair collector;
    Node left;
    Node right;

    GC_Protect(collector);
    GC_Protect(left);
    GC_Protect(right);

    left = pair->car;

    if (!pair_Create(left, NIL, &collector)) goto error;

    for (; isPair(pair->cdr) ;) {
        pair       = pair->cdr.pair;
        left       = pair->car;
        right.pair = collector;

        if (!pair_Create(left, right, &collector)) goto error;
    }

    if (!isNil(pair->cdr)) {
        fatal("\nerror: list_Reverse applied to a non-proper-list");
        return false;
    }

    ASSIGN(target, collector);

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
    fatal("\nerror: list_Select coded yet");
    return false;
}

extern bool list_Update(Pair pair, Operator func, const Node env, BitArray *array) {
    fatal("\nerror: list_Update coded yet");
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



