/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "count.h"
#include "treadmill.h"

extern bool count_Create(unsigned int value, Count *target) {
    if (!node_Allocate(_zero_space,
                       true,
                       sizeof(struct count),
                       0,
                       target)) return false;

    Count result = (*target);

    setKind(result, nt_count);
    result->value = value;

    return true;
}

extern bool count_Add(Count left, Count right, Count *target) {
    if (!left) {
        if (right) {
            *target = right;
            return true;
        }
        return count_Create(0, target);
    }
    if (!right) {
        *target = left;
        return true;
    }

    return count_Create(left->value + right->value, target);
}

/*****************
 ** end of file **
 *****************/

