/*-*- mode: c;-*-*/
#if !defined(_ea_input_h_)
#define _ea_input_h_
/***************************
 **
 ** Purpose
 **   -- external input
 **
 **  note: the descriptor has the close-on-exec flag set
 **/
#include "reference.h"

struct input {
    int input; // unix file descriptor (see man 2 open)
};

extern Input stdin_hg;  // standard input
extern Input stdctr_hg; // standard control

extern bool input_Create(int, Input*);
extern bool pipe_Create(Input*, Output*); // (see man 2 pipe2)

/***************************
 ** end of file
 **************************/
#endif
