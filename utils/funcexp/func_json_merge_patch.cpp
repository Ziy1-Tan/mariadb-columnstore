#include <string_view>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

#include "jsonhelpers.h"
using namespace funcexp::helpers;

namespace
{
int copyValuePatch(string& ret, json_engine_t* je)
{
  int firstKey = 1;

  if (je->value_type != JSON_VALUE_OBJECT)
  {
    const uchar *beg, *end;

    beg = je->value_begin;

    if (!json_value_scalar(je))
    {
      if (json_skip_level(je))
        return 1;
      end = je->s.c_str;
    }
    else
      end = je->value_end;

    try
    {
      ret.append((const char*)beg, (int)(end - beg));
    }
    catch (...)
    {
      return 1;
    }

    return 0;
  }
  /* JSON_VALUE_OBJECT */

  try
  {
    ret.append("{");
  }
  catch (...)
  {
    return 1;
  }

  while (json_scan_next(je) == 0 && je->state != JST_OBJ_END)
  {
    const uchar* keyStart;
    /* Loop through the Json_1 keys and compare with the Json_2 keys. */
    DBUG_ASSERT(je->state == JST_KEY);
    keyStart = je->s.c_str;

    if (json_read_value(je))
      return 1;

    if (je->value_type == JSON_VALUE_NULL)
      continue;

    if (!firstKey)
    {
      try
      {
        ret.append(", ");
      }
      catch (...)
      {
        return 3;
      }
    }
    else
      firstKey = 0;

    try
    {
      ret.append("\"");
      ret.append((const char*)keyStart, (int)(je->value_begin - keyStart));
      if (copyValuePatch(ret, je))
        return 1;
    }
    catch (...)
    {
      return 3;
    }
  }

  try
  {
    ret.append("}");
  }
  catch (...)
  {
    return 1;
  }

  return 0;
}

int doMergePatch(string& ret, json_engine_t* je1, json_engine_t* je2, bool& isEmpty)
{
  if (json_read_value(je1) || json_read_value(je2))
    return 1;

  if (je1->value_type == JSON_VALUE_OBJECT && je2->value_type == JSON_VALUE_OBJECT)
  {
    json_engine_t savJe1 = *je1;
    json_engine_t savJe2 = *je2;

    int firstKey = 1;
    json_string_t keyName;
    size_t savLen;
    bool mrgEmpty;

    isEmpty = false;
    json_string_set_cs(&keyName, je1->s.cs);

    try
    {
      ret.append("{");
    }
    catch (...)
    {
      return 3;
    }
    while (json_scan_next(je1) == 0 && je1->state != JST_OBJ_END)
    {
      const uchar *keyStart, *keyEnd;
      /* Loop through the Json_1 keys and compare with the Json_2 keys. */
      DBUG_ASSERT(je1->state == JST_KEY);
      keyStart = je1->s.c_str;
      do
      {
        keyEnd = je1->s.c_str;
      } while (json_read_keyname_chr(je1) == 0);

      if (je1->s.error)
        return 1;

      savLen = ret.size();

      if (!firstKey)
      {
        try
        {
          ret.append(", ", 2);
        }
        catch (...)
        {
          return 3;
        }
        *je2 = savJe2;
      }

      try
      {
        ret.append("\"");
        ret.append((const char*)keyStart, (int)(keyEnd - keyStart));
        ret.append("\":", 2);
      }
      catch (...)
      {
        return 3;
      }

      while (json_scan_next(je2) == 0 && je2->state != JST_OBJ_END)
      {
        int ires;
        DBUG_ASSERT(je2->state == JST_KEY);
        json_string_set_str(&keyName, keyStart, keyEnd);
        if (!json_key_matches(je2, &keyName))
        {
          if (je2->s.error || json_skip_key(je2))
            return 2;
          continue;
        }

        /* Json_2 has same key as Json_1. Merge them. */
        if ((ires = doMergePatch(ret, je1, je2, mrgEmpty)))
          return ires;

        if (mrgEmpty)
          ret = ret.substr(0, savLen);
        else
          firstKey = 0;

        goto merged_j1;
      }

      if (je2->s.error)
        return 2;

      keyStart = je1->s.c_str;
      /* Just append the Json_1 key value. */
      if (json_skip_key(je1))
        return 1;
      try
      {
        ret.append((const char*)keyStart, (int)(je1->s.c_str - keyStart));
      }
      catch (...)
      {
        return 3;
      }
      firstKey = 0;

    merged_j1:
      continue;
    }

    *je2 = savJe2;
    /*
      Now loop through the Json_2 keys.
      Skip if there is same key in Json_1
    */
    while (json_scan_next(je2) == 0 && je2->state != JST_OBJ_END)
    {
      const uchar *keyStart, *keyEnd;
      DBUG_ASSERT(je2->state == JST_KEY);
      keyStart = je2->s.c_str;
      do
      {
        keyEnd = je2->s.c_str;
      } while (json_read_keyname_chr(je2) == 0);

      if (je2->s.error)
        return 1;

      *je1 = savJe1;
      while (json_scan_next(je1) == 0 && je1->state != JST_OBJ_END)
      {
        DBUG_ASSERT(je1->state == JST_KEY);
        json_string_set_str(&keyName, keyStart, keyEnd);
        if (!json_key_matches(je1, &keyName))
        {
          if (je1->s.error || json_skip_key(je1))
            return 2;
          continue;
        }
        if (json_skip_key(je2) || json_skip_level(je1))
          return 1;
        goto continue_j2;
      }

      if (je1->s.error)
        return 2;

      savLen = ret.size();

      if (!firstKey)
      {
        try
        {
          ret.append(", ", 2);
        }
        catch (...)
        {
          return 3;
        }
      }

      try
      {
        ret.append("\"");
        ret.append((const char*)keyStart, (int)(keyEnd - keyStart));
        ret.append("\":", 2);
      }
      catch (...)
      {
        return 3;
      }

      if (json_read_value(je2))
        return 1;

      if (je2->value_type == JSON_VALUE_NULL)
        ret = ret.substr(0, savLen);
      else
      {
        if (copyValuePatch(ret, je2))
          return 1;
        firstKey = 0;
      }

    continue_j2:
      continue;
    }

    try
    {
      ret.append("}");
    }
    catch (...)
    {
      return 3;
    }
  }
  else
  {
    if (!json_value_scalar(je1) && json_skip_level(je1))
      return 1;

    isEmpty = (je2->value_type == JSON_VALUE_NULL);
    if (!isEmpty && copyValuePatch(ret, je2))
      return 1;
  }

  return 0;
}
}  // namespace

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_merge_patch::operationType(FunctionParm& fp,
                                                                   CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_merge_patch::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                        execplan::CalpontSystemCatalog::ColType& type)
{
  json_engine_t je1, je2;
  bool isEmpty, mergeToNull;
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  mergeToNull = isNull;
  if (isNull)
  {
    isNull = false;
  }

  je1.s.error = je2.s.error = 0;
  const char *js1 = 0, *js2 = 0;

  const CHARSET_INFO* js1Charset = fp[0]->data()->resultType().getCharset();

  string rawJs{tmpJs};
  string ret;
  for (size_t i = 1; i < fp.size(); i++)
  {
    const string_view tmpJs2 = fp[i]->data()->getStrVal(row, isNull);
    if (isNull)
    {
      mergeToNull = true;
      isNull = false;
      goto cont_point;
    }

    js1 = rawJs.data();
    js2 = tmpJs2.data();

    json_scan_start(&je2, fp[i]->data()->resultType().getCharset(), (const uchar*)js2,
                    (const uchar*)js2 + tmpJs2.size());

    if (mergeToNull)
    {
      if (json_read_value(&je2))
      {
        isNull = true;
        return "";
      }
      if (je2.value_type == JSON_VALUE_OBJECT)
      {
        mergeToNull = true;
        goto cont_point;
      }
      mergeToNull = false;
      ret.append(js2);
      goto cont_point;
    }

    json_scan_start(&je1, js1Charset, (const uchar*)js1, (const uchar*)js1 + rawJs.size());

    if (doMergePatch(ret, &je1, &je2, isEmpty))
    {
      isNull = true;
      return "";
    }

    if (isEmpty)
      ret.append("null");

  cont_point:
    // rawJs save the merge result for next loop
    rawJs.swap(ret);
    ret.clear();
  }

  if (mergeToNull)
  {
    isNull = true;
    return "";
  }

  json_scan_start(&je1, js1Charset, (const uchar*)rawJs.data(), (const uchar*)rawJs.data() + rawJs.size());

  ret.clear();
  if (doFormat(&je1, ret, Func_json_format::LOOSE))
  {
    isNull = true;
    return "";
  }

  isNull = false;
  return ret;
}
}  // namespace funcexp
