#include "db_cache.h"
#include "db_protobuf.h"
#include "db_cache_mgr.h"
#include "db_command_handler_proxy.h"

namespace db
{
#define _CACHE_EXPIRED_TIME 10

	CDbCache::CDbCache(CDbCacheMgr* pDbCacheMgr)
		: m_pDbCacheMgr(pDbCacheMgr)
		, m_nDataSize(0)
	{

	}

	CDbCache::~CDbCache()
	{

	}

	std::shared_ptr<google::protobuf::Message> CDbCache::getData(uint32_t nDataID)
	{
		auto iter = this->m_mapCacheInfo.find(nDataID);
		if (iter == this->m_mapCacheInfo.end())
			return nullptr;

		return iter->second.pData;
	}

	void CDbCache::addData(uint32_t nDataID, std::shared_ptr<google::protobuf::Message>& pData)
	{
		DebugAst(pData != nullptr);

		if (this->m_mapCacheInfo.find(nDataID) != this->m_mapCacheInfo.end())
			return;

		this->m_mapCacheInfo.insert(std::make_pair(nDataID, SCacheInfo{ pData, 0 }));
		this->m_nDataSize += pData->ByteSize();
	}

	void CDbCache::setData(uint32_t nDataID, std::shared_ptr<google::protobuf::Message>& pData)
	{
		DebugAst(pData != nullptr);

		auto iter = this->m_mapCacheInfo.find(nDataID);
		if (iter == this->m_mapCacheInfo.end())
		{
			this->addData(nDataID, pData);
			return;
		}

		this->m_nDataSize -= iter->second.pData->ByteSize();
		iter->second = SCacheInfo{ pData, time(nullptr) };
		this->m_nDataSize += pData->ByteSize();
	}

	void CDbCache::delData(uint32_t nDataID)
	{
		auto iter = this->m_mapCacheInfo.find(nDataID);
		if (iter == this->m_mapCacheInfo.end())
			return;

		this->m_nDataSize -= iter->second.pData->ByteSize();
		this->m_mapCacheInfo.erase(nDataID);
	}

	int32_t CDbCache::getDataSize() const
	{
		return this->m_nDataSize;
	}

	bool CDbCache::backup(int64_t nTime)
	{
		bool bDirty = false;
		for (auto iter = this->m_mapCacheInfo.begin(); iter != this->m_mapCacheInfo.end(); ++iter)
		{
			SCacheInfo& sCacheInfo = iter->second;
			if (sCacheInfo.nTime == 0)
				continue;

			int64_t nDeltaTime = nTime - sCacheInfo.nTime;
			if (nDeltaTime >= _CACHE_EXPIRED_TIME || nTime == 0)
				this->m_pDbCacheMgr->getDbCommandHandlerProxy()->flushCache(sCacheInfo.pData);
			else
				bDirty = true;
		}

		return bDirty;
	}
}