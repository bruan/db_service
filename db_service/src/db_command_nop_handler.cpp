#include "db_command_nop_handler.h"

namespace db
{
	CDbCommandNOPHandler::CDbCommandNOPHandler()
	{

	}

	CDbCommandNOPHandler::~CDbCommandNOPHandler()
	{

	}

	uint32_t CDbCommandNOPHandler::onDbCommand(const google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>& pResponse)
	{
		return kRC_OK;
	}
}