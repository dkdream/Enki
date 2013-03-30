/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "primitive.h"
#include "treadmill.h"
#include "symbol.h"
#include "type.h"

extern bool primitive_Create(Symbol label, Operator function, Primitive *target) {
    if (!function) return false;
    if (!label)    return false;

    if (!node_Allocate(_zero_space,
                       true,
                       sizeof(struct primitive),
                       target))
        return false;

    Primitive result = *target;

    setType(result, t_primitive);
    result->label    = label;
    result->function = function;

    return true;
}

/*****************
 ** end of file **
 *****************/

