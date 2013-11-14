#include "prefix.inc"

// size of pointer (in bytes)
void SUBR(sizeof) {
    Node kind;
    checkArgs(args, "sizeof", 1, s_symbol);
    forceArgs(args, &kind, 0);

    ASSIGN(result, NIL);

    if (isIdentical(kind, s_pointer)) {
        integer_Create(POINTER_SIZE, result.integer);
    }

    if (isIdentical(kind, s_word)) {
        integer_Create(WORD_SIZE, result.integer);
    }

    if (isIdentical(kind, s_header)) {
        integer_Create(sizeof(struct gc_header), result.integer);
    }

    if (isIdentical(kind, s_kind)) {
        integer_Create(sizeof(struct gc_kind), result.integer);
    }

    if (isIdentical(kind, s_node)) {
        integer_Create(sizeof(Node), result.integer);
    }
}
