#include "db_cache.h"
#include "db_protobuf.h"
#include "db_cache_mgr.h"
#include "db_thread.h"

using namespace std;
using namespace google::protobuf;
using namespace db;

#define _CACHE_EXPIRED_TIME 1

CDbCache::CDbCache(CDbCacheMgr* pDbCacheMgr)
	: m_pDbCacheMgr(pDbCacheMgr)
	, m_nDataSize(0)
{

}

CDbCache::~CDbCache()
{

}

shared_ptr<Message> CDbCache::getData(uint32_t nDataID)
{
	auto iter = this->m_mapCacheInfo.find(nDataID);
	if (iter == this->m_mapCacheInfo.end())
		return nullptr;

	return iter->second.pData;
}

bool CDbCache::addData(uint32_t nDataID, shared_ptr<Message>& pData)
{
	DebugAstEx(pData != nullptr, false);

	if (this->m_mapCacheInfo.find(nDataID) != this->m_mapCacheInfo.end())
		return false;

	this->m_mapCacheInfo.insert(make_pair(nDataID, SCacheInfo{ pData, 0 }));
	this->m_nDataSize += pData->ByteSize();

	return true;
}

bool CDbCache::setData(uint32_t nDataID, shared_ptr<Message>& pData)
{
	DebugAstEx(pData != nullptr, false);

	auto iter = this->m_mapCacheInfo.find(nDataID);
	if (iter == this->m_mapCacheInfo.end())
		return this->addData(nDataID, pData);

	this->m_nDataSize -= iter->second.pData->ByteSize();
	iter->second = SCacheInfo{ pData, time(nullptr) };
	this->m_nDataSize += pData->ByteSize();

	return true;
}

bool CDbCache::delData(uint32_t nDataID)
{
	auto iter = this->m_mapCacheInfo.find(nDataID);
	if (iter == this->m_mapCacheInfo.end())
		return false;

	this->m_nDataSize -= iter->second.pData->ByteSize();
	this->m_mapCacheInfo.erase(nDataID);

	return true;
}

int32_t CDbCache::getDataSize() const
{
	return this->m_nDataSize;
}

bool CDbCache::writeback(int64_t nTime)
{
	bool bDirty = false;
	for (auto iter = this->m_mapCacheInfo.begin(); iter != this->m_mapCacheInfo.end(); ++iter)
	{
		SCacheInfo& sCacheInfo = iter->second;
		if (sCacheInfo.nTime == 0)
			continue;

		int64_t nDeltaTime = nTime - sCacheInfo.nTime;
		if (nDeltaTime >= _CACHE_EXPIRED_TIME || nTime == 0)
			this->m_pDbCacheMgr->getDbThread()->flushCache(sCacheInfo.pData);
		else
			bDirty = true;
	}

	return bDirty;
}