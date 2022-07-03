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
CalpontSystemCatalog::ColType Func_json_normalize::operationType(FunctionParm& fp,
                                                                 CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

string Func_json_normalize::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                      execplan::CalpontSystemCatalog::ColType& type)
{
  const string tmp_js = fp[0]->data()->getStrVal(row, isNull);

  if (isNull)
    return "";

  const char* raw_json = tmp_js.c_str();

  DYNAMIC_STRING normalized_json;
  if (init_dynamic_string(&normalized_json, NULL, 0, 0))
  {
    isNull = true;
    dynstr_free(&normalized_json);
    return "";
  }

  if (json_normalize(&normalized_json, raw_json, strlen(raw_json), type.getCharset()))
  {
    isNull = true;
    dynstr_free(&normalized_json);
    return "";
  }

  string ret = normalized_json.str;
  dynstr_free(&normalized_json);
  return ret;
}
}  // namespace funcexp
