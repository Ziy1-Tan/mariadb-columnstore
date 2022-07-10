#include <string_view>
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
  return resultType;
}

/**
 * getBoolVal API definition
 */
bool Func_json_equals::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                  CalpontSystemCatalog::ColType& type)
{
  bool result = false;

  const DynamicString dynamicStringA;
  const DynamicString dynamicStringB;

  if (dynamicStringA.empty() || dynamicStringB.empty())
  {
    isNull = true;
    return false;
  }

  const string_view tmpJsA = fp[0]->data()->getStrVal(row, isNull);

  if (isNull)
  {
    return false;
  }

  const string_view tmpJsB = fp[0]->data()->getStrVal(row, isNull);

  if (isNull)
  {
    return false;
  }

  if (json_normalize(dynamicStringA.data(), tmpJsA.data(), tmpJsA.size(),
                     fp[0]->data()->resultType().getCharset()))
  {
    isNull = true;
    return result;
  }

  if (json_normalize(dynamicStringB.data(), tmpJsB.data(), tmpJsB.size(),
                     fp[1]->data()->resultType().getCharset()))
  {
    isNull = true;
    return result;
  }

  result = strcmp(dynamicStringA.data()->str, dynamicStringB.data()->str) ? false : true;
  return result;
}
}  // namespace funcexp
