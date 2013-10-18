/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#define debug_THIS

#include "all_types.inc"
#include "treadmill.h"
#include "text_buffer.h"
#include "apply.h"
#include "dump.h"
#include "debug.h"

/* */
#include <string.h>
#include <stdbool.h>
#include <error.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>


#define CHAR_BLANK    (1<<0)
#define CHAR_ALPHA    (1<<1)
#define CHAR_DIGIT10  (1<<2)
#define CHAR_DIGIT16  (1<<3)
#define CHAR_LETTER   (1<<4)
#define CHAR_PREFIX   (1<<5)
#define CHAR_SYMTAX   (1<<6)

static unsigned short chartab[]= {
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
    /*  0a nl  */   CHAR_BLANK,
    /*  0b vt  */   0,
    /*  0c np  */   CHAR_BLANK,
    /*  0d cr  */   CHAR_BLANK,
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
    /*  20 sp  */   CHAR_BLANK,
    /*  21  !  */   CHAR_PREFIX,
    /*  22  "  */   0,
    /*  23  #  */   0,
    /*  24  $  */   CHAR_PREFIX,
    /*  25  %  */   CHAR_PREFIX,
    /*  26  &  */   CHAR_PREFIX,
    /*  27  '  */   0,
    /*  28  (  */   0,
    /*  29  )  */   0,
    /*  2a  *  */   CHAR_PREFIX,
    /*  2b  +  */   CHAR_PREFIX,
    /*  2c  ,  */   0,
    /*  2d  -  */   CHAR_LETTER,
    /*  2e  .  */   CHAR_PREFIX,
    /*  2f  /  */   CHAR_PREFIX,
    /*  30  0  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  31  1  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  32  2  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  33  3  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  34  4  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  35  5  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  36  6  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  37  7  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  38  8  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  39  9  */   CHAR_DIGIT10 | CHAR_DIGIT16,
    /*  3a  :  */   0,
    /*  3b  ;  */   0,
    /*  3c  <  */   CHAR_LETTER,
    /*  3d  =  */   CHAR_LETTER,
    /*  3e  >  */   CHAR_LETTER,
    /*  3f  ?  */   CHAR_LETTER,
    /*  40  @  */   CHAR_LETTER,
    /*  41  A  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  42  B  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  43  C  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  44  D  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  45  E  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  46  F  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  47  G  */   CHAR_LETTER | CHAR_ALPHA,
    /*  48  H  */   CHAR_LETTER | CHAR_ALPHA,
    /*  49  I  */   CHAR_LETTER | CHAR_ALPHA,
    /*  4a  J  */   CHAR_LETTER | CHAR_ALPHA,
    /*  4b  K  */   CHAR_LETTER | CHAR_ALPHA,
    /*  4c  L  */   CHAR_LETTER | CHAR_ALPHA,
    /*  4d  M  */   CHAR_LETTER | CHAR_ALPHA,
    /*  4e  N  */   CHAR_LETTER | CHAR_ALPHA,
    /*  4f  O  */   CHAR_LETTER | CHAR_ALPHA,
    /*  50  P  */   CHAR_LETTER | CHAR_ALPHA,
    /*  51  Q  */   CHAR_LETTER | CHAR_ALPHA,
    /*  52  R  */   CHAR_LETTER | CHAR_ALPHA,
    /*  53  S  */   CHAR_LETTER | CHAR_ALPHA,
    /*  54  T  */   CHAR_LETTER | CHAR_ALPHA,
    /*  55  U  */   CHAR_LETTER | CHAR_ALPHA,
    /*  56  V  */   CHAR_LETTER | CHAR_ALPHA,
    /*  57  W  */   CHAR_LETTER | CHAR_ALPHA,
    /*  58  X  */   CHAR_LETTER | CHAR_ALPHA,
    /*  59  Y  */   CHAR_LETTER | CHAR_ALPHA,
    /*  5a  Z  */   CHAR_LETTER | CHAR_ALPHA,
    /*  5b  [  */   0,
    /*  5c  \  */   0,
    /*  5d  ]  */   0,
    /*  5e  ^  */   CHAR_PREFIX,
    /*  5f  _  */   CHAR_LETTER,
    /*  60  `  */   0,
    /*  61  a  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  62  b  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  63  c  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  64  d  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  65  e  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  66  f  */   CHAR_LETTER | CHAR_ALPHA | CHAR_DIGIT16,
    /*  67  g  */   CHAR_LETTER | CHAR_ALPHA,
    /*  68  h  */   CHAR_LETTER | CHAR_ALPHA,
    /*  69  i  */   CHAR_LETTER | CHAR_ALPHA,
    /*  6a  j  */   CHAR_LETTER | CHAR_ALPHA,
    /*  6b  k  */   CHAR_LETTER | CHAR_ALPHA,
    /*  6c  l  */   CHAR_LETTER | CHAR_ALPHA,
    /*  6d  m  */   CHAR_LETTER | CHAR_ALPHA,
    /*  6e  n  */   CHAR_LETTER | CHAR_ALPHA,
    /*  6f  o  */   CHAR_LETTER | CHAR_ALPHA,
    /*  70  p  */   CHAR_LETTER | CHAR_ALPHA,
    /*  71  q  */   CHAR_LETTER | CHAR_ALPHA,
    /*  72  r  */   CHAR_LETTER | CHAR_ALPHA,
    /*  73  s  */   CHAR_LETTER | CHAR_ALPHA,
    /*  74  t  */   CHAR_LETTER | CHAR_ALPHA,
    /*  75  u  */   CHAR_LETTER | CHAR_ALPHA,
    /*  76  v  */   CHAR_LETTER | CHAR_ALPHA,
    /*  77  w  */   CHAR_LETTER | CHAR_ALPHA,
    /*  78  x  */   CHAR_LETTER | CHAR_ALPHA,
    /*  79  y  */   CHAR_LETTER | CHAR_ALPHA,
    /*  7a  z  */   CHAR_LETTER | CHAR_ALPHA,
    /*  7b  {  */   0,
    /*  7c  | */    CHAR_PREFIX,
    /*  7d  }  */   0,
    /*  7e  ~  */   CHAR_PREFIX,
    /*  7f del */   0,
};

