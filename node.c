/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "node.h"
#include "all_types.inc"
#include "treadmill.h"
#include "dump.h"

/* */
#include <stdbool.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
/* */

extern HashCode node_HashCode(Node node)
{
    if (!node.reference) return 0;

    Node type = getType(node);

    if (isIdentical(type, s_symbol)) {
        return node.symbol->hashcode;
    }

    if (isIdentical(type, t_text)) {
        return node.text->hashcode;
    }

    if (isIdentical(type, t_integer)) {
        return node.integer->value;
    }

    HashCode result = 0;
    union {
        Node input;
        HashCode output;
    } convert;

    convert.output = 0;
    convert.input  = node;
    result ^= convert.output;

    return result;
}

bool node_Match(Node left, Node right)
{
    if (left.reference == right.reference) return true;
    if (0 == left.reference)  return false;
    if (0 == right.reference) return false;

    Node type = getType(left);

    if (!isIdentical(type, getType(right))) return false;

    if (isIdentical(type, t_text)) {
        if (left.text->size != right.text->size) return false;
        return 0 == memcmp(left.text->value,
                           right.text->value,
                           left.text->size);
    }

    if (isIdentical(type, t_integer)) {
        return left.integer->value == right.integer->value;
    }

    return false;
}

bool node_Iso(long depth, Node left, Node right)
{
#if 0
    fprintf(stderr, "iso[%ld]: (%p,%p) ",
            depth, left.reference, right.reference);
    print(stderr, left);
    fprintf(stderr, " to: ");
    print(stderr, right);
    fprintf(stderr, "\n");
#endif

    if (left.reference == right.reference) return true;
    if (0 == left.reference)  return false;
    if (0 == right.reference) return false;

    Node type = getType(left);

    if (!isIdentical(type, getType(right))) return false;

    if (1 > depth) return true;

    if (isIdentical(type, t_text)) {
        if (left.text->size != right.text->size) return false;
        return 0 == memcmp(left.text->value,
                           right.text->value,
                           left.text->size);
    }

    if (isIdentical(type, t_integer)) {
        return left.integer->value == right.integer->value;
    }

    if (isAtomic(left)) {
      return false;
    }
    if (isAtomic(right)) {
      return false;
    }

    const unsigned lhs_max = asKind(left)->count;
    const unsigned rhs_max = asKind(right)->count;

    if (lhs_max != rhs_max) return false;

    unsigned inx = 0;
    for (; inx < lhs_max ; ++inx) {
      if (!node_Iso(depth - 1,
                    left.tuple->item[inx],
                    right.tuple->item[inx])) {
        return false;
      }
    }

    return true;
}

extern void node_TypeOf(Node value, Target result)
{
    if (isNil(value)) {
        ASSIGN(result, value);
        return;
    }

    Node type = getType(value);

    if (isNil(type)) {
        ASSIGN(result, value);
        return;
    }

    ASSIGN(result, type);
    return;
}

/*****************
 ** end of file **
 *****************/
