#pragma once
#include "db_command_handler.h"

namespace db
{
	class CDbCommandNOPHandler :
		public CDbCommandHandler
	{
	public:
		CDbCommandNOPHandler();
		virtual ~CDbCommandNOPHandler();

		virtual uint32_t	onDbCommand(const google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>& pResponse);
	};
}