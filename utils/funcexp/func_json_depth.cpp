#include <string_view>
using namespace std;

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
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return 0;

  json_engine_t je;
  int depth = 0, currDepth = 0;
  bool incDepth = true;

  const char* js = tmpJs.data();

  json_scan_start(&je, fp[0]->data()->resultType().getCharset(), (const uchar*)js,
                  (const uchar*)js + tmpJs.size());

  do
  {
    switch (je.state)
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
  } while (json_scan_next(&je) == 0);

  if (likely(!je.s.error))
    return depth;

  isNull = true;
  return 0;
}
}  // namespace funcexp
