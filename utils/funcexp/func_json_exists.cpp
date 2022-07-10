#include <string_view>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
#include "rowgroup.h"
using namespace execplan;
using namespace rowgroup;

#include "dataconvert.h"

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_exists::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

/**
 * getBoolVal API definition
 */
bool Func_json_exists::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                  CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return false;

  json_engine_t je;
  uint arrayCounters[JSON_DEPTH_LIMIT];

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
      return false;

    if (json_path_setup(&path.p, fp[1]->data()->resultType().getCharset(), (const uchar*)tmpPath.data(),
                        (const uchar*)tmpPath.data() + tmpPath.size()))
    {
      isNull = true;
      return false;
    }
    path.parsed = path.constant;
  }

  json_scan_start(&je, fp[0]->data()->resultType().getCharset(), (const uchar*)tmpJs.data(),
                  (const uchar*)tmpJs.data() + tmpJs.size());

  path.cur_step = path.p.steps;
  if (json_find_path(&je, &path.p, &path.cur_step, arrayCounters))
  {
    if (je.s.error)
    {
      isNull = true;
      return false;
    }
    return false;
  }

  return true;
}
}  // namespace funcexp
