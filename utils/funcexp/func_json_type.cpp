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
CalpontSystemCatalog::ColType Func_json_type::operationType(FunctionParm& fp,
                                                            CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_type::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                 execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  json_engine_t je;
  string result;

  json_scan_start(&je, fp[0]->data()->resultType().getCharset(), (const uchar*)tmpJs.data(),
                  (const uchar*)tmpJs.data() + tmpJs.size());

  if (json_read_value(&je))
  {
    isNull = true;
    return "";
  }

  switch (je.value_type)
  {
    case JSON_VALUE_OBJECT: result = "OBJECT"; break;
    case JSON_VALUE_ARRAY: result = "ARRAY"; break;
    case JSON_VALUE_STRING: result = "STRING"; break;
    case JSON_VALUE_NUMBER: result = (je.num_flags & JSON_NUM_FRAC_PART) ? "DOUBLE" : "INTEGER"; break;
    case JSON_VALUE_TRUE:
    case JSON_VALUE_FALSE: result = "BOOLEAN"; break;
    default: result = "NULL"; break;
  }

  return result;
}
}  // namespace funcexp
