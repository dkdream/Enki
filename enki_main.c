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

    replFile(stdin);

    stopEnkiLibrary();
    return 0;
}



/*****************
 ** end of file **
 *****************/

