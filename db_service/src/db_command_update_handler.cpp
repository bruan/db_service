#include "db_command_update_handler.h"

#include "db_protobuf.h"

#include <sstream>

using namespace std;
using namespace google::protobuf;
using namespace db;

CDbCommandUpdateHandler::CDbCommandUpdateHandler()
{

}

CDbCommandUpdateHandler::~CDbCommandUpdateHandler()
{

}

uint32_t CDbCommandUpdateHandler::onDbCommand(const Message* pRequest, shared_ptr<Message>* pResponse)
{
	string szMessageName = pRequest->GetTypeName();
	const Message* pMessage = pRequest;

	string szTableName;
	DebugAstEx(getTableNameByMessageName(szMessageName, szTableName), kRC_PROTO_ERROR);

	vector<SFieldInfo> vecFieldInfo;
	DebugAstEx(getMessageFieldInfos(pMessage, vecFieldInfo), kRC_PROTO_ERROR);

	string szInsertClause;
	szInsertClause.reserve(1024);
	szInsertClause = "(";
	for (size_t i = 0; i < vecFieldInfo.size(); ++i)
	{
		const SFieldInfo& sFieldInfo = vecFieldInfo[i];

		if (i != 0)
			szInsertClause += ",";

		szInsertClause += sFieldInfo.szName;
	}
	szInsertClause += ") values (";

	for (size_t i = 0; i < vecFieldInfo.size(); ++i)
	{
		const SFieldInfo& sFieldInfo = vecFieldInfo[i];

		if (i != 0)
			szInsertClause += ",";

		if (sFieldInfo.bStr)
		{
			szInsertClause += "'";
			szInsertClause += this->m_pDbConnection->escape(sFieldInfo.szValue);
			szInsertClause += "'";
		}
		else
		{
			szInsertClause += sFieldInfo.szValue;
		}
	}
	szInsertClause += ")";

	string szUpdateClause;
	szUpdateClause.reserve(1024);
	for (size_t i = 0; i < vecFieldInfo.size(); ++i)
	{
		const SFieldInfo& sFieldInfo = vecFieldInfo[i];

		if (i != 0)
			szUpdateClause += ",";

		szUpdateClause += sFieldInfo.szName;
		szUpdateClause += "=";

		if (sFieldInfo.bStr)
		{
			szUpdateClause += "'";
			szUpdateClause += this->m_pDbConnection->escape(sFieldInfo.szValue);
			szUpdateClause += "'";
		}
		else
		{
			szUpdateClause += sFieldInfo.szValue;
		}
	}

	ostringstream oss;
	oss << "insert into " << szTableName << szInsertClause << " on duplicate key update " << szUpdateClause;
	string szSQL = oss.str();

#ifdef _DEBUG_SQL
	PrintInfo("%s", szSQL.c_str());
#endif

	uint32_t nErrorType = this->m_pDbConnection->execute(szSQL, nullptr);
	if (nErrorType == kMET_LostConnection)
		return kRC_LOST_CONNECTION;
	else if (nErrorType != kMET_OK)
		return kRC_MYSQL_ERROR;
	
	return (this->m_pDbConnection->getAffectedRow() >= 1) ? kRC_OK : kRC_MYSQL_ERROR;
}