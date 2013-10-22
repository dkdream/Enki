/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "tuple.h"
#include "pair.h"
#include "treadmill.h"
#include "type.h"
#include "bit_array.h"

#include <stdarg.h>

extern bool tuple_Create(unsigned size, Tuple* target) {
    if (1 > size) return false;

    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(Node) * size,
                       target))
        return false;

    Tuple result = (*target);

    setType(result, t_tuple);
    setConstructor(result, s_tuple);

    unsigned inx = 0;
    for (; inx < size ;++inx) {
        result->item[inx] = NIL;
    }

    return true;
}

extern bool tuple_SetItem(Tuple tuple, unsigned index, const Node value) {
    if (!tuple) return false;

    Kind kind = asKind(tuple);

    if (!kind)                return false;
    if (kind->atom)           return false;
    if (index >= kind->count) return false;

    darken_Node(value);

    tuple->item[index] = value;
    return true;
}

extern bool tuple_GetItem(Tuple tuple, unsigned index, Target value) {
    if (!tuple) return false;

    Kind kind = asKind(tuple);

    if (!kind)      return false;
    if (kind->atom) return false;

    if (index >= kind->count) {
        ASSIGN(value, NIL);
        return false;
    }

    Node item = tuple->item[index];

    ASSIGN(value, item);

    return true;
}

extern bool tuple_Fill(Tuple tuple, Pair list) {
    if (!tuple) return false;

    Kind kind = asKind(tuple);

    if (!kind)      return false;
    if (kind->atom) return false;

    unsigned max = kind->count;
    unsigned inx = 0;

    for (; inx < max ;++inx) {
        if (!list) return true;
        if (!isPair(list)) return true;

        Node value = list->car;

        darken_Node(value);

        tuple->item[inx] = value;

        list = list->cdr.pair;
    }

    return true;
}

extern bool tuple_Convert(Pair list, Tuple* target) {
    if (!list) return false;

    unsigned count  = 0;
    bool     dotted = false;

    if (!list_State(list, &count, &dotted)) return false;

    if (1 > count) return false;

    Tuple result;

    if (!tuple_Create(count,&result)) return false;
    if (!tuple_Fill(result,list)) return false;

    *target = result;

    return true;
}

extern bool tuple_Map(Tuple tuple, Operator func, const Node env, Target target) {
    if (!tuple) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!func) {
        fatal("\nerror: tuple_Map applied to a null function");
        return false;
    }

    if (!isTuple(tuple)) {
        func(tuple, env, target);
        return true;
    }

    Kind kind = asKind(tuple);

    if (!kind)      return false;
    if (kind->atom) return false;

    const unsigned count = kind->count;

    GC_Begin(3);

    Tuple result;
    Node  value;

    GC_Protect(result);
    GC_Protect(value);

    if (!tuple_Create(count, &result)) goto error;

    unsigned inx = 0;

    for (; inx < count ; ++inx) {
        Node item = tuple->item[inx];

        func(item, env, &value);

        result->item[inx] = value;
    }

    ASSIGN(target, result);

    GC_End();
    return true;

  error:
    GC_End();
    return false;
}

extern bool tuple_Filter(Tuple tuple, Selector func, const Node env, Target target) {
    if (!tuple) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!func) {
        fatal("\nerror: tuple_Filter applied to a null selector");
        return false;
    }

    if (!isTuple(tuple)) {
        fatal("\nerror: tuple_Filter applied to a non-tuple");
        return false;
    }

    Kind kind = asKind(tuple);

    BitArray array;

    bits_init(&array);

    unsigned max   = kind->count;
    unsigned count = 0;
    unsigned inx   = 0;

    for (; inx < max ;++inx) {
        Node value = tuple->item[inx];
        if (func(inx, value, env)) {
            ++count;
            bits_set(&array, inx, true);
        }
    }

    if (0 >= count) {
        ASSIGN(target, NIL);
        return true;
    }

    GC_Begin(3);

    Tuple result;
    Node  value;

    GC_Protect(result);
    GC_Protect(value);

    if (!tuple_Create(count, &result)) goto error;

    int      at  = bits_walk(&array, -1);
    unsigned jnx = 0;

    for (;; ++jnx) {
        if (0 > at) break;
        result->item[jnx] = tuple->item[at];
        at = bits_walk(&array, at);
    }

    ASSIGN(target, result);

    GC_End();
    bits_free(&array);
    return true;

  error:
    GC_End();
    bits_free(&array);
    return false;
}

