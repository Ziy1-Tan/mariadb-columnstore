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

  DYNAMIC_STRING tmpStringA;
  if (init_dynamic_string(&tmpStringA, NULL, 0, 0))
  {
    isNull = true;
    return true;
  }
  DynamicString dynamicStringA{&tmpStringA, dynstr_free};

  DYNAMIC_STRING tmpStringB;
  if (init_dynamic_string(&tmpStringB, NULL, 0, 0))
  {
    isNull = true;
    return true;
  }
  DynamicString dynamicStringB{&tmpStringB, dynstr_free};

  const string_view tmpJsA = fp[0]->data()->getStrVal(row, isNull);

  if (isNull)
  {
    return false;
  }

  const string_view tmpJsB = fp[1]->data()->getStrVal(row, isNull);

  if (isNull)
  {
    return false;
  }

  bool result = false;
  if (json_normalize(&*dynamicStringA, tmpJsA.data(), tmpJsA.size(),
                     fp[0]->data()->resultType().getCharset()))
  {
    isNull = true;
    return result;
  }

  if (json_normalize(&*dynamicStringB, tmpJsB.data(), tmpJsB.size(),
                     fp[1]->data()->resultType().getCharset()))
  {
    isNull = true;
    return result;
  }

  result = strcmp(dynamicStringA->str, dynamicStringB->str) ? false : true;
  return result;
}
}  // namespace funcexp
