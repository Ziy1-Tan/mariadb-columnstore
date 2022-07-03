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
int do_merge(string& ret, json_engine_t* je1, json_engine_t* je2)
{
  if (json_read_value(je1) || json_read_value(je2))
    return 1;

  if (je1->value_type == JSON_VALUE_OBJECT && je2->value_type == JSON_VALUE_OBJECT)
  {
    json_engine_t sav_je1 = *je1;
    json_engine_t sav_je2 = *je2;

    int first_key = 1;
    json_string_t key_name;

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

      if (unlikely(je1->s.error))
        return 1;

      if (first_key)
        first_key = 0;
      else
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
        ret.append((const char*)key_start, (size_t)(key_end - key_start));
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
        if ((ires = do_merge(ret, je1, je2)))
          return ires;
        goto merged_j1;
      }
      if (unlikely(je2->s.error))
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

      if (unlikely(je2->s.error))
        return 1;

      *je1 = sav_je1;
      while (json_scan_next(je1) == 0 && je1->state != JST_OBJ_END)
      {
        DBUG_ASSERT(je1->state == JST_KEY);
        json_string_set_str(&key_name, key_start, key_end);
        if (!json_key_matches(je1, &key_name))
        {
          if (unlikely(je1->s.error || json_skip_key(je1)))
            return 2;
          continue;
        }
        if (json_skip_key(je2) || json_skip_level(je1))
          return 1;
        goto continue_j2;
      }

      if (unlikely(je1->s.error))
        return 2;

      if (first_key)
        first_key = 0;
      else
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

      if (json_skip_key(je2))
        return 1;

      try
      {
        ret.append("\"");
        ret.append((const char*)key_start, (size_t)(je2->s.c_str - key_start));
      }
      catch (...)
      {
        return 3;
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
    const uchar *end1, *beg1, *end2, *beg2;
    int n_items1 = 1, n_items2 = 1;

    beg1 = je1->value_begin;

    /* Merge as a single array. */
    if (je1->value_type == JSON_VALUE_ARRAY)
    {
      if (json_skip_level_and_count(je1, &n_items1))
        return 1;

      end1 = je1->s.c_str - je1->sav_c_len;
    }
    else
    {
      try
      {
        ret.append("[");
      }
      catch (...)
      {
        return 3;
      }
      if (je1->value_type == JSON_VALUE_OBJECT)
      {
        if (json_skip_level(je1))
          return 1;
        end1 = je1->s.c_str;
      }
      else
        end1 = je1->value_end;
    }

    try
    {
      ret.append((const char*)beg1, end1 - beg1);
    }
    catch (...)
    {
      return 3;
    }

    if (json_value_scalar(je2))
    {
      beg2 = je2->value_begin;
      end2 = je2->value_end;
    }
    else
    {
      if (je2->value_type == JSON_VALUE_OBJECT)
      {
        beg2 = je2->value_begin;
        if (json_skip_level(je2))
          return 2;
      }
      else
      {
        beg2 = je2->s.c_str;
        if (json_skip_level_and_count(je2, &n_items2))
          return 2;
      }
      end2 = je2->s.c_str;
    }

    if (n_items1 && n_items2)
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
      ret.append((const char*)beg2, end2 - beg2);
    }
    catch (...)
    {
      return 3;
    }

    if (je2->value_type != JSON_VALUE_ARRAY)
    {
      try
      {
        ret.append("]");
      }
      catch (...)
      {
        return 3;
      }
    }
  }

  return 0;
}

}  // namespace

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_merge::operationType(FunctionParm& fp,
                                                             CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

string Func_json_merge::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                  execplan::CalpontSystemCatalog::ColType& type)
{
  string tmp_js1 = fp[0]->data()->getStrVal(row, isNull);

  if (isNull)
    return "";

  const char *js1 = tmp_js1.c_str(), *js2 = NULL;
  const CHARSET_INFO* js1_cs = fp[1]->data()->resultType().getCharset();
  LINT_INIT(js2)

  json_engine_t je1, je2;

  string ret;

  for (size_t i = 1; i < fp.size(); i++)
  {
    string tmp_js2 = fp[i]->data()->getStrVal(row, isNull);
    if (isNull)
      return "";

    js2 = tmp_js2.c_str();

    json_scan_start(&je1, js1_cs, (const uchar*)js1, (const uchar*)js1 + strlen(js1));

    json_scan_start(&je2, fp[i]->data()->resultType().getCharset(), (const uchar*)js2,
                    (const uchar*)js2 + strlen(js2));

    ret.clear();
    if (do_merge(ret, &je1, &je2))
    {
      isNull = true;
      return "";
    }

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