extern bool tuple_Section(Tuple tuple, unsigned start, unsigned end, Tuple* target) {
    if (!tuple) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!isTuple(tuple)) {
        fatal("\nerror: tuple_Section applied to a non-tuple");
        return false;
    }

    Kind    kind = asKind(tuple);
    unsigned max = kind->count;

    if (max <= start) {
        ASSIGN(target, NIL);
        return true;
    }

    if (end > start) {
        if (end < max) {
            ++end;
        } else {
            end = max;
        }
    } else {
        if (end > max) {
            ASSIGN(target, NIL);
            return true;
        }
        end = max - end;
        if (end <= start) {
            ASSIGN(target, NIL);
            return true;
        }
    }

    unsigned count = end - start;

    GC_Begin(3);

    Tuple result;

    GC_Protect(result);

    if (!tuple_Create(count, &result)) goto error;

    unsigned inx = 0;

    for (; inx < count ; ++inx, ++start) {
        result->item[inx] = tuple->item[start];
    }

    ASSIGN(target, result);
    GC_End();
    return true;

 error:
    GC_End();
    return true;
}

extern bool tuple_FoldLeft(Tuple tuple, const Node init, Flexor func, const Node env, Target target) {
    fatal("\nerror: tuple_FoldLeft coded yet");
    return false;
}

extern bool tuple_FoldRight(Tuple tuple, const Node init, Flexor func, const Node env, Target target) {
    fatal("\nerror: tuple_FoldRight coded yet");
    return false;
}

extern bool tuple_Reverse(Tuple tuple, Tuple* target) {
    if (!tuple) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!isTuple(tuple)) {
        fatal("\nerror: tuple_Reverse applied to a non-tuple");
        return false;
    }

    Kind    kind = asKind(tuple);
    unsigned max = kind->count;

    GC_Begin(3);

    Tuple result;

    GC_Protect(result);

    if (!tuple_Create(max, &result)) goto error;

    if (max < 1) goto done;

    if (max < 2) {
        result->item[0] = tuple->item[0];
        goto done;
    }

    unsigned bottom = 0;
    unsigned top    = max - 1;

    for ( ; bottom < top ; ++bottom, --top) {
        result->item[bottom] = tuple->item[top];
        result->item[top]    = tuple->item[bottom];
    }

    if (bottom == top) {
        result->item[bottom] = tuple->item[top];
    }

  done:
    ASSIGN(target, result);
    GC_End();
    return true;

 error:
    GC_End();
    return false;
}

extern bool tuple_Find(Tuple tuple, Selector func, const Node env, BitArray *array) {
    if (!tuple) return true;

    if (!func) {
        fatal("\nerror: tuple_Find applied to a null selector");
        return false;
    }

    if (!isTuple(tuple)) {
        fatal("\nerror: tuple_Find applied to a non-tuple");
        return false;
    }

    Kind kind = asKind(tuple);

    unsigned max   = kind->count;
    unsigned count = 0;
    unsigned inx   = 0;

    for (; inx < max ;++inx) {
        Node value = tuple->item[inx];
        if (func(inx, value, env)) {
            bits_set(array, inx, true);
        }
    }

    return true;
}

extern bool tuple_Select(Tuple tuple, unsigned count, BitArray *array, Target target) {
    if (!tuple) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!array) {
        fatal("\nerror: tuple_Select applied to a null bit array");
        return false;
    }

    if (0 >= count) {
        ASSIGN(target, NIL);
        return true;
    }

    if (!isTuple(tuple)) {
        fatal("\nerror: tuple_Select applied to a non-tuple");
        return false;
    }

    Kind kind = asKind(tuple);

    unsigned max = kind->count;
    unsigned inx = 0;

    GC_Begin(3);

    Tuple result;

    GC_Protect(result);

    if (!tuple_Create(count, &result)) goto error;

    int      at  = bits_walk(array, -1);
    unsigned jnx = 0;

    for (; jnx < count ; ++jnx) {
        if (0 > at) break;
        result->item[jnx] = tuple->item[at];
        at = bits_walk(array, at);
    }

    ASSIGN(target, result);

    GC_End();
    return true;

  error:
    GC_End();
    return false;
}

extern bool tuple_Update(Tuple tuple, Operator func, const Node env, BitArray *array) {
    if (!tuple) return true;

    if (!func) {
        fatal("\nerror: tuple_Update applied to a null function");
        return false;
    }

    if (!isTuple(tuple)) {
        fatal("\nerror: tuple_Update applied to a non-tuple");
        return false;
    }

    Kind    kind = asKind(tuple);
    unsigned max = kind->count;

    GC_Begin(3);

    Node output;

    GC_Protect(output);

    if (!array) {
        unsigned inx = 0;
        for (; inx < max ; ++inx) {
            Node input = tuple->item[inx];

            func(input, env, &output);

            tuple->item[inx] = output;
        }
    } else {
        int at = bits_walk(array, -1);

        for(; at < max ;) {
            if (0 > at) break;

            Node input = tuple->item[at];

            func(input, env, &output);

            tuple->item[at] = output;

            at = bits_walk(array, at);
        }
    }

    GC_End();
    return true;
}
