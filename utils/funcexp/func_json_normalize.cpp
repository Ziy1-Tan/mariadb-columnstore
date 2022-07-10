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
  return fp[0]->data()->resultType();
}

string Func_json_normalize::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                      execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  using DynamicString = unique_ptr<DYNAMIC_STRING, decltype(&dynstr_free)>;

  DYNAMIC_STRING tmpString;
  if (init_dynamic_string(&tmpString, NULL, 0, 0))
  {
    isNull = true;
    return "";
  }
  DynamicString dynamicString{&tmpString, dynstr_free};

  if (json_normalize(&*dynamicString, tmpJs.data(), tmpJs.size(), fp[0]->data()->resultType().getCharset()))
  {
    isNull = true;
    return "";
  }

  return dynamicString->str;
}
}  // namespace funcexp
