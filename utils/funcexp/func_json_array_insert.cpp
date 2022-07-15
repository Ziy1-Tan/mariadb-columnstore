#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

#include "jsonhelpers.h"
using namespace funcexp::helpers;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_array_insert::operationType(FunctionParm& fp,
                                                                    CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_array_insert::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                         execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  const CHARSET_INFO* cs = fp[0]->data()->resultType().getCharset();

  json_engine_t jsEg;
  string ret;
  ret.reserve(tmpJs.size() + 8);

  if (paths.size() == 0)
  {
    for (size_t i = 1; i < fp.size(); i += 2)
    {
      json_path_with_flags path;
      ConstantColumn* constCol = dynamic_cast<ConstantColumn*>(fp[i]->data());
      path.set_constant_flag((constCol != nullptr));
      paths.push_back(path);
    }
  }

  string rawJs{tmpJs};
  for (size_t i = 1, j = 0; i < fp.size(); i += 2, j++)
  {
    const char* js = rawJs.data();
    const size_t jsLen = rawJs.size();
    int arrayCounters[JSON_DEPTH_LIMIT];
    json_path_with_flags& currPath = paths[j];
    if (!currPath.parsed)
    {
      const string_view tmpPath = fp[i]->data()->getStrVal(row, isNull);
      if (isNull)
        return "";
      if (setupPathNoWildcard(&currPath.p, fp[i]->data()->resultType().getCharset(),
                              (const uchar*)tmpPath.data(), (const uchar*)tmpPath.data() + tmpPath.size()) ||
          currPath.p.last_step - 1 < currPath.p.steps || currPath.p.last_step->type != JSON_PATH_ARRAY)
      {
        if (currPath.p.s.error == 0)
          currPath.p.s.error = SHOULD_END_WITH_ARRAY;

        isNull = true;
        return "";
      }
      currPath.parsed = currPath.constant;
      currPath.p.last_step--;
    }

    json_scan_start(&jsEg, cs, (const uchar*)js, (const uchar*)js + jsLen);

    currPath.cur_step = currPath.p.steps;

    if (json_find_path(&jsEg, &currPath.p, &currPath.cur_step, arrayCounters))
    {
      if (jsEg.s.error)
      {
        isNull = true;
        return "";
      }

      /* Can't find the array to insert. */
      continue;
    }

    if (json_read_value(&jsEg))
    {
      isNull = true;
      return "";
    }

    if (jsEg.value_type != JSON_VALUE_ARRAY)
    {
      /* Must be an array. */
      continue;
    }

    const char* itemPos = 0;
    int itemSize = 0;

    while (json_scan_next(&jsEg) == 0 && jsEg.state != JST_ARRAY_END)
    {
      DBUG_ASSERT(jsEg.state == JST_VALUE);
      if (itemSize == currPath.p.last_step[1].n_item)
      {
        itemPos = (const char*)jsEg.s.c_str;
        break;
      }
      itemSize++;

      if (json_read_value(&jsEg) || (!json_value_scalar(&jsEg) && json_skip_level(&jsEg)))
      {
        isNull = true;
        return "";
      }
    }

    if (unlikely(jsEg.s.error || *jsEg.killed_ptr))
    {
      isNull = true;
      return "";
    }

    if (itemPos)
    {
      ret.append(js, itemPos - js);
      if (itemSize > 0)
        ret.append(" ");
      ret.append(getJsonValue(row, fp[i + 1]));
      ret.append(",");
      if (itemSize == 0)
        ret.append(" ", 1);
      ret.append(itemPos, js + jsLen - itemPos);
    }
    else
    {
      /* Insert position wasn't found - append to the array. */
      DBUG_ASSERT(jsEg.state == JST_ARRAY_END);
      itemPos = (const char*)(jsEg.s.c_str - jsEg.sav_c_len);
      ret.append(js, itemPos - js);
      if (itemSize > 0)
        ret.append(", ");
      ret.append(getJsonValue(row, fp[i + 1]));
      ret.append(itemPos, js + jsLen - itemPos);
    }

    // rawJs save the json string for next loop
    rawJs.swap(ret);
    ret.clear();
  }

  json_scan_start(&jsEg, cs, (const uchar*)rawJs.data(), (const uchar*)rawJs.data() + rawJs.size());

  ret.clear();
  if (doFormat(&jsEg, ret, Func_json_format::LOOSE))
  {
    isNull = true;
    return "";
  }

  isNull = false;
  return ret;
}

}  // namespace funcexp
