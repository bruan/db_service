#include "db_cache_mgr.h"

namespace db
{
	CDbCacheMgr::CDbCacheMgr()
		: m_nCurIndex(1)
		, m_nDataSize(0)
		, m_nMaxCacheSize(0)
	{

	}

	CDbCacheMgr::~CDbCacheMgr()
	{

	}

	bool CDbCacheMgr::init(uint64_t nMaxCacheSize)
	{
		this->m_nMaxCacheSize = nMaxCacheSize;
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

		CDbCache& sDbCache = this->m_mapCache[nID];

		return sDbCache.getData(nDataID);
	}

	void CDbCacheMgr::setData(uint64_t nID, std::shared_ptr<google::protobuf::Message> pData)
	{
		DebugAst(pData != nullptr);

		std::string szDataName = pData->GetTypeName();
		uint32_t nDataID = this->getDataID(szDataName);
		DebugAst(nDataID != -1);

		CDbCache& sDbCache = this->m_mapCache[nID];

		int32_t nSize = sDbCache.getDataSize();
		sDbCache.setData(nDataID, pData);
		this->m_nDataSize -= nSize;
		this->m_nDataSize += sDbCache.getDataSize();
	}

	void CDbCacheMgr::cleanData()
	{
		if (this->m_nDataSize < this->m_nMaxCacheSize)
			return;

		std::vector<std::unordered_map<uint64_t, CDbCache>::iterator> vecElement;
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
		CDbCache& sDbCache = vecElement[nPos]->second;
		this->m_nDataSize -= sDbCache.getDataSize();
		this->m_mapCache.erase(vecElement[nPos]);
	}

	void CDbCacheMgr::delData(uint64_t nID, const std::string& szDataName)
	{
		auto iter = this->m_mapCache.find(nID);
		if (iter == this->m_mapCache.end())
			return;

		uint32_t nDataID = this->getDataID(szDataName);
		DebugAst(nDataID != -1);

		CDbCache& sDbCache = iter->second;

		int32_t nSize = sDbCache.getDataSize();
		sDbCache.delData(nDataID);
		this->m_nDataSize -= nSize;
		this->m_nDataSize += sDbCache.getDataSize();
	}
}