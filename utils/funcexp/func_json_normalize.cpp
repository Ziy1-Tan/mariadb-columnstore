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
  const string_view jsExp = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  using DynamicString = unique_ptr<DYNAMIC_STRING, decltype(&dynstr_free)>;

  DynamicString dynStr{new DYNAMIC_STRING(), dynstr_free};
  if (init_dynamic_string(&*dynStr, NULL, 0, 0))
    goto error;

  if (json_normalize(&*dynStr, jsExp.data(), jsExp.size(), fp[0]->data()->resultType().getCharset()))
    goto error;

  return dynStr->str;

error:
  isNull = true;
  return "";
}
}  // namespace funcexp
