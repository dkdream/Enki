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

extern bool print(FILE* output, Node node) {
    if (!output) return false;

    if (!node.reference) {
        fprintf(output, "nil");
        return true;
    }

    switch (getKind(node)) {
    case nt_unknown:
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

    case nt_count:
        fprintf(output, "%u",
                node.count->value);
        return true;

    case nt_primitive:
        fprintf(output, "%s(%p %u)",
                (const char *) node.primitive->label->value,
                node.reference,
                node.primitive->size);
        return true;

    case nt_input:
        fprintf(output, "input(%p %u)",
                node.reference,
                node.input->input);
        return true;

    case nt_output:
        fprintf(output, "output(%p %u)",
                node.reference,
                node.output->output);
        return true;

    case nt_pair:
        fprintf(output, "pair(%p (%p, %p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
    }

    fprintf(output, "type[%d](%p)",
            getKind(node),
            node.reference);
    return true;
}

extern bool prettyPrint(FILE* output, unsigned level, Node node) {
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
    void open() {
        fprintf(output, "(");
    }
    void sub_tree(Node value) {
        fprintf(output, "\n");
        indent(level+1);
        node_PrintTree(output, level+1, value);
    }
    void close() {
        fprintf(output, ")");
    }

    switch (getKind(node)) {
    case nt_unknown:
        fprintf(output, "unknown(%p)",
                node.reference);
        return true;

    case nt_symbol:
        fprintf(output, "symbol(%s)",
                (const char *) node.symbol->value);
        return true;

    case nt_text:
        fprintf(output, "text(%s)",
                (const char *) node.text->value);
        return true;

    case nt_integer:
        fprintf(output, "integer(%ld)",
                node.integer->value);
        return true;

    case nt_count:
        fprintf(output, "count(%u)",
                node.count->value);
        return true;

    case nt_primitive:
        fprintf(output, "primitive(%p %u)",
                node.reference,
                node.primitive->size);
        return true;

    case nt_input:
        fprintf(output, "input(%p %u)",
                node.reference,
                node.input->input);
        return true;

    case nt_output:
        fprintf(output, "output(%p %u)",
                node.reference,
                node.output->output);
        return true;

    case nt_pair:
        fprintf(output, "pair(%p (%p, %p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
        sub_tree(node.pair->car);
        sub_tree(node.pair->cdr);
        return true;
    }

    fprintf(output, "type[%d](%p)",
            getKind(node),
            node.reference);
    return true;
}
