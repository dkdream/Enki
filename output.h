/*-*- mode: c;-*-*/
#if !defined(_ea_output_h_)
#define _ea_output_h_
/***************************
 **
 ** Purpose
 **   -- external output
 **
 **  note: the descriptor has the close-on-exec flag set
 **/
#include "reference.h"

struct output {
    int output; // unix file descriptor (see man 2 open)
};

extern Output stdout_hg; // standard output
extern Output stderr_hg; // standard error

extern bool output_Create(int, Output*);

/***************************
 ** end of file
 **************************/
#endif
