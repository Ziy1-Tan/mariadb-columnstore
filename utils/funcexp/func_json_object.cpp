#include <string>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

namespace
{
string getJsonKeyName(const string& str, const CHARSET_INFO* cs, bool& isNull)
{
  if (isNull)
    return "\"\": ";
  const char* js = str.c_str();
  char buf[1024];
  int str_len = strlen(js) * 12 * cs->mbmaxlen / cs->mbminlen;
  if ((str_len = json_escape(cs, (const uchar*)js, (const uchar*)js + strlen(js), cs, (uchar*)buf,
                             (uchar*)buf + str_len)) > 0)
  {
    buf[str_len] = '\0';
    string ret("\"");
    ret.append(buf);
    ret.append("\": ");
    return ret;
  }
  return "\"\": ";
}
string getJsonValue(const string& str, const CHARSET_INFO* cs, bool& isNull)
{
  if (isNull)
    return "null";

  const char* js = str.c_str();
  char buf[1024];
  int str_len = strlen(js) * 12 * cs->mbmaxlen / cs->mbminlen;
  if ((str_len = json_escape(cs, (const uchar*)js, (const uchar*)js + strlen(js), cs, (uchar*)buf,
                             (uchar*)buf + str_len)) > 0)
  {
    buf[str_len] = '\0';
    string ret("\"");
    ret.append(buf);
    ret.append("\"");
    return ret;
  }

  return "null";
}
}  // namespace
namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_object::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

std::string Func_json_object::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                        execplan::CalpontSystemCatalog::ColType& op_ct)
{
  const CHARSET_INFO* cs = op_ct.getCharset();
  string ret("{");
  string tmp;
  for (size_t i = 0; i < fp.size(); i += 2)
  {
    stringValue(fp[i], row, isNull, tmp);
    tmp = getJsonKeyName(tmp, cs, isNull);
    ret.append(tmp);
    stringValue(fp[i + 1], row, isNull, tmp);
    tmp = getJsonValue(tmp, cs, isNull);
    ret.append(tmp);
  }
  ret.append("}");
  return ret;
}

}  // namespace funcexp