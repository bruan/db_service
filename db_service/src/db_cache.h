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

		google::protobuf::Message*
				getData(uint32_t nDataID);
		bool	setData(uint32_t nDataID, const google::protobuf::Message* pData);
		bool	addData(uint32_t nDataID, const google::protobuf::Message* pData);
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
