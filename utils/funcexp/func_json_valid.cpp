#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
#include "rowgroup.h"
using namespace execplan;
using namespace rowgroup;

#include "dataconvert.h"

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_valid::operationType(FunctionParm& fp,
                                                             CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

/**
 * getBoolVal API definition
 */
bool Func_json_valid::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                 CalpontSystemCatalog::ColType& type)
{
  const string& tmp_js = fp[0]->data()->getStrVal(row, isNull);
  CHARSET_INFO* cs = fp[0]->data()->resultType().getCharset();

  if (isNull)
    return false;

  const char* js = tmp_js.c_str();

  return json_valid(js, strlen(js), cs);
}

}  // namespace funcexp
