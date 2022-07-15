#include <cassert>
#include <string>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "joblisttypes.h"
using namespace joblist;

#include "jsonhelpers.h"
using namespace funcexp::helpers;



namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_format::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_format::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                   execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  int tabSize = 4;
  json_engine_t je;

  if (fmt == DETAILED)
  {
    if (fp.size() > 1)
    {
      tabSize = fp[1]->data()->getIntVal(row, isNull);
      if (isNull)
        return "";

      if (tabSize < 0)
        tabSize = 0;
      else if (tabSize > TAB_SIZE_LIMIT)
        tabSize = TAB_SIZE_LIMIT;
    }
  }

  json_scan_start(&je, fp[0]->data()->resultType().getCharset(), (const uchar*)tmpJs.data(),
                  (const uchar*)tmpJs.data() + tmpJs.size());

  string ret;
  if (doFormat(&je, ret, fmt, tabSize))
  {
    isNull = true;
    return "";
  }

  isNull = false;
  return ret;
}
}  // namespace funcexp
