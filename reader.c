/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/

#include "all_types.inc"
#include "treadmill.h"

/* */
#include <string.h>
#include <stdbool.h>
#include <error.h>

extern void fatal(char *cstr, ...);// __attribute__ ((noreturn)(format (printf, 1, 2)));

#define GC_PROTECT(var)
#define GC_UNPROTECT(var)

#define NODE(val) ((Node)((Reference)(val)))

// make symbol
extern Node intern(char *cstr);
// constructors
extern Node newPair(Node, Node);
extern Node newString(char *cstr);
extern Node newLong(long val);

extern Node setCdr(Node, Node); //== (setCdr(pair,node); return node)
extern Node setCar(Node, Node); //== (setCar(pair,node); return node)

extern Node s_dot;
extern Node s_quasiquote;
extern Node s_quote;
extern Node s_unquote;
extern Node s_unquote_splicing;

struct buffer
{
    char *buffer;
    int    size;
    int    position;
};

#define BUFFER_INITIALISER { 0, 0, 0 }

static void buffer_reset(struct buffer *b) {
    b->position= 0;
}

static void buffer_append(struct buffer *b, int c)
{
    if (b->position == b->size) {
        if (b->buffer) {
            b->buffer = realloc(b->buffer, b->size *= 2);
        } else {
            b->buffer = malloc(b->size = 32);
        }
    }

    b->buffer[b->position++]= c;
}

#if 0
static void buffer_add(struct buffer *b, const char * string)
{
    if (!string) return;
    int len = strlen(string);

    if (0 >  len) return;
    if (0 == len) return;

    while ((b->position + len + 2) >  b->size) {
        if (b->buffer) {
            b->buffer = realloc(b->buffer, b->size *= 2);
        } else {
            b->buffer = malloc(b->size = 32);
        }
    }

    for ( ; 0 < len; --len) {
        b->buffer[b->position++] = *(string++);
    }
}
#endif

static char *buffer_contents(struct buffer *b)
{
    buffer_append(b, 0);
    b->position--;
    return b->buffer;
}

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

static inline bool isEof(Node value) {
    return value.reference == (Reference)EOF;
}


static Node read(FILE *fp);

static Node readList(FILE *fp, int delim)
{
    Node head = NIL;
    Node tail = head;
    Node obj  = NIL;

    GC_PROTECT(head);
    GC_PROTECT(obj);

    obj = read(fp);

    if (isEof(obj)) goto eof;

    head = tail = newPair(obj, NIL);

    for (;;) {
        obj= read(fp);

        if (isEof(obj)) goto eof;

        if (isIdentical(obj, s_dot)) {
            obj = read(fp);

            if (isEof(obj))      fatal("missing item after .");

            tail = setCdr(tail, obj);
            obj  = read(fp);

            if (isEof(obj))      fatal("extra item after .");

            goto eof;
        } else {
            obj  = newPair(obj, NIL);
            tail = setCdr(tail, obj);
        }
    }

 eof:;
    int c = getc(fp);
    if (c != delim) fatal("EOF while reading list");

    GC_UNPROTECT(obj);
    GC_UNPROTECT(head);

    return head;
}

static int digitValue(int c)
{
    switch (c) {
    case '0' ... '9':  return c - '0';
    case 'A' ... 'Z':  return c - 'A' + 10;
    case 'a' ... 'z':  return c - 'a' + 10;
    }
    fatal("illegal digit in character escape");
    return 0;
}

static int isHexadecimal(int c)
{
    switch (c) {
    case '0' ... '9':
    case 'A' ... 'F':
    case 'a' ... 'f':
        return 1;
    }
    return 0;
}

static int isOctal(int c)
{
    return '0' <= c && c <= '7';
}

