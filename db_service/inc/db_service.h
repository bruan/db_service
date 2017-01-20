#pragma once

#include <list>

#include "proto_src/request.pb.h"
#include "proto_src/response.pb.h"

namespace db
{
	class CDbThreadMgr;
	CDbThreadMgr*	create(const std::string& szHost, uint16_t nPort, const std::string& szDb, const std::string& szUser, const std::string& szPassword, const std::string& szCharacterset, uint32_t nDbThreadCount);
	void			query(CDbThreadMgr* pDbThreadMgr, uint32_t nServiceID, const proto::db::request* pRequest);
	void			release(CDbThreadMgr* pDbThreadMgr);
	void			getResult(CDbThreadMgr* pDbThreadMgr, std::list<std::pair<uint32_t, proto::db::response*>>& listResult);
}