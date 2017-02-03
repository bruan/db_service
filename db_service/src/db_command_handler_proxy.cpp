#include "db_command_handler_proxy.h"
#include "db_connection.h"
#include "db_command_call_handler.h"
#include "db_command_delete_handler.h"
#include "db_command_insert_handler.h"
#include "db_command_nop_handler.h"
#include "db_command_query_handler.h"
#include "db_command_select_handler.h"
#include "db_command_update_handler.h"
#include "db_common.h"
#include "db_protobuf.h"

#include "proto_src\select_command.pb.h"
#include "proto_src\delete_command.pb.h"

namespace db
{
	CDbCommandHandlerProxy::CDbCommandHandlerProxy()
	{
		this->m_mapDbCommandHandler[kOT_Call]	= new CDbCommandCallHandler();
		this->m_mapDbCommandHandler[kOT_Delete] = new CDbCommandDeleteHandler();
		this->m_mapDbCommandHandler[kOT_Insert] = new CDbCommandInsertHandler();
		this->m_mapDbCommandHandler[kOT_Nop] = new CDbCommandNOPHandler();
		this->m_mapDbCommandHandler[kOT_Query] = new CDbCommandQueryHandler();
		this->m_mapDbCommandHandler[kOT_Select] = new CDbCommandSelectHandler();
		this->m_mapDbCommandHandler[kOT_Update] = new CDbCommandUpdateHandler();
	}

	CDbCommandHandlerProxy::~CDbCommandHandlerProxy()
	{
		delete this->m_pDbCacheMgr;
	}

	bool CDbCommandHandlerProxy::init(uint64_t nMaxCacheSize)
	{
		this->m_pDbCacheMgr = new CDbCacheMgr();

		return this->m_pDbCacheMgr->init(nMaxCacheSize);
	}

	void CDbCommandHandlerProxy::onConnect(CDbConnection* pDbConnection)
	{
		pDbConnection->autoCommit(true);

		for (auto iter = this->m_mapDbCommandHandler.begin(); iter != this->m_mapDbCommandHandler.end(); ++iter)
		{
			CDbCommandHandler* pDbCommandHandler = iter->second;
			if (pDbCommandHandler == nullptr)
				continue;

			pDbCommandHandler->onConnect(pDbConnection);
		}
	}

	void CDbCommandHandlerProxy::onDisconnect()
	{
		for (auto iter = this->m_mapDbCommandHandler.begin(); iter != this->m_mapDbCommandHandler.end(); ++iter)
		{
			CDbCommandHandler* pDbCommandHandler = iter->second;
			if (pDbCommandHandler == nullptr)
				continue;

			pDbCommandHandler->onDisconnect();
		}
	}

	uint32_t CDbCommandHandlerProxy::onDbCommand(uint32_t nType, std::shared_ptr<google::protobuf::Message> pRequest, std::shared_ptr<google::protobuf::Message>& pResponse)
	{
		std::string szName = pRequest->GetTypeName();
		auto iter = this->m_mapDbCommandHandler.find(nType);
		if (iter == this->m_mapDbCommandHandler.end())
			return kRC_UNKNOWN;

		CDbCommandHandler* pDbCommandHandler = iter->second;
		if (pDbCommandHandler == nullptr)
			return kRC_UNKNOWN;

		switch (nType)
		{
		case kOT_Select:
			{
				const proto::db::select_command* pCommand = dynamic_cast<const proto::db::select_command*>(pRequest.get());
				DebugAstEx(pCommand != nullptr, kRC_PROTO_ERROR);

				std::shared_ptr<google::protobuf::Message> pData = this->m_pDbCacheMgr.getData(pCommand->id(), pCommand->table_name());
				if (pData != nullptr)
				{
					pResponse = pData;
					return kRC_OK;
				}
			}
			break;

		case kOT_Update:
			{
				uint64_t nID = 0;
				if (!getPrimaryValue(pRequest.get(), nID))
					return kRC_PROTO_ERROR;

				this->m_pDbCacheMgr.setData(nID, pRequest);
			}
			break;

		case kOT_Insert:
			{
				uint64_t nID = 0;
				if (!getPrimaryValue(pRequest.get(), nID))
					return kRC_PROTO_ERROR;

				this->m_pDbCacheMgr.setData(nID, pRequest);
			}
			break;

		case kOT_Delete:
			{
				const proto::db::delete_command* pCommand = dynamic_cast<const proto::db::delete_command*>(pRequest.get());
				DebugAstEx(pCommand != nullptr, kRC_PROTO_ERROR);

				this->m_pDbCacheMgr.delData(pCommand->id(), getMessageNameByTableName(pCommand->table_name()));
			}
			break;
		}

		uint32_t nErrorCode = pDbCommandHandler->onDbCommand(pRequest.get(), pResponse);
		if (nErrorCode != kRC_OK)
			return nErrorCode;

		if (nType == kOT_Select)
		{
			const proto::db::select_command* pCommand = dynamic_cast<const proto::db::select_command*>(pRequest.get());
			DebugAstEx(pCommand != nullptr, kRC_PROTO_ERROR);

			this->m_pDbCacheMgr.setData(pCommand->id(), pResponse);
		}

		return nErrorCode;
	}
}