#pragma once
#include "db_service_base.h"
#include "db_cache.h"

#include <unordered_map>
#include <map>
#include <memory>

#include "google\protobuf\message.h"

namespace db
{
	class CDbCacheMgr
	{
	public:
		CDbCacheMgr();
		~CDbCacheMgr();

		bool		init(uint64_t nMaxCacheSize);
		std::shared_ptr<google::protobuf::Message>
					getData(uint64_t nID, const std::string& szDataName);
		void		setData(uint64_t nID, std::shared_ptr<google::protobuf::Message> pData);
		void		delData(uint64_t nID, const std::string& szDataName);
		void		cleanData();

	private:
		uint32_t	getDataID(const std::string& szDataName);

	private:
		std::unordered_map<uint64_t, CDbCache>	m_mapCache;
		std::map<std::string, uint32_t>			m_mapDataIndex;
		uint32_t								m_nCurIndex;
		int64_t									m_nDataSize;
		int64_t									m_nMaxCacheSize;
	};
}