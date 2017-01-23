#pragma once
#include <string>
#include <vector>

#include "google/protobuf/message.h"
#include "google/protobuf/any.pb.h"

#include "db_record_set.h"

namespace db
{
	struct SFieldInfo
	{
		std::string szName;
		std::string	szValue;
		bool		bStr;
	};

	bool		importProtobuf(const std::string& szDir, const std::vector<std::string>& vecFile);

	std::string getMessageNameByTableName(const std::string& szTableName);

	bool		getTableNameByMessageName(const std::string& szMessageName, std::string& szTableName);

	std::string	getPrimaryName(const google::protobuf::Message* pMessage);
	
	bool		getMessageFieldInfos(const google::protobuf::Message* pMessage, std::vector<SFieldInfo>& vecFieldInfo);

	google::protobuf::Message* 
				createMessage(const std::string& szName);
	bool		fillNormalMessage(CDbRecordset* pDbRecordset, google::protobuf::Message* pMessage);
	google::protobuf::Message*
				createRepeatMessage(CDbRecordset* pDbRecordset, const std::string& szName);
}