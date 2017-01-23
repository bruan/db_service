#pragma once

#include <list>
#include <string>
#include <vector>

#include "proto_src/request.pb.h"
#include "proto_src/response.pb.h"

namespace db
{
	class CDbThreadMgr;
	struct SDbResultInfo
	{
		uint32_t	nServiceID;
		std::shared_ptr<proto::db::response>
					pResponse;
	};
	CDbThreadMgr*	create(const std::string& szHost, uint16_t nPort, const std::string& szDb, const std::string& szUser, const std::string& szPassword, const std::string& szCharacterset, const std::string& szProtoDir, uint32_t nDbThreadCount);
	void			query(CDbThreadMgr* pDbThreadMgr, uint32_t nServiceID, const proto::db::request* pRequest);
	void			release(CDbThreadMgr* pDbThreadMgr);
	void			getResultInfo(CDbThreadMgr* pDbThreadMgr, std::list<SDbResultInfo>& listResultInfo);
	void			getQPS(CDbThreadMgr* pDbThreadMgr, std::vector<uint32_t>& vecQPS);
	void			getQueueSize(CDbThreadMgr* pDbThreadMgr, std::vector<uint32_t>& vecSize);
}