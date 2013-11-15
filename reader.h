/*-*- mode: c;-*-*/
#if !defined(_ea_reader_h_)
#define _ea_reader_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include "reference.h"
#include <stdio.h>

typedef struct input_buffer* InputBuffer;
typedef int  (*PullChar)(InputBuffer buffer);
typedef bool (*PushChar)(InputBuffer buffer, int chr);


struct input_buffer
{
    void *data;
    bool interactive;
    unsigned offset;
    PullChar pull;
    PushChar push;
};


extern bool input_FileInit(InputBuffer buffer, FILE *stream);
extern bool input_StringInit(InputBuffer buffer, const char *stream);
extern bool input_Finit(InputBuffer buffer);

extern bool readExpr(InputBuffer stream, Target result);
extern void readFile(InputBuffer stream);

extern bool convertToAst(const char* text, Target result);

/***************************
 ** end of file
 **************************/
#endif
