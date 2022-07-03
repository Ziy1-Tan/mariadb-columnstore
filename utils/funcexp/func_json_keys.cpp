#include <cstring>
#include <string>
using namespace std;

#include "functor_json.h"
#include "jsonfunchelpers.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "dataconvert.h"
using namespace dataconvert;
namespace
{
bool checkKeyInList(const string& res, const uchar* key, int key_len)
{
  const uchar* c = (const uchar*)res.c_str() + 2;                /* beginning '["' */
  const uchar* end = (const uchar*)res.c_str() + res.size() - 1; /* ending '"' */

  while (c < end)
  {
    int n_char;
    for (n_char = 0; c[n_char] != '"' && n_char < key_len; n_char++)
    {
      if (c[n_char] != key[n_char])
        break;
    }
    if (c[n_char] == '"')
    {
      if (n_char == key_len)
        return true;
    }
    else
    {
      while (c[n_char] != '"')
        n_char++;
    }
    c += n_char + 4; /* skip ', "' */
  }
  return false;
}
}  // namespace

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_keys::operationType(FunctionParm& fp,
                                                            CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

string Func_json_keys::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                 execplan::CalpontSystemCatalog::ColType& type)
{
  const string tmp_js = fp[0]->data()->getStrVal(row, isNull);
  json_engine_t je;
  uint n_keys = 0;
  string ret("[");
  uint array_counters[JSON_DEPTH_LIMIT];

  if (isNull)
    return "";
  const char* js = tmp_js.c_str();
  const CHARSET_INFO* js_cs = fp[0]->data()->resultType().getCharset();

  json_scan_start(&je, js_cs, (const uchar*)js, (const uchar*)js + strlen(js));

  if (fp.size() > 1)
  {
    const string tmp_path = fp[1]->data()->getStrVal(row, isNull);
    if (isNull)
      return "";
    const char* path_str = tmp_path.c_str();
    const CHARSET_INFO* path_cs = fp[1]->data()->resultType().getCharset();
    if (path_str && helpers::path_setup_nwc(&path.p, path_cs, (const uchar*)path_str,
                                            (const uchar*)path_str + strlen(path_str)))
    {
      isNull = true;
      return "";
    }

    path.cur_step = path.p.steps;
    if (json_find_path(&je, &path.p, &path.cur_step, array_counters))
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
          if (n_keys > 0)
          {
            ret = ret.append(", ");
          }
          ret.append("\"");
          ret.append((const char*)key_start, key_len);
          ret.append("\"");
          n_keys++;
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
