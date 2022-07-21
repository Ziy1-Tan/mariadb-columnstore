#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
#include "rowgroup.h"
using namespace execplan;
using namespace rowgroup;

#include "dataconvert.h"

#include "jsonhelpers.h"
using namespace funcexp::helpers;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_exists::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

/**
 * getBoolVal API definition
 */
bool Func_json_exists::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                  CalpontSystemCatalog::ColType& type)
{
  const string_view jsExp = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return false;

  json_engine_t jsEg;

  if (!path.parsed)
  {
    // check if path column is const
    if (!path.constant)
      markConstFlag(path, fp[1]);

    const string_view pathExp = fp[1]->data()->getStrVal(row, isNull);
    const char* rawPath = pathExp.data();
    if (isNull || json_path_setup(&path.p, fp[1]->data()->resultType().getCharset(), (const uchar*)rawPath,
                                  (const uchar*)rawPath + pathExp.size()))
      goto error;

    path.parsed = path.constant;
  }

  json_scan_start(&jsEg, fp[0]->data()->resultType().getCharset(), (const uchar*)jsExp.data(),
                  (const uchar*)jsExp.data() + jsExp.size());

  path.currStep = path.p.steps;
  if (jsonFindPath(&jsEg, &path.p, &path.currStep))
  {
    if (jsEg.s.error)
      goto error;

    return false;
  }

  return true;

error:
  isNull = true;
  return false;
}
}  // namespace funcexp
