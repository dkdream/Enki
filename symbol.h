/*-*- mode: c;-*-*/
#if !defined(_ea_symbol_h_)
#define _ea_symbol_h_
/***************************
 **
 ** Purpose
 **   Allow fast comparision of text strings
 **
 ** Properties
 **   o. a string that is never collected (gc)
 **   o. if two symbols have the same content (0 == strcmp(A->value,B->value))
 **      then they have the same address (A == B)
 **
 **/
#include "reference.h"
#include "text_buffer.h"

struct symbol {
    // size in char
    unsigned int  size;
    HashCode      hashcode;
    unsigned long value[1];
};

extern Symbol _empty_symbol;
extern Symbol s_dot;
extern Symbol s_global;
extern Symbol s_current;
extern Symbol s_lambda;
extern Symbol s_nil;
extern Symbol s_quasiquote;
extern Symbol s_quote;
extern Symbol s_set;
extern Symbol s_t;
extern Symbol s_true;
extern Symbol s_unquote;
extern Symbol s_unquote_splicing;

extern Symbol s_comma;
extern Symbol s_colon;
extern Symbol s_semi;

// raw type names
extern Symbol s_integer;
extern Symbol s_primitive;
extern Symbol s_symbol;
extern Symbol s_text;
extern Symbol s_tuple;

//
extern Symbol s_pair;
extern Symbol s_lambda;
extern Symbol s_form;
extern Symbol s_fixed;
extern Symbol s_delay;
extern Symbol s_type;
extern Symbol s_sort;
extern Symbol s_forced;

extern bool symbol_Create(TextBuffer value, Symbol*);
extern bool symbol_Convert(const char* value, Symbol*);
extern void check_SymbolTable__(const char*, unsigned);
extern void init_global_symboltable();

/* macros */
extern inline const char* symbol_Text(Symbol) __attribute__((always_inline nonnull(1)));
extern inline const char* symbol_Text(Symbol symbol) {
    if (!symbol) return "";
    return (const char*)(symbol->value);
}


/***************************
 ** end of file
 **************************/
#endif
