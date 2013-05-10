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
#include <stdarg.h>

static inline void echo_format(TextBuffer *output, const char *format, ...) {
    char data[200];

    va_list ap;
    va_start(ap, format);

    vsprintf(data, format, ap);

    buffer_add(output, data);

    va_end(ap);
}

static inline void echo_string(TextBuffer *output, const char* text) {
    if (!text) return;

    while (*text) {
        int value = *text++;
        switch (value) {
        case '\a':
            buffer_add(output, "\\a");
            continue;
        case '\b':
            buffer_add(output, "\\b");
            continue;
        case '\f':
            buffer_add(output, "\\f");
            continue;
        case '\n':
            buffer_add(output, "\\n");
            continue;
        case '\r':
            buffer_add(output, "\\r");
            continue;
        case '\t':
            buffer_add(output, "\\t");
            continue;
        case '\v':
            buffer_add(output, "\\v");
            continue;
        case '\'':
            buffer_add(output, "\\'");
            continue;
        default: break;
        }

        if (0x20 > value) {
            echo_format(output, "\\%x", value);
            continue;
        }

        if (0x7e < value) {
            echo_format(output, "\\%x", value);
            continue;
        }

        buffer_append(output, value);
    }
}

extern bool buffer_print(TextBuffer *output, Node node) {
    if (!output) return false;

    if (!node.reference) {
        buffer_add(output, "nil");
        return true;
    }

    Node type = getType(node);

    if (isNil(type)) {
        if (isIdentical(node, true_v)) {
            buffer_add(output, "t");
            return true;
        }
        if (isAtomic(node)) {
            echo_format(output, "unknown(%p)",
                        node.reference);
        } else {
            echo_format(output, "unknown(%p (size=%d))",
                        node.reference,
                        (unsigned) asKind(node)->count);
        }
        return true;
    }

    if (isIdentical(type, s_symbol)) {
        buffer_add(output, (const char *) node.symbol->value);
        return true;
    }

    if (isIdentical(type, t_text)) {
        buffer_add(output, (const char *) node.text->value);
        return true;
    }

    if (isIdentical(type, t_integer)) {
        echo_format(output, "%lld",
                    node.integer->value);
        return true;
    }

    if (isIdentical(type, t_primitive)) {
        buffer_add(output,"@");
        buffer_add(output, (const char *) node.primitive->label->value);
        return true;
    }

    if (isIdentical(type, s_sort)) {
        echo_format(output, "sort(%p ", node.reference);
        buffer_add(output, (const char*)(node.sort->name->value));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(type, s_base)) {
        echo_format(output, "type(%p ", node.reference);
        buffer_add(output, (const char*)(node.type->name->value));
        buffer_add(output, " ");
        buffer_add(output, (const char*)(node.type->sort->name->value));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(type, t_pair)) {
        echo_format(output, "pair(%p (%p, %p))",
                    node.reference,
                    node.pair->car.reference,
                    node.pair->cdr.reference);
        return true;
    }

    if (isIdentical(type, t_tuple)) {
        echo_format(output, "tuple(%p (size=%d))",
                    node.reference,
                    (unsigned) asKind(node.reference)->count);
        return true;
    }

    if (isIdentical(type, t_fixed)) {
        const unsigned max = asKind(node)->count;
        unsigned inx = 0;
        buffer_add(output, "{");
        buffer_print(output, node.tuple->item[0]);
        buffer_add(output, "}");
        return;
    }

    buffer_add(output, "[");
    if (isType(type, s_symbol)) {
        echo_string(output, (const char *)type.symbol->value);
    } else {
        if (!isType(type, s_base)) {
            echo_format(output, "<type %p>", type.reference);
        } else if (isIdentical(type.type->sort, zero_s)) {
            echo_format(output, "<%s %s>",
                        (const char*)(type.type->name->value),
                        (const char*)(type.type->sort->name->value));
        } else {
            echo_format(output, "<type %p %s %s>",
                        type.reference,
                        (const char*)(type.type->name->value),
                        (const char*)(type.type->sort->name->value));
        }
    }

    if (isAtomic(node)) {
        echo_format(output, "|(%p)",
                    node.reference);
    } else {
        echo_format(output, "|(size=%d)",
                    (unsigned) asKind(node)->count);
    }
    buffer_add(output, "]");
    return true;
}

