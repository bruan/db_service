#include "db_command_nop_handler.h"

using namespace std;
using namespace google::protobuf;
using namespace db;

CDbCommandNOPHandler::CDbCommandNOPHandler()
{

}

CDbCommandNOPHandler::~CDbCommandNOPHandler()
{

}

uint32_t CDbCommandNOPHandler::onDbCommand(const Message* pRequest, shared_ptr<Message>& pResponse)
{
	return kRC_OK;
}