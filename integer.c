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

extern bool integer_Create(long value, Integer *target) {
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

#if 0
extern bool integer_Add(Integer left, Integer right, Integer *target) {

    if (!left)  {
        if (right) {
            *target = right;
            return true;
        }
        return  integer_Create(0, target);
    }

    if (!right) {
         *target = left;
        return true;
    }

    return integer_Create(left->value + right->value, target);
}

extern bool integer_Mult(Integer left, Integer right, Integer *target) {

    if (!left)  {
        if (right) {
            *target = right;
            return true;
        }
        return  integer_Create(1, target);
    }

    if (!right) {
         *target = left;
        return true;
    }

    return  integer_Create(left->value * right->value, target);
}

extern bool integer_Subtract(Integer left, Integer right, Integer *target) {

    if (!left)  {
        if (right) {
            return integer_Create(0 - right->value, target);
        }
        return integer_Create(0, target);
    }

    if (!right) {
         *target = left;
        return true;
    }

    return  integer_Create(left->value - right->value, target);
}
#endif

/*****************
 ** end of file **
 *****************/

