#include "db_command_delete_handler.h"
#include "proto_src/delete_command.pb.h"
#include "db_protobuf.h"

#include <sstream>

namespace db
{
	CDbCommandDeleteHandler::CDbCommandDeleteHandler()
	{

	}

	CDbCommandDeleteHandler::~CDbCommandDeleteHandler()
	{

	}

	uint32_t CDbCommandDeleteHandler::onDbCommand(const google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>& pResponse)
	{
		const proto::db::delete_command* pCommand = dynamic_cast<const proto::db::delete_command*>(pRequest);
		DebugAstEx(pCommand != nullptr, kRC_PROTO_ERROR);

		std::string szMessageName = getMessageNameByTableName(pCommand->table_name());
		std::unique_ptr<google::protobuf::Message> pMessage(createMessage(szMessageName));
		DebugAstEx(pMessage != nullptr, kRC_PROTO_ERROR);

		std::string szPrimaryName = getPrimaryName(pMessage.get());
		DebugAstEx(!szPrimaryName.empty(), kRC_PROTO_ERROR);
		
		std::ostringstream oss;
		oss << "delete from " << pCommand->table_name() << " where " << szPrimaryName << "=" << pCommand->id() << " limit 1";
		std::string szSQL(oss.str());

#ifdef _DEBUG_SQL
		PrintInfo("%s", szSQL.c_str());
#endif

		uint32_t nErrorType = this->m_pDbConnection->execute(szSQL, nullptr);
		if (nErrorType == kMET_LostConnection)
			return kRC_LOST_CONNECTION;
		else if (nErrorType != kMET_OK)
			return kRC_MYSQL_ERROR;
		
		return kRC_OK;
	}
}