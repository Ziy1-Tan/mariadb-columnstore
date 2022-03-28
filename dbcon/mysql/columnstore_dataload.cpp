/* Copyright (C) 2021 MariaDB Corporation

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of
   the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.
*/

#include <curl/curl.h>

#define NEED_CALPONT_INTERFACE
#define PREFER_MY_CONFIG_H 1
#include "ha_mcs_impl.h"
#include "ha_mcs_impl_if.h"
using namespace cal_impl_if;

#include "errorcodes.h"
#include "idberrorinfo.h"
#include "errorids.h"
using namespace logging;

#include "columnstoreversion.h"
#include "ha_mcs_sysvars.h"

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

extern "C"
{
  struct InitData
  {
    CURL* curl;
  };

  void columnstore_dataload_deinit(UDF_INIT* initid)
  {
    InitData* initData = (InitData*)(initid->ptr);
    curl_easy_cleanup(initData->curl);
  }

  const char* columnstore_dataload(UDF_INIT* initid, UDF_ARGS* args, char* result,
                                   unsigned long* length, char* /*is_null*/, char* /*error*/)
  {
    InitData* initData = (InitData*)(initid->ptr);
    CURLcode res;
    std::string readBuffer;
    std::string folder = args->args[0];
    std::string param = "{\"folder\" : \"" + folder + "\"}";
    if (initData->curl)
    {
      struct curl_slist *hs=NULL;
      hs = curl_slist_append(hs, "Content-Type: application/json");
      curl_easy_setopt(initData->curl, CURLOPT_HTTPHEADER, hs);
      curl_easy_setopt(initData->curl, CURLOPT_URL, "http://localhost");
      curl_easy_setopt(initData->curl, CURLOPT_PORT, 5000l);
      curl_easy_setopt(initData->curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(initData->curl, CURLOPT_POSTFIELDS, param.c_str());
      curl_easy_setopt(initData->curl, CURLOPT_WRITEDATA, &readBuffer);
      res = curl_easy_perform(initData->curl);
    }

    (void)res;

    strcpy(result, readBuffer.c_str());
    *length = readBuffer.size();

    return result;
  }

  my_bool columnstore_dataload_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
  {
    initid->max_length = 1000*1000;
    InitData* initData = new InitData;
    initData->curl = curl_easy_init();
    initid->ptr = (char*)(initData);

    if (args->arg_count != 1)
    {
      strcpy(message, "COLUMNSTORE_DATALOAD() takes one arguments");
      return 1;
    }

    return 0;
  }
}