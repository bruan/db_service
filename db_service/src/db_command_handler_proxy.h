#pragma once

#include "db_service_base.h"
#include "db_command_handler.h"

#include <map>

namespace db
{
	class CDbCommandHandlerProxy
	{
	public:
		CDbCommandHandlerProxy();
		~CDbCommandHandlerProxy();

		void		onConnect(CDbConnection* pDbConnection);
		void		onDisconnect();
		uint32_t	onDbCommand(uint32_t nType, const google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>& pResponse);

	private:
		std::map<uint32_t, CDbCommandHandler*>	m_mapDbCommandHandler;
	};
}