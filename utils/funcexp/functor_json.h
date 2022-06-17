#pragma once
#define PREFER_MY_CONFIG_H
#include <my_global.h>
#include <json_lib.h>
#include "functor_bool.h"
#include "functor_int.h"
#include "functor_str.h"

namespace funcexp
{
class json_path_with_flags
{
 public:
  json_path_t p;
  bool constant;
  bool parsed;
  json_path_step_t* cur_step;
  void set_constant_flag(bool s_constant)
  {
    constant = s_constant;
    parsed = false;
  }
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
                  execplan::CalpontSystemCatalog::ColType& op_ct);
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
                        execplan::CalpontSystemCatalog::ColType& op_ct);
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
                  execplan::CalpontSystemCatalog::ColType& op_ct);
};

/** @brief Func_json_equals class
 */
class Func_json_exists : public Func_Bool
{
 protected:
  json_path_with_flags path;

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
                  execplan::CalpontSystemCatalog::ColType& op_ct);
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
                    execplan::CalpontSystemCatalog::ColType& op_ct);
};
}  // namespace funcexp