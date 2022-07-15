#include <string_view>
#include <algorithm>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
#include "rowgroup.h"
using namespace execplan;
using namespace rowgroup;

#include "dataconvert.h"

#include "jsonhelpers.h"
using namespace funcexp::helpers;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_contains_path::operationType(
    FunctionParm& fp, CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

/**
 * getBoolVal API definition
 */
bool Func_json_contains_path::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                         CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  int arrayCounters[JSON_DEPTH_LIMIT];
  bool hasNegPath = false;

  if (isNull)
    return false;

  if (!isModeParsed)
  {
    if (!isModeConst)
    {
      ConstantColumn* constCol = dynamic_cast<ConstantColumn*>(fp[1]->data());
      isModeConst = (constCol != nullptr);
    }
    string mode = fp[1]->data()->getStrVal(row, isNull);
    if (isNull)
      return false;

    transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
    if (mode != "one" && mode != "all")
    {
      isNull = true;
      return false;
    }

    isModeOne = (mode == "one");
    isModeParsed = isModeConst;
  }

  // parse path flag
  if (paths.size() == 0)
  {
    for (size_t i = 2; i < fp.size(); i++)
    {
      json_path_with_flags path;
      ConstantColumn* constCol = dynamic_cast<ConstantColumn*>(fp[i]->data());
      path.set_constant_flag((constCol != nullptr));
      paths.push_back(path);
    }
    hasFound.assign(fp.size() - 2, false);
  }

  for (size_t i = 2; i < fp.size(); i++)
  {
    json_path_with_flags& currPath = paths[i - 2];

    if (!currPath.parsed)
    {
      const string_view tmpPath = fp[i]->data()->getStrVal(row, isNull);
      if (isNull)
        return false;

      if (json_path_setup(&currPath.p, fp[i]->data()->resultType().getCharset(), (const uchar*)tmpPath.data(),
                          (const uchar*)tmpPath.data() + tmpPath.size()))
      {
        isNull = true;
        return false;
      }
      currPath.parsed = currPath.constant;
      hasNegPath |= currPath.p.types_used & JSON_PATH_NEGATIVE_INDEX;
    }
  }

  json_engine_t jsEg;
  json_path_t p;
  json_get_path_start(&jsEg, fp[0]->data()->resultType().getCharset(), (const uchar*)tmpJs.data(),
                      (const uchar*)tmpJs.data() + tmpJs.size(), &p);

  bool result = false;
  int needFound = 0;

  if (!isModeOne)
  {
    hasFound.assign(fp.size() - 2, false);
    needFound = fp.size() - 2;
  }

  while (json_get_path_next(&jsEg, &p) == 0)
  {
    if (hasNegPath && jsEg.value_type == JSON_VALUE_ARRAY &&
        json_skip_array_and_count(&jsEg, arrayCounters + (p.last_step - p.steps)))
    {
      result = true;
      break;
    }

    for (int restSize = fp.size() - 2, curr = 0; restSize > 0; restSize--, curr++)
    {
      json_path_with_flags& currPath = paths[curr];
      if (json_path_compare(&currPath.p, &p, jsEg.value_type, arrayCounters) >= 0)
      {
        if (isModeOne)
        {
          result = true;
          break;
        }
        /* mode_all */
        if (hasFound[restSize - 1])
          continue; /* already found */
        if (--needFound == 0)
        {
          result = true;
          break;
        }
        hasFound[restSize - 1] = true;
      }
    }
  }

  if (likely(jsEg.s.error == 0))
    return result;

  isNull = true;
  return false;
}
}  // namespace funcexp
