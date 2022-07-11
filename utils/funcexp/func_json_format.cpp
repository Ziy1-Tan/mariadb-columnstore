#include <cassert>
#include <string>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

#include "jsonfunchelpers.h"
using namespace funcexp::helpers;

namespace
{
int appendTab(string& js, int depth, int tabSize)
{
  try
  {
    js.append("\n");
    for (int i = 0; i < depth; i++)
    {
      js.append(tab_arr, tabSize);
    }
  }
  catch (const std::exception& e)
  {
    return 1;
  }
  return 0;
}
}  // namespace

namespace funcexp
{
namespace helpers
{
int doFormat(json_engine_t* je, string& niceJs, Func_json_format::FORMATS mode, int tabSize)
{
  int depth = 0;
  static const char *comma = ", ", *colon = "\": ";
  uint commaLen, colonLen;
  int firstValue = 1;

  niceJs.reserve(je->s.str_end - je->s.c_str + 32);

  assert(mode != Func_json_format::DETAILED || (tabSize >= 0 && tabSize <= TAB_SIZE_LIMIT));

  if (mode == Func_json_format::LOOSE)
  {
    commaLen = 2;
    colonLen = 3;
  }
  else if (mode == Func_json_format::DETAILED)
  {
    commaLen = 1;
    colonLen = 3;
  }
  else
  {
    commaLen = 1;
    colonLen = 2;
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

        if (!firstValue)
          niceJs.append(comma, commaLen);

        if (mode == Func_json_format::DETAILED && appendTab(niceJs, depth, tabSize))
          goto error;

        niceJs.push_back('"');
        niceJs.append((const char*)key_start, (int)(key_end - key_start));
        niceJs.append(colon, colonLen);
      }
        /* now we have key value to handle, so no 'break'. */
        DBUG_ASSERT(je->state == JST_VALUE);
        goto handle_value;

      case JST_VALUE:
        if (!firstValue)
          niceJs.append(comma, commaLen);

        if (mode == Func_json_format::DETAILED && depth > 0 && appendTab(niceJs, depth, tabSize))
          goto error;

      handle_value:
        if (json_read_value(je))
          goto error;
        if (json_value_scalar(je))
        {
          niceJs.append((const char*)je->value_begin, (int)(je->value_end - je->value_begin));

          firstValue = 0;
        }
        else
        {
          if (mode == Func_json_format::DETAILED && depth > 0 && appendTab(niceJs, depth, tabSize))
            goto error;
          niceJs.append((je->value_type == JSON_VALUE_OBJECT) ? "{" : "[", 1);
          firstValue = 1;
          depth++;
        }

        break;

      case JST_OBJ_END:
      case JST_ARRAY_END:
        depth--;
        if (mode == Func_json_format::DETAILED && appendTab(niceJs, depth, tabSize))
          goto error;
        niceJs.append((je->state == JST_OBJ_END) ? "}" : "]", 1);
        firstValue = 0;
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
  return fp[0]->data()->resultType();
}

string Func_json_format::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  int tabSize = 4;
  json_engine_t je;

  if (fmt == DETAILED)
  {
    if (fp.size() > 1)
    {
      tabSize = fp[1]->data()->getIntVal(row, isNull);
      if (isNull)
        return "";

      if (tabSize < 0)
        tabSize = 0;
      else if (tabSize > TAB_SIZE_LIMIT)
        tabSize = TAB_SIZE_LIMIT;
    }
  }

  json_scan_start(&je, fp[0]->data()->resultType().getCharset(), (const uchar*)tmpJs.data(),
                  (const uchar*)tmpJs.data() + tmpJs.size());

  string ret;
  if (doFormat(&je, ret, fmt, tabSize))
  {
    isNull = true;
    return "";
  }

  isNull = false;
  return ret;
}
}  // namespace funcexp
