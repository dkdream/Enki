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
#include "hashcode.h"
#include "text_buffer.h"

struct symbol {
    HashCode      hashcode;
    // size in char
    unsigned int  size;
    unsigned long value[1];
};

extern Symbol s_dot;
extern Symbol s_comma;
extern Symbol s_colon;
extern Symbol s_semi;
extern Symbol s_btick;
extern Symbol s_ftick;
extern Symbol s_qmark;
extern Symbol s_bslash;
extern Symbol s_bsat;
extern Symbol s_uscore;

extern Symbol s_axiom;
extern Symbol s_base;
extern Symbol s_block;
extern Symbol s_box;
extern Symbol s_branch;
extern Symbol s_clink;
extern Symbol s_current;
extern Symbol s_delay;
extern Symbol s_escape; // escape continuation
extern Symbol s_fixed;
extern Symbol s_forced;
extern Symbol s_form;
extern Symbol s_global;
extern Symbol s_hashcode;
extern Symbol s_header;
extern Symbol s_index;
extern Symbol s_integer;
extern Symbol s_kind;
extern Symbol s_label;
extern Symbol s_lambda;
extern Symbol s_name;
extern Symbol s_nil;
extern Symbol s_node;
extern Symbol s_opaque;
extern Symbol s_pair;
extern Symbol s_path;
extern Symbol s_pointer;
extern Symbol s_primitive;
extern Symbol s_quasiquote;
extern Symbol s_quote;
extern Symbol s_rule;
extern Symbol s_set;
extern Symbol s_sort;
extern Symbol s_symbol;
extern Symbol s_t;
extern Symbol s_target;
extern Symbol s_text;
extern Symbol s_true;
extern Symbol s_tuple;
extern Symbol s_type;
extern Symbol s_union;
extern Symbol s_unquote;
extern Symbol s_unquote_splicing;
extern Symbol s_unsigned;
extern Symbol s_word;

extern bool symbol_Create(TextBuffer value, Symbol*);
extern bool symbol_Convert(const char* value, Symbol*);
extern void check_SymbolTable__(const char*, unsigned);
extern void init_global_symboltable();

/******************
  inline functions
 ******************/

extern inline const char* symbol_Text(Symbol) __attribute__((always_inline nonnull(1)));
extern inline const char* symbol_Text(Symbol symbol) {
    if (!symbol) return "";
    return (const char*)(symbol->value);
}

/***************************
 ** end of file
 **************************/
#endif
