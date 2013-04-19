#include "prefix.inc"

static ID_set loaded_inodes = SET_INITIALISER;

void SUBR(require) {
    Node path;

    unsigned xxx = __alloc_cycle;
    unsigned yyy = __scan_cycle;

    __alloc_cycle = 10000;
    __scan_cycle  = 1;

    checkArgs(args, "require", 1, t_text);
    forceArgs(args, &path, 0);

    FILE* file = fopen(text_Text(path.text), "r");

    if (!file) {
        fatal("failed to open \'%s\' for require", text_Text(path.text));
        return;
    }

    struct stat stbuf;
    int filedes = fileno(file);

    if (fstat(filedes, &stbuf) == -1) {
        fatal("failed retrieve fstat of \'%s\' for require", text_Text(path.text));
        return;
    }

    long long id = stbuf.st_ino;

    switch (set_add(&loaded_inodes, id)) {
    case -1:
        fatal("loaded_inodes not init-ed for require");
        break;

    case 1: // added
        readFile(file);
        fclose(file);

    default: // found
        break;
    }

    integer_Create(id, result.integer);

    __alloc_cycle = xxx;
    __scan_cycle  = yyy;
}

