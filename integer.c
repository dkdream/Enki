/***************************
 **
 ** Project: *current project*
 **
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "integer.h"
#include "treadmill.h"
#include "symbol.h"

extern bool integer_Create(long long value, Integer *target) {
    if (!node_Allocate(_zero_space,
                       true,
                       sizeof(struct integer),
                       target))
        return 0;

    Integer result = *target;

    setType(result, s_integer);

    result->value = value;

    return result;
}

/*****************
 ** end of file **
 *****************/

