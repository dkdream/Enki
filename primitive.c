/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "primitive.h"

extern bool primitive_Create(Symbol label, Operator function, unsigned int extend, Primitive *target) {
    if (!function) return false;
    if (!label)    return false;
    if (!node_Allocate(_zero_space, nt_primitive, sizeof(unsigned char) * extend, target)) return false;

    Primitive result = *target;

    result->label    = label;
    result->function = function;
    result->size     = extend;

    return true;
}

/*****************
 ** end of file **
 *****************/

