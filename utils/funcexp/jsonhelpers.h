#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#define PREFER_MY_CONFIG_H
#include <mariadb.h>
#include <mysql.h>
#include <my_sys.h>
#include <json_lib.h>

#include "collation.h"
#include "functor_json.h"
#include "functor_str.h"
#include "collation.h"
#include "rowgroup.h"
#include "treenode.h"
#include "functioncolumn.h"
#include "constantcolumn.h"

namespace funcexp
{
namespace helpers
{

static const int NO_WILDCARD_ALLOWED = 1;

/*
  Checks if the path has '.*' '[*]' or '**' constructions
  and sets the NO_WILDCARD_ALLOWED error if the case.
*/
int pathSetupNwc(json_path_t* path, CHARSET_INFO* cs, const uchar* str, const uchar* end);

// Return true if err occur, let the outer function handle the exception
bool appendEscapedJS(string& ret, const CHARSET_INFO* retCS, const string_view& js, const CHARSET_INFO* jsCS);
bool appendJSKeyName(string& ret, const CHARSET_INFO* retCS, rowgroup::Row& row, execplan::SPTP& parm);
bool appendJSValue(string& ret, const CHARSET_INFO* retCS, rowgroup::Row& row, execplan::SPTP& parm);

static const int TAB_SIZE_LIMIT = 8;
static const char tab_arr[TAB_SIZE_LIMIT + 1] = "        ";

// Format the json using format mode
int doFormat(json_engine_t* je, string& niceJS, Func_json_format::FORMATS mode, int tabSize = 4);

static const int SHOULD_END_WITH_ARRAY = 2;
static const int TRIVIAL_PATH_NOT_ALLOWED = 3;

bool findKeyInObject(json_engine_t* jsEg, json_string_t* key);

#ifdef MYSQL_GE_1009
using IntType = int;
#else
using IntType = uint;
#endif

/*
  Compatible with jsonFindPath function in json_lib
  before 10.9: uint* array_counters
  after 10.9: int* array_counters
 */
inline static int jsonFindPath(json_engine_t* jsEg, json_path_t* path, json_path_step_t** currStep)
{
  IntType arrayCounters[JSON_DEPTH_LIMIT];
  return json_find_path(jsEg, path, currStep, arrayCounters);
}

// Check and set the constant flag from function parameters
inline static void markConstFlag(JsonPath& path, const execplan::SPTP& parm)
{
  path.constant = (dynamic_cast<execplan::ConstantColumn*>(parm->data()) != nullptr);
}

int jsonPathCompare(const json_path_t* a, const json_path_t* b, enum json_value_types vt,
                    const int* arraySize = nullptr);
}  // namespace helpers
}  // namespace funcexp
