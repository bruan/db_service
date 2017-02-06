#pragma once
#include "db_service_base.h"
#include "db_cache.h"

#include <unordered_map>
#include <map>
#include <memory>

#include "google\protobuf\message.h"

namespace db
{
	class CDbThread;
	class CDbCacheMgr
	{
	public:
		CDbCacheMgr();
		~CDbCacheMgr();

		bool				init(CDbThread* pDbThread, uint64_t nMaxCacheSize);
		std::pair<const char*, size_t>
							getData(uint64_t nID, const std::string& szDataName);
		bool				setData(uint64_t nID, const std::string& szDataName, std::string& szData);
		bool				addData(uint64_t nID, const std::string& szDataName, std::string& szData);
		bool				delData(uint64_t nID, const std::string& szDataName);
		int64_t				getMaxCacheSize() const;
		const std::string&	getDataName(uint32_t nIndex) const;
		void				flushAllCache();
		CDbThread*			getDbThread() const;
		void				update();

	private:
		uint32_t			getDataID(const std::string& szDataName);
		void				cleanCache(int64_t nTime);
		void				writeback(uint64_t nTime);

	private:
		CDbThread*												m_pDbThread;
		std::unordered_map<uint64_t, std::shared_ptr<CDbCache>>	m_mapCache;
		std::map<std::string, uint32_t>							m_mapDataIndex;
		std::map<uint32_t, std::string>							m_mapDataName;
		uint32_t												m_nCurIndex;
		std::map<uint64_t, std::shared_ptr<CDbCache>>			m_mapDirtyCache;
		int64_t													m_nDataSize;
		int64_t													m_nMaxCacheSize;
		int64_t													m_nLastCleanCacheTime;
		int64_t													m_nLastWritebackTime;
	};
}