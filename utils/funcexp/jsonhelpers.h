#pragma once

#include <cstddef>
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
int setupPathNoWildcard(json_path_t* path, CHARSET_INFO* cs, const uchar* str, const uchar* end);

std::string getStrEscaped(const char* js, const size_t jsLen, const CHARSET_INFO* cs);
std::string getJsonKeyName(rowgroup::Row& row, execplan::SPTP& parm);
std::string getJsonValue(rowgroup::Row& row, execplan::SPTP& parm);

static const int TAB_SIZE_LIMIT = 8;
static const char tab_arr[TAB_SIZE_LIMIT + 1] = "        ";

// format the json using format mode
int doFormat(json_engine_t* je, string& niceJs, Func_json_format::FORMATS mode, int tab_size = 4);

#define SHOULD_END_WITH_ARRAY 2
#define TRIVIAL_PATH_NOT_ALLOWED 3
}  // namespace helpers
}  // namespace funcexp
