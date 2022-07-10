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
CalpontSystemCatalog::ColType Func_json_normalize::operationType(FunctionParm& fp,
                                                                 CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

string Func_json_normalize::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                      execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);

  if (isNull)
    return "";

  const DynamicString dynamicString;

  if (dynamicString.empty())
    return "";

  if (json_normalize(dynamicString.data(), tmpJs.data(), tmpJs.size(),
                     fp[0]->data()->resultType().getCharset()))
  {
    isNull = true;
    return "";
  }

  return dynamicString.data()->str;
}
}  // namespace funcexp