extern bool buffer_dump(TextBuffer *output, Node node) {
    if (!output) return false;

    if (!node.reference) {
        buffer_add(output, "nil");
        return true;
    }

    Node type = getType(node);

    if (isNil(type)) {
        if (isIdentical(node, true_v)) {
            buffer_add(output, "t");
            return true;
        }
        if (isAtomic(node)) {
            echo_format(output, "unknown(%p)",
                        node.reference);
        } else {
            echo_format(output, "unknown(%p (size=%d))",
                        node.reference,
                        (unsigned) asKind(node)->count);
        }
        return true;
    }

    if (isIdentical(type, s_symbol)) {
        echo_format(output, "symbol(%llx,", node.symbol->hashcode);
        echo_string(output, (const char *) node.symbol->value);
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(type, t_text)) {
        echo_format(output, "text(%llx,\"", node.text->hashcode);
        echo_string(output, (const char *) node.text->value);
        buffer_add(output, "\")");
        return true;
    }

    if (isIdentical(type, t_integer)) {
        echo_format(output, "integer(%lld)", node.integer->value);
        return true;
    }

    if (isIdentical(type, t_primitive)) {
        buffer_add(output, "@");
        buffer_add(output, (const char*)(node.primitive->label->value));
        return true;
    }

    if (isIdentical(type, s_sort)) {
        echo_format(output, "sort(%p ", node.reference);
        buffer_add(output, (const char*)(node.sort->name->value));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(type, s_base)) {
        echo_format(output, "type(%p ", node.reference);
        buffer_add(output, (const char*)(node.type->name->value));
        buffer_add(output, " ");
        buffer_add(output, (const char*)(node.type->sort->name->value));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(type, t_pair)) {
        echo_format(output, "pair(%p (%p, %p))",
                    node.reference,
                    node.pair->car.reference,
                    node.pair->cdr.reference);
        return true;
    }

    if (isIdentical(type, t_tuple)) {
        echo_format(output, "tuple(%p (size=%d))",
                    node.reference,
                    (unsigned) asKind(node)->count);
        return true;
    }

    if (isIdentical(type, t_fixed)) {
        const unsigned max = asKind(node)->count;
        unsigned inx = 0;
        buffer_add(output, "{");
        buffer_dump(output, node.tuple->item[0]);
        buffer_add(output, "}");
        return true;
    }

    buffer_add(output, "[");
    if (isType(type, s_symbol)) {
        echo_string(output, (const char *)type.symbol->value);
    } else {
        buffer_add(output, "<");
        if (!isType(type, s_base)) {
            echo_format(output, "type %p", type.reference);
        } else if (isIdentical(type.type->sort, zero_s)) {
            buffer_add(output, (const char*)(type.type->name->value));
            buffer_add(output, " ");
            buffer_add(output, (const char*)(type.type->sort->name->value));
        } else {
            echo_format(output, "type %p ", type.reference);
            buffer_add(output, (const char*)(type.type->name->value));
            buffer_add(output, " ");
            buffer_add(output, (const char*)(type.type->sort->name->value));
        }
        buffer_add(output, ">");
    }

    if (isAtomic(node)) {
        echo_format(output, "|(%p)", node.reference);
    } else {
        echo_format(output, "|(size=%d)", (unsigned) asKind(node)->count);
    }
    buffer_add(output, "]");

    return true;
}

extern bool buffer_dumpTree(TextBuffer *output, unsigned level, Node node) {
    if (!output) return false;

    if (!node.reference) {
        buffer_add(output, "nil");
        return true;
    }

    void indent(unsigned to) {
        unsigned count = 0;
        for ( ; count < to ; ++count ) {
            buffer_add(output, "   ");
        }
    }

    void sub_tree(Node value) {
        buffer_add(output, "\n");
        indent(level+1);
        buffer_dumpTree(output, level+1, value);
    }

    if (isType(node, t_pair)) {
        if (!buffer_dump(output,node)) return false;
        sub_tree(node.pair->car);
        sub_tree(node.pair->cdr);
        return true;
    }

    return buffer_dump(output, node);
}


