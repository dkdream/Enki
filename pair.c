/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "pair.h"
#include "treadmill.h"

extern bool pair_SetCar(Pair pair, const Node car) {
    if (!pair) return false;
    if (!darken_Node(car)) return false;

    pair->car = car;

    return true;
}

extern bool pair_SetCdr(Pair pair, const Node cdr) {
    if (!pair) return false;
    if (!darken_Node(cdr)) return false;

    pair->cdr = cdr;

    return true;
}

extern bool pair_Create(const Node car, const Node cdr, Pair* target) {
    if (!node_Allocate(_zero_space,
                       false,
                       sizeof(struct pair),
                       0,
                       target))
        return false;

    Pair result = (*target);

    if (!darken_Node(car)) return false;
    if (!darken_Node(cdr)) return false;

    setKind(result, nt_pair);
    result->car = car;
    result->cdr = cdr;

    return true;
}

