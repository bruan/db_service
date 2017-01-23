#include "db_common.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#endif
#include <vector>
#include <string>
#include "google/protobuf/util/json_util.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif

namespace db
{

}

#ifdef _WIN32
#pragma warning(pop)
#endif