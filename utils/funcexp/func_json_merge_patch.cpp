#include <string>
using namespace std;

#include "functor_json.h"
#include "jsonfunchelpers.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

namespace
{
int copy_value_patch(string& ret, json_engine_t* je)
{
  int first_key = 1;

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
    const uchar* key_start;
    /* Loop through the Json_1 keys and compare with the Json_2 keys. */
    DBUG_ASSERT(je->state == JST_KEY);
    key_start = je->s.c_str;

    if (json_read_value(je))
      return 1;

    if (je->value_type == JSON_VALUE_NULL)
      continue;

    if (!first_key)
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
      first_key = 0;

    try
    {
      ret.append("\"");
      ret.append((const char*)key_start, (int)(je->value_begin - key_start));
      if (copy_value_patch(ret, je))
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

int do_merge_patch(string& ret, json_engine_t* je1, json_engine_t* je2, bool& empty_result)
{
  if (json_read_value(je1) || json_read_value(je2))
    return 1;

  if (je1->value_type == JSON_VALUE_OBJECT && je2->value_type == JSON_VALUE_OBJECT)
  {
    json_engine_t sav_je1 = *je1;
    json_engine_t sav_je2 = *je2;

    int first_key = 1;
    json_string_t key_name;
    size_t sav_len;
    bool mrg_empty;

    empty_result = false;
    json_string_set_cs(&key_name, je1->s.cs);

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
      const uchar *key_start, *key_end;
      /* Loop through the Json_1 keys and compare with the Json_2 keys. */
      DBUG_ASSERT(je1->state == JST_KEY);
      key_start = je1->s.c_str;
      do
      {
        key_end = je1->s.c_str;
      } while (json_read_keyname_chr(je1) == 0);

      if (je1->s.error)
        return 1;

      sav_len = ret.size();

      if (!first_key)
      {
        try
        {
          ret.append(", ", 2);
        }
        catch (...)
        {
          return 3;
        }
        *je2 = sav_je2;
      }

      try
      {
        ret.append("\"");
        ret.append((const char*)key_start, (int)(key_end - key_start));
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
        json_string_set_str(&key_name, key_start, key_end);
        if (!json_key_matches(je2, &key_name))
        {
          if (je2->s.error || json_skip_key(je2))
            return 2;
          continue;
        }

        /* Json_2 has same key as Json_1. Merge them. */
        if ((ires = do_merge_patch(ret, je1, je2, mrg_empty)))
          return ires;

        if (mrg_empty)
          ret.reserve(sav_len);
        else
          first_key = 0;

        goto merged_j1;
      }

      if (je2->s.error)
        return 2;

      key_start = je1->s.c_str;
      /* Just append the Json_1 key value. */
      if (json_skip_key(je1))
        return 1;
      try
      {
        ret.append((const char*)key_start, (int)(je1->s.c_str - key_start));
      }
      catch (...)
      {
        return 3;
      }
      first_key = 0;

    merged_j1:
      continue;
    }

    *je2 = sav_je2;
    /*
      Now loop through the Json_2 keys.
      Skip if there is same key in Json_1
    */
    while (json_scan_next(je2) == 0 && je2->state != JST_OBJ_END)
    {
      const uchar *key_start, *key_end;
      DBUG_ASSERT(je2->state == JST_KEY);
      key_start = je2->s.c_str;
      do
      {
        key_end = je2->s.c_str;
      } while (json_read_keyname_chr(je2) == 0);

      if (je2->s.error)
        return 1;

      *je1 = sav_je1;
      while (json_scan_next(je1) == 0 && je1->state != JST_OBJ_END)
      {
        DBUG_ASSERT(je1->state == JST_KEY);
        json_string_set_str(&key_name, key_start, key_end);
        if (!json_key_matches(je1, &key_name))
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

      sav_len = ret.size();

      if (!first_key)
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
        ret.append((const char*)key_start, (int)(key_end - key_start));
        ret.append("\":", 2);
      }
      catch (...)
      {
        return 3;
      }

      if (json_read_value(je2))
        return 1;

      if (je2->value_type == JSON_VALUE_NULL)
        ret.reserve(sav_len);
      else
      {
        if (copy_value_patch(ret, je2))
          return 1;
        first_key = 0;
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

    empty_result = (je2->value_type == JSON_VALUE_NULL);
    if (!empty_result && copy_value_patch(ret, je2))
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
  return resultType;
}

string Func_json_merge_patch::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                        execplan::CalpontSystemCatalog::ColType& type)
{
  json_engine_t je1, je2;
  const string tmp_js1 = fp[0]->data()->getStrVal(row, isNull);
  bool empty_result, merge_to_null;

  je1.s.error = je2.s.error = 0;
  merge_to_null = isNull;

  const char *js1 = isNull ? NULL : tmp_js1.c_str(), *js2 = NULL;
  const CHARSET_INFO* js1_cs = fp[0]->data()->resultType().getCharset();

  string ret;
  for (size_t i = 1; i < fp.size(); i++)
  {
    const string tmp_js2 = fp[i]->data()->getStrVal(row, isNull);
    if (isNull)
    {
      merge_to_null = true;
      goto cont_point;
    }

    js2 = tmp_js2.c_str();

    json_scan_start(&je2, fp[i]->data()->resultType().getCharset(), (const uchar*)js2,
                    (const uchar*)js2 + strlen(js2));

    if (merge_to_null)
    {
      if (json_read_value(&je2))
      {
        isNull = true;
        return "";
      }
      if (je2.value_type == JSON_VALUE_OBJECT)
      {
        merge_to_null = true;
        goto cont_point;
      }
      merge_to_null = false;
      ret.append(js2);
      goto cont_point;
    }

    ret.clear();

    json_scan_start(&je1, js1_cs, (const uchar*)js1, (const uchar*)js1 + strlen(js1));

    if (do_merge_patch(ret, &je1, &je2, empty_result))
    {
      isNull = true;
      return "";
    }

    if (empty_result)
      ret.append("null");

  cont_point:
  {
    if (ret == tmp_js1)
    {
      ret = js1;
      js1 = tmp_js1.c_str();
    }
    else
    {
      string tmp(ret);
      js1 = tmp.c_str();
      ret = tmp_js1;
    }
  }
  }

  if (merge_to_null)
  {
    isNull = true;
    return "";
  }

  json_scan_start(&je1, js1_cs, (const uchar*)js1, (const uchar*)js1 + strlen(js1));

  ret.clear();
  if (helpers::json_nice(&je1, ret, Func_json_format::LOOSE))
  {
    isNull = true;
    return "";
  }

  isNull = false;
  return ret;
}
}  // namespace funcexp
