#include <string_view>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

#include "jsonfunchelpers.h"
using namespace funcexp::helpers;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_array_append::operationType(FunctionParm& fp,
                                                                    CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_array_append::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                         execplan::CalpontSystemCatalog::ColType& type)
{
  string tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  const CHARSET_INFO* cs = fp[0]->data()->resultType().getCharset();

  json_engine_t jsEg;
  const uchar* arrEnd;
  size_t strRestLen;
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

  for (size_t i = 1, j = 0; i < fp.size(); i += 2, j++)
  {
    const char* js = tmpJs.data();
    const size_t jsLen = tmpJs.size();
    uint arrayCounters[JSON_DEPTH_LIMIT];
    json_path_with_flags& currPath = paths[j];
    if (!currPath.parsed)
    {
      const string_view tmpPath = fp[i]->data()->getStrVal(row, isNull);
      if (isNull)
        return "";
      if (setupPathNoWildcard(&currPath.p, fp[i]->data()->resultType().getCharset(),
                              (const uchar*)tmpPath.data(), (const uchar*)tmpPath.data() + tmpPath.size()))
      {
        isNull = true;
        return "";
      }
      currPath.parsed = currPath.constant;
    }

    json_scan_start(&jsEg, cs, (const uchar*)js, (const uchar*)js + jsLen);

    currPath.cur_step = currPath.p.steps;

    if (json_find_path(&jsEg, &currPath.p, &currPath.cur_step, arrayCounters))
    {
      if (jsEg.s.error)
      {
      }
      isNull = true;
      return "";
    }

    if (json_read_value(&jsEg))
    {
      isNull = true;
      return "";
    }

    if (jsEg.value_type == JSON_VALUE_ARRAY)
    {
      int itemSize;
      if (json_skip_level_and_count(&jsEg, &itemSize))
      {
        isNull = true;
        return "";
      }

      arrEnd = jsEg.s.c_str - jsEg.sav_c_len;
      strRestLen = jsLen - (arrEnd - (const uchar*)js);
      ret.append(js, arrEnd - (const uchar*)js);
      if (itemSize)
        ret.append(", ", 2);
      ret.append(getJsonValue(row, fp[i + 1]));

      ret.append((const char*)arrEnd, strRestLen);
    }
    else
    {
      const uchar *start, *end;

      /* Wrap as an array. */
      ret.append(js, (const char*)jsEg.value_begin - js);
      start = jsEg.value_begin;

      if (jsEg.value_type == JSON_VALUE_OBJECT)
      {
        if (json_skip_level(&jsEg))
        {
          isNull = true;
          return "";
        }
        end = jsEg.s.c_str;
      }
      else
        end = jsEg.value_end;

      ret.append("[");
      ret.append((const char*)start, end - start);
      ret.append(", ");
      ret.append(getJsonValue(row, fp[i + 1]));
      ret.append("]");
      ret.append((const char*)jsEg.s.c_str, js + jsLen - (const char*)jsEg.s.c_str);
    }

    tmpJs.swap(ret);
    ret.clear();
  }

  json_scan_start(&jsEg, cs, (const uchar*)tmpJs.data(), (const uchar*)tmpJs.data() + tmpJs.size());

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