/* syntax: ! " # $ % & ' ( ) * + , . / : ; < = > ? [ ] \ ` { } | ^ */
/*
** !  -> PREFIX
** "  -> string
** #  -> comment
** $  -> PREFIX
** %  -> PREFIX
** &  -> PREFIX
** '  -> quote or s_ftick
** (  -> list ')'
** +  -> PREFIX
** ,  -> s_comma
** .  -> PREFIX
** /  -> PREFIX
** :  -> type or s_colon
** ;  -> s_semi
** <  -> PREFIX
** =  -> PREFIX
** >  -> PREFIX
** ?  -> char-code or s_qmark
** [  -> tuple ']'
** \  -> (unquote, \@ -> unquote_splicing) or (s_bslash,  \@ -> s_bsat)
** ^  -> PREFIX
** `  -> quasiquote or s_btick
** {  -> block '}'
** |  -> PREFIX
** ^  -> PREFIX
*/

static inline int isPrint(int c)   { return 0x20 <= c && c <= 0x7e; }
static inline int isAlpha(int c)   { return 0 <= c && c <= 127 && (CHAR_ALPHA    & chartab[c]); }
static inline int isDigit10(int c) { return 0 <= c && c <= 127 && (CHAR_DIGIT10  & chartab[c]); }
static inline int isDigit16(int c) { return 0 <= c && c <= 127 && (CHAR_DIGIT16  & chartab[c]); }
static inline int isLetter(int c)  { return 0 <= c && c <= 127 && (CHAR_LETTER   & chartab[c]); }
static inline int isPrefix(int c)  { return 0 <= c && c <= 127 && (CHAR_PREFIX   & chartab[c]); }
static inline int isBlank(int c)   { return 0 <= c && c <= 127 && (CHAR_BLANK    & chartab[c]); }

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

static inline bool checkChar(FILE *fp, int match)
{
    int chr = getc(fp);
    ungetc(chr, fp);
    return (match == chr);
}

static inline bool matchChar(FILE *fp, int match)
{
    int chr = getc(fp);
    if (match == chr) return true;
    ungetc(chr, fp);
    return false;
}

