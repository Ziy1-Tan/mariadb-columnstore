#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "dataconvert.h"
using namespace dataconvert;

#include "jsonhelpers.h"
using namespace funcexp::helpers;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_remove::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_remove::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  json_engine_t jsEg;

  json_string_t keyName;
  const CHARSET_INFO* cs = fp[0]->data()->resultType().getCharset();
  json_string_set_cs(&keyName, cs);

  if (paths.size() == 0)
  {
    for (size_t i = 1; i < fp.size(); i++)
    {
      json_path_with_flags path;
      ConstantColumn* constCol = dynamic_cast<ConstantColumn*>(fp[i]->data());
      path.set_constant_flag((constCol != nullptr));
      paths.push_back(path);
    }
  }

  string ret;
  string rawJs{tmpJs};
  for (size_t i = 1, j = 0; i < fp.size(); i++, j++)
  {
    const char* js = rawJs.data();
    const size_t jsLen = rawJs.size();

    int arrayCounters[JSON_DEPTH_LIMIT];
    json_path_with_flags& currPath = paths[j];
    const json_path_step_t* lastStep;
    const char *remStart = nullptr, *remEnd = nullptr;
    int itemSize = 0;

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

      currPath.p.last_step--;
      if (currPath.p.last_step < currPath.p.steps)
      {
        currPath.p.s.error = TRIVIAL_PATH_NOT_ALLOWED;
        {
          isNull = true;
          return "";
        }
      }
      currPath.parsed = currPath.constant;
    }

    json_scan_start(&jsEg, cs, (const uchar*)js, (const uchar*)js + jsLen);

    if (currPath.p.last_step < currPath.p.steps)
      goto v_found;

    currPath.cur_step = currPath.p.steps;

    if (json_find_path(&jsEg, &currPath.p, &currPath.cur_step, arrayCounters))
    {
      if (jsEg.s.error)
      {
        isNull = true;
        return "";
      }
    }

    if (json_read_value(&jsEg))
    {
      isNull = true;
      return "";
    }

    lastStep = currPath.p.last_step + 1;
    if (lastStep->type & JSON_PATH_ARRAY)
    {
      if (jsEg.value_type != JSON_VALUE_ARRAY)
        continue;

      while (json_scan_next(&jsEg) == 0 && jsEg.state != JST_ARRAY_END)
      {
        switch (jsEg.state)
        {
          case JST_VALUE:
            if (itemSize == lastStep->n_item)
            {
              remStart = (const char*)(jsEg.s.c_str - (itemSize ? jsEg.sav_c_len : 0));
              goto v_found;
            }
            itemSize++;
            if (json_skip_array_item(&jsEg))
            {
              isNull = true;
              return "";
            }
            break;
          default: break;
        }
      }

      if (unlikely(jsEg.s.error))
      {
        isNull = true;
        return "";
      }

      continue;
    }
    else /*JSON_PATH_KEY*/
    {
      if (jsEg.value_type != JSON_VALUE_OBJECT)
        continue;

      while (json_scan_next(&jsEg) == 0 && jsEg.state != JST_OBJ_END)
      {
        switch (jsEg.state)
        {
          case JST_KEY:
            if (itemSize == 0)
              remStart = (const char*)(jsEg.s.c_str - jsEg.sav_c_len);
            json_string_set_str(&keyName, lastStep->key, lastStep->key_end);
            if (json_key_matches(&jsEg, &keyName))
              goto v_found;

            if (json_skip_key(&jsEg))
            {
              isNull = true;
              return "";
            }

            remStart = (const char*)jsEg.s.c_str;
            itemSize++;
            break;
          default: break;
        }
      }

      if (unlikely(jsEg.s.error))
      {
        isNull = true;
        return "";
      }

      continue;
    }

  v_found:

    if (json_skip_key(&jsEg) || json_scan_next(&jsEg))
    {
      isNull = true;
      return "";
    }

    remEnd = (jsEg.state == JST_VALUE && itemSize == 0) ? (const char*)jsEg.s.c_str
                                                        : (const char*)(jsEg.s.c_str - jsEg.sav_c_len);
    ret.clear();

    ret.append(js, remStart - js);
    if (jsEg.state == JST_KEY && itemSize > 0)
      ret.append(",", 1);
    ret.append(remEnd, js + jsLen - remEnd);

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
