#pragma once
#include <stdint.h>

#include "google/protobuf/message.h"

namespace db
{
	enum EOperatorType
	{
		kOT_Select,
		kOT_Update,
		kOT_Insert,
		kOT_Delete,
		kOT_Query,
		kOT_Call,
		kOT_Nop,

		kOT_None,
	};

	enum EResultCode
	{
		kRC_OK,
		kRC_PROTO_ERROR,
		kRC_MYSQL_ERROR,
		kRC_SQLPARM_ERROR,
		kRC_LOST_CONNECTION,

		kRC_UNKNOWN,
	};

#define PrintInfo(szFormat, ...)

#define PrintWarning(szFormat, ...)

	bool parseProtoText(const std::string& szText, google::protobuf::Message* pMessage);
	bool parseProtoBin(const std::string& szBin, google::protobuf::Message* pMessage);
}