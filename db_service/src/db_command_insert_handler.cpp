#include "db_command_insert_handler.h"
#include "db_protobuf.h"

#include <sstream>

namespace db
{
	CDbCommandInsertHandler::CDbCommandInsertHandler()
	{

	}

	CDbCommandInsertHandler::~CDbCommandInsertHandler()
	{

	}

	uint32_t CDbCommandInsertHandler::onDbCommand(const google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>& pResponse)
	{
		std::string szMessageName = pRequest->GetTypeName();
		const google::protobuf::Message* pMessage = pRequest;
		
		std::string szTableName;
		DebugAstEx(getTableNameByMessageName(szMessageName, szTableName), kRC_PROTO_ERROR);

		std::vector<SFieldInfo> vecFieldInfo;
		DebugAstEx(getMessageFieldInfos(pMessage, vecFieldInfo), kRC_PROTO_ERROR);

		std::string szClause;
		szClause.reserve(1024);

		szClause = "(";
		for (size_t i = 0; i < vecFieldInfo.size(); ++i)
		{
			const SFieldInfo& sFieldInfo = vecFieldInfo[i];

			if (i != 0)
				szClause += ",";

			szClause += sFieldInfo.szName;
		}
		szClause += ") values (";

		for (size_t i = 0; i < vecFieldInfo.size(); ++i)
		{
			const SFieldInfo& sFieldInfo = vecFieldInfo[i];

			if (i != 0)
				szClause += ",";

			if (sFieldInfo.bStr)
			{
				szClause += "'";
				szClause += this->m_pDbConnection->escape(sFieldInfo.szValue);
				szClause += "'";
			}
			else
			{
				szClause += sFieldInfo.szValue;
			}
		}
		szClause += ")";
		std::ostringstream oss;
		oss << "insert into " << szTableName << szClause;
		std::string szSQL = oss.str();

#ifdef _DEBUG_SQL
		PrintInfo("%s", szSQL.c_str());
#endif

		this->m_pDbConnection->begintrans();
		uint32_t nErrorType = this->m_pDbConnection->execute(szSQL, nullptr);
		if (nErrorType == kMET_LostConnection)
		{
			this->m_pDbConnection->rollback();
			return kRC_LOST_CONNECTION;
		}
		else if (nErrorType != kMET_OK)
		{
			this->m_pDbConnection->rollback();
			return kRC_MYSQL_ERROR;
		}
		this->m_pDbConnection->endtrans();

		return this->m_pDbConnection->getAffectedRow() == 1 ? kRC_OK : kRC_MYSQL_ERROR;
	}
}