/*-*- mode: c;-*-*/
#if !defined(_ea_symbol_list_h_)
#define _ea_symbol_list_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct symbol_list *SymbolList;

/*
 * used only inside of a context
 * never needs scanning
 *
 * this is NOT a gc node
 */
struct symbol_list {
    struct symbol_list* next;
    unsigned int        size;
    unsigned int        used;
    Symbol              value[];
};
extern bool         symbol_list_Allocate(unsigned int size, SymbolList*);
extern unsigned int symbol_list_Count(SymbolList);
extern bool         symbol_list_Copy(SymbolList, SymbolList*);
extern bool         symbol_list_Find(SymbolList, Symbol, unsigned int *);
extern bool         symbol_list_Remove(SymbolList, Symbol, unsigned int *);
extern bool         symbol_list_Add(SymbolList, Symbol, unsigned int *);
extern bool         symbol_list_Element(SymbolList, unsigned int, Symbol*);
extern void         symbol_list_Release(SymbolList);
extern void         symbol_list_Print(FILE*, SymbolList);

/***************************
 ** end of file
 **************************/
#endif
