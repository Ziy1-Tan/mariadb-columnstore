#include <cstring>
#include <string>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "dataconvert.h"
using namespace dataconvert;

#include "jsonfunchelpers.h"
using namespace funcexp::helpers;

namespace
{
bool checkKeyInList(const string& res, const uchar* key, int key_len)
{
  const uchar* c = (const uchar*)res.c_str() + 2;                /* beginning '["' */
  const uchar* end = (const uchar*)res.c_str() + res.size() - 1; /* ending '"' */

  while (c < end)
  {
    int i;
    for (i = 0; c[i] != '"' && i < key_len; i++)
    {
      if (c[i] != key[i])
        break;
    }
    if (c[i] == '"')
    {
      if (i == key_len)
        return true;
    }
    else
    {
      while (c[i] != '"')
        i++;
    }
    c += i + 4; /* skip ', "' */
  }
  return false;
}
}  // namespace

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_keys::operationType(FunctionParm& fp,
                                                            CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_keys::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                 execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  json_engine_t je;
  uint keySize = 0;
  uint arrayCounters[JSON_DEPTH_LIMIT];

  json_scan_start(&je, fp[0]->data()->resultType().getCharset(), (const uchar*)tmpJs.data(),
                  (const uchar*)tmpJs.data() + tmpJs.size());

  if (fp.size() > 1)
  {
    if (!path.parsed)
    {
      // check if path column is const
      if (!path.constant)
      {
        ConstantColumn* constCol = dynamic_cast<ConstantColumn*>(fp[1]->data());
        if (constCol != nullptr)
          path.set_constant_flag(true);
        else
          path.set_constant_flag(false);
      }

      const string_view tmpPath = fp[1]->data()->getStrVal(row, isNull);
      if (isNull)
        return "";
      if (setupPathNoWildcard(&path.p, fp[1]->data()->resultType().getCharset(), (const uchar*)tmpPath.data(),
                              (const uchar*)tmpPath.data() + tmpPath.size()))
      {
        isNull = true;
        return "";
      }
      path.parsed = path.constant;
    }

    path.cur_step = path.p.steps;
    if (json_find_path(&je, &path.p, &path.cur_step, arrayCounters))
    {
      if (je.s.error)
      {
      }
      isNull = true;
      return "";
    }
  }

  if (json_read_value(&je))
  {
    isNull = true;
    return "";
  }

  if (je.value_type != JSON_VALUE_OBJECT)
  {
    isNull = true;
    return "";
  }

  string ret("[");

  while (json_scan_next(&je) == 0 && je.state != JST_OBJ_END)
  {
    const uchar *key_start, *key_end;
    int key_len;

    switch (je.state)
    {
      case JST_KEY:
        key_start = je.s.c_str;
        do
        {
          key_end = je.s.c_str;
        } while (json_read_keyname_chr(&je) == 0);

        if (unlikely(je.s.error))
        {
          isNull = true;
          return "";
        }

        key_len = (int)(key_end - key_start);

        if (!checkKeyInList(ret, key_start, key_len))
        {
          if (keySize > 0)
          {
            ret.append(", ");
          }
          ret.append("\"");
          ret.append((const char*)key_start, key_len);
          ret.append("\"");
          keySize++;
        }
        break;
      case JST_OBJ_START:
      case JST_ARRAY_START:
        if (json_skip_level(&je))
          break;
        break;
      default: break;
    }
  }

  if (unlikely(!je.s.error))
  {
    ret.append("]");
    return ret;
  }

  isNull = true;
  return "";
}
}  // namespace funcexp
