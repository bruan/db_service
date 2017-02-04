#pragma once
#include "db_service_base.h"
#include "db_cache.h"

#include <unordered_map>
#include <map>
#include <memory>

#include "google\protobuf\message.h"

namespace db
{
	class CDbCommandHandlerProxy;
	class CDbCacheMgr
	{
	public:
		CDbCacheMgr();
		~CDbCacheMgr();

		bool		init(CDbCommandHandlerProxy* pDbCommandHandlerProxy, uint64_t nMaxCacheSize);
		std::shared_ptr<google::protobuf::Message>
					getData(uint64_t nID, const std::string& szDataName);
		void		setData(uint64_t nID, std::shared_ptr<google::protobuf::Message>& pData);
		void		addData(uint64_t nID, std::shared_ptr<google::protobuf::Message>& pData);
		void		delData(uint64_t nID, const std::string& szDataName);
		void		cleanData();
		void		flushAllCache();
		CDbCommandHandlerProxy*
					getDbCommandHandlerProxy() const;

	private:
		uint32_t	getDataID(const std::string& szDataName);
		void		backup(uint64_t nTime);

	private:
		CDbCommandHandlerProxy*									m_pDbCommandHandlerProxy;
		std::unordered_map<uint64_t, std::shared_ptr<CDbCache>>	m_mapCache;
		std::map<std::string, uint32_t>							m_mapDataIndex;
		uint32_t												m_nCurIndex;
		std::map<uint64_t, std::shared_ptr<CDbCache>>			m_mapDirtyCache;
		int64_t													m_nDataSize;
		int64_t													m_nMaxCacheSize;
	};
}