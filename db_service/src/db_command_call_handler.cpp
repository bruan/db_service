#include "db_command_call_handler.h"
#include "proto_src/call_command.pb.h"
#include "proto_src/result_set.pb.h"
#include "db_protobuf.h"

#include <sstream>

namespace db
{
#define _TRY_DEAD_LOOP_COUNT 5

	CDbCommandCallHandler::CDbCommandCallHandler()
	{

	}

	CDbCommandCallHandler::~CDbCommandCallHandler()
	{

	}

	uint32_t CDbCommandCallHandler::onDbCommand(const google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>& pResponse)
	{
		const proto::db::call_command* pCommand = dynamic_cast<const proto::db::call_command*>(pRequest);
		DebugAstEx(pCommand != nullptr, kRC_PROTO_ERROR);

		std::ostringstream oss;
		std::string szSQL = pCommand->sql();
		for (int32_t i = 0; i < pCommand->args_size(); ++i)
		{
			const std::string& szArg = pCommand->args(i);
			std::string szSafeArg = this->m_pDbConnection->escape(szArg);
			oss.str("");
			oss << "{" << i << "}";
			size_t pos = szSQL.find(oss.str());
			if (pos == std::string::npos)
				return kRC_SQLPARM_ERROR;

			szSQL.replace(pos, oss.str().size(), szSafeArg);
		}

#ifdef _DEBUG_SQL
		PrintInfo("%s", szSQL.c_str());
#endif

		CDbRecordset* pDbRecordset = nullptr;
		DEFER(delete pDbRecordset);

		bool bOK = false;
		for (size_t i = 0; i < _TRY_DEAD_LOOP_COUNT; ++i)
		{
			this->m_pDbConnection->begintrans();
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
			this->m_pDbConnection->endtrans();
			break;
		}

		if (!bOK)
			return kRC_MYSQL_ERROR;

		if (pDbRecordset == nullptr)
			return kRC_OK;

		std::shared_ptr<proto::db::result_set> pResultset = std::make_shared<proto::db::result_set>();
		for (uint32_t i = 0; i < pDbRecordset->getFieldCount(); ++i)
		{
			pResultset->add_field_name(pDbRecordset->getFieldName(i));
		}

		for (uint64_t i = 0; i < pDbRecordset->getRowCount(); ++i)
		{
			pDbRecordset->fatchNextRow();

			proto::db::row* pRow = pResultset->add_rows();
			DebugAstEx(pRow != nullptr, kRC_PROTO_ERROR);

			for (uint32_t j = 0; j < pDbRecordset->getFieldCount(); ++j)
			{
				const char* szValue = pDbRecordset->getData(j);
				DebugAstEx(szValue != nullptr, kRC_PROTO_ERROR);

				pRow->add_value(szValue);
			}
		}

		pResponse = pResultset;

		return kRC_OK;
	}
}