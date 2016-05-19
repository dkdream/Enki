/*-*- mode: c;-*-*/
#if !defined(_link_enki_h_)
#define _link_enki_h_
/***************************
 **
 ** Link Enki
 **
 ** Purpose
 **
 ** Coding Invariant:
 **
 **
 ** ----------------------------------------------------------------
 **
 **
 ***************************
 **
 **/
#include <limits.h>

typedef unsigned long ptr;
typedef int bool;

#define BITS_PER_WORD (sizeof(long) * CHAR_BIT)
#define WORD_SIZE     (sizeof(long))
#define POINTER_SIZE  (sizeof(void*))
#define PTR_SIZE      (sizeof(ptr))

#define BOX_TAG_MASK 0x03
#define BOX_TAG      0x00
#define BOX_NIL      0x00

#define FIX_TAG_MASK 0x03
#define FIX_TAG      0x01
#define FIX_SHIFT    2

#define BOOL_TAG_MASK 0x03
#define BOOL_TAG      0x02
#define BOOL_TRUE     0xf2
#define BOOL_FALSE    0x02


extern ptr enki_entry();

static inline bool isNil(ptr)  __attribute__((always_inline));
static inline bool isNil(ptr val) {
  return 0 == val;
}

static inline bool isVoid(ptr)  __attribute__((always_inline));
static inline bool isVoid(ptr val) {
  return -1 == val;
}

static inline bool isBoxed(ptr)  __attribute__((always_inline));
static inline bool isBoxed(ptr val) {
  return ((val & BOX_TAG_MASK) == BOX_TAG);
}

static inline bool isFixed(ptr)  __attribute__((always_inline));
static inline bool isFixed(ptr val) {
  return ((val & FIX_TAG_MASK) == FIX_TAG);
}

static inline bool isBool(ptr)  __attribute__((always_inline));
static inline bool isBool(ptr val) {
  if ((val & BOOL_TAG_MASK) == BOOL_TAG)
    return -1;
  else
    return 0;
}

static inline int fromFixed(ptr)  __attribute__((always_inline));
static inline int fromFixed(ptr val) {
  return ((int)val) >> FIX_SHIFT;
}

static inline ptr toFixed(int)  __attribute__((always_inline));
static inline ptr toFixed(int val) {
  return ((((ptr) val) << FIX_SHIFT) | FIX_TAG);
}

static inline bool fromBool(ptr) __attribute__((always_inline));
static inline bool fromBool(ptr val) {
  return ((val & BOOL_TRUE) == BOOL_TRUE);
}

static inline ptr toBool(bool) __attribute__((always_inline));
static inline ptr toBool(bool val) {
  if (val) return BOOL_TRUE;
  return BOOL_FALSE;
}

/***************************
 ** end of file
 **************************/
#endif
