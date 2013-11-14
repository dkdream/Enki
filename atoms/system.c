#include "prefix.inc"

void SUBR(system) {
    Node command = NIL;
    checkArgs(args, "system", 1, t_text);
    forceArgs(args, &command, 0);

    int state = system(text_Text(command.text));

    if (WIFEXITED(state)) {
        int status = WEXITSTATUS(state);
        integer_Create(status, result.integer);
    } else {
        if (!WIFSIGNALED(state)) {
            ASSIGN(result, NIL);
        } else {
            int signo = (WTERMSIG(state) * -1);
            integer_Create(signo, result.integer);
        }
    }
}
