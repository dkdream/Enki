/***************************
 **
 ** Project: *current project*
 **
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "integer.h"

extern bool integer_Create(long value, Integer *target) {
    if (!node_Allocate(_zero_space, nt_integer, 0, target)) return 0;

    Integer result = *target;

    result->value = value;

    return result;
}

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

/*****************
 ** end of file **
 *****************/

