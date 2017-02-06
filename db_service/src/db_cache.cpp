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

pair<const char*, size_t> CDbCache::getData(uint32_t nDataID)
{
	auto iter = this->m_mapCacheInfo.find(nDataID);
	if (iter == this->m_mapCacheInfo.end())
		return make_pair(nullptr, 0);

	return make_pair(iter->second.szData.c_str(), iter->second.szData.size());
}

bool CDbCache::addData(uint32_t nDataID, string& szData)
{
	if (this->m_mapCacheInfo.find(nDataID) != this->m_mapCacheInfo.end())
		return false;

	int32_t nSize = (int32_t)szData.size();
	SCacheInfo& sCacheInfo = this->m_mapCacheInfo[nDataID];
	sCacheInfo.nTime = 0;
	sCacheInfo.szData = move(szData);
	this->m_nDataSize += nSize;

	return true;
}

bool CDbCache::setData(uint32_t nDataID, string& szData)
{
	auto iter = this->m_mapCacheInfo.find(nDataID);
	if (iter == this->m_mapCacheInfo.end())
		return this->addData(nDataID, szData);

	int32_t nSize = (int32_t)szData.size();
	this->m_nDataSize -= (int32_t)iter->second.szData.size();
	iter->second.nTime =time(nullptr);
	iter->second.szData = move(szData);
	this->m_nDataSize += nSize;

	return true;
}

bool CDbCache::delData(uint32_t nDataID)
{
	auto iter = this->m_mapCacheInfo.find(nDataID);
	if (iter == this->m_mapCacheInfo.end())
		return false;

	this->m_nDataSize -= (int32_t)iter->second.szData.size();
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
		{
			const string& szDataName = this->m_pDbCacheMgr->getDataName(iter->first);
			if (szDataName.empty())
			{
				PrintWarning("");
				continue;
			}
			auto pMessage = std::unique_ptr<Message>(createMessage(szDataName));
			if (nullptr == pMessage)
			{
				PrintWarning("");
				continue;
			}
			if (!pMessage->ParseFromString(sCacheInfo.szData))
			{
				PrintWarning("");
				continue;
			}
			uint32_t nErrorCode = m_pDbCacheMgr->getDbThread()->getDbCommandHandlerProxy().onDbCommand(kOT_Update, pMessage.get(), nullptr);
			if (nErrorCode != kRC_OK)
			{
				PrintWarning("");
			}
		}
		else
		{
			bDirty = true;
		}
	}

	return bDirty;
}