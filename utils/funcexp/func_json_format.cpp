#include <cassert>
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

namespace funcexp
{
namespace helpers
{
int append_tab(string& js, int depth, int tab_size)
{
  try
  {
    js.append("\n");
    for (int i = 0; i < depth; i++)
    {
      js.append(tab_arr, tab_size);
    }
  }
  catch (const std::exception& e)
  {
    return 1;
  }
  return 0;
}

int json_nice(json_engine_t* je, string& nice_js, Func_json_format::formats mode, int tab_size)
{
  int depth = 0;
  static const char *comma = ", ", *colon = "\": ";
  uint comma_len, colon_len;
  int first_value = 1;

  nice_js.reserve(je->s.str_end - je->s.c_str + 32);

  assert(mode != Func_json_format::DETAILED || (tab_size >= 0 && tab_size <= TAB_SIZE_LIMIT));

  if (mode == Func_json_format::LOOSE)
  {
    comma_len = 2;
    colon_len = 3;
  }
  else if (mode == Func_json_format::DETAILED)
  {
    comma_len = 1;
    colon_len = 3;
  }
  else
  {
    comma_len = 1;
    colon_len = 2;
  }

  do
  {
    switch (je->state)
    {
      case JST_KEY:
      {
        const uchar* key_start = je->s.c_str;
        const uchar* key_end;

        do
        {
          key_end = je->s.c_str;
        } while (json_read_keyname_chr(je) == 0);

        if (unlikely(je->s.error))
          goto error;

        if (!first_value)
          nice_js.append(comma, comma_len);

        if (mode == Func_json_format::DETAILED && append_tab(nice_js, depth, tab_size))
          goto error;

        nice_js.push_back('"');
        nice_js.append((const char*)key_start, (int)(key_end - key_start));
        nice_js.append(colon, colon_len);
      }
        /* now we have key value to handle, so no 'break'. */
        DBUG_ASSERT(je->state == JST_VALUE);
        goto handle_value;

      case JST_VALUE:
        if (!first_value)
          nice_js.append(comma, comma_len);

        if (mode == Func_json_format::DETAILED && depth > 0 && append_tab(nice_js, depth, tab_size))
          goto error;

      handle_value:
        if (json_read_value(je))
          goto error;
        if (json_value_scalar(je))
        {
          nice_js.append((const char*)je->value_begin, (int)(je->value_end - je->value_begin));

          first_value = 0;
        }
        else
        {
          if (mode == Func_json_format::DETAILED && depth > 0 && append_tab(nice_js, depth, tab_size))
            goto error;
          nice_js.append((je->value_type == JSON_VALUE_OBJECT) ? "{" : "[", 1);
          first_value = 1;
          depth++;
        }

        break;

      case JST_OBJ_END:
      case JST_ARRAY_END:
        depth--;
        if (mode == Func_json_format::DETAILED && append_tab(nice_js, depth, tab_size))
          goto error;
        nice_js.append((je->state == JST_OBJ_END) ? "}" : "]", 1);
        first_value = 0;
        break;

      default: break;
    };
  } while (json_scan_next(je) == 0);

  return je->s.error || *je->killed_ptr;

error:
  return 1;
}
}  // namespace helpers
}  // namespace funcexp

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_format::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

string Func_json_format::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& type)
{
  const string tmp_js = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  int tab_size = 4;
  json_engine_t je;

  if (fmt == DETAILED)
  {
    if (fp.size() > 1)
    {
      tab_size = fp[1]->data()->getIntVal(row, isNull);
      if (isNull)
        return "";

      if (tab_size < 0)
        tab_size = 0;
      else if (tab_size > helpers::TAB_SIZE_LIMIT)
        tab_size = helpers::TAB_SIZE_LIMIT;
    }
  }
  const char* js = tmp_js.c_str();
  const CHARSET_INFO* cs = fp[0]->data()->resultType().getCharset();

  json_scan_start(&je, cs, (const uchar*)js, (const uchar*)js + strlen(js));

  string ret;
  if (helpers::json_nice(&je, ret, fmt, tab_size))
  {
    isNull = true;
    return "";
  }

  isNull = false;
  return ret;
}
}  // namespace funcexp