static inline bool checkBlank(FILE *fp)
{
    int chr = getc(fp);
    ungetc(chr, fp);
    return isBlank(chr);
}

// use by both readCode and readString
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
    case 's':   return ' ';
    case 't':   return '\t';
    case 'v':   return '\v';

    case '\'':  return '\'';
    case '\"':  return '\"';
    case '\\':  return '\\';

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

extern bool readExpr(FILE *fp, Target result);
static bool readSegment(FILE *fp, int delim, Target result);
static bool readList(FILE *fp, int delim, Target result);
static bool readCode(FILE *fp, Target result);
static bool readInteger(FILE *fp, int first, Target result);
static bool readString(FILE *fp, int end, Target result);
static bool readQuote(FILE *fp, Node symbol, Target result);
static bool readSymbol(FILE *fp, int first, Target result);
static bool skipBlock(FILE *fp, const int delim);
static bool skipComment(FILE *fp);
//
//
static bool readSegment(FILE *fp, int delim, Target result)
{
    GC_Begin(7);

    const char *error = 0;
    Pair  head;
    Pair  tail;
    Pair  list;
    Node  hold;

    GC_Protect(head);
    GC_Protect(tail);
    GC_Protect(list);
    GC_Protect(hold);

    if (!readExpr(fp, &(hold.reference))) {
        ASSIGN(result, NIL);
        goto eof;
    }

    if (!pair_Create(hold, NIL, result.pair)) goto failure;

    head = *result.pair;
    tail = head;

    for (;;) {
        if (!readExpr(fp, &(hold.reference)))      goto eof;
        if (!pair_Create(hold, NIL, &(hold.pair))) goto failure;
        if (!pair_SetCdr(tail, hold))              goto failure;
        tail = hold.pair;
    }

 eof:
    if (!checkChar(fp, delim)) {
        error = "EOF while reading segment";
        goto failure;
    }

 done:
    GC_End();
    return true;

 failure:
    if (!error) {
        fatal("%s", error);
    }

    GC_End();
    return false;
}

// (...)
static bool readList(FILE *fp, int delim, Target result)
{
    GC_Begin(7);

    const char *error = 0;
    Pair  head;
    Pair  tail;
    Pair  list;
    Node  hold;

    GC_Protect(head);
    GC_Protect(tail);
    GC_Protect(list);
    GC_Protect(hold);

    if (!readExpr(fp, &(hold.reference))) {
        ASSIGN(result, NIL);
        goto eof;
    }

    if (!pair_Create(hold, NIL, result.pair))    goto failure;
    if (!setType(*(result.reference), t_ASTree)) goto failure;

    head = *result.pair;
    tail = head;

    for (;;) {
        if (!readExpr(fp, &(hold.reference))) goto eof;

        if (isIdentical(hold.symbol, s_dot)) {
            if (!readExpr(fp, &(hold.reference))) {
                error = "missing item after .";
                goto failure;
            }

            if (!pair_SetCdr(tail, hold)) goto failure;

            goto eof;
        }

        if (!pair_Create(hold, NIL, &(hold.pair))) goto failure;
        if (!setType(hold, t_ASTree)) goto failure;
        if (!pair_SetCdr(tail, hold)) goto failure;
        tail = hold.pair;
    }

 eof:
    if (!matchChar(fp, delim)) {
        error = "EOF while reading list";
        goto failure;
    }

 done:
    GC_End();
    return true;

 failure:
    if (!error) {
        fatal("%s", error);
    }

    GC_End();
    return false;
}

static bool matchItem(Node value, Node env) {
    return isIdentical(value, env);
}

