/*-*- mode: c;-*-*/
#ifndef _ea_id_set_h_
#define _ea_id_set_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include <stdlib.h>
#include <string.h>

typedef unsigned long long  ID;
typedef struct id_set_cell* ID_cell;
typedef struct id_set_set   ID_set;

struct id_set_cell {
  ID      id;
  ID_cell next;
};

struct id_set_set {
  long size;
  ID_cell  free;
  ID_cell* columns;
};

#define SET_INITIALISER { 0, 0, 0 }
#define SET_COLUMNS     210

static inline void set_reset(ID_set *set) __attribute__((always_inline nonnull(1)));
static inline void set_reset(ID_set *set) {
    long inx = set->size;

    for (; 0 < inx-- ;) {
        if (!set->columns[inx]) continue;

        ID_cell entry = set->columns[inx];
        ID_cell next = entry->next;

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

static inline void set_init(ID_set *set, long size) __attribute__((always_inline nonnull(1)));
static inline void set_init(ID_set *set, long size) {
    set->size    = 0;
    set->free    = 0;
    set->columns = 0;

    if (0 >= size) return;

    long fullsize = sizeof(ID_cell) * size;

    set->columns = malloc(fullsize);

    if (!set->columns) return;

    set->size = size;

    memset(set->columns, 0, fullsize);
}

static inline int set_add(ID_set *set, ID id) __attribute__((always_inline nonnull(1)));
static inline int set_add(ID_set *set, ID id) {
    if (!set->columns) return -1;

    const long inx = (id % set->size);

    ID_cell entry = set->columns[inx];

    while (entry) {
        if (entry->id == id) return 0;
        entry = entry->next;
    }

    if (!set->free) {
        entry = malloc(sizeof(struct id_set_cell));
    } else {
        entry = set->free;
        set->free = entry->next;
    }

    entry->id   = id;
    entry->next = set->columns[inx];

    set->columns[inx] = entry;

    return 1;
}

static inline bool set_find(ID_set *set, ID id) __attribute__((always_inline nonnull(1)));
static inline bool set_find(ID_set *set, ID id) {
    if (0 >= set->size) return false;

    const long inx = (id % set->size);

    ID_cell entry = set->columns[inx];

    while (entry) {
        if (entry->id == id) {
            return true;
        }
        entry = entry->next;
    }

    return false;
}

static inline bool set_remove(ID_set *set, ID id) __attribute__((always_inline nonnull(1)));
static inline bool set_remove(ID_set *set, ID id) {
    if (0 >= set->size) return false;

    const long inx = (id % set->size);

    ID_cell entry  = set->columns[inx];
    ID_cell before = 0;

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
