/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "debug.h"

/* */
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <execinfo.h>
/* */

extern void debug_Message(const char *filename, unsigned int linenum, bool newline, const char *format, ...)
{
    va_list ap; va_start (ap, format);

    fprintf(stderr, "file %s line %u :: ", filename, linenum);
    vfprintf(stderr, format, ap);
    if (newline) fprintf(stderr, "\n");
}

extern void error_Message(const char *filename, unsigned int linenum,
                          const char *format, ...)
{
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    va_list ap; va_start (ap, format);

    fflush(stdout);
    fprintf(stderr, "file %s line %u :: ", filename, linenum);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");

    size    = backtrace (array, 100);
    strings = backtrace_symbols (array, size);

    fprintf(stderr, "Obtained %zd stack frames.\n", size);

    for (i = 0; i < size; i++)
        fprintf(stderr, "%s\n", strings[i]);

    free (strings);

    exit(1);
}
/*****************
 ** end of file **
 *****************/

