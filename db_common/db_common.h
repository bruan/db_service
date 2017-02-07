#pragma once
#include <stdint.h>

#include "google/protobuf/message.h"

namespace db
{
	enum EOperatorType
	{
		kOT_SELECT = 0,
		kOT_UPDATE,
		kOT_INSERT,
		kOT_DELETE,
		kOT_QUERY,
		kOT_CALL,
		kOT_NOP,
		kOT_FLUSH,

		kOT_None,
	};

	enum EResultCode
	{
		kRC_OK = 0,
		kRC_PROTO_ERROR,
		kRC_MYSQL_ERROR,
		kRC_SQLPARM_ERROR,
		kRC_LOST_CONNECTION,

		kRC_UNKNOWN,
	};

	enum EFlushCacheType
	{
		kFCT_NORMAL = 0,
		kFCT_DEL,
	};

#define PrintInfo(szFormat, ...)

#define PrintWarning(szFormat, ...)
}