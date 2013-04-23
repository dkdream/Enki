
extern void startEnkiLibrary();
extern void stopEnkiLibrary();

int main(int argc, char** argv) {
    int result = 0;

    startEnkiLibrary();

    asm("movl %1,%%eax\n\t"
        "movl %2,%%ebx\n\t"
        "movl %%eax,-8(%%esp)\n\t"  // int argc
        "movl %%ebx,-12(%%esp)\n\t" // char** argv
        "lea -12(%%esp),%%eax\n\t"  // bottom-of-frame
        "call alloc_gc\n\t"
        "movl %%eax,%0"
        : "=m" (result)
        : "m" (argc), "m" (argv)
        : "%eax", "%ebx", "%ecx", "%esi", "edi");

    return result;
}
