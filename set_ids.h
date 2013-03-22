/*-*- mode: c;-*-*/
#ifndef _ea_set_ids_h_
#define _ea_set_ids_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include <stdlib.h>
#include <string.h>

struct set_id_cell {
  long long id;
  struct set_id_cell* next;
};

struct set_id_set {
  long size;
  struct set_id_cell*  free;
  struct set_id_cell** columns;
};

typedef struct set_id_set SetIds;

#define SET_INITIALISER { 0, 0, 0 }
#define SET_COLUMNS     210

extern inline void set_reset(SetIds *set) __attribute__((always_inline nonnull(1)));
extern inline void set_reset(SetIds *set) {
  long inx = set->size;
  for (; 0 < inx-- ;) {
    if (!set->columns[inx]) continue;
    struct set_id_cell* entry = set->columns[inx];
    struct set_id_cell* next = entry->next;
    set->columns[inx] = 0;
    
    if (!next) {
      entry->next = set->free;
      set->free = entry;
      continue;
    }
   
    while (next->next) next = next->next;
    
    next->next = set->free;
    set->free = entry;
  }
}

extern inline void set_init(SetIds *set, long size) __attribute__((always_inline nonnull(1)));
extern inline void set_init(SetIds *set, long size) {
  set_reset(set);
  
  if (0 >= set->size) {
    set->columns = calloc(sizeof(struct set_cell*), set->size);
    if (set->columns) {
      set->size = size;    
    }
    return;
  }
    
  if (size <= set->size) return;

  long fullsize = sizeof(struct set_cell*) * size;

  set->columns = realloc(set->columns, fullsize);
  if (!set->columns) {
    set->size = 0;
    return;
  }

  set->size = size;
  memset(set->columns, 0, fullsize);
}

extern inline int set_add(SetIds *set, long long id) __attribute__((always_inline nonnull(1)));
extern inline int set_add(SetIds *set, long long id) {
  if (!set->columns) return -1;

  const long inx = (id % set->size);

  struct set_id_cell* entry = set->columns[inx];
  
  while (entry) {
    if (entry->id == id) return 0;
    entry = entry->next;
  }
 
  if (!set->free) {
    entry = malloc(sizeof(struct set_id_cell));
  } else {
    entry = set->free;
    set->free = entry->next;
  }

  entry->id   = id;
  entry->next = set->columns[inx];
  
  set->columns[inx] = entry;
  
  return 1;
}

extern inline bool set_find(SetIds *set, long long id) __attribute__((always_inline nonnull(1)));
extern inline bool set_find(SetIds *set, long long id) {
  if (0 >= set->size) return false;

  const long inx = (id % set->size);

  struct set_id_cell* entry = set->columns[inx];
  
  while (entry) {
    if (entry->id == id) {
      return true;
    }
    entry = entry->next;
  }

  return false;
}

extern inline bool set_remove(SetIds *set, long long id) __attribute__((always_inline nonnull(1)));
extern inline bool set_remove(SetIds *set, long long id) {
  if (0 >= set->size) return false;

  const long inx = (id % set->size);

  struct set_id_cell* entry  = set->columns[inx];
  struct set_id_cell* before = 0;
  
  if (!entry) return false;
  
  if (entry->id == id) {
    set->columns[inx] = entry->next;
    entry->next = set->free;
    set->free   = entry;
    return true;
  }

  while (entry) {
    if (entry->id == id) break;
    before = entry;
    entry  = entry->next;
  }

  if (entry) {
    before->next = entry->next;
    entry->next = set->free;
    set->free   = entry;
    return true;
  }

  return false;
}

/***************************
 ** end of file
 **************************/
#endif
