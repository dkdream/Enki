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
extern Symbol s_quasiquote;
extern Symbol s_quote;
extern Symbol s_unquote;
extern Symbol s_unquote_splicing;
extern Symbol s_delay;
extern Symbol s_t;
extern Symbol s_nil;
extern Symbol s_lambda;
extern Symbol s_set;

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
