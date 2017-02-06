#include "db_command_insert_handler.h"

#include "db_protobuf.h"

#include <sstream>

using namespace std;
using namespace google::protobuf;
using namespace db;

CDbCommandInsertHandler::CDbCommandInsertHandler()
{

}

CDbCommandInsertHandler::~CDbCommandInsertHandler()
{

}

uint32_t CDbCommandInsertHandler::onDbCommand(const Message* pRequest, shared_ptr<Message>& pResponse)
{
	string szMessageName = pRequest->GetTypeName();
	const Message* pMessage = pRequest;
	
	string szTableName;
	DebugAstEx(getTableNameByMessageName(szMessageName, szTableName), kRC_PROTO_ERROR);

	vector<SFieldInfo> vecFieldInfo;
	DebugAstEx(getMessageFieldInfos(pMessage, vecFieldInfo), kRC_PROTO_ERROR);

	string szClause;
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
	ostringstream oss;
	oss << "insert into " << szTableName << szClause;
	string szSQL = oss.str();

#ifdef _DEBUG_SQL
		PrintInfo("%s", szSQL.c_str());
#endif

	uint32_t nErrorType = this->m_pDbConnection->execute(szSQL, nullptr);
	if (nErrorType == kMET_LostConnection)
		return kRC_LOST_CONNECTION;
	else if (nErrorType != kMET_OK)
		return kRC_MYSQL_ERROR;
	
	return this->m_pDbConnection->getAffectedRow() == 1 ? kRC_OK : kRC_MYSQL_ERROR;
}