static int readChar(int c, FILE *fp)
{
    if ('\\' == c) {
        c= getc(fp);
        switch (c) {
        case 'a':   return '\a';
        case 'b':   return '\b';
        case 'f':   return '\f';
        case 'n':   return '\n';
        case 'r':   return '\r';
        case 't':   return '\t';
        case 'v':   return '\v';
        case '\'':  return '\'';
        case 'u': {
            int a= getc(fp),
                b= getc(fp),
                c= getc(fp),
                d= getc(fp);
            return (digitValue(a) << 24)
                + (digitValue(b) << 16)
                + (digitValue(c) << 8)
                + digitValue(d);
        }
        case 'x': {
            int x= 0;
            if (isHexadecimal(c= getc(fp))) {
                x= digitValue(c);
                if (isHexadecimal(c= getc(fp))) {
                    x= x * 16 + digitValue(c);
                    c= getc(fp);
                }
            }
            ungetc(c, fp);
            return x;
        }
        case '0' ... '7': {
            int x= digitValue(c);
            if (isOctal(c= getc(fp))) {
                x= x * 8 + digitValue(c);
                if (isOctal(c= getc(fp))) {
                    x= x * 8 + digitValue(c);
                    c= getc(fp);
                }
            }
            ungetc(c, fp);
            return x;
        }
        default:
            if (isAlpha(c) || isDigit10(c)) fatal("illegal character escape: \\%c", c);
            return c;
        }
    }
    return c;
}

static Node read(FILE *fp)
{
    for (;;) {
        int c= getc(fp);
        switch (c) {
        case EOF: {
            return NODE(EOF);
        }
        case '\t':  case '\n':  case '\r':  case ' ' : {
            continue;
        }
        case ';': {
            for (;;) {
                c= getc(fp);
                if ('\n' == c || '\r' == c || EOF == c) break;
            }
            continue;
        }
        case '"': {
            static struct buffer buf= BUFFER_INITIALISER;
            buffer_reset(&buf);
            for (;;) {
                c= getc(fp);
                if ('"' == c) break;
                c= readChar(c, fp);
                if (EOF == c)         fatal("EOF in string literal");
                buffer_append(&buf, c);
            }
            Node obj= newString(buffer_contents(&buf));
            //buffer_free(&buf);
            return obj;
        }
        case '?': {
            return newLong(readChar(getc(fp), fp));
        }
        case '\'': {
            Node obj= read(fp);
            GC_PROTECT(obj);
            obj= newPair(obj, NIL);
            obj= newPair(s_quote, obj);
            GC_UNPROTECT(obj);
            return obj;
        }
        case '`': {
            Node obj= read(fp);
            GC_PROTECT(obj);
            obj= newPair(obj, NIL);
            obj= newPair(s_quasiquote, obj);
            GC_UNPROTECT(obj);
            return obj;
        }
        case ',': {
            Node sym= s_unquote;
            c= getc(fp);
            if ('@' == c) sym= s_unquote_splicing;
            else          ungetc(c, fp);
            Node obj= read(fp);
            GC_PROTECT(obj);
            obj= newPair(obj, NIL);
            obj= newPair(sym, obj);
            GC_UNPROTECT(obj);
            return obj;
        }
        case '0' ... '9':
        doDigits:   {
                static struct buffer buf= BUFFER_INITIALISER;
                buffer_reset(&buf);
                do {
                    buffer_append(&buf, c);
                    c= getc(fp);
                } while (isDigit10(c));
                if (('x' == c) && (1 == buf.position))
                    do {
                        buffer_append(&buf, c);
                        c= getc(fp);
                    } while (isDigit16(c));
                ungetc(c, fp);
                Node obj= newLong(strtoul(buffer_contents(&buf), 0, 0));
                //buffer_free(&buf);
                return obj;
            }
        case '(': return readList(fp, ')');      case ')': ungetc(c, fp);  return NODE(EOF);
        case '[': return readList(fp, ']');      case ']': ungetc(c, fp);  return NODE(EOF);
        case '{': return readList(fp, '}');      case '}': ungetc(c, fp);  return NODE(EOF);
        case '-': {
            int d= getc(fp);
            ungetc(d, fp);
            if (isDigit10(d)) goto doDigits;
            /* fall through... */
        }
        default: {
            if (isLetter(c)) {
                static struct buffer buf= BUFFER_INITIALISER;
                buffer_reset(&buf);
                while (isLetter(c) || isDigit10(c)) {
                    buffer_append(&buf, c);
                    c= getc(fp);
                }
                ungetc(c, fp);
                Node obj= intern(buffer_contents(&buf));
                //buffer_free(&buf);
                return obj;
            }
            fatal(isPrint(c) ? "illegal character: 0x%02x '%c'" : "illegal character: 0x%02x", c, c);
        }
        }
    }
}
