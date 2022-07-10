#include <string_view>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_unquote::operationType(FunctionParm& fp,
                                                               CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

std::string Func_json_unquote::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                         execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  json_engine_t je;
  int strLen;

  const char* js = tmpJs.data();
  const CHARSET_INFO* cs = type.getCharset();

  json_scan_start(&je, cs, (const uchar*)js, (const uchar*)js + tmpJs.size());

  json_read_value(&je);

  if (unlikely(je.s.error) || je.value_type != JSON_VALUE_STRING)
    return js;

  char buf[je.value_len];
  if ((strLen = json_unescape(cs, je.value, je.value + je.value_len, &my_charset_utf8mb3_general_ci,
                              (uchar*)buf, (uchar*)(buf + je.value_len))) >= 0)
  {
    buf[strLen] = '\0';
    return strLen == 0 ? "" : string(buf);
  }

  return js;
}
}  // namespace funcexp