// [...] - tuple
// {...} - block
static bool readTuple(FILE *fp, Symbol ctor, int delim, Target result)
{
    GC_Begin(7);

    const char *error = 0;
    unsigned size;
    bool     dotted;

    Pair  head;
    Pair  tail;
    Pair  list;
    Node  hold;

    GC_Protect(head);
    GC_Protect(tail);
    GC_Protect(list);
    GC_Protect(hold);

    if (!readSegment(fp, delim, &list)) {
        ASSIGN(result, NIL);
        goto failure;
    }

    if (!matchChar(fp, delim)) {
        error = "EOF while reading tuple";
        goto failure;
    }

    for (;list_SplitLast(matchItem, list, s_dot, &tail);) {
        Pair next = tail;

        if (!isPair(next->cdr)) {
            error = "invalid dot while reading tuple";
            goto failure;
        }

        Pair segment = next->cdr.pair;

        next->cdr = NIL;

        if (!tuple_Convert(segment, &(next->car.tuple))) goto failure;

        if (!list_SetEnd(list, next))         goto failure;
        if (!setConstructor(next->car, ctor)) goto failure;
        if (!setType(next->car, t_ASTree))    goto failure;
    }

 eof:
    if (!tuple_Convert(list, result.tuple))         goto failure;
    if (!setConstructor(*(result.reference), ctor)) goto failure;
    if (!setType(*(result.reference), t_ASTree))    goto failure;

 done:
    GC_End();
    return true;

 failure:
    if (!error) {
        fatal("%s", error);
    }

    GC_End();
    return false;
}

// ?xxx
static bool readCode(FILE *fp, Target result)
{
    int chr = getc(fp);

    if (EOF == chr) return false;

    chr = readChar(chr, fp);

    return integer_Create(chr, result.integer);
}

// "..."
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

    return text_Create(buf, result.text);

    failure:
    fatal("EOF in string literal");
    return false;
}

// -?[0-9]+
// +?[0-9]+
// 0x[0-9a-fA-F]+
static bool readInteger(FILE *fp, int first, Target result)
{
    int  chr = first;

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

    long value = strtoul(buffer_contents(&buf), 0, 0);

    return integer_Create(value, result.integer);
}

static bool readQuote(FILE *fp, Node symbol, Target result)
{
    Node hold;

    if (!readExpr(fp, &(hold.reference))) goto failure;
    if (!pair_Create(hold, NIL, &hold.pair)) goto failure;
    if (!pair_Create(symbol, hold, result.pair)) goto failure;
    if (!setConstructor(*(result.reference), s_quote)) goto failure;
    if (!setType(*(result.reference), t_ASTree)) goto failure;
    return true;

    failure:
    return false;
}

static bool readSymbol(FILE *fp, int first, Target result)
{
    int chr = first;

    static TextBuffer buf = BUFFER_INITIALISER;
    buffer_reset(&buf);

    if (!isLetter(chr)) {
        if (!isPrefix(chr)) goto failure;
        for (;;) {
            buffer_append(&buf, chr);
            chr = getc(fp);
            if (!isPrefix(chr)) break;
        }
    }

    while (isLetter(chr) || isDigit10(chr)) {
        buffer_append(&buf, chr);
        chr = getc(fp);
    }

    if (isPrefix(chr)) {
        Symbol    head;
        Reference tail;
        symbol_Create(buf, &head);


        bool rtn = readSymbol(fp, chr, &tail);

        if (!pair_Create(head, tail, result.pair)) goto failure;

        return rtn;
    }

    ungetc(chr, fp);

    return symbol_Create(buf, result.symbol);

    failure:
    fatal(isPrint(chr) ? "illegal character: 0x%02x '%c'" : "illegal character: 0x%02x", chr, chr);
    return false;
}

static bool skipBlock(FILE *fp, const int delim)
{
    for (;;) {
        int chr = getc(fp);

        if (EOF == chr) return false;

        if (delim == chr) {
            chr = getc(fp);
            if ('#' == chr) return true;
        }
    }
}

static bool skipComment(FILE *fp)
{
    int chr = getc(fp);

    switch (chr) {
    case '(':
        return skipBlock(fp, ')');
    case '[':
        return skipBlock(fp, ']');
    case '{':
        return skipBlock(fp, '}');
    default:
        break;
    }

    for (;;) {
        if ('\n' == chr || '\r' == chr || EOF == chr) break;
        chr = getc(fp);
    }

    return true;
}

