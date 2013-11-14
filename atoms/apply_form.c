#include "prefix.inc"

void SUBR(apply_form)
{
    Node form, cargs, func;

    pair_GetCar(args.pair, &form);
    pair_GetCdr(args.pair, &cargs);
    pair_GetCar(form.pair, &func);

    apply(func, cargs, env, result);
}
