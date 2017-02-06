#pragma once
#include "db_command_handler.h"

namespace db
{
	class CDbCommandInsertHandler :
		public CDbCommandHandler
	{
	public:
		CDbCommandInsertHandler();
		virtual ~CDbCommandInsertHandler();

		virtual uint32_t	onDbCommand(const google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>* pResponse);
	};
}