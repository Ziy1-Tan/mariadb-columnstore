#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
#include "rowgroup.h"
using namespace execplan;
using namespace rowgroup;

#include "dataconvert.h"

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_exists::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

/**
 * getBoolVal API definition
 */
bool Func_json_exists::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                  CalpontSystemCatalog::ColType& type)
{
  const string tmp_js = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return false;

  json_engine_t je;
  uint array_counters[JSON_DEPTH_LIMIT];

  const char* js = tmp_js.c_str();
  const CHARSET_INFO* js_cs = fp[0]->data()->resultType().getCharset();

  if (!path.parsed)
  {
    const string tmp_path = fp[1]->data()->getStrVal(row, isNull);
    if (isNull)
      return false;

    const char* path_str = tmp_path.c_str();
    const CHARSET_INFO* path_cs = fp[1]->data()->resultType().getCharset();
    if (path_str &&
        json_path_setup(&path.p, path_cs, (const uchar*)path_str, (const uchar*)path_str + strlen(path_str)))
    {
      isNull = true;
      return false;
    }
  }

  isNull = false;
  json_scan_start(&je, js_cs, (const uchar*)js, (const uchar*)js + strlen(js));

  path.cur_step = path.p.steps;
  if (json_find_path(&je, &path.p, &path.cur_step, array_counters))
  {
    if (je.s.error)
    {
      isNull = true;
      return false;
    }
    return false;
  }

  return true;
}
}  // namespace funcexp
