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
CalpontSystemCatalog::ColType Func_json_array_append::operationType(FunctionParm& fp,
                                                                    CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_array_append::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                         execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view jsExp = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  const CHARSET_INFO* cs = fp[0]->data()->resultType().getCharset();

  json_engine_t jsEg;
  const uchar* arrEnd;
  size_t strRestLen;
  string ret;
  ret.reserve(jsExp.size() + 8);

  if (paths.size() == 0)
  {
    for (size_t i = 1; i < fp.size(); i += 2)
    {
      JsonPath path;
      markConstFlag(path, fp[i]);
      paths.push_back(path);
    }
  }

  string rawJS{jsExp};
  for (size_t i = 1, j = 0; i < fp.size(); i += 2, j++)
  {
    const char* js = rawJS.data();
    const size_t jsLen = rawJS.size();
    JsonPath& currPath = paths[j];
    if (!currPath.parsed)
    {
      const string_view pathExp = fp[i]->data()->getStrVal(row, isNull);
      const char* rawPath = pathExp.data();
      if (isNull || pathSetupNwc(&currPath.p, fp[i]->data()->resultType().getCharset(), (const uchar*)rawPath,
                                 (const uchar*)rawPath + pathExp.size()))
        goto error;
      currPath.parsed = currPath.constant;
    }

    json_scan_start(&jsEg, cs, (const uchar*)js, (const uchar*)js + jsLen);

    currPath.currStep = currPath.p.steps;

    if (jsonFindPath(&jsEg, &currPath.p, &currPath.currStep))
    {
      if (jsEg.s.error)
      {
      }
      goto error;
    }

    if (json_read_value(&jsEg))
      goto error;

    if (jsEg.value_type == JSON_VALUE_ARRAY)
    {
      int itemSize;
      if (json_skip_level_and_count(&jsEg, &itemSize))
        goto error;

      arrEnd = jsEg.s.c_str - jsEg.sav_c_len;
      strRestLen = jsLen - (arrEnd - (const uchar*)js);
      ret.append(js, arrEnd - (const uchar*)js);
      if (itemSize)
        ret.append(", ");
      if (appendJSValue(ret, cs, row, fp[i + 1]))
        goto error;

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
          goto error;
        end = jsEg.s.c_str;
      }
      else
        end = jsEg.value_end;

      ret.append("[");
      ret.append((const char*)start, end - start);
      ret.append(", ");
      if (appendJSValue(ret, cs, row, fp[i + 1]))
        goto error;
      ret.append("]");
      ret.append((const char*)jsEg.s.c_str, js + jsLen - (const char*)jsEg.s.c_str);
    }

    // rawJS save the json string for next loop
    rawJS.swap(ret);
    ret.clear();
  }

  json_scan_start(&jsEg, cs, (const uchar*)rawJS.data(), (const uchar*)rawJS.data() + rawJS.size());

  ret.clear();
  if (doFormat(&jsEg, ret, Func_json_format::LOOSE))
    goto error;

  isNull = false;
  return ret;

error:
  isNull = true;
  return "";
}

}  // namespace funcexp
