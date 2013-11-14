/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "primitive.h"
#include "treadmill.h"
#include "type.h"

#if 0
extern bool atomic_Create(Symbol label,
                          Operator evaluator,
                          Atomic *target)
{
    if (!evaluator) return false;
    if (!label)     return false;

    if (!node_Allocate(_zero_space,
                       true,
                       sizeof(struct atomic),
                       target))
        return false;

    Atomic result = *target;

    setConstructor(result, s_atomic);

    result->label    = label;
    result->evaluator = evaluator;

    return true;
}
#endif

extern bool primitive_Create(Symbol label,
                             Operator evaluator,
                             Analyser analyser,
                             Primitive *target)
{
    if (!evaluator) return false;
    if (!label)     return false;

    if (!node_Allocate(_zero_space,
                       true,
                       sizeof(struct primitive),
                       target))
        return false;

    Primitive result = *target;

    setConstructor(result, s_primitive);

    result->label    = label;
    result->evaluator = evaluator;
    result->analyser = analyser;

    return true;
}

extern bool composite_Create(Symbol label,
                             Operator evaluator,
                             Analyser analyser,
                             Operator encoder,
                             Composite *target)
{
    if (!evaluator) return false;
    if (!encoder)   return false;
    if (!label)     return false;

    if (!node_Allocate(_zero_space,
                       true,
                       sizeof(struct composite),
                       target))
        return false;

    Composite result = *target;

    setConstructor(result, s_composite);

    result->label     = label;
    result->evaluator = evaluator;
    result->analyser  = analyser;
    result->encoder   = encoder;

    return true;
}

/*****************
 ** end of file **
 *****************/

