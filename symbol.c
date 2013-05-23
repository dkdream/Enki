/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "symbol.h"
#include "type.h"
#include "treadmill.h"
#include "debug.h"

/* */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* */
Symbol _empty_symbol = 0;
Symbol s_symbol = 0;
Symbol s_dot = 0;
Symbol s_comma = 0;
Symbol s_colon = 0;
Symbol s_semi  = 0;
Symbol s_btick = 0;
Symbol s_ftick = 0;
Symbol s_qmark = 0;
Symbol s_bslash = 0;
Symbol s_bsat = 0;

Symbol s_axiom = 0;
Symbol s_base = 0;
Symbol s_block = 0;
Symbol s_clink;
Symbol s_current = 0;
Symbol s_delay = 0;
Symbol s_expression = 0;
Symbol s_fixed = 0;
Symbol s_forced = 0;
Symbol s_form = 0;
Symbol s_global = 0;
Symbol s_hashcode;
Symbol s_header = 0;
Symbol s_infile = 0;
Symbol s_integer = 0;
Symbol s_integer;
Symbol s_kind = 0;
Symbol s_lambda = 0;
Symbol s_name = 0;
Symbol s_nil = 0;
Symbol s_node = 0;
Symbol s_opaque = 0;
Symbol s_outfile = 0;
Symbol s_pair = 0;
Symbol s_pair;
Symbol s_pointer = 0;
Symbol s_primitive = 0;
Symbol s_quasiquote = 0;
Symbol s_quote = 0;
Symbol s_rule = 0;
Symbol s_set = 0;
Symbol s_sort = 0;
Symbol s_t = 0;
Symbol s_target;
Symbol s_text = 0;
Symbol s_true = 0;
Symbol s_tuple = 0;
Symbol s_type = 0;
Symbol s_unquote = 0;
Symbol s_unquote_splicing = 0;
Symbol s_unsigned;
Symbol s_word = 0;


struct _internal_SymbolRow {
    unsigned lock;
    Header   first;
};

struct _internal_SymbolTable {
    unsigned lock;
    unsigned size;
    struct _internal_SymbolRow row[1];
};

static struct _internal_SymbolTable *_global_symboltable = 0;

#define MK_SYM(x) symbol_Convert(#x, &s_ ##x)

extern void init_global_symboltable() {
    if (_global_symboltable) return;

    const unsigned int rows = 1000;
    const unsigned int fullsize
        = sizeof(struct _internal_SymbolTable)
        + (sizeof(struct _internal_SymbolRow) * rows);

    struct _internal_SymbolTable * result = malloc(fullsize);

    memset(result, 0, fullsize);

    result->size = rows;

    _global_symboltable = result;

    Header empty_symbol = fresh_atom(0, sizeof(struct symbol));

    _empty_symbol = (Symbol)asReference(empty_symbol);

    MK_SYM(symbol); // must be first;

    empty_symbol->kind.type = (Node)s_symbol;

    symbol_Convert(".", &s_dot);
    symbol_Convert(",", &s_comma);
    symbol_Convert(":", &s_colon);
    symbol_Convert(";", &s_semi);
    symbol_Convert("`", &s_btick);
    symbol_Convert("'", &s_ftick);
    symbol_Convert("?", &s_qmark);
    symbol_Convert("\\", &s_bslash);
    symbol_Convert("\\@", &s_bsat);
    symbol_Convert("unquote-splicing", &s_unquote_splicing);

    MK_SYM(axiom);
    MK_SYM(base);
    MK_SYM(block);
    MK_SYM(clink);
    MK_SYM(current);
    MK_SYM(delay);
    MK_SYM(fixed);
    MK_SYM(forced);
    MK_SYM(form);
    MK_SYM(global);
    MK_SYM(hashcode);
    MK_SYM(header);
    MK_SYM(infile);
    MK_SYM(integer);
    MK_SYM(integer);
    MK_SYM(kind);
    MK_SYM(lambda);
    MK_SYM(lambda);
    MK_SYM(name);
    MK_SYM(nil);
    MK_SYM(node);
    MK_SYM(opaque);
    MK_SYM(outfile);
    MK_SYM(pair);
    MK_SYM(pair);
    MK_SYM(pointer);
    MK_SYM(primitive);
    MK_SYM(quasiquote);
    MK_SYM(quote);
    MK_SYM(rule);
    MK_SYM(set);
    MK_SYM(sort);
    MK_SYM(t);
    MK_SYM(target);
    MK_SYM(text);
    MK_SYM(true);
    MK_SYM(tuple);
    MK_SYM(type);
    MK_SYM(unquote);
    MK_SYM(unsigned);
    MK_SYM(word);

    int row = _global_symboltable->size;

    for ( ; row-- ; ) {
        Header group = _global_symboltable->row[row].first;
        for ( ; group; group = group->after) {
            group->kind.type = (Node)s_symbol;
        }
    }
}



