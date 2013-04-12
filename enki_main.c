/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "reference.h"
#include "reader.h"
#include "dump.h"
#include "apply.h"
#include "treadmill.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static void usage(char *name)
{
    fprintf(stderr, "usage: %s [<option>...] [<file>...]\n", name);
    fprintf(stderr, "where <option> can be\n");
    fprintf(stderr, "  -h          print this help information\n");
    fprintf(stderr, "  -o <ofile>  write output to <ofile>\n");
    fprintf(stderr, "  -v          be verbose\n");
    fprintf(stderr, "if no <file> is given, input is read from stdin\n");
    fprintf(stderr, "if no <ofile> is given, output is written to stdout\n");
    exit(1);
}


int main(int argc, char **argv)
{
    int chr;
    while (-1 != (chr = getopt(argc, argv, "hvro:t")))
        {
            switch (chr)
                {
                case 'h':
                    usage(argv[0]);
                    break;

                case 'v':
                    ++ea_global_debug ;
                    break;

                case 't':
                    ++ea_global_trace;
                    break;

#if 0
                case 'o':
                    if (!(output = fopen(optarg, "w")))
                        {
                            perror(optarg);
                            exit(1);
                        }
                    break;
#endif

                default:
                    fprintf(stderr, "for usage try: %s -h\n", argv[0]);
                    exit(1);
                }
        }

    startEnkiLibrary();

    argc -= optind;
    argv += optind;

    if (!argc) {
        readFile(stdin);
    } else {
        for (; argc; --argc, ++argv) {
            const char* arg = *argv;
            if (!strcmp(arg, "-")) {
                readFile(stdin);
            } else {
                FILE* input = 0;
                if (!(input = fopen(arg, "r"))) {
                    fprintf(stderr, "unable to open %s for reading\n", arg);
                    exit(1);
                } else {
                    fprintf(stderr, "reading %s\n", arg);
                    fflush(stderr);
                    readFile(input);
                    fclose(input);
                    input = 0;
                }
            }
        }
    }

    stopEnkiLibrary();

    return 0;
}



/*****************
 ** end of file **
 *****************/

