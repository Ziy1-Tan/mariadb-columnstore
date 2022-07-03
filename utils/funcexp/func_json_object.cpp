#include <string>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

#include "mcs_datatype.h"
using namespace datatypes;
namespace funcexp
{
namespace helpers
{
string getStrEscaped(const char* js, const CHARSET_INFO* cs)
{
  char buf[1024];
  int str_len = strlen(js) * 12 * cs->mbmaxlen / cs->mbminlen;
  if ((str_len = json_escape(cs, (const uchar*)js, (const uchar*)js + strlen(js), cs, (uchar*)buf,
                             (uchar*)buf + str_len)) > 0)
  {
    buf[str_len] = '\0';
    return string(buf);
  }
  return "";
}

string getJsonKeyName(rowgroup::Row& row, SPTP& parm, bool& isNull)
{
  const string tmp_js = parm->data()->getStrVal(row, isNull);
  if (isNull)
  {
    string ret("");
    ret.append("\"\": ");
    return ret;
  }

  string ret("\"");
  ret.append(getStrEscaped(tmp_js.c_str(), parm->data()->resultType().getCharset()));
  ret.append("\": ");
  return ret;
}

string getJsonValue(rowgroup::Row& row, SPTP& parm, bool& isNull)
{
  const string tmp_js = parm->data()->getStrVal(row, isNull);
  if (isNull)
  {
    isNull = false;
    return "null";
  }

  datatypes::SystemCatalog::ColDataType dataType = parm->data()->resultType().colDataType;
  if (dataType == datatypes::SystemCatalog::BIGINT && (tmp_js == "true" || tmp_js == "false"))
  {
    return tmp_js;
  }

  const string strEscaped = getStrEscaped(tmp_js.c_str(), parm->data()->resultType().getCharset());

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

}  // namespace helpers
}  // namespace funcexp
namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_object::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

string Func_json_object::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& type)
{
  if (fp.size() == 0)
    return "{}";

  string ret("{");

  ret.append(helpers::getJsonKeyName(row, fp[0], isNull));
  ret.append(helpers::getJsonValue(row, fp[1], isNull));

  for (size_t i = 2; i < fp.size(); i += 2)
  {
    ret.append(", ");
    ret.append(helpers::getJsonKeyName(row, fp[i], isNull));
    ret.append(helpers::getJsonValue(row, fp[i + 1], isNull));
  }
  ret.append("}");
  return ret;
}
}  // namespace funcexp