extern void final_global_symboltable() {
    if (0 == _global_symboltable) return;
}

extern void check_SymbolTable__(const char* filename, unsigned line) {
    if (!_global_symboltable) return;

    int row = _global_symboltable->size;

    for ( ; row-- ; ) {
        Header group = _global_symboltable->row[row].first;
        for ( ; group; group = group->after) {
            if (isIdentical(group->kind.type, s_symbol)) continue;
            fprintf(stderr, "%s:%u",filename, line);
            fatal("found a non-symbol in row %d of the symbol-table", row);
        }
    }
}

static inline HashCode hash_full(TextBuffer value) {
    HashCode result = 5381;

    unsigned int length = value.position;
    const char*  begin  = value.buffer;

    for ( ; length-- ; ) {
        int val = begin[length];
        result = ((result << 5) + result) + val;
    }

    return result;
}

extern bool symbol_Create(TextBuffer value, Symbol *target) {
    if (!value.buffer) {
        *target = _empty_symbol;
        return true;
    }

    if (0 == value.position) {
        *target = _empty_symbol;
        return true;
    }

    const unsigned int size = value.position;
    const HashCode hashcode = hash_full(value);

    const int       row = hashcode % _global_symboltable->size;
    const int     cells = size / sizeof(const unsigned long);
    const int remainder = size % sizeof(const unsigned long);

    Header group = _global_symboltable->row[row].first;

    for ( ; group; group = group->after) {
        Symbol test = (Symbol) asReference(group);

        if (test->size != size)         continue;
        if (test->hashcode != hashcode) continue;

        const unsigned long *checking = test->value;
        const unsigned long *end      = checking + cells;
        const unsigned long *source   = (const unsigned long *) value.buffer;

        while (checking < end) {
            if (*checking++ != *source++) goto next;
        }

        int inx = 0;
        for ( ; inx < remainder; ++inx) {
            if ((((const char*)checking)[inx]) != (((const char*)source)[inx])) goto next;
        }

        *target = test;

        return true;
    next:
        continue;
    }

    const unsigned int fullsize = (sizeof(struct symbol) + (size + 5));

    Header entry = fresh_atom(0, fullsize);

    if (!entry) return false;

    Symbol result = (Symbol)asReference(entry);

    if (!boxed_Tag(result)) {
        VM_ERROR("unable to allocate a boxed node %p as a symbol",
                 result);
    }

    entry->kind.type     = (Node)s_symbol;
    entry->kind.constant = 1;
    entry->after         = _global_symboltable->row[row].first;

    result->size     = size;
    result->hashcode = hashcode;

    memcpy(result->value, value.buffer, size);

    ((char*)(result->value))[size] = 0;

    _global_symboltable->row[row].first = entry;

    (*target) = result;

    return true;
}

extern bool symbol_Convert(const char* value, Symbol* target) {
    if (!value) return false;

    TextBuffer data = BUFFER_INITIALISER;
    data.position = strlen(value);
    data.buffer   = (char*)value;

    return symbol_Create(data, target);
}

/*****************
 ** end of file **
 *****************/

