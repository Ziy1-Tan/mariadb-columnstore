#include <string>
using namespace std;

#include "functor_json.h"
#include "jsonfunchelpers.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

#include "mcs_datatype.h"
using namespace datatypes;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_quote::operationType(FunctionParm& fp,
                                                             CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

std::string Func_json_quote::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                       execplan::CalpontSystemCatalog::ColType& type)
{
  const string tmp_js = fp[0]->data()->getStrVal(row, isNull);
  if (isNull || !datatypes::isCharType(fp[0]->data()->resultType().colDataType))
    return "";

  string ret;

  ret.append("\"");
  ret.append(helpers::getStrEscaped(tmp_js.c_str(), type.getCharset()));
  ret.append("\"");

  return ret;
}
}  // namespace funcexp
