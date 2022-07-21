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
  const string_view jsExp = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  const CHARSET_INFO* cs = fp[0]->data()->resultType().getCharset();

  json_engine_t jsEg;
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
      if (isNull ||
          pathSetupNwc(&currPath.p, fp[i]->data()->resultType().getCharset(), (const uchar*)rawPath,
                       (const uchar*)rawPath + pathExp.size()) ||
          currPath.p.last_step - 1 < currPath.p.steps || currPath.p.last_step->type != JSON_PATH_ARRAY)
      {
        if (currPath.p.s.error == 0)
          currPath.p.s.error = SHOULD_END_WITH_ARRAY;

        goto error;
      }
      currPath.parsed = currPath.constant;
      currPath.p.last_step--;
    }

    json_scan_start(&jsEg, cs, (const uchar*)js, (const uchar*)js + jsLen);

    currPath.currStep = currPath.p.steps;

    if (jsonFindPath(&jsEg, &currPath.p, &currPath.currStep))
    {
      if (jsEg.s.error)
        goto error;

      /* Can't find the array to insert. */
      continue;
    }

    if (json_read_value(&jsEg))
      goto error;

    if (jsEg.value_type != JSON_VALUE_ARRAY)
    {
      /* Must be an array. */
      continue;
    }

    const char* itemPos = 0;
    IntType itemSize = 0;

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
        goto error;
    }

    if (unlikely(jsEg.s.error || *jsEg.killed_ptr))
      goto error;

    if (itemPos)
    {
      ret.append(js, itemPos - js);
      if (itemSize > 0)
        ret.append(" ");
      if (appendJSValue(ret, cs, row, fp[i + 1]))
        goto error;
      ret.append(",");
      if (itemSize == 0)
        ret.append(" ");
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
      if (appendJSValue(ret, cs, row, fp[i + 1]))
        goto error;
      ret.append(itemPos, js + jsLen - itemPos);
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