extern bool readExpr(FILE *fp, Target result)
{
    for (;;) {
        int chr = getc(fp);
        switch (chr) {
        case EOF:  return false;
        case '#':
            {
                skipComment(fp);
                continue;
            }

        case '"':
            return readString(fp, '"', result);

        case '?':
            {
                 if (!checkBlank(fp)) {
                     return readCode(fp, result);
                 } else {
                     ASSIGN(result, s_qmark);
                     return true;
                 }
             }

        case '\'':
            {
                if (!checkBlank(fp)) {
                    return readQuote(fp, s_quote, result);
                } else {
                    ASSIGN(result, s_ftick);
                    return true;
                }
            }

        case '`':
            {
                if (!checkBlank(fp)) {
                    return readQuote(fp, s_quasiquote, result);
                } else {
                    ASSIGN(result, s_btick);
                    return true;
                }
            }

        case '(':
            return readList(fp, ')', result);

        case '[':
            return readTuple(fp, s_tuple, ']', result);

        case '{':
            return readTuple(fp, s_block, '}', result);

        case ',':
            ASSIGN(result, s_comma);
            return true;

        case ';':
            ASSIGN(result, s_semi);
            return true;

        case ':':
            {
                if (!checkBlank(fp)) {
                    return readQuote(fp, s_type, result);
                } else {
                    ASSIGN(result, s_colon);
                    return true;
                }
            }

        case '0' ... '9':
            return readInteger(fp, chr, result);

        case '\\':
            {
                if (checkBlank(fp)) {
                    ASSIGN(result, s_bslash);
                    return true;
                } else if (matchChar(fp, '@')) {
                    if (checkBlank(fp)) {
                        ASSIGN(result, s_bsat);
                        return true;
                    } else {
                        return readQuote(fp, s_unquote_splicing, result);
                    }
                } else {
                    return readQuote(fp, s_unquote, result);
                }
            }

        case '}':
        case ']':
        case ')':
            {
                ungetc(chr, fp);
                return false;
            }

        case '-':
        case '+':
            {
              int dhr = getc(fp); ungetc(dhr, fp);
              if (isDigit10(dhr)) return readInteger(fp, chr, result);
            }

        default:
            {
              if (isBlank(chr))  continue;
              if (!isPrint(chr)) continue;
              bool rtn = readSymbol(fp, chr, result);
              if (isPair(*(result.reference))) {
                  if (!list_UnDot(*(result.pair))) return false;
                  if (!tuple_Convert(*(result.pair), result.tuple)) return false;
                  if (!setConstructor(*(result.reference), s_path)) return false;
                  if (!setType(*(result.reference), t_ASTree)) return false;
              }
              return rtn;
            }
        }
    }
}

static void enki_sigint(int signo)
{
    fatal("\nInterrupt(%d)",signo);
}

extern void readFile(FILE *stream)
{
    GC_Begin(2);

    Node obj = NIL;

    GC_Protect(obj);

    signal(SIGINT, enki_sigint);

    for (;;) {
        if (stream == stdin) {
            printf("-] ");
            fflush(stdout);
        }

        check_SymbolTable__(__FILE__, __LINE__);

        if (!readExpr(stream, &obj)) break;

        VM_DEBUG(1, " read ");

        VM_ON_DEBUG(9, {
                prettyPrint(stderr, obj);
                fprintf(stderr,"\n");
                fflush(stderr);
            });

        expand(obj, NIL, &obj);

        VM_DEBUG(9, " expanded ");

        VM_ON_DEBUG(9, {
                prettyPrint(stderr, obj);
                fprintf(stderr,"\n");
                fflush(stderr);
            });

        encode(obj, NIL, &obj);

        VM_DEBUG(9, " encoded ");

        VM_ON_DEBUG(1, {
                prettyPrint(stderr, obj);
                fprintf(stderr,"\n");
                fflush(stderr);
            });

        eval(obj, NIL, &obj);

        VM_DEBUG(9, " evaluated ");

        VM_ON_DEBUG(9, {
                prettyPrint(stderr, obj);
                fprintf(stderr,"\n\n\n");
                fflush(stderr);
            });

        if (stream == stdin) {
            printf(" => ");
            fflush(stdout);
            prettyPrint(stdout, obj);
            printf("\n");
            fflush(stdout);
        }
    }

    if (stream == stdin) {
        GC_End();
        return;
    }

    int c = getc(stream);

    if (EOF != c)
        fatal("expected 0x%02x received 0x%02x '%c'\n", EOF, c, c);

    GC_End();
}
