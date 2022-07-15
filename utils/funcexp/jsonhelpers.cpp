#include "jsonhelpers.h"

using namespace std;

namespace funcexp
{
namespace helpers
{

int setupPathNoWildcard(json_path_t* path, CHARSET_INFO* cs, const uchar* str, const uchar* end)
{
  if (!json_path_setup(path, cs, str, end))
  {
    if ((path->types_used & (JSON_PATH_WILD | JSON_PATH_DOUBLE_WILD | JSON_PATH_ARRAY_RANGE)) == 0)
      return 0;
    path->s.error = NO_WILDCARD_ALLOWED;
  }
  return 1;
}

string getStrEscaped(const char* js, const size_t jsLen, const CHARSET_INFO* cs)
{
  int strLen = jsLen * 12 * cs->mbmaxlen / cs->mbminlen;
  char* buf = new char[strLen];
  if ((strLen = json_escape(cs, (const uchar*)js, (const uchar*)js + jsLen, cs, (uchar*)buf,
                            (uchar*)buf + strLen)) > 0)
  {
    buf[strLen] = '\0';
    string ret = buf;
    delete[] buf;
    return ret;
  }

  delete[] buf;
  return "";
}

string getJsonKeyName(rowgroup::Row& row, execplan::SPTP& parm)
{
  bool isNull = false;
  const string_view tmpJs = parm->data()->getStrVal(row, isNull);
  if (isNull)
  {
    return "\"\": ";
  }

  string ret("\"");
  ret.append(getStrEscaped(tmpJs.data(), tmpJs.size(), parm->data()->resultType().getCharset()));
  ret.append("\": ");
  return ret;
}

string getJsonValue(rowgroup::Row& row, execplan::SPTP& parm)
{
  bool isNull = false;
  const string_view tmpJs = parm->data()->getStrVal(row, isNull);
  if (isNull)
  {
    return "null";
  }

  datatypes::SystemCatalog::ColDataType dataType = parm->data()->resultType().colDataType;
  if (dataType == datatypes::SystemCatalog::BIGINT && (tmpJs == "true" || tmpJs == "false"))
  {
    return tmpJs.data();
  }

  const string strEscaped =
      getStrEscaped(tmpJs.data(), tmpJs.size(), parm->data()->resultType().getCharset());

  string ret;
  if (isCharType(dataType))
  {
    ret.push_back('\"');
    ret.append(strEscaped);
    ret.push_back('\"');
  }
  else
    ret.append(strEscaped);

  return ret;
}

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
