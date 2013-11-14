#include "prefix.inc"

void SUBR(inode) {
    struct stat stbuf;
    Node fname;

    checkArgs(args, "inode", 1, t_text);
    forceArgs(args, &fname, 0);

    if (!isType(fname, t_text)) {
        fatal("inode: not text");
    }

    const char *name = text_Text(fname.text);

    int filedes = open(name, O_RDONLY|O_DIRECT);

    if (-1 == filedes) {
        ASSIGN(result, NIL);
    }

    if (fstat(filedes, &stbuf) == -1) {
        ASSIGN(result, NIL);
    }

    close(filedes);

    integer_Create(stbuf.st_ino, result.integer);
}
