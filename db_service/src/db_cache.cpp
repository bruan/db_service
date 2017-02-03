#include "db_cache.h"

namespace db
{
	CDbCache::CDbCache()
	{

	}

	CDbCache::~CDbCache()
	{

	}

	std::shared_ptr<google::protobuf::Message> CDbCache::getData(uint32_t nDataID)
	{
		auto iter = this->m_mapCache.find(nDataID);
		if (iter == this->m_mapCache.end())
			return nullptr;

		return iter->second;
	}

	void CDbCache::setData(uint32_t nDataID, std::shared_ptr<google::protobuf::Message>& pData)
	{
		DebugAst(pData != nullptr);

		this->m_mapCache[nDataID] = pData;
	}

	void CDbCache::delData(uint32_t nDataID)
	{
		this->m_mapCache.erase(nDataID);
	}
}