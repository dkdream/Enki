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
#include "symbol.h"
#include "type.h"

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

    Node type = getType(node);

    if (isNil(type)) {
        if (isIdentical(node, true_v)) {
            fprintf(output, "t");
            return true;
        }
        if (isAtomic(node)) {
            fprintf(output, "unknown(%p)",
                    node.reference);
        } else {
            fprintf(output, "unknown(%p (size=%d))",
                    node.reference,
                    (unsigned) asKind(node)->count);
        }
        return true;
    }

    if (isIdentical(type, s_symbol)) {
        fprintf(output, "%s",
                (const char *) node.symbol->value);
        return true;
    }

    if (isIdentical(type, t_text)) {
        fprintf(output, "%s",
                (const char *) node.text->value);
        return true;
    }

    if (isIdentical(type, t_integer)) {
        fprintf(output, "%lld",
                node.integer->value);
        return true;
    }

    if (isIdentical(type, t_primitive)) {
        fprintf(output, "%s(%p)",
                (const char *) node.primitive->label->value,
                node.reference);
        return true;
    }

    if (isIdentical(type, s_sort)) {
        fprintf(output, "sort(%p %s)",
                node.reference,
                (const char*)(node.sort->name->value));
        return true;
    }

    if (isIdentical(type, s_base)) {
        fprintf(output, "type(%p %s %s)",
                node.reference,
                (const char*)(node.type->name->value),
                (const char*)(node.type->sort->name->value));
        return true;
    }

    if (isIdentical(type, s_pair)) {
        fprintf(output, "pair(%p (%p, %p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
        return true;
    }

    if (isIdentical(type, s_tuple)) {
        fprintf(output, "tuple(%p (size=%d))",
                node.reference,
                (unsigned) asKind(node.reference)->count);
        return true;
    }

    if (isType(type, s_symbol)) {
        echo_string(output, (const char *)type.symbol->value);
    } else {
        fprintf(output, "<type %p>", type.reference);
    }

    if (isAtomic(node)) {
        fprintf(output, "(%p)",
                node.reference);
    } else {
        fprintf(output, "(%p (size=%d))",
                node.reference,
                (unsigned) asKind(node)->count);
    }
    return true;
}

extern bool dump(FILE* output, Node node) {
    if (!output) return false;

    if (!node.reference) {
        fprintf(output, "nil");
        return true;
    }

    Node type = getType(node);

    if (isNil(type)) {
        if (isIdentical(node, true_v)) {
            fprintf(output, "t");
            return true;
        }
        if (isAtomic(node)) {
            fprintf(output, "unknown(%p)",
                    node.reference);
        } else {
            fprintf(output, "unknown(%p (size=%d))",
                    node.reference,
                    (unsigned) asKind(node)->count);
        }
        return true;
    }

    if (isIdentical(type, s_symbol)) {
        fprintf(output, "symbol(%llx,", node.symbol->hashcode);
        echo_string(output, (const char *) node.symbol->value);
        fprintf(output, ")");
        return true;
    }

    if (isIdentical(type, t_text)) {
        fprintf(output, "text(%llx,\"", node.text->hashcode);
        echo_string(output, (const char *) node.text->value);
        fprintf(output, "\")");
        return true;
    }

    if (isIdentical(type, t_integer)) {
        fprintf(output, "integer(%lld)",
                node.integer->value);
        return true;
    }

    if (isIdentical(type, t_primitive)) {
        fprintf(output, "primitive(%p %s)",
                node.reference,
                (const char*)(node.primitive->label->value));
        return true;
    }

    if (isIdentical(type, s_sort)) {
        fprintf(output, "sort(%p %s)",
                node.reference,
                (const char*)(node.sort->name->value));
        return true;
    }

    if (isIdentical(type, s_base)) {
        fprintf(output, "type(%p %s %s)",
                node.reference,
                (const char*)(node.type->name->value),
                (const char*)(node.type->sort->name->value));
        return true;
    }

    if (isIdentical(type, s_pair)) {
        fprintf(output, "pair(%p (%p, %p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
        return true;
    }

    if (isIdentical(type, s_tuple)) {
        fprintf(output, "tuple(%p (size=%d))",
                node.reference,
                (unsigned) asKind(node)->count);
        return true;
    }

    if (isType(type, s_symbol)) {
        echo_string(output, (const char *)type.symbol->value);
    } else {
        fprintf(output, "<type %p>", type.reference);
    }
    if (isAtomic(node)) {
        fprintf(output, "(%p)",
                node.reference);
    } else {
        fprintf(output, "(%p (size=%d))",
                node.reference,
                (unsigned) asKind(node)->count);
    }

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

    if (isType(node, s_pair)) {
        if (!dump(output,node)) return false;
        sub_tree(node.pair->car);
        sub_tree(node.pair->cdr);
        return true;
    }

    return dump(output, node);
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

        Node type = getType(node);

        if (isNil(type)) {
            if (isIdentical(node, true_v)) {
                offset += 2;
                fprintf(output, "t");
                return;
            }
            offset += 10;
            fprintf(output, "unknown(%p)", node.reference);
            return;
        }

        if (isIdentical(type, s_symbol)) {
            offset += node.symbol->size + 1;
            echo_string(output, (const char *) node.symbol->value);
            return;
        }

        if (isIdentical(type, t_text)) {
            offset += node.text->size + 3;
            fprintf(output, "\"");
            echo_string(output, (const char *) node.text->value);
            fprintf(output, "\"");
            return;
        }

        if (isIdentical(type, t_integer)) {
            offset += 10;
            fprintf(output, "%lld", node.integer->value);
            return;
        }

        if (isIdentical(type, t_primitive)) {
            offset += 20;
            fprintf(output, "primitive(%p %s)",
                    node.reference,
                    (const char*)(node.primitive->label->value));
            return;
        }

        if (isIdentical(type, s_sort)) {
            offset += 20;
            fprintf(output, "sort(%p %s)",
                    node.reference,
                    (const char*)(node.sort->name->value));
            return;
        }

        if (isIdentical(type, s_base)) {
            offset += 20;
            fprintf(output, "type(%p %s %s)",
                    node.reference,
                    (const char*)(node.type->name->value),
                    (const char*)(node.type->sort->name->value));
            return;
        }

        if (isIdentical(type, s_pair)) {
            Node tail = node.pair->cdr;
            fprintf(output, "(");
            prettyPrint_intern(node.pair->car, level+1);
            while (tail.reference) {
                if (isType(tail, s_pair)) {
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

        if (isIdentical(type, s_tuple)) {
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

        if (isType(type, s_symbol)) {
            offset += type.symbol->size + 1;
            echo_string(output, (const char *)type.symbol->value);
        } else {
            offset += 10;
            fprintf(output, "<type %p>", type.reference);
        }

        if (isAtomic(node)) {
            offset += 10;
            fprintf(output, "(%p)", node.reference);
        } else {
            offset += 20;
            fprintf(output, "(%p (size=%d))",
                    node.reference,
                    (unsigned) asKind(node)->count);
        }
        return;
    }

    prettyPrint_intern(node, 0);

}
