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
int doMerge(string& ret, json_engine_t* je1, json_engine_t* je2)
{
  if (json_read_value(je1) || json_read_value(je2))
    return 1;

  if (je1->value_type == JSON_VALUE_OBJECT && je2->value_type == JSON_VALUE_OBJECT)
  {
    json_engine_t savJe1 = *je1;
    json_engine_t savJe2 = *je2;

    int firstKey = 1;
    json_string_t keyName;

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

      if (unlikely(je1->s.error))
        return 1;

      if (firstKey)
        firstKey = 0;
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
        *je2 = savJe2;
      }

      try
      {
        ret.append("\"");
        ret.append((const char*)keyStart, (size_t)(keyEnd - keyStart));
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
        if ((ires = doMerge(ret, je1, je2)))
          return ires;
        goto merged_j1;
      }
      if (unlikely(je2->s.error))
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

      if (unlikely(je2->s.error))
        return 1;

      *je1 = savJe1;
      while (json_scan_next(je1) == 0 && je1->state != JST_OBJ_END)
      {
        DBUG_ASSERT(je1->state == JST_KEY);
        json_string_set_str(&keyName, keyStart, keyEnd);
        if (!json_key_matches(je1, &keyName))
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

      if (firstKey)
        firstKey = 0;
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
        ret.append((const char*)keyStart, (size_t)(je2->s.c_str - keyStart));
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
    int itemSize1 = 1, itemSize2 = 1;

    beg1 = je1->value_begin;

    /* Merge as a single array. */
    if (je1->value_type == JSON_VALUE_ARRAY)
    {
      if (json_skip_level_and_count(je1, &itemSize1))
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
        if (json_skip_level_and_count(je2, &itemSize2))
          return 2;
      }
      end2 = je2->s.c_str;
    }

    if (itemSize1 && itemSize2)
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
  return fp[0]->data()->resultType();
}

string Func_json_merge::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                  execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  const CHARSET_INFO* js1Charset = fp[0]->data()->resultType().getCharset();

  json_engine_t je1, je2;

  string rawJs{tmpJs};
  string ret;

  for (size_t i = 1; i < fp.size(); i++)
  {
    const string_view tmpJs2 = fp[i]->data()->getStrVal(row, isNull);
    if (isNull)
      return "";

    json_scan_start(&je1, js1Charset, (const uchar*)rawJs.data(), (const uchar*)rawJs.data() + rawJs.size());

    json_scan_start(&je2, fp[i]->data()->resultType().getCharset(), (const uchar*)tmpJs2.data(),
                    (const uchar*)tmpJs2.data() + tmpJs2.size());

    if (doMerge(ret, &je1, &je2))
    {
      isNull = true;
      return "";
    }

    // rawJs save the merge result for next loop
    rawJs.swap(ret);
    ret.clear();
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
