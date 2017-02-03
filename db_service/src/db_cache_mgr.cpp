#include "db_cache_mgr.h"

namespace db
{
	CDbCacheMgr::CDbCacheMgr()
		: m_nCurIndex(1)
	{

	}

	CDbCacheMgr::~CDbCacheMgr()
	{

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

		sDbCache.setData(nDataID, pData);
	}

	void CDbCacheMgr::cleanData()
	{

	}

	void CDbCacheMgr::delData(uint64_t nID, const std::string& szDataName)
	{
		auto iter = this->m_mapCache.find(nID);
		if (iter == this->m_mapCache.end())
			return;

		uint32_t nDataID = this->getDataID(szDataName);
		DebugAst(nDataID != -1);

		CDbCache& sDbCache = iter->second;

		sDbCache.delData(nDataID);
	}
}