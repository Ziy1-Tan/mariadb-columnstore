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
CalpontSystemCatalog::ColType Func_json_equals::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

/**
 * getBoolVal API definition
 */
bool Func_json_equals::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                  CalpontSystemCatalog::ColType& op_ct)
{
  bool result = false;

  DYNAMIC_STRING a_res;
  if (init_dynamic_string(&a_res, NULL, 0, 0))
  {
    isNull = true;
    return true;
  }

  DYNAMIC_STRING b_res;
  if (init_dynamic_string(&b_res, NULL, 0, 0))
  {
    dynstr_free(&a_res);
    isNull = true;
    return true;
  }

  const std::string a = fp[0]->data()->getStrVal(row, isNull);

  if (isNull)
  {
    dynstr_free(&b_res);
    dynstr_free(&a_res);
    return false;
  }

  const std::string b = fp[0]->data()->getStrVal(row, isNull);

  if (isNull)
  {
    dynstr_free(&b_res);
    dynstr_free(&a_res);
    return false;
  }

  const char* a_js = a.c_str();
  CHARSET_INFO* a_cs = fp[0]->data()->resultType().getCharset();
  const char* b_js = b.c_str();
  CHARSET_INFO* b_cs = fp[1]->data()->resultType().getCharset();

  if (json_normalize(&a_res, a_js, strlen(a_js), a_cs))
  {
    isNull = true;
    dynstr_free(&b_res);
    dynstr_free(&a_res);
    return result;
  }

  if (json_normalize(&b_res, b_js, strlen(b_js), b_cs))
  {
    isNull = true;
    dynstr_free(&b_res);
    dynstr_free(&a_res);
    return result;
  }

  result = strcmp(a_res.str, b_res.str) ? false : true;
  return result;
}

}  // namespace funcexp
