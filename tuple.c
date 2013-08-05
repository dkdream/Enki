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

#include <stdarg.h>

extern Type t_tuple;

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
