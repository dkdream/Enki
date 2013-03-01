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


int main(int argc, char **argv)
{
    startEnkiLibrary();

    FILE* data = fopen("./test_reader.scm", "r");

    if (!data) {
        printf("unable to open file ./test_reader.scm");
        return -1;
    }

    replFile(data);

    stopEnkiLibrary();
    return 0;
}



/*****************
 ** end of file **
 *****************/

