#include "db_cache_mgr.h"

using namespace google::protobuf;
using namespace db;
using namespace std;

#define _CLEAN_CACHE_TIME 1
#define _CHECK_WRITE_BACK_TIME 1

static string s_szErrorName;

CDbCacheMgr::CDbCacheMgr()
	: m_pDbThread(nullptr)
	, m_nCurIndex(1)
	, m_nDataSize(0)
	, m_nMaxCacheSize(0)
	, m_nLastCleanCacheTime(0)
	, m_nLastWritebackTime(0)
	, m_nWritebackTime(0)
{

}

CDbCacheMgr::~CDbCacheMgr()
{

}

bool CDbCacheMgr::init(CDbThread* pDbThread, uint64_t nMaxCacheSize, uint32_t nWritebackTime)
{
	DebugAstEx(pDbThread != nullptr, false);

	this->m_pDbThread = pDbThread;
	this->m_nMaxCacheSize = nMaxCacheSize;
	this->m_nWritebackTime = nWritebackTime;

	return true;
}

uint32_t CDbCacheMgr::getDataID(const string& szDataName)
{
	uint32_t nDataID = 0;
	auto iter = this->m_mapDataIndex.find(szDataName);
	if (iter == this->m_mapDataIndex.end())
	{
		nDataID = this->m_nCurIndex++;
		this->m_mapDataIndex[szDataName] = nDataID;
		this->m_mapDataName[nDataID] = szDataName;
	}
	else
	{
		nDataID = iter->second;
	}

	return nDataID;
}

const string& CDbCacheMgr::getDataName(uint32_t nIndex) const
{
	auto iter = this->m_mapDataName.find(nIndex);
	if (iter == this->m_mapDataName.end())
		return s_szErrorName;

	return iter->second;
}

Message* CDbCacheMgr::getData(uint64_t nID, const string& szDataName)
{
	uint32_t nDataID = this->getDataID(szDataName);

	DebugAstEx(nDataID != -1, nullptr);

	auto iter = this->m_mapCache.find(nID);
	if (iter == this->m_mapCache.end())
		return nullptr;

	auto pDbCache = iter->second;

	return pDbCache->getData(nDataID);
}

bool CDbCacheMgr::setData(uint64_t nID, const google::protobuf::Message* pData)
{
	if (this->m_nMaxCacheSize <= 0)
		return false;

	if (this->m_mapCache.find(nID) == this->m_mapCache.end())
		return this->addData(nID, pData);

	const string szDataName = pData->GetTypeName();
	uint32_t nDataID = this->getDataID(szDataName);
	DebugAstEx(nDataID != -1, false);

	auto pDbCache = this->m_mapCache[nID];

	int32_t nSize = pDbCache->getDataSize();
	if (!pDbCache->setData(nDataID, szDataName, pData))
		return false;

	this->m_nDataSize -= nSize;
	this->m_nDataSize += pDbCache->getDataSize();

	this->m_mapDirtyCache[nID] = pDbCache;

	return true;
}

bool CDbCacheMgr::addData(uint64_t nID, const google::protobuf::Message* pData)
{
	if (this->m_nMaxCacheSize <= 0)
		return false;

	uint32_t nDataID = this->getDataID(pData->GetTypeName());
	DebugAstEx(nDataID != -1, false);

	if (this->m_mapCache.find(nID) != this->m_mapCache.end())
		return false;

	auto pDbCache = make_shared<CDbCache>(this);
	this->m_mapCache[nID] = pDbCache;

	int32_t nSize = pDbCache->getDataSize();
	if (!pDbCache->addData(nDataID, pData))
		return false;

	this->m_nDataSize -= nSize;
	this->m_nDataSize += pDbCache->getDataSize();

	return true;
}

bool CDbCacheMgr::delData(uint64_t nID, const string& szDataName)
{
	if (this->m_nMaxCacheSize <= 0)
		return false;

	auto iter = this->m_mapCache.find(nID);
	if (iter == this->m_mapCache.end())
		return false;

	uint32_t nDataID = this->getDataID(szDataName);
	DebugAstEx(nDataID != -1, false);

	auto pDbCache = iter->second;

	int32_t nSize = pDbCache->getDataSize();
	if (!pDbCache->delData(nDataID))
		return false;

	this->m_nDataSize -= nSize;
	this->m_nDataSize += pDbCache->getDataSize();

	return true;
}

void CDbCacheMgr::cleanCache(int64_t nTime)
{
	if (this->m_nMaxCacheSize <= 0)
		return;

	if (this->m_nLastCleanCacheTime != 0 && this->m_nLastCleanCacheTime - nTime < _CLEAN_CACHE_TIME)
		return;

	if (this->m_nDataSize < this->m_nMaxCacheSize)
		return;

	this->m_nLastCleanCacheTime = nTime;

	vector<unordered_map<uint64_t, shared_ptr<CDbCache>>::iterator> vecElement;
	vecElement.reserve(5);
	for (size_t i = 0; i < 5; ++i)
	{
		if (this->m_mapCache.bucket_count() == 0)
			break;

		int32_t nPos = rand() % this->m_mapCache.bucket_count();
		if (this->m_mapCache.begin(nPos) != this->m_mapCache.end(nPos))
			vecElement.push_back(this->m_mapCache.begin(nPos));
	}

	if (vecElement.empty())
		return;

	int32_t nPos = rand() % vecElement.size();
	auto pDbCache = vecElement[nPos]->second;
	uint64_t nID = vecElement[nPos]->first;
	this->m_nDataSize -= pDbCache->getDataSize();
	this->m_mapCache.erase(vecElement[nPos]);
	this->m_mapDirtyCache.erase(nID);
	pDbCache->writeback(0);
}

void CDbCacheMgr::writeback(uint64_t nTime)
{
	if (this->m_nMaxCacheSize <= 0)
		return;

	if (this->m_nLastWritebackTime != 0 && this->m_nLastWritebackTime - nTime < this->m_nWritebackTime)
		return;

	this->m_nLastWritebackTime = nTime;

	for (auto iter = this->m_mapDirtyCache.begin(); iter != this->m_mapDirtyCache.end();)
	{
		if (!iter->second->writeback(nTime))
			this->m_mapDirtyCache.erase(iter++);
		else
			++iter;
	}
}

void CDbCacheMgr::flushCache(uint64_t nKey, bool bDel)
{
	if (this->m_nMaxCacheSize <= 0)
		return;

	if (nKey != 0)
	{
		auto iter = this->m_mapCache.find(nKey);
		if (iter == this->m_mapCache.end())
			return;

		iter->second->writeback(0);
		this->m_mapCache.erase(iter);
	}
	else
	{
		this->writeback(0);
		this->m_mapCache.clear();
	}
}

CDbThread* CDbCacheMgr::getDbThread() const
{
	return this->m_pDbThread;
}

int64_t CDbCacheMgr::getMaxCacheSize() const
{
	return this->m_nMaxCacheSize;
}

void CDbCacheMgr::setMaxCacheSize(uint64_t nSize)
{
	this->m_nMaxCacheSize = nSize;
}

void CDbCacheMgr::update()
{
	int64_t nCurTime = time(nullptr);

	this->cleanCache(nCurTime);
	this->writeback(nCurTime);
}
