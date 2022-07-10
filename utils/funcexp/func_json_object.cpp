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

#include "jsonfunchelpers.h"
using namespace funcexp::helpers;

namespace funcexp
{
namespace helpers
{
string getStrEscaped(const char* js, const size_t jsLen, const CHARSET_INFO* cs)
{
  int strLen = jsLen * 12 * cs->mbmaxlen / cs->mbminlen;
  char* buf = new char[jsLen + strLen + 1024];
  if ((strLen = json_escape(cs, (const uchar*)js, (const uchar*)js + jsLen, cs, (uchar*)buf,
                            (uchar*)buf + jsLen + strLen + 1024)) > 0)
  {
    buf[strLen] = '\0';
    string ret = buf;
    delete[] buf;
    return ret;
  }

  return "";
}

string getJsonKeyName(rowgroup::Row& row, SPTP& parm)
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

string getJsonValue(rowgroup::Row& row, SPTP& parm)
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
}  // namespace helpers
}  // namespace funcexp

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_object::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp.size() > 0 ? fp[0]->data()->resultType() : resultType;
}

string Func_json_object::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& type)
{
  if (fp.size() == 0)
    return "{}";

  string ret("{");

  ret.append(getJsonKeyName(row, fp[0]));
  ret.append(getJsonValue(row, fp[1]));

  for (size_t i = 2; i < fp.size(); i += 2)
  {
    ret.append(", ");
    ret.append(getJsonKeyName(row, fp[i]));
    ret.append(getJsonValue(row, fp[i + 1]));
  }
  ret.append("}");
  return ret;
}
}  // namespace funcexp
