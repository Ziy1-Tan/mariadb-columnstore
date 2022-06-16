#include <string>
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
CalpontSystemCatalog::ColType Func_json_type::operationType(FunctionParm& fp,
                                                            CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

std::string Func_json_type::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                      execplan::CalpontSystemCatalog::ColType& op_ct)
{
  const std::string& str = fp[0]->data()->getStrVal(row, isNull);
  CHARSET_INFO* cs = fp[0]->data()->resultType().getCharset();

  json_engine_t je;
  string type;

  if (isNull)
    return "";

  const char* js = str.c_str();

  json_scan_start(&je, cs, (const uchar*)js, (const uchar*)js + strlen(js));

  if (json_read_value(&je))
  {
    // TODO: report_json_error_ex()
    isNull = true;
    return "";
  }

  switch (je.value_type)
  {
    case JSON_VALUE_OBJECT: type = "OBJECT"; break;
    case JSON_VALUE_ARRAY: type = "ARRAY"; break;
    case JSON_VALUE_STRING: type = "STRING"; break;
    case JSON_VALUE_NUMBER: type = (je.num_flags & JSON_NUM_FRAC_PART) ? "DOUBLE" : "INTEGER"; break;
    case JSON_VALUE_TRUE:
    case JSON_VALUE_FALSE: type = "BOOLEAN"; break;
    default: type = "NULL"; break;
  }

  return type;
}

}  // namespace funcexp