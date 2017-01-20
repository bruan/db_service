#include "db_thread_mgr.h"

namespace db
{

	CDbThreadMgr::CDbThreadMgr()
	{
	}

	CDbThreadMgr::~CDbThreadMgr()
	{
		
	}

	bool CDbThreadMgr::init(const std::string& szHost, uint16_t nPort, const std::string& szDb, const std::string& szUser, const std::string& szPassword, const std::string& szCharacterset, uint32_t nDbThreadCount)
	{
		this->m_sDbConnectionInfo.szHost = szHost;
		this->m_sDbConnectionInfo.nPort = nPort;
		this->m_sDbConnectionInfo.szDb = szDb;
		this->m_sDbConnectionInfo.szUser = szUser;
		this->m_sDbConnectionInfo.szPassword = szPassword;
		this->m_sDbConnectionInfo.szCharacterset = szCharacterset;

		this->m_vecDbThread.resize(nDbThreadCount);
		for (uint32_t i = 0; i < nDbThreadCount; ++i)
		{
			this->m_vecDbThread[i] = new CDbThread(this);
			if (!this->m_vecDbThread[i]->init())
				return false;
		}

		return true;
	}

	void CDbThreadMgr::exit()
	{
		for (uint32_t i = 0; i < this->m_vecDbThread.size(); ++i)
		{
			this->m_vecDbThread[i]->join();
			delete this->m_vecDbThread[i];
		}
	}

	void CDbThreadMgr::query(uint32_t nThreadIndex, const SDbCommand& sDbCommand)
	{
		nThreadIndex = nThreadIndex % this->m_vecDbThread.size();
		
		this->m_vecDbThread[nThreadIndex]->query(sDbCommand);
	}

	uint32_t CDbThreadMgr::getThreadCount() const
	{
		return (uint32_t)this->m_vecDbThread.size();
	}

	const SDbConnectionInfo& CDbThreadMgr::getDbConnectionInfo() const
	{
		return this->m_sDbConnectionInfo;
	}

	void CDbThreadMgr::addResult(uint32_t nServiceID, proto::db::response* pResponse)
	{
		std::unique_lock<std::mutex> lock(this->m_tResultLock);
		this->m_listResult.push_back(std::make_pair(nServiceID, pResponse));
	}

	void CDbThreadMgr::getResult(std::list<std::pair<uint32_t, proto::db::response*>>& listResult)
	{
		std::unique_lock<std::mutex> lock(this->m_tResultLock);
		listResult.splice(listResult.end(), this->m_listResult);
	}
}