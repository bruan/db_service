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

	bool parseProtoText(const std::string& szText, google::protobuf::Message* pMessage)
	{
		if (nullptr == pMessage)
			return false;

		return google::protobuf::util::JsonStringToMessage(szText, pMessage).ok();
	}

	bool parseProtoBin(const std::string& szBin, google::protobuf::Message* pMessage)
	{
		if (nullptr == pMessage)
			return false;

		return pMessage->ParseFromString(szBin);
	}
}

#ifdef _WIN32
#pragma warning(pop)
#endif