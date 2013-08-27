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

static inline unsigned echo_format(TextBuffer *output, const char *format, ...) {
    const unsigned start = buffer_marker(output);

    char data[200];

    va_list ap;
    va_start(ap, format);

    vsprintf(data, format, ap);

    buffer_add(output, data);

    va_end(ap);

    const unsigned stop = buffer_marker(output);

    if (stop < start) return 0;

    return stop - start;
}

static inline unsigned echo_string(TextBuffer *output, const char* text) {
    if (!text) return 0;

    const unsigned start = buffer_marker(output);

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

 done: {
        const unsigned stop = buffer_marker(output);

        if (stop < start) return 0;

        return stop - start;
    }
}

static inline unsigned echo_type(TextBuffer *output, Node type) {
    const unsigned start = buffer_marker(output);

    if (isSymbol(type)) {
        echo_string(output, (const char *)type.symbol->value);
        goto done;
    }

    if (isIdentical(type, t_opaque)) {
        echo_format(output, "<opaque>");
        goto done;
    }

    if (!fromCtor(type, s_base)) {
        echo_format(output, "<type %p>", type.reference);
        goto done;
    }

    if (!fromCtor(type, s_index)) {
        echo_format(output, "<type-index %p>", type.reference);
        goto done;
    }

    if (!fromCtor(type, s_label)) {
        echo_format(output, "<type-label %p>", type.reference);
        goto done;
    }

    if (isIdentical(type.type->sort, zero_s)) {
        echo_format(output, "<%s %s>",
                    type_ConstantName(type.type),
                    (const char*)(type.type->sort->name->value));
        goto done;
    }

    if (isIdentical(type.type->sort, void_s)) {
        echo_format(output, "<%s %s>",
                    type_ConstantName(type.type),
                    (const char*)(type.type->sort->name->value));
        goto done;
    }

    echo_format(output, "<type %p %s %s>",
                type.reference,
                type_ConstantName(type.type),
                (const char*)(type.type->sort->name->value));


 done: {
        const unsigned stop = buffer_marker(output);

        if (stop < start) return 0;

        return stop - start;
    }
}

