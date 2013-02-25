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

unsigned int ea_global_debug = 0;

extern HashCode node_HashCode(Node node)
{
    if (!node.reference) return 0;

    HashCode result = 0;
    union {
        Node input;
        HashCode output;
    } convert;

    switch (hasType(node)) {
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

    EA_Type type = hasType(left);

    if (type != hasType(right)) return false;

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

extern void node_Print(FILE* output, Node node) {
    if (!output) return;

    if (!node.reference) {
        fprintf(output, "nil");
        return;
    }

    switch (hasType(node)) {
    case nt_unknown:
        fprintf(output, "unknown(0x%p)",
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

    case nt_set_cell:
        fprintf(output, "cell(0x%p (0x%p, 0x%p))",
                node.reference,
                node.set_cell->first.reference,
                node.set_cell->rest);
        return;

    case nt_hash_entry:
        fprintf(output, "entry(0x%p (%s, 0x%p))",
                node.reference,
                (const char *) node.hash_entry->symbol->value,
                node.hash_entry->value.reference);
        return;

    case nt_hash_block:
        fprintf(output, "hash_block(0x%p (%u, 0x%p))",
                node.reference,
                node.hash_block->size,
                node.hash_block->next);
        return;

    case nt_hash:
        fprintf(output, "hash(0x%p %u)",
                node.reference,
                node.hash->fullsize);
        return;

    case nt_set_block:
        fprintf(output, "set_block(0x%p (%u, 0x%p))",
                node.reference,
                node.set_block->size,
                node.set_block->next);
        return;

    case nt_set:
        fprintf(output, "set(0x%p %u)",
                node.reference,
                node.set->fullsize);
        return;

    case nt_primitive:
        fprintf(output, "%s(0x%p %u)",
                (const char *) node.primitive->label->value,
                node.reference,
                node.primitive->size);
        return;

    case nt_input:
        fprintf(output, "input(0x%p %u)",
                node.reference,
                node.input->input);
        return;

    case nt_output:
        fprintf(output, "output(0x%p %u)",
                node.reference,
                node.output->output);
        return;

    case nt_pair:
        fprintf(output, "pair(0x%p (0x%p, 0x%p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
    }

    fprintf(output, "type[%d](0x%p)",
            hasType(node),
            node.reference);
    return;
}

const char* node_type_Name(enum node_type type)
{
    switch (type) {
    case nt_unknown:       return "unknown";
    case nt_count:         return "count";
    case nt_hash:          return "hash";
    case nt_hash_block:    return "hash_block";
    case nt_hash_entry:    return "hash_entry";
    case nt_input:         return "input";
    case nt_integer:       return "integer";
    case nt_output:        return "output";
    case nt_pair:          return "pair";
    case nt_primitive:     return "primitive";
    case nt_set:           return "set";
    case nt_set_block:     return "set_block";
    case nt_set_cell:      return "set_cell";
    case nt_symbol:        return "symbol";
    case nt_text:          return "text";
    }

    return "undefined";
}

unsigned int node_type_FixSize(enum node_type type)
{
    unsigned base = 0;

    switch (type) {
    case nt_unknown:       base = sizeof(struct gc_header);     break;
    case nt_count:         base = sizeof(struct count);         break;
    case nt_hash:          base = sizeof(struct hash);          break;
    case nt_hash_block:    base = sizeof(struct hash_block);    break;
    case nt_hash_entry:    base = sizeof(struct hash_entry);    break;
    case nt_input:         base = sizeof(struct input);         break;
    case nt_integer:       base = sizeof(struct integer);       break;
    case nt_output:        base = sizeof(struct output);        break;
    case nt_pair:          base = sizeof(struct pair);          break;
    case nt_primitive:     base = sizeof(struct primitive);     break;
    case nt_set:           base = sizeof(struct set);           break;
    case nt_set_block:     base = sizeof(struct set_block);     break;
    case nt_set_cell:      base = sizeof(struct set_cell);      break;
    case nt_symbol:        base = sizeof(struct symbol);        break;
    case nt_text:          base = sizeof(struct text);          break;
    }

    return base;
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

    switch (hasType(node)) {
    case nt_unknown:
        fprintf(output, "unknown(0x%p)",
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
        fprintf(output, "hash_block(0x%p (%u, 0x%p))",
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
        fprintf(output, "set_block(0x%p (%u, 0x%p))",
                node.reference,
                node.set_block->size,
                node.set_block->next);
        return;

    case nt_set:
        set_Print(output, node.set);
        return;

    case nt_primitive:
        fprintf(output, "primitive(0x%p %u)",
                node.reference,
                node.primitive->size);
        return;

    case nt_input:
        fprintf(output, "input(0x%p %u)",
                node.reference,
                node.input->input);
        return;

    case nt_output:
        fprintf(output, "output(0x%p %u)",
                node.reference,
                node.output->output);
        return;

    case nt_pair:
        fprintf(output, "pair(0x%p (0x%p, 0x%p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);

    }

    fprintf(output, "type[%d](0x%p)",
            hasType(node),
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

    switch (hasType(node)) {
    case nt_unknown:
        fprintf(output, "unknown(0x%p)",
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
        fprintf(output, "hash_block(0x%p (%u, 0x%p))",
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
        fprintf(output, "set_block(0x%p (%u, 0x%p))",
                node.reference,
                node.set_block->size,
                node.set_block->next);
        return;

    case nt_set:
        set_Print(output, node.set);
        return;

    case nt_primitive:
        fprintf(output, "primitive(0x%p %u)",
                node.reference,
                node.primitive->size);
        return;

    case nt_input:
        fprintf(output, "input(0x%p %u)",
                node.reference,
                node.input->input);
        return;

    case nt_output:
        fprintf(output, "output(0x%p %u)",
                node.reference,
                node.output->output);
        return;

    case nt_pair:
        fprintf(output, "pair(0x%p (0x%p, 0x%p))",
                node.reference,
                node.pair->car.reference,
                node.pair->cdr.reference);
    }

    fprintf(output, "type[%d](0x%p)",
            hasType(node),
            node.reference);
    return;
}

/*****************
 ** end of file **
 *****************/

