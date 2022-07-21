#include "functor_json.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "dataconvert.h"
using namespace dataconvert;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_depth::operationType(FunctionParm& fp,
                                                             CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

int64_t Func_json_depth::getIntVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& op_ct)
{
  const string_view jsExp = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return 0;

  json_engine_t jsEg;
  int depth = 0, currDepth = 0;
  bool incDepth = true;

  const char* js = jsExp.data();

  json_scan_start(&jsEg, fp[0]->data()->resultType().getCharset(), (const uchar*)js,
                  (const uchar*)js + jsExp.size());

  do
  {
    switch (jsEg.state)
    {
      case JST_VALUE:
      case JST_KEY:
        if (incDepth)
        {
          currDepth++;
          incDepth = false;
          if (currDepth > depth)
            depth = currDepth;
        }
        break;
      case JST_OBJ_START:
      case JST_ARRAY_START: incDepth = true; break;
      case JST_OBJ_END:
      case JST_ARRAY_END:
        if (!incDepth)
          currDepth--;
        incDepth = false;
        break;
      default: break;
    }
  } while (json_scan_next(&jsEg) == 0);

  if (likely(!jsEg.s.error))
    return depth;

  isNull = true;
  return 0;
}
}  // namespace funcexp
