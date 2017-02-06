#include "db_command_query_handler.h"
#include "proto_src/query_command.pb.h"

#include "db_protobuf.h"

#include <sstream>

using namespace std;
using namespace google::protobuf;
using namespace db;
using namespace proto::db;

CDbCommandQueryHandler::CDbCommandQueryHandler()
{

}

CDbCommandQueryHandler::~CDbCommandQueryHandler()
{

}

uint32_t CDbCommandQueryHandler::onDbCommand(const Message* pRequest, shared_ptr<Message>* pResponse)
{
	const query_command* pCommand = dynamic_cast<const query_command*>(pRequest);
	DebugAstEx(pCommand != nullptr, kRC_PROTO_ERROR);

	ostringstream oss;
	string szWhereClause = pCommand->where_clause();
	for (int32_t i = 0; i < pCommand->args_size(); ++i)
	{
		const string& szArg = pCommand->args(i);
		string szSafeArg = this->m_pDbConnection->escape(szArg);
		oss.str("");
		oss << "{" << i << "}";
		size_t pos = szWhereClause.find(oss.str());
		if (pos == string::npos)
			return kRC_SQLPARM_ERROR;

		szWhereClause.replace(pos, oss.str().size(), szSafeArg);
	}
	oss.str("");
	if (!szWhereClause.empty())
		oss << "select * from " << pCommand->table_name() << " where " << szWhereClause;
	else
		oss << "select * from " << pCommand->table_name();
	string szSQL(oss.str());

#ifdef _DEBUG_SQL
	PrintInfo("%s", szSQL.c_str());
#endif

	CDbRecordset* pDbRecordset = nullptr;
	DEFER(delete pDbRecordset);

	uint32_t nErrorType = this->m_pDbConnection->execute(szSQL, &pDbRecordset);
	if (nErrorType == kMET_LostConnection)
		return kRC_LOST_CONNECTION;
	else if (nErrorType != kMET_OK)
		return kRC_MYSQL_ERROR;

	DebugAstEx(pDbRecordset != nullptr, kRC_MYSQL_ERROR);

	string szMessageName = getMessageNameByTableName(pCommand->table_name());
	*pResponse = shared_ptr<Message>(createRepeatMessage(pDbRecordset, szMessageName));
	DebugAstEx(*pResponse != nullptr, kRC_PROTO_ERROR);
	
	return kRC_OK;
}