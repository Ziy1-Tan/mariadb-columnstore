#include <cstring>
#include <string>
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
  return resultType;
}

int64_t Func_json_depth::getIntVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& type)
{
  const string tmp_js = fp[0]->data()->getStrVal(row, isNull);
  const CHARSET_INFO* cs = type.getCharset();

  json_engine_t je;
  uint depth = 0, c_depth = 0;
  bool inc_depth = true;

  if (isNull)
    return 0;

  const char* js = tmp_js.c_str();

  json_scan_start(&je, cs, (const uchar*)js, (const uchar*)js + strlen(js));

  do
  {
    switch (je.state)
    {
      case JST_VALUE:
      case JST_KEY:
        if (inc_depth)
        {
          c_depth++;
          inc_depth = false;
          if (c_depth > depth)
            depth = c_depth;
        }
        break;
      case JST_OBJ_START:
      case JST_ARRAY_START: inc_depth = true; break;
      case JST_OBJ_END:
      case JST_ARRAY_END:
        if (!inc_depth)
          c_depth--;
        inc_depth = false;
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
