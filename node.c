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

    case nt_count:
        result ^= node.count->value;
        break;

    default:
        convert.output = 0;
        convert.input  = node;
        result ^= convert.output;
        break;
    }

    return result;
}

bool node_Match(Node left, Node right) {

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

    case nt_count:
        return left.count->value == right.count->value;

    default:
        return false;
    }
}

extern void node_TypeOf(Node value, Target result)
{
    const char* text = 0;

    if (!isNil(value)) {
        goto as_self;
    }

    switch (getKind(value)) {
    case nt_unknown:
        goto as_self;

    case nt_count:
        text = "count";
        goto as_symbol;

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

 as_symbol:
    symbol_Convert(text, result.symbol);
    return;

 as_self:
    ASSIGN(result, value);
    return;
}

extern void debug_Message(const char *filename, unsigned int linenum,
                          bool newline, const char *format, ...)
{
    va_list ap; va_start (ap, format);

    printf("file %s line %u :: ", filename, linenum);
    vprintf(format, ap);
    if (newline) printf("\n");
}

extern void node_PrintFul(FILE* output, Node node) {
    if (!output) return;
    if (!node.reference) {
        fprintf(output, "nil");
        return;
    }

    switch (getKind(node)) {
    case nt_unknown:
        fprintf(output, "unknown(%p)",
                node.reference);
        return;

    case nt_symbol:
        fprintf(output, "symbol(%s)",
                (const char *) node.symbol->value);
        return;

    case nt_text:
        fprintf(output, "text(%s)",
                (const char *) node.text->value);
        return;

    case nt_integer:
        fprintf(output, "integer(%ld)",
                node.integer->value);
        return;

    case nt_count:
        fprintf(output, "count(%u)",
                node.count->value);
        return;

    case nt_primitive:
        fprintf(output, "primitive(%p %s)",
                node.reference,
                (const char*)(node.primitive->label->value));
        return;

    case nt_pair:
        fprintf(output, "pair(%p (%p, %p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
        return;

    case nt_tuple:
        break;

    default:
        break;

#if 0
    case nt_input:
        fprintf(output, "input(%p %u)",
                node.reference,
                node.input->input);
        return;

    case nt_output:
        fprintf(output, "output(%p %u)",
                node.reference,
                node.output->output);
        return;

    case nt_hash_entry:
        if (!(node.hash_entry)) {
            fprintf(output, "nil");
        } else {
            Hash_entry entry = node.hash_entry;
            fprintf(output, "entry_%p[", entry);
            fprintf(output, "%s -> ", symbol_Text(entry->symbol));
            node_Print(output, entry->value);
            fprintf(output, "]");
        }
        return;

    case nt_hash_block:
        fprintf(output, "hash_block(%p (%u, %p))",
                node.reference,
                node.hash_block->size,
                node.hash_block->next);
        return;

    case nt_hash:
        hash_Print(output, node.hash);
        return;

    case nt_set_cell:
        if (!(node.set_cell)) {
            fprintf(output, "nil");
        } else {
            Set_cell at = node.set_cell;
            fprintf(output, "cell_%p[", at);

            for ( ; at ; at = at->rest) {
                fprintf(output, " ");
                node_Print(output, at->first);
            }

            fprintf(output, " ]");
        }
        return;

    case nt_set_block:
        fprintf(output, "set_block(%p (%u, %p))",
                node.reference,
                node.set_block->size,
                node.set_block->next);
        return;

    case nt_set:
        set_Print(output, node.set);
        return;
#endif
    }

    fprintf(output, "type[%d](%p)",
            getKind(node),
            node.reference);
    return;
}

extern void node_PrintTree(FILE* output, unsigned level, Node node) {
    if (!output) return;
    if (!node.reference) {
        fprintf(output, "nil");
        return;
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
        return;

    case nt_symbol:
        fprintf(output, "symbol(%s)",
                (const char *) node.symbol->value);
        return;

    case nt_text:
        fprintf(output, "text(%s)",
                (const char *) node.text->value);
        return;

    case nt_integer:
        fprintf(output, "integer(%ld)",
                node.integer->value);
        return;

    case nt_count:
        fprintf(output, "count(%u)",
                node.count->value);
        return;

    case nt_primitive:
        fprintf(output, "primitive(%p %s)",
                node.reference,
                (const char*)(node.primitive->label->value));
        return;

    case nt_pair:
        fprintf(output, "pair(%p (%p, %p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
        return;

    case nt_tuple:
        break;

    default:
        break;

#if 0
    case nt_input:
        fprintf(output, "input(%p %u)",
                node.reference,
                node.input->input);
        return;

    case nt_output:
        fprintf(output, "output(%p %u)",
                node.reference,
                node.output->output);
        return;

    case nt_hash_entry:
        if (!(node.hash_entry)) {
            fprintf(output, "nil");
        } else {
            Hash_entry entry = node.hash_entry;
            fprintf(output, "entry_%p[", entry);
            fprintf(output, "%s -> ", symbol_Text(entry->symbol));
            node_Print(output, entry->value);
            fprintf(output, "]");
        }
        return;

    case nt_hash_block:
        fprintf(output, "hash_block(%p (%u, %p))",
                node.reference,
                node.hash_block->size,
                node.hash_block->next);
        return;

    case nt_hash:
        hash_Print(output, node.hash);
        return;

    case nt_set_cell:
        fprintf(output, "set_cell");
        open();
        {
            Set_cell at = node.set_cell;
            for ( ; at ; at = at->rest) {
                sub_tree(at->first);
            }
        }
        close();
        return;

    case nt_set_block:
        fprintf(output, "set_block(%p (%u, %p))",
                node.reference,
                node.set_block->size,
                node.set_block->next);
        return;

    case nt_set:
        set_Print(output, node.set);
        return;
#endif
    }

    fprintf(output, "type[%d](%p)",
            getKind(node),
            node.reference);
    return;
}

/*****************
 ** end of file **
 *****************/

