/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/

#include "all_types.inc"
#include "treadmill.h"

/* */
#include <string.h>
#include <stdbool.h>
#include <error.h>

static bool __initialized = false;

unsigned int ea_global_debug = 0;
Space     _zero_space = 0;
Primitive _enclose = 0;
Primitive _apply = 0;
Primitive _closure = 0;
Primitive _hash = 0;
Primitive _set = 0;
Primitive _entry = 0;
Primitive _context = 0;
Primitive _one_or_more = 0;
Primitive _add = 0;
Primitive _depth = 0;
Primitive _contract = 0;
Primitive _drop = 0;

Symbol s_dot = 0;
Symbol s_quasiquote = 0;
Symbol s_quote = 0;
Symbol s_unquote = 0;
Symbol s_unquote_splicing = 0;

void fill_Symbol(const char* value, Symbol* result) {
    TextBuffer hold = BUFFER_INITIALISER;

    hold.buffer   = (char*)value;
    hold.position = strlen(value);

    symbol_Create(hold, result);
}

void startEnkiLibrary() {
    if (__initialized) return;

    init_global_symboltable();

    fill_Symbol(".", &s_dot);
    fill_Symbol("quasiquote", &s_quasiquote);
    fill_Symbol("quote", &s_quote);
    fill_Symbol("unquote", &s_unquote);
    fill_Symbol("unquote_splicing", &s_unquote_splicing);
}

void stopEnkiLibrary() {
    if (!__initialized) return;
}
