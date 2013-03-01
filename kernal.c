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

Node enki_globals = NIL;

Primitive f_quote = 0;       // quote a syntax tree
Primitive p_eval_symbol = 0;
Primitive p_eval_pair = 0;
Primitive p_apply_expr = 0;
Primitive p_apply_form = 0;

Symbol s_dot = 0;
Symbol s_quasiquote = 0;
Symbol s_quote = 0;
Symbol s_unquote = 0;
Symbol s_unquote_splicing = 0;
Symbol s_delay = 0;

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
    fill_Symbol("delay", &s_delay);
}

void stopEnkiLibrary() {
    if (!__initialized) return;
}
