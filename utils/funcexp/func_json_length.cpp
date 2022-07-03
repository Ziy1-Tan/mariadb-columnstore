#include <cstring>
#include <string>
using namespace std;

#include "functor_json.h"
#include "jsonfunchelpers.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "dataconvert.h"
using namespace dataconvert;

namespace funcexp
{
namespace helpers
{
int path_setup_nwc(json_path_t* p, CHARSET_INFO* i_cs, const uchar* str, const uchar* end)
{
  if (!json_path_setup(p, i_cs, str, end))
  {
    if ((p->types_used & (JSON_PATH_WILD | JSON_PATH_DOUBLE_WILD)) == 0)
      return 0;
    p->s.error = NO_WILDCARD_ALLOWED;
  }

  return 1;
}
}  // namespace helpers
}  // namespace funcexp
namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_length::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  return resultType;
}

int64_t Func_json_length::getIntVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                    execplan::CalpontSystemCatalog::ColType& type)
{
  const string tmp_js = fp[0]->data()->getStrVal(row, isNull);
  json_engine_t je;
  uint length = 0;
  uint array_counters[JSON_DEPTH_LIMIT];
  int err;

  if (isNull)
    return 0;
  const char* js = tmp_js.c_str();
  const CHARSET_INFO* js_cs = fp[0]->data()->resultType().getCharset();

  json_scan_start(&je, js_cs, (const uchar*)js, (const uchar*)js + strlen(js));

  if (fp.size() > 1)
  {
    const string tmp_path = fp[1]->data()->getStrVal(row, isNull);
    if (isNull)
      return 0;
    const char* path_str = tmp_path.c_str();
    const CHARSET_INFO* path_cs = fp[1]->data()->resultType().getCharset();
    if (path_str && helpers::path_setup_nwc(&path.p, path_cs, (const uchar*)path_str,
                                            (const uchar*)path_str + strlen(path_str)))
    {
      isNull = true;
      return 0;
    }

    path.cur_step = path.p.steps;
    if (json_find_path(&je, &path.p, &path.cur_step, array_counters))
    {
      if (je.s.error)
      {
      }
      isNull = true;
      return 0;
    }
  }

  if (json_read_value(&je))
  {
    isNull = true;
    return 0;
  }

  if (json_value_scalar(&je))
    return 1;

  while (!(err = json_scan_next(&je)) && je.state != JST_OBJ_END && je.state != JST_ARRAY_END)
  {
    switch (je.state)
    {
      case JST_VALUE:
      case JST_KEY: length++; break;
      case JST_OBJ_START:
      case JST_ARRAY_START:
        if (json_skip_level(&je))
        {
          isNull = true;
          return 0;
        }
        break;
      default: break;
    };
  }

  if (!err)
  {
    /* Parse to the end of the JSON just to check it's valid. */
    while (json_scan_next(&je) == 0)
    {
    }
  }

  if (likely(!je.s.error))
    return length;

  isNull = true;
  return 0;
}
}  // namespace funcexp
