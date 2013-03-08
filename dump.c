/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/

#include "all_types.inc"
#include "treadmill.h"
#include "text_buffer.h"

/* */
#include <string.h>
#include <stdbool.h>
#include <error.h>

static inline void echo_string(FILE* output, const char* text) {
    if (!text) return;

    while (*text) {
        int value = *text++;
        switch (value) {
        case '\a':
            fprintf(output, "\\a");
            continue;
        case '\b':
            fprintf(output, "\\b");
            continue;
        case '\f':
            fprintf(output, "\\f");
            continue;
        case '\n':
            fprintf(output, "\\n");
            continue;
        case '\r':
            fprintf(output, "\\r");
            continue;
        case '\t':
            fprintf(output, "\\t");
            continue;
        case '\v':
            fprintf(output, "\\v");
            continue;
        case '\'':
            fprintf(output, "\\'");
            continue;
        default: break;
        }

        if (0x20 > value) {
            fprintf(output, "\\%x", value);
            continue;
        }

        if (0x7e < value) {
            fprintf(output, "\\%x", value);
            continue;
        }
        fprintf(output, "%c", value);
    }
}

extern bool print(FILE* output, Node node) {
    if (!output) return false;

    if (!node.reference) {
        fprintf(output, "nil");
        return true;
    }

    switch (getTribe(node)) {
    case nt_unknown:
        if (isIdentical(node, true_v)) {
            fprintf(output, "t");
            return true;
        }
        fprintf(output, "unknown(%p)",
                node.reference);
        return true;

    case nt_symbol:
        fprintf(output, "%s",
                (const char *) node.symbol->value);
        return true;

    case nt_text:
        fprintf(output, "%s",
                (const char *) node.text->value);
        return true;

    case nt_integer:
        fprintf(output, "%ld",
                node.integer->value);
        return true;

    case nt_primitive:
        fprintf(output, "%s(%p)",
                (const char *) node.primitive->label->value,
                node.reference);
        return true;

    case nt_pair:
        fprintf(output, "pair(%p (%p, %p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
        return true;

    case nt_tuple:
        fprintf(output, "tuple(%p (size=%d))",
                node.reference,
                (unsigned) asKind(node.reference)->count);
        return true;

    default:
        break;
    }

    fprintf(output, "type[%d](%p)",
            getTribe(node),
            node.reference);
    return true;
}

extern bool dump(FILE* output, Node node) {
    if (!output) return false;

    if (!node.reference) {
        fprintf(output, "nil");
        return true;
    }

    switch (getTribe(node)) {
    case nt_unknown:
        if (isIdentical(node, true_v)) {
            fprintf(output, "t");
            return true;
        }
        fprintf(output, "unknown(%p)",
                node.reference);
        return true;

    case nt_symbol:
        fprintf(output, "symbol(%llx,", node.symbol->hashcode);
        echo_string(output, (const char *) node.symbol->value);
        fprintf(output, ")");
        return true;

    case nt_text:
        fprintf(output, "text(%llx,\"", node.text->hashcode);
        echo_string(output, (const char *) node.text->value);
        fprintf(output, "\")");
        return true;

    case nt_integer:
        fprintf(output, "integer(%ld)",
                node.integer->value);
        return true;

    case nt_primitive:
        fprintf(output, "primitive(%p %s)",
                node.reference,
                (const char*)(node.primitive->label->value));
        return true;

    case nt_pair:
        fprintf(output, "pair(%p (%p, %p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
        return true;

    case nt_tuple:
        fprintf(output, "tuple(%p (size=%d))",
                node.reference,
                (unsigned) asKind(node.reference)->count);
        return true;

    default:
        break;
    }

    fprintf(output, "type[%d](%p)",
            getTribe(node),
            node.reference);
    return true;
}

extern bool dumpTree(FILE* output, unsigned level, Node node) {
    if (!output) return false;

    if (!node.reference) {
        fprintf(output, "nil");
        return true;
    }

    void indent(unsigned to) {
        unsigned count = 0;
        for ( ; count < to ; ++count ) {
            fprintf(output, "   ");
        }
    }

    void sub_tree(Node value) {
        fprintf(output, "\n");
        indent(level+1);
        dumpTree(output, level+1, value);
    }

    switch (getTribe(node)) {
    default:
        return dump(output, node);

    case nt_pair:
        if (!dump(output,node)) return false;
        sub_tree(node.pair->car);
        sub_tree(node.pair->cdr);
        return true;
    }

    return true;
}


extern void prettyPrint(FILE* output, Node node) {
    unsigned offset = 0;

    if (!output) return;

    void prettyPrint_intern(Node node, unsigned level) {
        if (!node.reference) {
            fprintf(output, "nil");
            return;
        }

        void indent() {
            unsigned count = 0;
            for ( ; count < (level + 1) ; ++count ) {
                fprintf(output, "   ");
            }
        }

        if (offset > 50) {
            fprintf(output, "\n");
            indent();
            offset = 0;
        }

        switch (getTribe(node)) {
        case nt_unknown:
            if (isIdentical(node, true_v)) {
                offset += 2;
                fprintf(output, "t");
                return;
            }
            offset += 10;
            fprintf(output, "unknown(%p)", node.reference);
            return;

        case nt_symbol:
            offset += node.symbol->size + 1;
            echo_string(output, (const char *) node.symbol->value);
            return;

        case nt_text:
            offset += node.text->size + 3;
            fprintf(output, "\"");
            echo_string(output, (const char *) node.text->value);
            fprintf(output, "\"");
            return;

        case nt_integer:
            offset += 10;
            fprintf(output, "%ld", node.integer->value);
            return;

        case nt_primitive:
            offset += 20;
            fprintf(output, "primitive(%p %s)",
                    node.reference,
                    (const char*)(node.primitive->label->value));
            return;

        case nt_pair:
            {
                Node tail = node.pair->cdr;
                fprintf(output, "(");
                prettyPrint_intern(node.pair->car, level+1);
                while (tail.reference) {
                    EA_Type type = getTribe(tail);
                    if (nt_pair == type) {
                        fprintf(output, " ");
                        prettyPrint_intern(tail.pair->car, level+1);
                        tail = tail.pair->cdr;
                        continue;
                    }
                    fprintf(output, " . ");
                    prettyPrint_intern(tail, level+1);
                    break;
                }
                fprintf(output, ")");
                return;
            }

        case nt_tuple:
            {
                const unsigned max = asKind(node)->count;
                unsigned inx = 0;
                fprintf(output, "[");
                for (; inx < max ;++inx) {
                    if (0 < inx) fprintf(output, " ");
                    prettyPrint_intern(node.tuple->item[inx], level+1);
                }
                fprintf(output, "]");
                return;
            }

        case nt_expression:
            offset += 20;
            fprintf(output, "expression(%p)",
                    node.reference);
            return;

        case nt_form:
            offset += 20;
            fprintf(output, "form(%p)",
                    node.reference);
            return;

        case nt_fixed:
            offset += 20;
            fprintf(output, "fixed(%p)",
                    node.reference);
            return;


        default:
            break;
        }

        offset += 20;
        fprintf(output, "tribe[%d](%p)",
                getTribe(node),
                node.reference);
        return;
    }

    prettyPrint_intern(node, 0);

}
