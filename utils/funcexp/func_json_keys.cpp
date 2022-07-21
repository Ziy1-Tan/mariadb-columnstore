#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
using namespace execplan;

#include "rowgroup.h"
using namespace rowgroup;

#include "dataconvert.h"
using namespace dataconvert;

#include "jsonhelpers.h"
using namespace funcexp::helpers;

namespace
{
bool checkKeyInList(const string& res, const uchar* key, const int keyLen)
{
  const uchar* curr = (const uchar*)res.c_str() + 2;             /* beginning '["' */
  const uchar* end = (const uchar*)res.c_str() + res.size() - 1; /* ending '"' */

  while (curr < end)
  {
    int i;
    for (i = 0; curr[i] != '"' && i < keyLen; i++)
    {
      if (curr[i] != key[i])
        break;
    }
    if (curr[i] == '"')
    {
      if (i == keyLen)
        return true;
    }
    else
    {
      while (curr[i] != '"')
        i++;
    }
    curr += i + 4; /* skip ', "' */
  }
  return false;
}
}  // namespace

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_keys::operationType(FunctionParm& fp,
                                                            CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

string Func_json_keys::getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                                 execplan::CalpontSystemCatalog::ColType& type)
{
  const string_view jsExp = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return "";

  json_engine_t je;
  IntType keySize = 0;
  string ret;

  json_scan_start(&je, fp[0]->data()->resultType().getCharset(), (const uchar*)jsExp.data(),
                  (const uchar*)jsExp.data() + jsExp.size());

  if (fp.size() > 1)
  {
    if (!path.parsed)
    {
      // check if path column is const
      if (!path.constant)
        markConstFlag(path, fp[1]);

      const string_view pathExp = fp[1]->data()->getStrVal(row, isNull);
      const char* rawPath = pathExp.data();
      if (isNull)
        return "";
      if (pathSetupNwc(&path.p, fp[1]->data()->resultType().getCharset(), (const uchar*)rawPath,
                       (const uchar*)rawPath + pathExp.size()))
        goto error;

      path.parsed = path.constant;
    }

    path.currStep = path.p.steps;
    if (jsonFindPath(&je, &path.p, &path.currStep))
    {
      if (je.s.error)
      {
      }
      goto error;
    }
  }

  if (json_read_value(&je))
    goto error;

  if (je.value_type != JSON_VALUE_OBJECT)
    goto error;

  ret.append("[");
  while (json_scan_next(&je) == 0 && je.state != JST_OBJ_END)
  {
    const uchar *keyStart, *keyEnd;
    int keyLen;

    switch (je.state)
    {
      case JST_KEY:
        keyStart = je.s.c_str;
        do
        {
          keyEnd = je.s.c_str;
        } while (json_read_keyname_chr(&je) == 0);

        if (unlikely(je.s.error))
          goto error;

        keyLen = (int)(keyEnd - keyStart);

        if (!checkKeyInList(ret, keyStart, keyLen))
        {
          if (keySize > 0)
            ret.append(", ");
          ret.append("\"");
          ret.append((const char*)keyStart, keyLen);
          ret.append("\"");
          keySize++;
        }
        break;
      case JST_OBJ_START:
      case JST_ARRAY_START:
        if (json_skip_level(&je))
          break;
        break;
      default: break;
    }
  }

  if (unlikely(!je.s.error))
  {
    ret.append("]");
    return ret;
  }

error:
  isNull = true;
  return "";
}
}  // namespace funcexp
