#include <cstring>
#include <string>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "dataconvert.h"
using namespace dataconvert;

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_length::operationType(FunctionParm& fp,
                                                              CalpontSystemCatalog::ColType& resultType)
{
  if (fp.size() > 1)
    path.set_constant_flag(fp[2]->data()->getIntVal());
  return resultType;
}

int64_t Func_json_length::getIntVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                    execplan::CalpontSystemCatalog::ColType& op_ct)
{
  const std::string tmp_js = fp[0]->data()->getStrVal(row, isNull);
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
    if (!path.parsed)
    {
      const std::string tmp_path = fp[1]->data()->getStrVal(row, isNull);
      if (isNull)
        return 0;
      const char* s_p = tmp_path.c_str();
      const CHARSET_INFO* s_p_cs = fp[1]->data()->resultType().getCharset();
      if (s_p && path_setup_nwc(&path.p, s_p_cs, (const uchar*)s_p, (const uchar*)s_p + strlen(s_p)))
      {
        isNull = true;
        return 0;
      }
      path.parsed = path.constant;
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
