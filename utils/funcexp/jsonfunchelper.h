#pragma once

#include <string>
#include "functor_json.h"
#include "functor_str.h"

#define PREFER_MY_CONFIG_H
#include <mariadb.h>
#include <mysql.h>
#include <my_sys.h>
#include <json_lib.h>

#include "collation.h"
#include "rowgroup.h"
#include "treenode.h"
#include "functioncolumn.h"

namespace funcexp
{
namespace helpers
{

#define NO_WILDCARD_ALLOWED 1
/*
  Checks if the path has '.*' '[*]' or '**' constructions
  and sets the NO_WILDCARD_ALLOWED error if the case.
*/
inline int setupPathNoWildcard(json_path_t* p, CHARSET_INFO* i_cs, const uchar* str, const uchar* end)
{
  if (!json_path_setup(p, i_cs, str, end))
  {
    if ((p->types_used & (JSON_PATH_WILD | JSON_PATH_DOUBLE_WILD)) == 0)
      return 0;
    p->s.error = NO_WILDCARD_ALLOWED;
  }
  return 1;
}

std::string getStrEscaped(const char* js, const CHARSET_INFO* cs);
std::string getJsonKeyName(rowgroup::Row& row, execplan::SPTP& parm, bool& isNull);
std::string getJsonValue(rowgroup::Row& row, execplan::SPTP& parm, bool& isNull);
}  // namespace helpers
}  // namespace funcexp
