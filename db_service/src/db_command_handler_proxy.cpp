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

using namespace std;
using namespace google::protobuf;
using namespace db;

CDbCommandHandlerProxy::CDbCommandHandlerProxy()
{
	this->m_mapDbCommandHandler[kOT_Call]	= new CDbCommandCallHandler();
	this->m_mapDbCommandHandler[kOT_Delete] = new CDbCommandDeleteHandler();
	this->m_mapDbCommandHandler[kOT_Insert] = new CDbCommandInsertHandler();
	this->m_mapDbCommandHandler[kOT_Query]	= new CDbCommandQueryHandler();
	this->m_mapDbCommandHandler[kOT_Select] = new CDbCommandSelectHandler();
	this->m_mapDbCommandHandler[kOT_Update] = new CDbCommandUpdateHandler();
	this->m_mapDbCommandHandler[kOT_Nop]	= new CDbCommandNOPHandler();
}

CDbCommandHandlerProxy::~CDbCommandHandlerProxy()
{
	for (auto iter = this->m_mapDbCommandHandler.begin(); iter != this->m_mapDbCommandHandler.end(); ++iter)
	{
		delete iter->second;
	}
}

bool CDbCommandHandlerProxy::init()
{
	return true;
}

void CDbCommandHandlerProxy::onConnect(CDbConnection* pDbConnection)
{
	pDbConnection->autoCommit(true);

	for (auto iter = this->m_mapDbCommandHandler.begin(); iter != this->m_mapDbCommandHandler.end(); ++iter)
	{
		auto& pDbCommandHandler = iter->second;
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

uint32_t CDbCommandHandlerProxy::onDbCommand(uint32_t nType, shared_ptr<Message>& pRequest, shared_ptr<Message>& pResponse)
{
	string szName = pRequest->GetTypeName();
	auto iter = this->m_mapDbCommandHandler.find(nType);
	if (iter == this->m_mapDbCommandHandler.end())
		return kRC_UNKNOWN;

	CDbCommandHandler* pDbCommandHandler = iter->second;
	if (pDbCommandHandler == nullptr)
		return kRC_UNKNOWN;

	uint32_t nErrorCode = pDbCommandHandler->onDbCommand(pRequest.get(), pResponse);
	if (nErrorCode != kRC_OK)
		return nErrorCode;

	return nErrorCode;
}