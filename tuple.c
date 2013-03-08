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
#include <stdarg.h>

extern bool tuple_Create(unsigned size, Tuple* target) {
    if (1 > size) return false;

    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(Node) * size,
                       0,
                       target))
        return false;

    Tuple result = (*target);

    setKind(result, nt_tuple);

    unsigned inx = 0;
    for (; inx < size ;++inx) {
        result->item[inx] = NIL;
    }

    return true;
}

extern bool tuple_SetItem(Tuple tuple, unsigned index, const Node value) {
    if (!tuple) return false;

    Header header = asHeader(tuple);

    if (header->atom)           return false;
    if (index >= header->count) return false;

    darken_Node(tuple);
    darken_Node(value);

    if (header->prefix) {
        tuple->item[index+1] = value;
    } else {
        tuple->item[index] = value;
    }

    return true;
}

extern bool tuple_GetItem(Tuple tuple, unsigned index, Target value) {
    if (!tuple) return false;

    Header header = asHeader(tuple);

    if (header->atom) return false;

    if (index >= header->count) {
      ASSIGN(value, NIL);
      return false;
    }

    darken_Node(tuple);

    Node item = NIL;

    if (header->prefix) {
        item = tuple->item[index+1];
    } else {
        item = tuple->item[index];
    }

    darken_Node(item);

    ASSIGN(value, item);

    return true;
}

extern bool tuple_Fill(Tuple tuple, Pair list) {
    if (!tuple) return false;

    Header header = asHeader(tuple);

    if (header->atom) return false;

    darken_Node(tuple);

    unsigned max = header->count;
    unsigned inx = 0;

    if (header->prefix) {
        max += 1;
        inx += 1;
    }

    for (; inx < max ;++inx) {
        if (!list) return true;
        if (nt_pair != getKind(list)) return true;

        Node value = list->car;

        darken_Node(value);

        tuple->item[inx] = value;

        list = list->cdr.pair;
    }

    return true;
}
