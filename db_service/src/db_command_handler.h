#pragma once
#include "db_service_base.h"
#include "db_connection.h"
#include "db_common.h"

#include <string>

#include "google/protobuf/message.h"

namespace db
{
	struct SDbCommand
	{
		uint32_t	nServiceID;
		uint32_t	nType;
		uint64_t	nSessionID;
		int64_t		nTimeout;
		std::shared_ptr<google::protobuf::Message>
					pMessage;
	};

	struct SDbResult
	{
		uint32_t					nServiceID;
		uint32_t					nErrCode;
		uint64_t					nSessionID;
		google::protobuf::Message*	pMessage;
	};

	class CDbCommandHandler
	{
	public:
		CDbCommandHandler() : m_pDbConnection(nullptr) { }
		virtual ~CDbCommandHandler() {}

		virtual void		onConnect(CDbConnection* pDbConnection) { this->m_pDbConnection = pDbConnection; }
		virtual void		onDisconnect() { this->m_pDbConnection = nullptr; }
		virtual uint32_t	onDbCommand(const google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>* pResponse) = 0;

	protected:
		CDbConnection*	m_pDbConnection;
	};
}