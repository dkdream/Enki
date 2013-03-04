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

extern void debug_Message(const char *filename, unsigned int linenum,
                          bool newline, const char *format, ...)
{
    va_list ap; va_start (ap, format);

    printf("file %s line %u :: ", filename, linenum);
    vprintf(format, ap);
    if (newline) printf("\n");
}

extern HashCode node_HashCode(Node node)
{
    if (!node.reference) return 0;

    HashCode result = 0;
    union {
        Node input;
        HashCode output;
    } convert;

    switch (getKind(node)) {
    case nt_symbol:
        result = node.symbol->hashcode;
        break;

    case nt_text:
        result = node.text->hashcode;
        break;

    case nt_integer:
        result = node.integer->value;
        break;

    default:
        convert.output = 0;
        convert.input  = node;
        result ^= convert.output;
        break;
    }

    return result;
}

bool node_Match(Node left, Node right)
{
    if (left.reference == right.reference) return true;
    if (0 == left.reference)  return false;
    if (0 == right.reference) return false;

    EA_Type type = getKind(left);

    if (type != getKind(right)) return false;

    switch (type) {
    case nt_text:
        if (left.text->size != right.text->size) return false;
        return 0 == memcmp(left.text->value,
                           right.text->value,
                           left.text->size);
    case nt_integer:
        return left.integer->value == right.integer->value;

    default:
        return false;
    }
}

bool node_Iso(long depth, Node left, Node right)
{
#if 0
    printf("iso[%ld]: ", depth);
    print(stdout, left);
    printf(" to: ");
    print(stdout, right);
    printf("\n");
#endif

    if (left.reference == right.reference) return true;
    if (0 == left.reference)  return false;
    if (0 == right.reference) return false;

    EA_Type type = getKind(left);

    if (type != getKind(right)) return false;

    if (1 > depth) return true;


    switch (type) {
    case nt_text:
        if (left.text->size != right.text->size) return false;
        return 0 == memcmp(left.text->value,
                           right.text->value,
                           left.text->size);
    case nt_integer:
        return left.integer->value == right.integer->value;

    case nt_pair:
        if (!node_Iso(depth - 1,
                      left.pair->car,
                      right.pair->car)) {
            return false;
        }
        return node_Iso(depth,
                        left.pair->cdr,
                        right.pair->cdr);

    case nt_tuple:
        {
            const unsigned lhs_max = asHeader(left)->count;
            const unsigned rhs_max = asHeader(right)->count;
            unsigned inx = 0;
            if (lhs_max != rhs_max) return false;
            for (; inx < lhs_max ; ++inx) {
                if (!node_Iso(depth - 1,
                              left.tuple->item[inx],
                              right.tuple->item[inx])) {
                    return false;
                }
            }
            return true;
        }

    default:
        return false;
    }
}

extern void node_TypeOf(Node value, Target result)
{
    const char* text = 0;

    if (isNil(value)) {
        goto as_self;
    }

    switch (getKind(value)) {
    case nt_unknown:
        goto as_self;

    case nt_integer:
        text = "integer";
        goto as_symbol;

    case nt_pair:
        text = "pair";
        goto as_symbol;

    case nt_primitive:
        text = "primitive";
        goto as_symbol;

    case nt_symbol:
        text = "symbol";
        goto as_symbol;

    case nt_text:
        text = "text";
        goto as_symbol;

    case nt_tuple:
        text = "tuple";
        goto as_symbol;

    case nt_expression:
        text = "expression";
        goto as_symbol;

    case nt_form:
        text = "form";
        goto as_symbol;

    case nt_fixed:
        text = "fixed";
        goto as_symbol;
    }

 as_self:
    if (isIdentical(true_v, value)) {
        text = "true";
        goto as_symbol;
    }

    ASSIGN(result, value);
    return;

 as_symbol:
    symbol_Convert(text, result.symbol);
    return;
}

/*****************
 ** end of file **
 *****************/
