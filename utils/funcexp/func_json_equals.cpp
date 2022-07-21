#include <string_view>
#include <memory>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
#include "rowgroup.h"
using namespace execplan;
using namespace rowgroup;

#include "dataconvert.h"

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_equals::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

/**
 * getBoolVal API definition
 */
bool Func_json_equals::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                  CalpontSystemCatalog::ColType& type)
{
  // auto release the DYNAMIC_STRING
  using DynamicString = unique_ptr<DYNAMIC_STRING, decltype(&dynstr_free)>;

  DynamicString dynStr1{new DYNAMIC_STRING(), dynstr_free};
  if (init_dynamic_string(&*dynStr1, NULL, 0, 0))
  {
    isNull = true;
    return true;
  }

  DynamicString dynStr2{new DYNAMIC_STRING(), dynstr_free};
  if (init_dynamic_string(&*dynStr2, NULL, 0, 0))
  {
    isNull = true;
    return true;
  }

  const string_view jsExp1 = fp[0]->data()->getStrVal(row, isNull);

  if (isNull)
  {
    return false;
  }

  const string_view jsExp2 = fp[1]->data()->getStrVal(row, isNull);

  if (isNull)
  {
    return false;
  }

  bool result = false;
  if (json_normalize(&*dynStr1, jsExp1.data(), jsExp1.size(), fp[0]->data()->resultType().getCharset()))
  {
    isNull = true;
    return result;
  }

  if (json_normalize(&*dynStr2, jsExp2.data(), jsExp2.size(), fp[1]->data()->resultType().getCharset()))
  {
    isNull = true;
    return result;
  }

  result = strcmp(dynStr1->str, dynStr2->str) ? false : true;
  return result;
}
}  // namespace funcexp
