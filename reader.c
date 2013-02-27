/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/

#include "all_types.inc"
#include "treadmill.h"
#include "text_buffer.h"

/* */
#include <string.h>
#include <stdbool.h>
#include <error.h>
#include <stdarg.h>

static void fatal(const char *reason, ...) __attribute__ ((noreturn format (printf, 1, 2)));
static void fatal(const char *reason, ...)
{
    if (reason) {
        va_list ap;
        va_start(ap, reason);
        fprintf(stderr, "\nerror: ");
        vfprintf(stderr, reason, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    }

#if 0
    if (nil != cdr(backtrace)) {
        oop args= newLong(traceDepth);      GC_PROTECT(args);
        args= newPair(args, nil);
        args= newPair(traceStack, args);
        apply(cdr(backtrace), args, globals);   GC_UNPROTECT(args);
    }
    else {
        int i= traceDepth;
        while (i--) {
            printf("%3d: ", i);
            dumpln(arrayAt(traceStack, i));
        }
    }
#endif

    exit(1);
}


#define GC_PROTECT(var)
#define GC_UNPROTECT(var)

#define CHAR_PRINT    (1<<0)
#define CHAR_BLANK    (1<<1)
#define CHAR_ALPHA    (1<<2)
#define CHAR_DIGIT10  (1<<3)
#define CHAR_DIGIT16  (1<<4)
#define CHAR_LETTER   (1<<5)

static char chartab[]= {
    /*  00 nul */   0,
    /*  01 soh */   0,
    /*  02 stx */   0,
    /*  03 etx */   0,
    /*  04 eot */   0,
    /*  05 enq */   0,
    /*  06 ack */   0,
    /*  07 bel */   0,
    /*  08 bs  */   0,
    /*  09 ht  */   0,
    /*  0a nl  */   CHAR_PRINT | CHAR_BLANK,
    /*  0b vt  */   0,
    /*  0c np  */   CHAR_PRINT | CHAR_BLANK,
    /*  0d cr  */   CHAR_PRINT | CHAR_BLANK,
    /*  0e so  */   0,
    /*  0f si  */   0,
    /*  10 dle */   0,
    /*  11 dc1 */   0,
    /*  12 dc2 */   0,
    /*  13 dc3 */   0,
    /*  14 dc4 */   0,
    /*  15 nak */   0,
    /*  16 syn */   0,
    /*  17 etb */   0,
    /*  18 can */   0,
    /*  19 em  */   0,
    /*  1a sub */   0,
    /*  1b esc */   0,
    /*  1c fs  */   0,
    /*  1d gs  */   0,
    /*  1e rs  */   0,
    /*  1f us  */   0,
    /*  20 sp  */   CHAR_PRINT | CHAR_BLANK,
    /*  21  !  */   CHAR_PRINT | CHAR_LETTER,
    /*  22  "  */   CHAR_PRINT | CHAR_PRINT,
    /*  23  #  */   CHAR_PRINT | CHAR_LETTER,
    /*  24  $  */   CHAR_PRINT | CHAR_LETTER,
    /*  25  %  */   CHAR_PRINT | CHAR_LETTER,
    /*  26  &  */   CHAR_PRINT | CHAR_LETTER,
    /*  27  '  */   CHAR_PRINT,
    /*  28  (  */   CHAR_PRINT,
    /*  29  )  */   CHAR_PRINT,
    /*  2a  *  */   CHAR_PRINT | CHAR_LETTER,
    /*  2b  +  */   CHAR_PRINT | CHAR_LETTER,
    /*  2c  ,  */   CHAR_PRINT | CHAR_LETTER,
    /*  2d  -  */   CHAR_PRINT | CHAR_LETTER,
    /*  2e  .  */   CHAR_PRINT | CHAR_LETTER,
    /*  2f  /  */   CHAR_PRINT | CHAR_LETTER,
    /*  30  0  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  31  1  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  32  2  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  33  3  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  34  4  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  35  5  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  36  6  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  37  7  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  38  8  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  39  9  */   CHAR_PRINT | CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  3a  :  */   CHAR_PRINT | CHAR_LETTER,
    /*  3b  ;  */   CHAR_PRINT,
    /*  3c  <  */   CHAR_PRINT | CHAR_LETTER,
    /*  3d  =  */   CHAR_PRINT | CHAR_LETTER,
    /*  3e  >  */   CHAR_PRINT | CHAR_LETTER,
    /*  3f  ?  */   CHAR_PRINT | CHAR_LETTER,
    /*  40  @  */   CHAR_PRINT | CHAR_LETTER,
    /*  41  A  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  42  B  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  43  C  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  44  D  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  45  E  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  46  F  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  47  G  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  48  H  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  49  I  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  4a  J  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  4b  K  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  4c  L  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  4d  M  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  4e  N  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  4f  O  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  50  P  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  51  Q  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  52  R  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  53  S  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  54  T  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  55  U  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  56  V  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  57  W  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  58  X  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  59  Y  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  5a  Z  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  5b  [  */   CHAR_PRINT,
    /*  5c  \  */   CHAR_PRINT | CHAR_LETTER,
    /*  5d  ]  */   CHAR_PRINT,
    /*  5e  ^  */   CHAR_PRINT | CHAR_LETTER,
    /*  5f  _  */   CHAR_PRINT | CHAR_LETTER,
    /*  60  `  */   CHAR_PRINT,
    /*  61  a  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  62  b  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  63  c  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  64  d  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  65  e  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  66  f  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  67  g  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  68  h  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  69  i  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  6a  j  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  6b  k  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  6c  l  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  6d  m  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  6e  n  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  6f  o  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  70  p  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  71  q  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  72  r  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  73  s  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  74  t  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  75  u  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  76  v  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  77  w  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  78  x  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  79  y  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  7a  z  */   CHAR_PRINT | CHAR_LETTER | CHAR_ALPHA,
    /*  7b  {  */   CHAR_PRINT,
    /*  7c  | */    CHAR_PRINT | CHAR_LETTER,
    /*  7d  }  */   CHAR_PRINT,
    /*  7e  ~  */   CHAR_PRINT | CHAR_LETTER,
    /*  7f del */   0,
};

static inline int isPrint(int c)   { return 0 <= c && c <= 127 && (CHAR_PRINT    & chartab[c]); }
static inline int isAlpha(int c)   { return 0 <= c && c <= 127 && (CHAR_ALPHA    & chartab[c]); }
static inline int isDigit10(int c) { return 0 <= c && c <= 127 && (CHAR_DIGIT10  & chartab[c]); }
static inline int isDigit16(int c) { return 0 <= c && c <= 127 && (CHAR_DIGIT16  & chartab[c]); }
static inline int isLetter(int c)  { return 0 <= c && c <= 127 && (CHAR_LETTER   & chartab[c]); }

static inline int digitValue(int c)
{
    switch (c) {
    case '0' ... '9':  return c - '0';
    case 'A' ... 'Z':  return c - 'A' + 10;
    case 'a' ... 'z':  return c - 'a' + 10;
    }
    fatal("illegal digit in character escape");
    return 0;
}

static inline int isHexadecimal(int c)
{
    switch (c) {
    case '0' ... '9':
    case 'A' ... 'F':
    case 'a' ... 'f':
        return 1;
    }
    return 0;
}

static inline int isOctal(int c)
{
    return '0' <= c && c <= '7';
}

static inline bool matchChar(FILE *fp, int match)
{
    int chr = getc(fp);
    if (match == chr) return true;
    ungetc(chr, fp);
    return false;
}

static int readChar(int chr, FILE *fp)
{
    if ('\\' != chr) return chr;

    chr = getc(fp);

    switch (chr) {
    case 'a':   return '\a';
    case 'b':   return '\b';
    case 'f':   return '\f';
    case 'n':   return '\n';
    case 'r':   return '\r';
    case 't':   return '\t';
    case 'v':   return '\v';
    case '\'':  return '\'';

    case 'u': {
        int ahr = getc(fp),
            bhr = getc(fp),
            chr = getc(fp),
            dhr = getc(fp);
        return (digitValue(ahr) << 24)
            + (digitValue(bhr) << 16)
            + (digitValue(chr) << 8)
            + digitValue(dhr);
    }

    case 'x': {
        if (!isHexadecimal(chr = getc(fp))) {
            ungetc(chr, fp);
            return 0;
        }

        int value = digitValue(chr);

        if (!isHexadecimal(chr = getc(fp))) {
            ungetc(chr, fp);
            return value;
        }

        return value * 16 + digitValue(chr);
    }

    case '0' ... '7': {
        int value = digitValue(chr);
        if (!isOctal(chr = getc(fp))) {
            ungetc(chr, fp);
            return value;
        }

        value = value * 8 + digitValue(chr);

        if (!isOctal(chr = getc(fp))) {
            ungetc(chr, fp);
            return value;
        }
        return value * 8 + digitValue(chr);
    }

    default:
        if (isAlpha(chr))   fatal("illegal character escape: \\%c", chr);
        if (isDigit10(chr)) fatal("illegal character escape: \\%c", chr);
        return chr;
    }
}

extern bool read(FILE *fp, Target result);
static bool readList(FILE *fp, int delim, Target result);
static bool readCode(FILE *fp, Target result);
static bool readInteger(FILE *fp, int first, Target result);
static bool readString(FILE *fp, int end, Target result);
static bool readQuote(FILE *fp, Node symbol, Target result);
static bool readSymbol(FILE *fp, int first, Target result);
static bool readComment(FILE *fp);

static bool readList(FILE *fp, int delim, Target result)
{
    const char *error = 0;
    Pair  head = 0;
    Pair  tail = head;
    Node  hold = NIL;

    GC_PROTECT(head);
    GC_PROTECT(hold);

    if (!read(fp, &(hold.reference))) goto eof;

    if (!pair_Create(hold, NIL, result.pair)) goto failure;

    head = *result.pair;
    tail = head;

    for (;;) {
        if (!read(fp, &(hold.reference))) goto eof;

        if (!isIdentical(hold.symbol, s_dot)) {
            if (!pair_Create(hold, NIL, &(hold.pair))) goto failure;
            if (!pair_SetCdr(tail, hold)) goto failure;
            tail = hold.pair;
            continue;
        }

        if (!read(fp, &(hold.reference))) {
            error = "missing item after .";
            goto failure;
        }

        if (!pair_SetCdr(tail, hold)) goto failure;

        if (!read(fp, &(hold.reference))) goto eof;

        error = "extra item after .";
        goto failure;
    }

    eof:
    if (!matchChar(fp, delim)) {
        error = "EOF while reading list";
        goto failure;
    }

    printf("read list %p\n", head);

    GC_UNPROTECT(hold);
    GC_UNPROTECT(head);
    return true;

    failure:
    if (!error) {
        fatal(error);
    }

    GC_UNPROTECT(hold);
    GC_UNPROTECT(head);
    return false;
}

static bool readCode(FILE *fp, Target result)
{
    int chr = getc(fp);

    if (EOF == chr) return false;

    chr = readChar(chr, fp);

    return integer_Create(chr, result.integer);
}

static bool readString(FILE *fp, int end, Target result)
{
    static TextBuffer buf = BUFFER_INITIALISER;
    buffer_reset(&buf);

    for (;;) {
        int chr = getc(fp);

        if (end == chr) break;
        if (EOF == chr) goto failure;

        chr = readChar(chr, fp);

        buffer_append(&buf, chr);
    }

    printf("read string %s\n", buffer_contents(&buf));

    return text_Create(buf, result.text);

    failure:
    fatal("EOF in string literal");
    return false;
}

static bool readInteger(FILE *fp, int first, Target result)
{
    int chr = first;

    static TextBuffer buf= BUFFER_INITIALISER;
    buffer_reset(&buf);

    do {
        buffer_append(&buf, chr);
        chr = getc(fp);
    } while (isDigit10(chr));

    if (('x' == chr) && (1 == buf.position)) {
        do {
            buffer_append(&buf, chr);
            chr = getc(fp);
        } while (isDigit16(chr));
    }

    ungetc(chr, fp);

    printf("read integer %s\n", buffer_contents(&buf));

    long value = strtoul(buffer_contents(&buf), 0, 0);

    return integer_Create(value, result.integer);
}

static bool readQuote(FILE *fp, Node symbol, Target result)
{
    Node hold;
    GC_PROTECT(hold);

    if (!read(fp, &(hold.reference))) goto failure;
    if (!pair_Create(hold, NIL, &hold.pair)) goto failure;
    if (!pair_Create(symbol, hold, result.pair)) goto failure;

    GC_UNPROTECT(hold);
    return true;

    failure:
    GC_UNPROTECT(hold);
    return false;
}

static bool readSymbol(FILE *fp, int first, Target result)
{
    int chr = first;

    static TextBuffer buf = BUFFER_INITIALISER;
    buffer_reset(&buf);

    if (!isLetter(chr)) goto failure;

    while (isLetter(chr) || isDigit10(chr)) {
        buffer_append(&buf, chr);
        chr = getc(fp);
    }

    ungetc(chr, fp);

    printf("read symbol %s\n", buffer_contents(&buf));

    return symbol_Create(buf, result.symbol);

    failure:
    fatal(isPrint(chr) ? "illegal character: 0x%02x '%c'" : "illegal character: 0x%02x", chr, chr);
    return false;
}

static bool readComment(FILE *fp)
{
    for (;;) {
        int chr = getc(fp);
        if ('\n' == chr || '\r' == chr || EOF == chr) break;
    }
    return true;
}

extern bool read(FILE *fp, Target result)
{
    for (;;) {
        int chr = getc(fp);
        switch (chr) {
        case EOF:  return false;
        case '\t': continue;
        case '\n': continue;
        case '\r': continue;
        case ' ' : continue;
        case '"':  return readString(fp, '"', result);
        case '?':  return readCode(fp, result);
        case '\'': return readQuote(fp, s_quote, result);
        case '`':  return readQuote(fp, s_quasiquote, result);
        case '(':  return readList(fp, ')', result);
        case '[':  return readList(fp, ']', result);
        case '{':  return readList(fp, '}', result);

        case ';':  {
            readComment(fp);
            continue;
        }

        case '0' ... '9': {
            return readInteger(fp, chr, result);
        }

        case ',': {
            if (matchChar(fp, '@')) {
                return readQuote(fp, s_unquote_splicing, result);
            } else {
                return readQuote(fp, s_unquote, result);
            }
        }

        case '}':
        case ']':
        case ')': {
            ungetc(chr, fp);
            return false;
        }

        case '-': {
            int dhr = getc(fp); ungetc(dhr, fp);
            if (isDigit10(dhr)) return readInteger(fp, chr, result);
            else return readSymbol(fp, chr, result);
        }

        default: return readSymbol(fp, chr, result);
        }
    }
}