extern void buffer_prettyPrint(TextBuffer *output, Node node) {
    unsigned offset = 0;

    if (!output) return;

    void prettyPrint_intern(Node node, unsigned level) {
        if (!node.reference) {
            buffer_add(output, "nil");
            return;
        }

        void indent() {
            unsigned count = 0;
            for ( ; count < (level + 1) ; ++count ) {
                buffer_add(output, "   ");
            }
        }

        if (offset > 50) {
            buffer_add(output, "\n");
            indent();
            offset = 0;
        }

        Node type = getType(node);

        if (isNil(type)) {
            if (isIdentical(node, true_v)) {
                offset += 2;
                buffer_add(output, "t");
                return;
            }
            offset += 10;
            echo_format(output, "unknown(%p)", node.reference);
            return;
        }

        if (isIdentical(type, s_symbol)) {
            offset += node.symbol->size + 1;
            echo_string(output, (const char *) node.symbol->value);
            return;
        }

        if (isIdentical(type, t_text)) {
            offset += node.text->size + 3;
            buffer_add(output, "\"");
            echo_string(output, (const char *) node.text->value);
            buffer_add(output, "\"");
            return;
        }

        if (isIdentical(type, t_integer)) {
            offset += 10;
            echo_format(output, "%lld", node.integer->value);
            return;
        }

        if (isIdentical(type, t_primitive)) {
            offset += 20;
            buffer_add(output, "@");
            buffer_add(output, (const char*)(node.primitive->label->value));
            return;
        }

        if (isIdentical(type, s_sort)) {
            offset += 20;
            echo_format(output, "sort(%p ", node.reference);
            buffer_add(output, (const char*)(node.sort->name->value));
            buffer_add(output, ")");
            return;
        }

        if (isIdentical(type, s_base)) {
            offset += 20;
            echo_format(output, "type(%p ", node.reference);
            buffer_add(output, (const char*)(node.type->name->value));
            buffer_add(output, " ");
            buffer_add(output, (const char*)(node.type->sort->name->value));
            buffer_add(output, ")");
            return;
        }

        if (isIdentical(type, t_pair)) {
            Node tail = node.pair->cdr;
            buffer_add(output, "(");
            prettyPrint_intern(node.pair->car, level+1);
            while (tail.reference) {
                if (isType(tail, t_pair)) {
                    buffer_add(output, " ");
                    prettyPrint_intern(tail.pair->car, level+1);
                    tail = tail.pair->cdr;
                    continue;
                }
                buffer_add(output, " . ");
                prettyPrint_intern(tail, level+1);
                break;
            }
            buffer_add(output, ")");
            return;
        }

        if (isIdentical(type, t_tuple)) {
            const unsigned max = asKind(node)->count;
            unsigned inx = 0;
            buffer_add(output, "[");
            for (; inx < max ;++inx) {
                if (0 < inx) buffer_add(output, " ");
                prettyPrint_intern(node.tuple->item[inx], level+1);
            }
            buffer_add(output, "]");
            return;
        }

        if (isIdentical(type, t_fixed)) {
            const unsigned max = asKind(node)->count;
            unsigned inx = 0;
            buffer_add(output, "{");
            prettyPrint_intern(node.tuple->item[0], level+1);
            buffer_add(output, "}");
            return;
        }

        buffer_add(output, "[");
        if (isType(type, s_symbol)) {
            offset += type.symbol->size + 1;
            echo_string(output, (const char *)type.symbol->value);
        } else {
            if (!isType(type, s_base)) {
                offset += 10;
                echo_format(output, "<type %p>", type.reference);
            } else if (isIdentical(type.type->sort, zero_s)) {
                offset += 20;
                buffer_add(output, "<");
                buffer_add(output, (const char*)(type.type->name->value));
                buffer_add(output, " ");
                buffer_add(output, (const char*)(type.type->sort->name->value));
                buffer_add(output, ">");
            } else {
                offset += 20;
                echo_format(output, "<type %p a", type.reference);
                buffer_add(output, (const char*)(type.type->name->value));
                buffer_add(output, " ");
                buffer_add(output, (const char*)(type.type->sort->name->value));
                buffer_add(output, ">");
            }
        }

        if (isAtomic(node)) {
            offset += 10;
            echo_format(output, "|(%p)", node.reference);
        } else {
            const unsigned max = asKind(node)->count;
            unsigned inx = 0;
            buffer_add(output, "|");
            for (; inx < max ;++inx) {
                if (0 < inx) buffer_add(output, " ");
                prettyPrint_intern(node.tuple->item[inx], level+1);
            }
        }
        buffer_add(output, "]");
        return;
    }

    prettyPrint_intern(node, 0);
}

extern bool print(FILE *fp, Node result) {
    if (!fp) return false;

    static TextBuffer buffer = BUFFER_INITIALISER;

    buffer_reset(&buffer);

    if (buffer_print(&buffer, result)) {
        fprintf(fp, "%s", buffer_contents(&buffer));
        return true;
    }

    return false;
}

extern bool dump(FILE *fp, Node result) {
    if (!fp) return false;

    static TextBuffer buffer = BUFFER_INITIALISER;

    buffer_reset(&buffer);

    if (buffer_dump(&buffer, result)) {
        fprintf(fp, "%s", buffer_contents(&buffer));
        return true;
    }
    return false;
}

extern void prettyPrint(FILE *fp, Node result) {
    if (!fp) return;

    static TextBuffer buffer = BUFFER_INITIALISER;

    buffer_reset(&buffer);
    buffer_prettyPrint(&buffer, result);

    fprintf(fp, "%s", buffer_contents(&buffer));
}