extern bool buffer_print(TextBuffer *output, Node node) {
    if (!output) return false;

    if (!node.reference) {
        buffer_add(output, "nil");
        return true;
    }

    if (isIdentical(node, true_v)) {
        buffer_add(output, "true");
        return true;
    }
    if (isIdentical(node, void_v)) {
        buffer_add(output, "void");
        return true;
    }

    Node type = getType(node);
    Node ctor = getConstructor(node);

    if (isNil(type) && isNil(ctor)) {
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

    if (isIdentical(ctor, s_symbol)) {
        buffer_add(output, (const char *) node.symbol->value);
        return true;
    }

    if (isIdentical(ctor, s_text)) {
        buffer_add(output, (const char *) node.text->value);
        return true;
    }

    if (isIdentical(ctor, s_integer)) {
        echo_format(output, "%lld",
                    node.integer->value);
        return true;
    }

    if (isIdentical(ctor, s_primitive)) {
        buffer_add(output,"@");
        buffer_add(output, (const char *) node.primitive->label->value);
        return true;
    }

    if (isIdentical(ctor, s_sort)) {
        echo_format(output, "sort(%p ", node.reference);
        buffer_add(output, (const char*)(node.sort->name->value));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(ctor, s_base)) {
        echo_format(output, "type(%p ", node.reference);
        buffer_add(output, type_ConstantName(node.type));
        buffer_add(output, " ");
        buffer_add(output, type_SortName(node.type));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(ctor, s_index)) {
        echo_format(output, "type(%p ", node.reference);
        echo_format(output, "%u : %p ",
                    type_IndexOffset(node.type),
                    type_IndexSlot(node.type));
        buffer_add(output, type_SortName(node.type));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(ctor, s_label)) {
        echo_format(output, "type(%p ", node.reference);
        buffer_add(output, type_LabelName(node.type));
        echo_format(output, " : %p ", type_LabelSlot(node.type));
        buffer_add(output, type_SortName(node.type));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(ctor, s_pair)) {
        echo_format(output, "pair(%p (%p, %p))",
                    node.reference,
                    node.pair->car.reference,
                    node.pair->cdr.reference);
        return true;
    }

    if (isIdentical(ctor, s_tuple)) {
        echo_format(output, "tuple(%p (size=%d))",
                    node.reference,
                    (unsigned) asKind(node.reference)->count);
        return true;
    }

    if (isIdentical(ctor, s_fixed)) {
        const unsigned max = asKind(node)->count;
        unsigned inx = 0;
        buffer_add(output, "{");
        buffer_print(output, node.tuple->item[0]);
        buffer_add(output, "}");
        return true;
    }

    buffer_add(output, "[");
    echo_type(output,type);

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

    if (isIdentical(node, true_v)) {
        buffer_add(output, "true");
        return true;
    }

    if (isIdentical(node, void_v)) {
        buffer_add(output, "void");
        return true;
    }

    Node type = getType(node);
    Node ctor = getConstructor(node);

    if (isNil(type) && isNil(ctor)) {
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

    if (isIdentical(ctor, s_symbol)) {
        echo_format(output, "symbol(%llx,", node.symbol->hashcode);
        echo_string(output, (const char *) node.symbol->value);
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(ctor, s_text)) {
        echo_format(output, "text(%llx,\"", node.text->hashcode);
        echo_string(output, (const char *) node.text->value);
        buffer_add(output, "\")");
        return true;
    }

    if (isIdentical(ctor, s_integer)) {
        echo_format(output, "integer(%lld)", node.integer->value);
        return true;
    }

    if (isIdentical(ctor, s_primitive)) {
        buffer_add(output, "@");
        buffer_add(output, (const char*)(node.primitive->label->value));
        return true;
    }

    if (isIdentical(ctor, s_sort)) {
        echo_format(output, "sort(%p ", node.reference);
        buffer_add(output, (const char*)(node.sort->name->value));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(ctor, s_base)) {
        echo_format(output, "type(%p ", node.reference);
        buffer_add(output, type_ConstantName(node.type));
        buffer_add(output, " ");
        buffer_add(output, type_SortName(node.type));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(ctor, s_index)) {
        echo_format(output, "type(%p ", node.reference);
        echo_format(output, "%u : %p ",
                    type_IndexOffset(node.type),
                    type_IndexSlot(node.type));
        buffer_add(output, type_SortName(node.type));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(ctor, s_label)) {
        echo_format(output, "type(%p ", node.reference);
        buffer_add(output, type_LabelName(node.type));
        echo_format(output, " : %p ", type_LabelSlot(node.type));
        buffer_add(output, type_SortName(node.type));
        buffer_add(output, ")");
        return true;
    }

    if (isIdentical(ctor, s_pair)) {
        echo_format(output, "pair(%p (%p, %p))",
                    node.reference,
                    node.pair->car.reference,
                    node.pair->cdr.reference);
        return true;
    }

    if (isIdentical(ctor, s_tuple)) {
        echo_format(output, "tuple(%p (size=%d))",
                    node.reference,
                    (unsigned) asKind(node)->count);
        return true;
    }

    if (isIdentical(ctor, s_fixed)) {
        const unsigned max = asKind(node)->count;
        unsigned inx = 0;
        buffer_add(output, "{");
        buffer_dump(output, node.tuple->item[0]);
        buffer_add(output, "}");
        return true;
    }

    buffer_add(output, "[");
    echo_type(output, type);

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

    if (isPair(node)) {
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

        if (isIdentical(node, true_v)) {
            offset += 4;
            buffer_add(output, "true");
            return;
        }

        if (isIdentical(node, void_v)) {
            offset += 4;
            buffer_add(output, "void");
            return;
        }

        Node type = getType(node);
        Node ctor = getConstructor(node);

        if (isNil(type) && isNil(ctor)) {
            if (isAtomic(node)) {
                offset += echo_format(output, "unknown(%p)", node.reference);
            } else {
                offset += echo_format(output, "unknown(%p (size=%d))",
                                      node.reference,
                                      (unsigned) asKind(node)->count);
            }
            return;
        }

        if (isIdentical(ctor, s_symbol)) {
            offset += echo_string(output, (const char *) node.symbol->value);
            return;
        }

        if (isIdentical(ctor, s_text)) {
            offset += node.text->size + 3;
            buffer_add(output, "\"");
            echo_string(output, (const char *) node.text->value);
            buffer_add(output, "\"");
            return;
        }

        if (isIdentical(ctor, s_integer)) {
            offset += echo_format(output, "%lld", node.integer->value);
            return;
        }

        if (isIdentical(ctor, s_primitive)) {
            offset += 20;
            buffer_add(output, "@");
            buffer_add(output, (const char*)(node.primitive->label->value));
            return;
        }

        if (isIdentical(ctor, s_sort)) {
            offset += 20;
            echo_format(output, "sort(%p ", node.reference);
            buffer_add(output, (const char*)(node.sort->name->value));
            buffer_add(output, ")");
            return;
        }

        if (isIdentical(ctor, s_base)) {
            offset += 20;
            echo_format(output, "type(%p ", node.reference);
            buffer_add(output, type_ConstantName(node.type));
            buffer_add(output, " ");
            buffer_add(output, type_SortName(node.type));
            buffer_add(output, ")");
            return;
        }

        if (isIdentical(ctor, s_index)) {
            offset += 20;
            echo_format(output, "type(%p ", node.reference);
            echo_format(output, "%u : %p ",
                        type_IndexOffset(node.type),
                        type_IndexSlot(node.type));
            buffer_add(output, type_SortName(node.type));
            buffer_add(output, ")");
            return;
        }

        if (isIdentical(ctor, s_label)) {
            offset += 20;
            echo_format(output, "type(%p ", node.reference);
            buffer_add(output, type_LabelName(node.type));
            echo_format(output, " : %p ",
                        type_LabelSlot(node.type));
            buffer_add(output, type_SortName(node.type));
            buffer_add(output, ")");
            return;
        }

        if (isIdentical(ctor, s_pair)) {
            Node tail = node.pair->cdr;
            buffer_add(output, "(");
            prettyPrint_intern(node.pair->car, level+1);
            while (tail.reference) {
                if (isPair(tail)) {
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

        if (isIdentical(ctor, s_tuple)) {
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

        if (isIdentical(ctor, s_fixed)) {
            const unsigned max = asKind(node)->count;
            unsigned inx = 0;
            buffer_add(output, "{");
            prettyPrint_intern(node.tuple->item[0], level+1);
            buffer_add(output, "}");
            return;
        }

        buffer_add(output, "[");
        offset += echo_type(output, type);

        /*
        if (isSymbol(type)) {
            offset += type.symbol->size + 1;
            echo_string(output, (const char *)type.symbol->value);
        } else {
            if (isIdentical(type, t_opaque)) {
                offset += 10;
                echo_format(output, "<opaque>");
            } else if (!fromCtor(type, s_base)) {
                offset += 10;
                echo_format(output, "<type %p>", type.reference);
            } else if (isIdentical(type.type->sort, zero_s)) {
                offset += 20;
                buffer_add(output, "<");
                buffer_add(output, type_ConstantName(type.type));
                buffer_add(output, " ");
                buffer_add(output, (const char*)(type.type->sort->name->value));
                buffer_add(output, ">");
            } else if (isIdentical(type.type->sort, void_s)) {
                offset += 20;
                buffer_add(output, "<");
                buffer_add(output, type_ConstantName(type.type));
                buffer_add(output, " ");
                buffer_add(output, (const char*)(type.type->sort->name->value));
                buffer_add(output, ">");
            } else {
                offset += 20;
                echo_format(output, "<type %p ", type.reference);
                buffer_add(output, type_ConstantName(type.type));
                buffer_add(output, " ");
                buffer_add(output, (const char*)(type.type->sort->name->value));
                buffer_add(output, ">");
            }
        }
        */

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
