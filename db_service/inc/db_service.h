#pragma once

#include <list>
#include <string>
#include <vector>

#include "proto_src/request.pb.h"
#include "proto_src/response.pb.h"

namespace db
{
	struct SDbResultInfo
	{
		uint32_t	nServiceID;
		std::shared_ptr<proto::db::response>
					pResponse;
	};
	
	uint32_t	create(const std::string& szHost, uint16_t nPort, const std::string& szDb, const std::string& szUser, const std::string& szPassword, const std::string& szCharacterset, const std::string& szProtoDir, uint32_t nDbThreadCount, uint64_t nMaxCacheSize, uint32_t nWritebackTime);
	void		query(uint32_t nID, uint32_t nServiceID, const proto::db::request* pRequest);
	void		release(uint32_t nID);
	void		getResultInfo(uint32_t nID, std::list<SDbResultInfo>& listResultInfo);
	void		setMaxCacheSize(uint32_t nID, uint64_t nSize);
	void		flushCache(uint32_t nID, uint64_t nKey, bool bDel);
	void		getQPS(uint32_t nID, std::vector<uint32_t>& vecQPS);
	void		getQueueSize(uint32_t nID, std::vector<uint32_t>& vecSize);
}