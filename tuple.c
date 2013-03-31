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
#include "symbol.h"
#include "type.h"

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

    unsigned inx = 0;
    for (; inx < size ;++inx) {
        result->item[inx] = NIL;
    }

    return true;
}

extern bool tuple_SetItem(Tuple tuple, unsigned index, const Node value) {
    if (!tuple) return false;

    Header header = asHeader(tuple);

    if (header->kind.atom)           return false;
    if (index >= header->kind.count) return false;

    darken_Node(tuple);
    darken_Node(value);

    tuple->item[index] = value;
    return true;
}

extern bool tuple_GetItem(Tuple tuple, unsigned index, Target value) {
    if (!tuple) return false;

    Header header = asHeader(tuple);

    if (header->kind.atom) return false;

    if (index >= header->kind.count) {
      ASSIGN(value, NIL);
      return false;
    }

    Node item = tuple->item[index];

    darken_Node(tuple);
    darken_Node(item);

    ASSIGN(value, item);

    return true;
}

extern bool tuple_Fill(Tuple tuple, Pair list) {
    if (!tuple) return false;

    Header header = asHeader(tuple);

    if (header->kind.atom) return false;

    darken_Node(tuple);

    unsigned max = header->kind.count;
    unsigned inx = 0;

    for (; inx < max ;++inx) {
        if (!list) return true;
        if (!isType(list, s_pair)) return true;

        Node value = list->car;

        darken_Node(value);

        tuple->item[inx] = value;

        list = list->cdr.pair;
    }

    return true;
}
