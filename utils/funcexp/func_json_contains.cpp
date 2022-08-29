#include <string_view>
using namespace std;

#include "functor_json.h"
#include "functioncolumn.h"
#include "constantcolumn.h"
#include "rowgroup.h"
using namespace execplan;
using namespace rowgroup;

#include "dataconvert.h"

#include "jsonhelpers.h"
using namespace funcexp::helpers;

namespace
{
static bool findKeyInObject(json_engine_t* jsEg, json_string_t* key)
{
  const uchar* str = key->c_str;

  while (json_scan_next(jsEg) == 0 && jsEg->state != JST_OBJ_END)
  {
    DBUG_ASSERT(jsEg->state == JST_KEY);
    if (json_key_matches(jsEg, key))
      return true;
    if (json_skip_key(jsEg))
      return false;
    key->c_str = str;
  }

  return false;
}

static bool checkContains(json_engine_t* jsEg, json_engine_t* valEg)
{
  json_engine_t localJsEg;
  bool isEgSet;

  switch (jsEg->value_type)
  {
    case JSON_VALUE_OBJECT:
    {
      json_string_t keyName;

      if (valEg->value_type != JSON_VALUE_OBJECT)
        return false;

      localJsEg = *jsEg;
      isEgSet = false;
      json_string_set_cs(&keyName, valEg->s.cs);
      while (json_scan_next(valEg) == 0 && valEg->state != JST_OBJ_END)
      {
        const uchar *keyStart, *keyEnd;

        DBUG_ASSERT(valEg->state == JST_KEY);
        keyStart = valEg->s.c_str;
        do
        {
          keyEnd = valEg->s.c_str;
        } while (json_read_keyname_chr(valEg) == 0);

        if (unlikely(valEg->s.error) || json_read_value(valEg))
          return false;

        if (isEgSet)
          *jsEg = localJsEg;
        else
          isEgSet = true;

        json_string_set_str(&keyName, keyStart, keyEnd);
        if (!findKeyInObject(jsEg, &keyName) || json_read_value(jsEg) || !checkContains(jsEg, valEg))
          return false;
      }

      return valEg->state == JST_OBJ_END && !json_skip_level(jsEg);
    }
    case JSON_VALUE_ARRAY:
      if (valEg->value_type != JSON_VALUE_ARRAY)
      {
        localJsEg = *valEg;
        isEgSet = false;
        while (json_scan_next(jsEg) == 0 && jsEg->state != JST_ARRAY_END)
        {
          int currLevel, isScaler;
          DBUG_ASSERT(jsEg->state == JST_VALUE);
          if (json_read_value(jsEg))
            return false;

          if (!(isScaler = json_value_scalar(jsEg)))
            currLevel = json_get_level(jsEg);

          if (isEgSet)
            *valEg = localJsEg;
          else
            isEgSet = true;

          if (checkContains(jsEg, valEg))
          {
            if (json_skip_level(jsEg))
              return false;
            return true;
          }
          if (unlikely(valEg->s.error) || unlikely(jsEg->s.error) ||
              (!isScaler && json_skip_to_level(jsEg, currLevel)))
            return false;
        }
        return false;
      }
      /* else */
      localJsEg = *jsEg;
      isEgSet = false;
      while (json_scan_next(valEg) == 0 && valEg->state != JST_ARRAY_END)
      {
        DBUG_ASSERT(valEg->state == JST_VALUE);
        if (json_read_value(valEg))
          return false;

        if (isEgSet)
          *jsEg = localJsEg;
        else
          isEgSet = true;
        if (!checkContains(jsEg, valEg))
          return false;
      }

      return valEg->state == JST_ARRAY_END;

    case JSON_VALUE_STRING:
      if (valEg->value_type != JSON_VALUE_STRING)
        return false;
      /*
         TODO: make proper json-json comparison here that takes excipient
               into account.
       */
      return valEg->value_len == jsEg->value_len && memcmp(valEg->value, jsEg->value, valEg->value_len) == 0;
    case JSON_VALUE_NUMBER:
      if (valEg->value_type == JSON_VALUE_NUMBER)
      {
        double jsEgVal, valEgVal;
        char* end;
        int err;

        jsEgVal = jsEg->s.cs->strntod((char*)jsEg->value, jsEg->value_len, &end, &err);
        ;
        valEgVal = valEg->s.cs->strntod((char*)valEg->value, valEg->value_len, &end, &err);
        ;

        return (fabs(jsEgVal - valEgVal) < 1e-12);
      }
      else
        return false;

    default: break;
  }

  /*
    We have these not mentioned in the 'switch' above:

    case JSON_VALUE_TRUE:
    case JSON_VALUE_FALSE:
    case JSON_VALUE_NULL:
  */
  return valEg->value_type == jsEg->value_type;
}
}  // namespace

namespace funcexp
{
CalpontSystemCatalog::ColType Func_json_contains::operationType(FunctionParm& fp,
                                                                CalpontSystemCatalog::ColType& resultType)
{
  return fp[0]->data()->resultType();
}

/**
 * getBoolVal API definition
 */
bool Func_json_contains::getBoolVal(Row& row, FunctionParm& fp, bool& isNull,
                                    CalpontSystemCatalog::ColType& type)
{
  const string_view tmpJs = fp[0]->data()->getStrVal(row, isNull);
  if (isNull)
    return false;

  const string_view tmpVal = fp[1]->data()->getStrVal(row, isNull);
  if (isNull)
    return false;

  if (!arg2Parsed)
  {
    if (!arg2Const)
    {
      ConstantColumn* constCol = dynamic_cast<ConstantColumn*>(fp[1]->data());
      arg2Const = (constCol != nullptr);
    }
    arg2Val = tmpVal;
    arg2Parsed = arg2Const;
  }

  json_engine_t jsEg;
  json_scan_start(&jsEg, fp[0]->data()->resultType().getCharset(), (const uchar*)tmpJs.data(),
                  (const uchar*)tmpJs.data() + tmpJs.size());

  if (fp.size() > 2)
  {
    int arrayCounters[JSON_DEPTH_LIMIT];
    if (!path.parsed)
    {
      // check if path column is const
      if (!path.constant)
      {
        ConstantColumn* constCol = dynamic_cast<ConstantColumn*>(fp[2]->data());
        path.set_constant_flag((constCol != nullptr));
      }

      const string_view tmpPath = fp[2]->data()->getStrVal(row, isNull);
      if (isNull)
        return false;

      if (setupPathNoWildcard(&path.p, fp[2]->data()->resultType().getCharset(), (const uchar*)tmpPath.data(),
                              (const uchar*)tmpPath.data() + tmpPath.size()))
      {
        isNull = true;
        return false;
      }
      path.parsed = path.constant;
    }

    path.cur_step = path.p.steps;
    if (json_find_path(&jsEg, &path.p, &path.cur_step, arrayCounters))
    {
      if (jsEg.s.error)
      {
      }
      isNull = true;
      return false;
    }
  }

  json_engine_t valEg;
  json_scan_start(&valEg, fp[1]->data()->resultType().getCharset(), (const uchar*)arg2Val.data(),
                  (const uchar*)arg2Val.data() + arg2Val.size());

  if (json_read_value(&jsEg) || json_read_value(&valEg))

  {
    isNull = true;
    return false;
  }
  bool result = checkContains(&jsEg, &valEg);

  if (unlikely(jsEg.s.error || valEg.s.error))
  {
    isNull = true;
    return false;
  }

  return result;
}
}  // namespace funcexp
