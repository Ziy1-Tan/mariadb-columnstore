#include <string_view>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "dataconvert.h"
using namespace dataconvert;

#include "jsonfunchelper.h"
using namespace funcexp::helpers;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_length::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

int64_t Func_json_length::getIntVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                    execplan::CalpontSystemCatalog::ColType& op_ct)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return 0;

  json_engine_t je;
  uint length = 0;
  uint arrayCounters[JSON_DEPTH_LIMIT];
  int err;

  const char* js = tmpJs.data();

  json_scan_start(&je, fp[0]->data()->resultType().getCharset(), (const uchar*)js,
                  (const uchar*)js + tmpJs.size());

  if (fp.size() > 1)
  {
    if (!path.parsed)
    {
      // check if path column is const
      if (!path.constant)
      {
        ConstantColumn* constCol = dynamic_cast<ConstantColumn*>(fp[1]->data());
        if (constCol != nullptr)
          path.set_constant_flag(true);
        else
          path.set_constant_flag(false);
      }

      const string_view tmpPath = fp[1]->data()->getStrVal(row, isNull);
      if (isNull)
        return 0;
      const char* pathStr = tmpPath.data();
      if (setupPathNoWildcard(&path.p, fp[1]->data()->resultType().getCharset(),
                                       (const uchar*)pathStr, (const uchar*)pathStr + tmpPath.size()))
      {
        isNull = true;
        return 0;
      }
      path.parsed = path.constant;
    }

    path.cur_step = path.p.steps;
    if (json_find_path(&je, &path.p, &path.cur_step, arrayCounters))
    {
      if (je.s.error)
      {
      }
      isNull = true;
      return 0;
    }
  }

  if (json_read_value(&je))
  {
    isNull = true;
    return 0;
  }

  if (json_value_scalar(&je))
    return 1;

  while (!(err = json_scan_next(&je)) && je.state != JST_OBJ_END && je.state != JST_ARRAY_END)
  {
    switch (je.state)
    {
      case JST_VALUE:
      case JST_KEY: length++; break;
      case JST_OBJ_START:
      case JST_ARRAY_START:
        if (json_skip_level(&je))
        {
          isNull = true;
          return 0;
        }
        break;
      default: break;
    };
  }

  if (!err)
  {
    // Parse to the end of the JSON just to check it's valid.
    while (json_scan_next(&je) == 0)
    {
    }
  }

  if (likely(!je.s.error))
    return length;

  isNull = true;
  return 0;
}
}  // namespace funcexp
