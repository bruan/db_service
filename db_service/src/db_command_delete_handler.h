#pragma once
#include "db_command_handler.h"

namespace db
{
	class CDbCommandDeleteHandler :
		public CDbCommandHandler
	{
	public:
		CDbCommandDeleteHandler();
		virtual ~CDbCommandDeleteHandler();

		virtual uint32_t	onDbCommand(const google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>& pResponse);
	};
}