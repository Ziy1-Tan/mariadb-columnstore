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
class Func_json_format;
}

namespace funcexp
{
namespace helpers
{
std::string getStrEscaped(const char* js, const CHARSET_INFO* cs);
std::string getJsonKeyName(rowgroup::Row& row, execplan::SPTP& parm, bool& isNull);
std::string getJsonValue(rowgroup::Row& row, execplan::SPTP& parm, bool& isNull);

#define NO_WILDCARD_ALLOWED 1
/*
  Checks if the path has '.*' '[*]' or '**' constructions
  and sets the NO_WILDCARD_ALLOWED error if the case.
*/
int path_setup_nwc(json_path_t* p, CHARSET_INFO* i_cs, const uchar* str, const uchar* end);

static const int TAB_SIZE_LIMIT = 8;
static const char tab_arr[TAB_SIZE_LIMIT + 1] = "        ";

int json_nice(json_engine_t* je, string& nice_js, funcexp::Func_json_format::formats mode, int tab_size = 4);
}  // namespace helpers
}  // namespace funcexp
