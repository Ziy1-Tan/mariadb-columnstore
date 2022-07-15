#include <string_view>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

#include "mcs_datatype.h"
using namespace datatypes;

#include "jsonhelpers.h"
using namespace funcexp::helpers;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_quote::operationType(FunctionParm& fp,
                                                             CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

std::string Func_json_quote::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                       execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull || !isCharType(fp[0]->data()->resultType().colDataType))
    return "";

  string ret;

  ret.append("\"");
  ret.append(getStrEscaped(tmpJs.data(), tmpJs.size(), fp[0]->data()->resultType().getCharset()));
  ret.append("\"");

  return ret;
}
}  // namespace funcexp
