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
  const string_view jsExp = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return false;

#ifdef MYSQL_GE_1009
  int arrayCounters[JSON_DEPTH_LIMIT];
  bool hasNegPath = false;
#endif
  const int argSize = fp.size() - 2;

  if (!isModeParsed)
  {
    if (!isModeConst)
      isModeConst = (dynamic_cast<ConstantColumn*>(fp[1]->data()) != nullptr);

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
      JsonPath path;
      markConstFlag(path, fp[i]);
      paths.push_back(path);
    }
    hasFound.assign(argSize, false);
  }

  for (size_t i = 2; i < fp.size(); i++)
  {
    JsonPath& currPath = paths[i - 2];

    if (!currPath.parsed)
    {
      const string_view pathExp = fp[i]->data()->getStrVal(row, isNull);
      const char* rawPath = pathExp.data();
      if (isNull || json_path_setup(&currPath.p, fp[i]->data()->resultType().getCharset(),
                                    (const uchar*)rawPath, (const uchar*)rawPath + pathExp.size()))
      {
        isNull = true;
        return false;
      }
      currPath.parsed = currPath.constant;
#ifdef MYSQL_GE_1009
      hasNegPath |= currPath.p.types_used & JSON_PATH_NEGATIVE_INDEX;
#endif
    }
  }

  json_engine_t jsEg;
  json_path_t p;
  json_get_path_start(&jsEg, fp[0]->data()->resultType().getCharset(), (const uchar*)jsExp.data(),
                      (const uchar*)jsExp.data() + jsExp.size(), &p);

  bool result = false;
  int needFound = 0;

  if (!isModeOne)
  {
    hasFound.assign(argSize, false);
    needFound = argSize;
  }

  while (json_get_path_next(&jsEg, &p) == 0)
  {
#ifdef MYSQL_GE_1009
    if (hasNegPath && jsEg.value_type == JSON_VALUE_ARRAY &&
        json_skip_array_and_count(&jsEg, arrayCounters + (p.last_step - p.steps)))
    {
      result = true;
      break;
    }
#endif

    for (int restSize = argSize, curr = 0; restSize > 0; restSize--, curr++)
    {
      JsonPath& currPath = paths[curr];
#ifdef MYSQL_GE_1009
      if (jsonPathCompare(&currPath.p, &p, jsEg.value_type, arrayCounters) >= 0)
#else
      if (jsonPathCompare(&currPath.p, &p, jsEg.value_type) >= 0)
#endif
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
