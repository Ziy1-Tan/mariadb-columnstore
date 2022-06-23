#pragma once

#define PREFER_MY_CONFIG_H
#include <mariadb.h>
#include <mysql.h>
#include <my_sys.h>
#include <json_lib.h>

#include "collation.h"
#include "functor_bool.h"
#include "functor_int.h"
#include "functor_str.h"

namespace funcexp
{

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

}  // namespace funcexp
