#include "db_command_select_handler.h"
#include "proto_src/select_command.pb.h"

#include "db_protobuf.h"

#include <sstream>

using namespace std;
using namespace google::protobuf;
using namespace db;
using namespace proto::db;

CDbCommandSelectHandler::CDbCommandSelectHandler()
{

}

CDbCommandSelectHandler::~CDbCommandSelectHandler()
{

}

uint32_t CDbCommandSelectHandler::onDbCommand(const Message* pRequest, shared_ptr<Message>* pResponse)
{
	const select_command* pCommand = dynamic_cast<const select_command*>(pRequest);
	DebugAstEx(pCommand != nullptr, kRC_PROTO_ERROR);

	string szMessageName = getMessageNameByTableName(pCommand->table_name());
	shared_ptr<Message> pMessage(createMessage(szMessageName));
	DebugAstEx(pMessage != nullptr, kRC_PROTO_ERROR);

	string szPrimaryName = getPrimaryName(pMessage.get());
	DebugAstEx(!szPrimaryName.empty(), kRC_PROTO_ERROR);

	ostringstream oss;
	oss << "select * from " << pCommand->table_name() << " where " << szPrimaryName << "=" << pCommand->id() << " limit 1";
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

	DebugAstEx(fillNormalMessage(pDbRecordset, pMessage.get()), kRC_PROTO_ERROR);
	
	*pResponse = pMessage;

	return kRC_OK;
}