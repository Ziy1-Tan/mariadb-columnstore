#pragma once

#include <string>
#define PREFER_MY_CONFIG_H
#include <mariadb.h>
#include <mysql.h>
#include <my_sys.h>
#include <json_lib.h>

#include "collation.h"
#include "functor_bool.h"
#include "functor_int.h"
#include "functor_str.h"

// Check if mariadb version >= 10.9
#if MYSQL_VERSION_ID >= 100900
#ifndef MYSQL_GE_1009
#define MYSQL_GE_1009
#endif
#endif

namespace funcexp
{
// The json_path_t wrapper include some flags
struct JsonPath
{
 public:
  JsonPath() : constant(false), parsed(false), currStep(nullptr)
  {
  }
  json_path_t p;
  bool constant;  // check if the argument is constant
  bool parsed;    // check if the argument is parsed
  json_path_step_t* currStep;
};

class JSEngineScanner : public json_engine_t
{
 public:
  JSEngineScanner(CHARSET_INFO* cs, const uchar* str, const uchar* end)
  {
    json_scan_start(this, cs, str, end);
  }
  JSEngineScanner(const std::string& str, CHARSET_INFO* cs)
   : JSEngineScanner(cs, (const uchar*)str.data(), (const uchar*)str.data() + str.size())
  {
  }
  bool checkAndGetScalar(std::string& ret, int* error);
  bool checkAndGetComplexVal(std::string& ret, int* error);
};

class JSPathExtractor : public JsonPath
{
 protected:
  virtual ~JSPathExtractor()
  {
  }
  virtual bool checkAndGetValue(JSEngineScanner* je, std::string& ret, int* error) = 0;
  bool extract(std::string& ret, rowgroup::Row& row, execplan::SPTP& funcParmJS,
               execplan::SPTP& funcParmPath);
};
/** @brief Func_json_valid class
 */
class Func_json_valid : public Func_Bool
{
 public:
  Func_json_valid() : Func_Bool("json_valid")
  {
  }
  ~Func_json_valid()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  bool getBoolVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                  execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_depth class
 */
class Func_json_depth : public Func_Int
{
 public:
  Func_json_depth() : Func_Int("json_depth")
  {
  }
  virtual ~Func_json_depth()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  int64_t getIntVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                    execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_length class
 */
class Func_json_length : public Func_Int
{
 protected:
  JsonPath path;

 public:
  Func_json_length() : Func_Int("json_length")
  {
  }
  virtual ~Func_json_length()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  int64_t getIntVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                    execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_equals class
 */
class Func_json_equals : public Func_Bool
{
 public:
  Func_json_equals() : Func_Bool("json_equals")
  {
  }
  ~Func_json_equals()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  bool getBoolVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                  execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_normalize class
 */
class Func_json_normalize : public Func_Str
{
 public:
  Func_json_normalize() : Func_Str("json_normalize")
  {
  }
  virtual ~Func_json_normalize()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_type class
 */
class Func_json_type : public Func_Str
{
 public:
  Func_json_type() : Func_Str("json_type")
  {
  }
  virtual ~Func_json_type()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_object class
 */
class Func_json_object : public Func_Str
{
 public:
  Func_json_object() : Func_Str("json_object")
  {
  }
  virtual ~Func_json_object()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_array class
 */
class Func_json_array : public Func_Str
{
 public:
  Func_json_array() : Func_Str("json_array")
  {
  }
  virtual ~Func_json_array()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};
/** @brief Func_json_keys class
 */
class Func_json_keys : public Func_Str
{
 protected:
  JsonPath path;

 public:
  Func_json_keys() : Func_Str("json_keys")
  {
  }
  virtual ~Func_json_keys()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};
/** @brief Func_json_exists class
 */
class Func_json_exists : public Func_Bool
{
 protected:
  JsonPath path;

 public:
  Func_json_exists() : Func_Bool("json_exists")
  {
  }
  ~Func_json_exists()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  bool getBoolVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                  execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_quote class
 */
class Func_json_quote : public Func_Str
{
 protected:
  JsonPath path;

 public:
  Func_json_quote() : Func_Str("json_quote")
  {
  }
  virtual ~Func_json_quote()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_unquote class
 */
class Func_json_unquote : public Func_Str
{
 protected:
  JsonPath path;

 public:
  Func_json_unquote() : Func_Str("json_unquote")
  {
  }
  virtual ~Func_json_unquote()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_format class
 */
class Func_json_format : public Func_Str
{
 public:
  enum FORMATS
  {
    NONE,
    COMPACT,
    LOOSE,
    DETAILED
  };

 protected:
  FORMATS fmt;

 public:
  Func_json_format() : Func_Str("json_detailed"), fmt(DETAILED)
  {
  }
  Func_json_format(FORMATS format) : fmt(format)
  {
    assert(format != NONE);
    switch (format)
    {
      case DETAILED: Func_Str::Func::funcName("json_detailed"); break;
      case LOOSE: Func_Str::Func::funcName("json_loose"); break;
      case COMPACT: Func_Str::Func::funcName("json_compact"); break;
      default: break;
    }
  }
  virtual ~Func_json_format()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};
/** @brief Func_json_merge_preserve class
 */
class Func_json_merge : public Func_Str
{
 public:
  Func_json_merge() : Func_Str("json_merge_preserve")
  {
  }
  virtual ~Func_json_merge()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_merge_patch class
 */
class Func_json_merge_patch : public Func_Str
{
 public:
  Func_json_merge_patch() : Func_Str("json_merge_patch")
  {
  }
  virtual ~Func_json_merge_patch()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_value class
 */
class Func_json_value : public Func_Str, public JSPathExtractor
{
 public:
  Func_json_value() : Func_Str("json_value")
  {
  }
  virtual ~Func_json_value()
  {
  }

