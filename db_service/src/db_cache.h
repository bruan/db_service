#pragma once

#include <map>
#include <vector>
#include <memory>

#include "db_service_base.h"

#include "google\protobuf\message.h"

namespace db
{
	class CDbCacheMgr;
	class CDbCache
	{
	public:
		CDbCache(CDbCacheMgr* pDbCacheMgr);
		~CDbCache();

		std::pair<const char*, size_t>
				getData(uint32_t nDataID);
		bool	setData(uint32_t nDataID, std::string& szData);
		bool	addData(uint32_t nDataID, std::string& szData);
		bool	delData(uint32_t nDataID);
		int32_t	getDataSize() const;
		bool	writeback(int64_t nTime);

	private:
		struct SCacheInfo
		{
			std::string	szData;
			uint64_t	nTime;
		};

		CDbCacheMgr*					m_pDbCacheMgr;
		std::map<uint32_t, SCacheInfo>	m_mapCacheInfo;
		int32_t							m_nDataSize;
	};
}
