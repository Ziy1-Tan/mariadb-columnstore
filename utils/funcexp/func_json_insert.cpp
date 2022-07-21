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
CalpontSystemCatalog::ColType Func_json_insert::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_insert::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view jsExp = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  const bool isInsertMode = mode == INSERT || mode == SET;
  const bool isReplaceMode = mode == REPLACE || mode == SET;

  json_engine_t jsEg;

  json_string_t keyName;
  const CHARSET_INFO* cs = fp[0]->data()->resultType().getCharset();
  json_string_set_cs(&keyName, cs);

  if (paths.size() == 0)
  {
    for (size_t i = 1; i < fp.size(); i += 2)
    {
      JsonPath path;
      markConstFlag(path, fp[i]);
      paths.push_back(path);
    }
  }

  // Save the result of each merge and the result of the final merge separately
  string ret;
  string rawJS{jsExp};
  for (size_t i = 1, j = 0; i < fp.size(); i += 2, j++)
  {
    const char* js = rawJS.data();
    const size_t jsLen = rawJS.size();

    JsonPath& currPath = paths[j];
    const json_path_step_t* lastStep;
    const char* valEnd;

    if (!currPath.parsed)
    {
      const string_view pathExp = fp[i]->data()->getStrVal(row, isNull);
      const char* rawPath = pathExp.data();

      if (isNull || pathSetupNwc(&currPath.p, fp[i]->data()->resultType().getCharset(), (const uchar*)rawPath,
                                 (const uchar*)rawPath + pathExp.size()))
        goto error;

      currPath.p.last_step--;
      currPath.parsed = currPath.constant;
    }

    json_scan_start(&jsEg, cs, (const uchar*)js, (const uchar*)js + jsLen);

    if (currPath.p.last_step < currPath.p.steps)
      goto v_found;

    currPath.currStep = currPath.p.steps;

    if (currPath.p.last_step >= currPath.p.steps && jsonFindPath(&jsEg, &currPath.p, &currPath.currStep))
    {
      if (jsEg.s.error)
        goto error;
      continue;
    }

    if (json_read_value(&jsEg))
      goto error;

    lastStep = currPath.p.last_step + 1;
    if (lastStep->type & JSON_PATH_ARRAY)
    {
      IntType itemSize = 0;

      if (jsEg.value_type != JSON_VALUE_ARRAY)
      {
        const uchar* valStart = jsEg.value_begin;
        bool isArrAutoWrap;

        if (isInsertMode)
        {
          if (isReplaceMode)
            isArrAutoWrap = lastStep->n_item > 0;
          else
          {
            if (lastStep->n_item == 0)
              continue;
            isArrAutoWrap = true;
          }
        }
        else
        {
          if (lastStep->n_item)
            continue;
          isArrAutoWrap = false;
        }

        ret.clear();
        /* Wrap the value as an array. */
        ret.append(js, (const char*)valStart - js);
        if (isArrAutoWrap)
          ret.append("[");

        if (jsEg.value_type == JSON_VALUE_OBJECT)
        {
          if (json_skip_level(&jsEg))
            goto error;
        }

        if (isArrAutoWrap)
          ret.append((const char*)valStart, jsEg.s.c_str - valStart);
        ret.append(", ");
        if (appendJSValue(ret, cs, row, fp[i + 1]))
          goto error;
        if (isArrAutoWrap)
          ret.append("]");
        ret.append((const char*)jsEg.s.c_str, js + jsLen - (const char*)jsEg.s.c_str);

        goto continue_point;
      }

      while (json_scan_next(&jsEg) == 0 && jsEg.state != JST_ARRAY_END)
      {
        switch (jsEg.state)
        {
          case JST_VALUE:
            if (itemSize == lastStep->n_item)
              goto v_found;
            itemSize++;
            if (json_skip_array_item(&jsEg))
              goto error;
            break;
          default: break;
        }
      }

      if (unlikely(jsEg.s.error))
        goto error;

      if (!isInsertMode)
        continue;

      valEnd = (const char*)(jsEg.s.c_str - jsEg.sav_c_len);
      ret.clear();
      ret.append(js, valEnd - js);
      if (itemSize > 0)
        ret.append(", ");
      if (appendJSValue(ret, cs, row, fp[i + 1]))
        goto error;
      ret.append(valEnd, js + jsLen - valEnd);
    }
    else /*JSON_PATH_KEY*/
    {
      IntType keySize = 0;

      if (jsEg.value_type != JSON_VALUE_OBJECT)
        continue;

      while (json_scan_next(&jsEg) == 0 && jsEg.state != JST_OBJ_END)
      {
        switch (jsEg.state)
        {
          case JST_KEY:
            json_string_set_str(&keyName, lastStep->key, lastStep->key_end);
            if (json_key_matches(&jsEg, &keyName))
              goto v_found;
            keySize++;
            if (json_skip_key(&jsEg))
              goto error;
            break;
          default: break;
        }
      }

      if (unlikely(jsEg.s.error))
        goto error;

      if (!isInsertMode)
        continue;

      valEnd = (const char*)(jsEg.s.c_str - jsEg.sav_c_len);

      ret.clear();
      ret.append(js, valEnd - js);

      if (keySize > 0)
        ret.append(", ");

      ret.append("\"");
      ret.append((const char*)lastStep->key, lastStep->key_end - lastStep->key);
      ret.append("\":");

      if (appendJSValue(ret, cs, row, fp[i + 1]))
        goto error;
      ret.append(valEnd, js + jsLen - valEnd);
    }

    goto continue_point;

  v_found:

    if (!isReplaceMode)
      continue;

    if (json_read_value(&jsEg))
      goto error;

    valEnd = (const char*)jsEg.value_begin;
    ret.clear();
    if (!json_value_scalar(&jsEg))
    {
      if (json_skip_level(&jsEg))
        goto error;
    }

    ret.append(js, valEnd - js);
    if (appendJSValue(ret, cs, row, fp[i + 1]))
      goto error;
    ret.append((const char*)jsEg.s.c_str, js + jsLen - (const char*)jsEg.s.c_str);

  continue_point:
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
