/*-*- mode: c;-*-*/
#if !defined(_ea_hash_h_)
#define _ea_hash_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include "symbol_list.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct hash       *Hash;
typedef struct hash_state *Hash_state;
typedef struct hash_block *Hash_block;
typedef struct hash_entry *Hash_entry;

struct hash_entry {
    Hash_entry  next;
    Symbol      symbol;
    Node        value;
};

struct hash_block {
    Hash_block next; // next block
    Hash_entry list[];
};

struct hash_state {
    unsigned int count;    // number of entries
    unsigned int fullsize; // = sum(block[0..n].size)
};

struct hash {
    Hash_state state;
    Hash_block first;
};

extern bool hash_Create(unsigned, Hash*);
extern bool hash_Find(Hash, Symbol, Node*);
extern bool hash_Change(Hash, Symbol, Node);
extern bool hash_Remove(Hash, Symbol);
extern bool hash_Add(Hash, Symbol, Node);
extern bool hash_Expand(Hash, unsigned);
extern bool hash_Contract(Hash, unsigned);

/* */
extern void hash_Print(FILE*, Hash);

/* marcos */

/***************************
 ** end of file
 **************************/
#endif
