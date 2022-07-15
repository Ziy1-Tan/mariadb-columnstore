#include <string_view>
#include <memory>
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
CalpontSystemCatalog::ColType Func_json_object::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp.size() > 0 ? fp[0]->data()->resultType() : resultType;
}

string Func_json_object::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& type)
{
  if (fp.size() == 0)
    return "{}";

  string ret("{");

  ret.append(getJsonKeyName(row, fp[0]));
  ret.append(getJsonValue(row, fp[1]));

  for (size_t i = 2; i < fp.size(); i += 2)
  {
    ret.append(", ");
    ret.append(getJsonKeyName(row, fp[i]));
    ret.append(getJsonValue(row, fp[i + 1]));
  }
  ret.append("}");
  return ret;
}
}  // namespace funcexp
