/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "debug.h"
#include "apply.h"

/* */
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <execinfo.h>
/* */

static void dump_c_stack()
{
    void *array[100];
    size_t size;
    char **strings;
    size_t i;

    size    = backtrace (array, 100);
    strings = backtrace_symbols (array, size);

    fprintf(stderr, "Obtained %zd stack frames.\n", size);

    for (i = 0; i < size; i++)
        fprintf(stderr, "%s\n", strings[i]);

    free (strings);
}

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
    va_list ap; va_start (ap, format);

    fflush(stdout);
    fprintf(stderr, "file %s line %u :: ", filename, linenum);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");

    dump_enki_stack();
    dump_c_stack();

    exit(1);
}

// print error message followed be oop stacktrace
extern void fatal(const char *reason, ...)
{
    fflush(stdout);

    if (reason) {
        va_list ap;
        va_start(ap, reason);
        fprintf(stderr, "\nerror: ");
        vfprintf(stderr, reason, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    }

    dump_enki_stack();
    dump_c_stack();

    fprintf(stderr, "\n");
    exit(1);
}

extern void boom() {
    fatal("BOOM");
}

/*****************
 ** end of file **
 *****************/

