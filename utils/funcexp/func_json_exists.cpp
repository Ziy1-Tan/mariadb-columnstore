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
CalpontSystemCatalog::ColType Func_json_exists::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

/**
 * getBoolVal API definition
 */
bool Func_json_exists::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                  CalpontSystemCatalog::ColType& op_ct)
{
  json_engine_t je;
  uint array_counters[JSON_DEPTH_LIMIT];

  const std::string json_str = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return false;
  const char* js = json_str.c_str();
  const CHARSET_INFO* js_cs = fp[0]->data()->resultType().getCharset();

  // TODO: path.parsed
  const std::string path_str = fp[1]->data()->getStrVal(row, isNull);
  if (isNull)
    return false;

  const char* s_p = path_str.c_str();
  CHARSET_INFO* s_p_cs = fp[1]->data()->resultType().getCharset();
  if (s_p && json_path_setup(&path.p, s_p_cs, (const uchar*)s_p, (const uchar*)s_p + strlen(s_p)))
  {
    isNull = true;
    return false;
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
