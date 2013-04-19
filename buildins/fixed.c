#include "prefix.inc"


void SUBR(fixed)
{
  Tuple tuple;
  Node  func = NIL;
  Node  enc = NIL;

  int count = checkArgs(args, "fixed", 1, NIL);

  ASSIGN(result,NIL);

  if (1 < count) {
      forceArgs(args, &func, &enc, 0);
      tuple_Create(2, &tuple);
      tuple_SetItem(tuple, 0, func);
      tuple_SetItem(tuple, 1, enc);
  } else {
      forceArgs(args, &func, 0);
      tuple_Create(1, &tuple);
      tuple_SetItem(tuple, 0, func);
  }

  setType(tuple, t_fixed);

  ASSIGN(result, (Node)tuple);
}