  bool checkAndGetValue(JSEngineScanner* je, string& res, int* error) override
  {
    return je->checkAndGetScalar(res, error);
  }

  execplan::CalpontSystemCatalog::ColType operationType(
      FunctionParm& fp, execplan::CalpontSystemCatalog::ColType& resultType) override;

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type) override;
};

/** @brief Func_json_query class
 */
class Func_json_query : public Func_Str, public JSPathExtractor
{
 public:
  Func_json_query() : Func_Str("json_query")
  {
  }
  virtual ~Func_json_query()
  {
  }

  bool checkAndGetValue(JSEngineScanner* je, string& res, int* error) override
  {
    return je->checkAndGetComplexVal(res, error);
  }

  execplan::CalpontSystemCatalog::ColType operationType(
      FunctionParm& fp, execplan::CalpontSystemCatalog::ColType& resultType) override;

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type) override;
};
/** @brief Func_json_contains class
 */
class Func_json_contains : public Func_Bool
{
 protected:
  JsonPath path;
  bool arg2Const;
  bool arg2Parsed;  // argument 2 is a constant or has been parsed
  std::string_view arg2Val;

 public:
  Func_json_contains() : Func_Bool("json_contains"), arg2Const(false), arg2Parsed(false), arg2Val("")
  {
  }
  virtual ~Func_json_contains()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  bool getBoolVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                  execplan::CalpontSystemCatalog::ColType& type);
};
/** @brief Func_json_array_append class
 */
class Func_json_array_append : public Func_Str
{
 protected:
  std::vector<JsonPath> paths;

 public:
  Func_json_array_append() : Func_Str("json_array_append")
  {
  }
  virtual ~Func_json_array_append()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};
/** @brief Func_json_array_insert class
 */
class Func_json_array_insert : public Func_Str
{
 protected:
  std::vector<JsonPath> paths;

 public:
  Func_json_array_insert() : Func_Str("json_array_insert")
  {
  }
  virtual ~Func_json_array_insert()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_insert class
 */
class Func_json_insert : public Func_Str
{
 public:
  enum MODE
  {
    NONE,
    INSERT,
    REPLACE,
    SET
  };

 protected:
  MODE mode;
  std::vector<JsonPath> paths;

 public:
  Func_json_insert() : Func_Str("json_insert"), mode(INSERT)
  {
  }
  Func_json_insert(MODE m) : mode(m)
  {
    assert(m != NONE);
    switch (m)
    {
      case INSERT: Func_Str::Func::funcName("json_insert"); break;
      case REPLACE: Func_Str::Func::funcName("json_replace"); break;
      case SET: Func_Str::Func::funcName("json_set"); break;
      default: break;
    }
  }
  virtual ~Func_json_insert()
  {
  }

  MODE getMode() const
  {
    return mode;
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};
/** @brief Func_json_remove class
 */
class Func_json_remove : public Func_Str
{
 protected:
  std::vector<JsonPath> paths;

 public:
  Func_json_remove() : Func_Str("json_remove")
  {
  }
  virtual ~Func_json_remove()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_contains_path class
 */
class Func_json_contains_path : public Func_Bool
{
 protected:
  std::vector<JsonPath> paths;
  std::vector<bool> hasFound;
  bool isModeOne;
  bool isModeConst;
  bool isModeParsed;

 public:
  Func_json_contains_path()
   : Func_Bool("json_contains_path"), isModeOne(false), isModeConst(false), isModeParsed(false)
  {
  }
  virtual ~Func_json_contains_path()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  bool getBoolVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                  execplan::CalpontSystemCatalog::ColType& type);
};

/** @brief Func_json_overlaps class
 */
class Func_json_overlaps : public Func_Bool
{
 protected:
  JsonPath path;

 public:
  Func_json_overlaps() : Func_Bool("json_overlaps")
  {
  }
  virtual ~Func_json_overlaps()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  bool getBoolVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                  execplan::CalpontSystemCatalog::ColType& type);
};
/** @brief Func_json_search class
 */
class Func_json_search : public Func_Str
{
 protected:
  std::vector<JsonPath> paths;
  bool isModeParsed;
  bool isModeConst;
  bool isModeOne;
  int escapeStr;

 public:
  Func_json_search()
   : Func_Str("json_search"), isModeParsed(false), isModeConst(false), isModeOne(false), escapeStr('\\')
  {
  }
  virtual ~Func_json_search()
  {
  }

  execplan::CalpontSystemCatalog::ColType operationType(FunctionParm& fp,
                                                        execplan::CalpontSystemCatalog::ColType& resultType);

  std::string getStrVal(rowgroup::Row& row, FunctionParm& fp, bool& isNull,
                        execplan::CalpontSystemCatalog::ColType& type);
};
}  // namespace funcexp
