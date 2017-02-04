#include "db_cache_mgr.h"
#include "db_command_handler_proxy.h"

namespace db
{
	CDbCacheMgr::CDbCacheMgr()
		: m_pDbCommandHandlerProxy(nullptr)
		, m_nCurIndex(1)
		, m_nDataSize(0)
		, m_nMaxCacheSize(0)
	{

	}

	CDbCacheMgr::~CDbCacheMgr()
	{

	}

	bool CDbCacheMgr::init(CDbCommandHandlerProxy* pDbCommandHandlerProxy, uint64_t nMaxCacheSize)
	{
		DebugAstEx(pDbCommandHandlerProxy != nullptr, false);

		this->m_nMaxCacheSize = nMaxCacheSize;
		this->m_pDbCommandHandlerProxy = pDbCommandHandlerProxy;
		return true;
	}

	uint32_t CDbCacheMgr::getDataID(const std::string& szDataName)
	{
		uint32_t nDataID = 0;
		auto iter = this->m_mapDataIndex.find(szDataName);
		if (iter == this->m_mapDataIndex.end())
		{
			nDataID = this->m_nCurIndex++;
			this->m_mapDataIndex[szDataName] = nDataID;
		}
		else
		{
			nDataID = iter->second;
		}

		return nDataID;
	}

	std::shared_ptr<google::protobuf::Message> CDbCacheMgr::getData(uint64_t nID, const std::string& szDataName)
	{
		uint32_t nDataID = this->getDataID(szDataName);

		DebugAstEx(nDataID != -1, nullptr);

		auto iter = this->m_mapCache.find(nID);
		if (iter == this->m_mapCache.end())
			return nullptr;

		auto pDbCache = iter->second;

		return pDbCache->getData(nDataID);
	}

	void CDbCacheMgr::setData(uint64_t nID, std::shared_ptr<google::protobuf::Message>& pData)
	{
		DebugAst(pData != nullptr);

		std::string szDataName = pData->GetTypeName();
		uint32_t nDataID = this->getDataID(szDataName);
		DebugAst(nDataID != -1);

		auto pDbCache = this->m_mapCache[nID];

		int32_t nSize = pDbCache->getDataSize();
		pDbCache->setData(nDataID, pData);
		this->m_nDataSize -= nSize;
		this->m_nDataSize += pDbCache->getDataSize();

		this->m_mapDirtyCache[nID] = pDbCache;
	}

	void CDbCacheMgr::addData(uint64_t nID, std::shared_ptr<google::protobuf::Message>& pData)
	{
		DebugAst(pData != nullptr);

		std::string szDataName = pData->GetTypeName();
		uint32_t nDataID = this->getDataID(szDataName);
		DebugAst(nDataID != -1);

		DebugAst(this->m_mapCache.find(nID) == this->m_mapCache.end());

		auto pDbCache = std::make_shared<CDbCache>(this);
		this->m_mapCache[nID] = pDbCache;

		int32_t nSize = pDbCache->getDataSize();
		pDbCache->addData(nDataID, pData);
		this->m_nDataSize -= nSize;
		this->m_nDataSize += pDbCache->getDataSize();
	}

	void CDbCacheMgr::delData(uint64_t nID, const std::string& szDataName)
	{
		auto iter = this->m_mapCache.find(nID);
		if (iter == this->m_mapCache.end())
			return;

		uint32_t nDataID = this->getDataID(szDataName);
		DebugAst(nDataID != -1);

		auto pDbCache = iter->second;

		int32_t nSize = pDbCache->getDataSize();
		pDbCache->delData(nDataID);
		this->m_nDataSize -= nSize;
		this->m_nDataSize += pDbCache->getDataSize();
	}

	void CDbCacheMgr::cleanData()
	{
		if (this->m_nDataSize < this->m_nMaxCacheSize)
			return;

		std::vector<std::unordered_map<uint64_t, std::shared_ptr<CDbCache>>::iterator> vecElement;
		vecElement.reserve(5);
		for (size_t i = 0; i < 5; ++i)
		{
			if (this->m_mapCache.bucket_count() == 0)
				break;

			int32_t nPos = std::rand() % this->m_mapCache.bucket_count();
			if (this->m_mapCache.begin(nPos) != this->m_mapCache.end(nPos))
				vecElement.push_back(this->m_mapCache.begin(nPos));
		}

		if (vecElement.empty())
			return;

		int32_t nPos = std::rand() % vecElement.size();
		auto pDbCache = vecElement[nPos]->second;
		uint64_t nID = vecElement[nPos]->first;
		this->m_nDataSize -= pDbCache->getDataSize();
		this->m_mapCache.erase(vecElement[nPos]);
		this->m_mapDirtyCache.erase(nID);
		pDbCache->backup(0);
	}

	void CDbCacheMgr::backup(uint64_t nTime)
	{
		for (auto iter = this->m_mapDirtyCache.begin(); iter != this->m_mapDirtyCache.end();)
		{
			if (!iter->second->backup(nTime))
				this->m_mapDirtyCache.erase(iter);
			else
				++iter;
		}
	}

	void CDbCacheMgr::flushAllCache()
	{
		this->backup(0);
	}

	CDbCommandHandlerProxy* CDbCacheMgr::getDbCommandHandlerProxy() const
	{
		return this->m_pDbCommandHandlerProxy;
	}

}