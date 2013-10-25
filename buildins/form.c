#include "prefix.inc"

void SUBR(form)
{
  Tuple tuple; Node func = NIL;

  checkArgs(args, "form", 1, NIL);
  forceArgs(args, &func, 0);

  tuple_Create(1, &tuple);
  tuple_SetItem(tuple, 0, func);
  setConstructor(tuple, s_form);

  ASSIGN(result, (Node)tuple);
}
