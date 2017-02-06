#include "db_command_call_handler.h"
#include "proto_src/call_command.pb.h"
#include "proto_src/result_set.pb.h"

#include "db_protobuf.h"

#include <sstream>

using namespace std;
using namespace google::protobuf;
using namespace db;
using namespace proto::db;

#define _TRY_DEAD_LOOP_COUNT 5

CDbCommandCallHandler::CDbCommandCallHandler()
{

}

CDbCommandCallHandler::~CDbCommandCallHandler()
{

}

uint32_t CDbCommandCallHandler::onDbCommand(const Message* pRequest, shared_ptr<Message>* pResponse)
{
	const call_command* pCommand = dynamic_cast<const call_command*>(pRequest);
	DebugAstEx(pCommand != nullptr, kRC_PROTO_ERROR);

	ostringstream oss;
	string szSQL = pCommand->sql();
	for (int32_t i = 0; i < pCommand->args_size(); ++i)
	{
		const string& szArg = pCommand->args(i);
		string szSafeArg = this->m_pDbConnection->escape(szArg);
		oss.str("");
		oss << "{" << i << "}";
		size_t pos = szSQL.find(oss.str());
		if (pos == string::npos)
			return kRC_SQLPARM_ERROR;

		szSQL.replace(pos, oss.str().size(), szSafeArg);
	}

#ifdef _DEBUG_SQL
	PrintInfo("%s", szSQL.c_str());
#endif

	CDbRecordset* pDbRecordset = nullptr;
	DEFER(delete pDbRecordset);
	
	this->m_pDbConnection->autoCommit(false);
	DEFER(this->m_pDbConnection->autoCommit(true));

	bool bOK = false;
	for (size_t i = 0; i < _TRY_DEAD_LOOP_COUNT; ++i)
	{
		this->m_pDbConnection->begin();
		uint32_t nErrorType = this->m_pDbConnection->execute(szSQL, &pDbRecordset);
		
		if (nErrorType == kMET_Deadloop)
		{
			// 死锁回滚继续重来
			this->m_pDbConnection->rollback();
			continue;
		}
		else if (nErrorType == kMET_LostConnection)
		{
			this->m_pDbConnection->rollback();
			return kRC_LOST_CONNECTION;
		}
		else if (nErrorType != kMET_OK)
		{
			this->m_pDbConnection->rollback();
			return kRC_MYSQL_ERROR;
		}

		bOK = true;
		this->m_pDbConnection->commit();
		break;
	}

	if (!bOK)
		return kRC_MYSQL_ERROR;

	if (pDbRecordset == nullptr)
		return kRC_OK;

	shared_ptr<result_set> pResultset = make_shared<result_set>();
	for (uint32_t i = 0; i < pDbRecordset->getFieldCount(); ++i)
	{
		pResultset->add_field_name(pDbRecordset->getFieldName(i));
	}

	for (uint64_t i = 0; i < pDbRecordset->getRowCount(); ++i)
	{
		pDbRecordset->fatchNextRow();

		row* pRow = pResultset->add_rows();
		DebugAstEx(pRow != nullptr, kRC_PROTO_ERROR);

		for (uint32_t j = 0; j < pDbRecordset->getFieldCount(); ++j)
		{
			const char* szValue = pDbRecordset->getData(j);
			DebugAstEx(szValue != nullptr, kRC_PROTO_ERROR);

			pRow->add_value(szValue);
		}
	}

	*pResponse = pResultset;

	return kRC_OK;
}